#pragma once

#include <cstddef>
#include <cstdint>

namespace train_controller::hal
{

enum class I2cController : std::uint8_t
{
    I2c0,
    I2c1
};

enum class I2cResult : std::uint8_t
{
    Ok,
    InvalidArgument,
    AddressNotAcknowledged,
    DataNotAcknowledged,
    Timeout,
    BusError
};

struct I2cConfig
{
    I2cController controller;
    std::uint8_t sda_pin;
    std::uint8_t scl_pin;
    std::uint32_t baud_rate_hz;
    std::uint32_t system_clock_hz;
};

class I2cMaster final
{
public:
    explicit I2cMaster(const I2cConfig& config);

    [[nodiscard]] I2cResult initialize();

    [[nodiscard]] I2cResult write(
        std::uint8_t address,
        const std::uint8_t* data,
        std::size_t length
    );

private:
    I2cConfig config_;
};

}