#include "hal/i2c_master.hpp"

#include "hardware/gpio.h"
#include "hardware/regs/i2c.h"
#include "hardware/regs/pads_bank0.h"
#include "hardware/regs/resets.h"
#include "hardware/structs/i2c.h"
#include "hardware/structs/io_bank0.h"
#include "hardware/structs/pads_bank0.h"
#include "hardware/structs/resets.h"

namespace train_controller::hal
{

namespace
{

i2c_hw_t* get_registers(I2cController controller)
{
    return controller == I2cController::I2c0
        ? i2c0_hw
        : i2c1_hw;
}

std::uint32_t get_reset_mask(I2cController controller)
{
    return controller == I2cController::I2c0
        ? RESETS_RESET_I2C0_BITS
        : RESETS_RESET_I2C1_BITS;
}

bool pins_are_valid(const I2cConfig& config)
{
    if (config.controller == I2cController::I2c0)
    {
        return config.sda_pin == 4 && config.scl_pin == 5;
    }

    return config.sda_pin == 6 && config.scl_pin == 7;
}

void configure_i2c_pin(std::uint8_t pin)
{
    // Remove RP2350 pad isolation and output-disable state.
    // Enable the input path required for reading the I2C bus.
    pads_bank0_hw->io[pin] &=
        ~(PADS_BANK0_GPIO0_ISO_BITS |
          PADS_BANK0_GPIO0_OD_BITS);

    pads_bank0_hw->io[pin] |=
        PADS_BANK0_GPIO0_IE_BITS;

    // Connect the physical GPIO pin to the I2C peripheral.
    io_bank0_hw->io[pin].ctrl = GPIO_FUNC_I2C;
}

}

I2cMaster::I2cMaster(const I2cConfig& config)
    : config_(config)
{
}

I2cResult I2cMaster::initialize()
{
    if (!pins_are_valid(config_))
    {
        return I2cResult::InvalidArgument;
    }

    if (config_.baud_rate_hz == 0 ||
        config_.system_clock_hz == 0)
    {
        return I2cResult::InvalidArgument;
    }

    const std::uint32_t period =
        (config_.system_clock_hz + config_.baud_rate_hz / 2) /
        config_.baud_rate_hz;

    const std::uint32_t low_count = period * 3 / 5;
    const std::uint32_t high_count = period - low_count;

    if (low_count < 8 ||
        high_count < 8 ||
        low_count > 0xFFFF ||
        high_count > 0xFFFF)
    {
        return I2cResult::InvalidArgument;
    }

    const std::uint32_t sda_hold_count =
        ((config_.system_clock_hz * 3) / 10'000'000) + 1;

    if (sda_hold_count > low_count - 2)
    {
        return I2cResult::InvalidArgument;
    }

    configure_i2c_pin(config_.sda_pin);
    configure_i2c_pin(config_.scl_pin);

    const std::uint32_t reset_mask =
        get_reset_mask(config_.controller);

    // Put the selected I2C peripheral into reset.
    resets_hw->reset |= reset_mask;

    // Release the peripheral from reset.
    resets_hw->reset &= ~reset_mask;

    // Wait until the hardware reports that reset has completed.
    while ((resets_hw->reset_done & reset_mask) == 0)
    {
    }

    i2c_hw_t* const registers =
        get_registers(config_.controller);

    // The DesignWare I2C controller must be disabled while
    // its configuration registers are modified.
    registers->enable = 0;

    // Master mode, 7-bit addressing, repeated START support.
    // Fast-mode timing registers are also used for 100 kHz.
    registers->con =
        (I2C_IC_CON_SPEED_VALUE_FAST <<
         I2C_IC_CON_SPEED_LSB) |
        I2C_IC_CON_MASTER_MODE_BITS |
        I2C_IC_CON_IC_SLAVE_DISABLE_BITS |
        I2C_IC_CON_IC_RESTART_EN_BITS |
        I2C_IC_CON_TX_EMPTY_CTRL_BITS;

    // FIFO threshold value 0 means a one-entry threshold.
    registers->tx_tl = 0;
    registers->rx_tl = 0;

    registers->fs_scl_hcnt = high_count;
    registers->fs_scl_lcnt = low_count;

    registers->fs_spklen =
        low_count < 16 ? 1 : low_count / 16;

    registers->sda_hold =
        sda_hold_count <<
        I2C_IC_SDA_HOLD_IC_SDA_TX_HOLD_LSB;

    // Enable the configured I2C controller.
    registers->enable = 1;

    return I2cResult::Ok;
}

I2cResult I2cMaster::write(
    std::uint8_t address,
    const std::uint8_t* data,
    std::size_t length
)
{
    // The lowest and highest I2C address ranges are reserved.
    if (address < 0x08 || address > 0x77)
    {
        return I2cResult::InvalidArgument;
    }

    if (data == nullptr || length == 0)
    {
        return I2cResult::InvalidArgument;
    }

    constexpr std::uint32_t TIMEOUT_ITERATIONS = 1'000'000;

    i2c_hw_t* const registers =
        get_registers(config_.controller);

    // The target address can only be modified while the
    // controller is disabled.
    registers->enable = 0;
    registers->tar = address;
    registers->enable = 1;

    // Clear flags left behind by an earlier transaction.
    static_cast<void>(registers->clr_tx_abrt);
    static_cast<void>(registers->clr_stop_det);

    for (std::size_t index = 0; index < length; ++index)
    {
        const bool is_last_byte = index == length - 1;

        std::uint32_t command = data[index];

        if (is_last_byte)
        {
            command |= I2C_IC_DATA_CMD_STOP_BITS;
        }

        // Writing to DATA_CMD places one byte in the TX FIFO.
        registers->data_cmd = command;

        std::uint32_t remaining = TIMEOUT_ITERATIONS;

        // With TX_EMPTY_CTRL enabled, TX_EMPTY is asserted after
        // the byte has left the internal transmit path.
        while ((registers->raw_intr_stat &
                I2C_IC_RAW_INTR_STAT_TX_EMPTY_BITS) == 0)
        {
            if (remaining == 0)
            {
                return I2cResult::Timeout;
            }

            --remaining;
        }

        const std::uint32_t abort_reason =
            registers->tx_abrt_source;

        if (abort_reason != 0)
        {
            // Reading this register clears the abort interrupt.
            static_cast<void>(registers->clr_tx_abrt);

            if ((abort_reason &
                 I2C_IC_TX_ABRT_SOURCE_ABRT_7B_ADDR_NOACK_BITS) != 0)
            {
                return I2cResult::AddressNotAcknowledged;
            }

            if ((abort_reason &
                 I2C_IC_TX_ABRT_SOURCE_ABRT_TXDATA_NOACK_BITS) != 0)
            {
                return I2cResult::DataNotAcknowledged;
            }

            return I2cResult::BusError;
        }
    }

    std::uint32_t remaining = TIMEOUT_ITERATIONS;

    // Wait until the STOP condition is physically generated.
    while ((registers->raw_intr_stat &
            I2C_IC_RAW_INTR_STAT_STOP_DET_BITS) == 0)
    {
        if (remaining == 0)
        {
            return I2cResult::Timeout;
        }

        --remaining;
    }

    // Clear the STOP detection interrupt.
    static_cast<void>(registers->clr_stop_det);

    return I2cResult::Ok;
}

}