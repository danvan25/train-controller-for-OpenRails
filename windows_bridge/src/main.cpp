#include "evm_mapper.hpp"
#include "message_parser.hpp"
#include "openrails_client.hpp"
#include "serial_port.hpp"

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>

int main(int argc, char* argv[])
{
    constexpr int THROTTLE_PICKUP_TOLERANCE = 2;
    constexpr int TRAIN_BRAKE_PICKUP_TOLERANCE = 2;

    if (argc != 2)
    {
        std::cerr
            << "Usage: openrails_bridge.exe COM_PORT\n"
            << "Example: openrails_bridge.exe COM7\n";

        return 1;
    }

    try
    {
        train_controller::bridge::SerialPort serial_port(argv[1]);
        train_controller::bridge::OpenRailsClient open_rails;

        std::cout
            << "Connected to " << argv[1] << '\n'
            << "Open Rails API: localhost:2150\n"
            << "Waiting for state changes...\n"
            << "Press Ctrl+C to stop.\n\n";

        std::string line;

        std::optional<int> last_throttle;
        std::optional<int> physical_throttle;
        std::optional<int> open_rails_throttle;
        std::optional<int> last_train_brake;
        std::optional<int> physical_train_brake;
        std::optional<int> open_rails_train_brake;
        std::optional<int> last_sent_speed;
        std::optional<train_controller::bridge::SignalAspect>
            last_signal_aspect;
        std::optional<int> last_signal_limit;
        std::optional<std::string> last_signal_error;
        std::optional<std::string> last_speed_error;
        std::optional<std::string> last_throttle_error;
        std::optional<std::string> last_throttle_feedback_error;
        std::optional<std::string> last_train_brake_error;
        std::optional<DWORD> last_serial_write_error;
        std::optional<train_controller::bridge::EvmState>
            last_evm_state;
        bool signal_state_initialized = false;
        bool last_signal_was_present = false;
        bool throttle_pickup_active = false;
        bool train_brake_pickup_active = false;
        bool train_brake_control_missing_reported = false;

        auto last_speed_request =
            std::chrono::steady_clock::now();

        std::chrono::milliseconds speed_request_interval(1000);
        std::chrono::milliseconds signal_request_interval(3000);

        auto last_throttle_feedback_request =
            std::chrono::steady_clock::now();

        const std::chrono::milliseconds
            throttle_feedback_interval(2000);

        std::optional<std::chrono::steady_clock::time_point>
            last_throttle_command;
        std::optional<std::chrono::steady_clock::time_point>
            last_train_brake_command;

        auto last_signal_request =
            std::chrono::steady_clock::now();

        const auto send_to_pico =
            [&](const std::string& message) -> bool
            {
                if (serial_port.write_line(message))
                {
                    if (last_serial_write_error.has_value())
                    {
                        std::cout
                            << "Serial communication recovered.\n";
                        last_serial_write_error.reset();
                    }

                    return true;
                }

                const DWORD error_code =
                    serial_port.last_write_error();

                if (!last_serial_write_error.has_value() ||
                    error_code != *last_serial_write_error)
                {
                    std::cerr
                        << "Serial write error: Windows error: "
                        << error_code
                        << ". Retrying later.\n";
                    last_serial_write_error = error_code;
                }

                return false;
            };

        while (true)
        {
            const auto now =
                std::chrono::steady_clock::now();

            if (now - last_speed_request >= speed_request_interval)
            {
                try
                {
                    const double speed =
                        open_rails.get_speed_kmh();

                    if (last_speed_error.has_value())
                    {
                        std::cout << "Speed API recovered.\n";
                        last_speed_error.reset();
                    }

                    speed_request_interval =
                        std::chrono::milliseconds(1000);

                    const int rounded_speed =
                        static_cast<int>(
                            std::lround(std::abs(speed))
                        );

                    if (!last_sent_speed.has_value() ||
                        rounded_speed != *last_sent_speed)
                    {
                        const std::string speed_message =
                            "SPEED=" + std::to_string(rounded_speed);

                        if (send_to_pico(speed_message))
                        {
                            last_sent_speed = rounded_speed;
                        }
                    }

                }
                catch (const std::exception& error)
                {
                    const std::string error_message = error.what();

                    if (!last_speed_error.has_value() ||
                        error_message != *last_speed_error)
                    {
                        std::cerr
                            << "Speed API error: "
                            << error_message
                            << " Retrying later.\n";
                        last_speed_error = error_message;
                    }

                    speed_request_interval *= 2;

                    if (speed_request_interval >
                        std::chrono::seconds(15))
                    {
                        speed_request_interval =
                            std::chrono::seconds(15);
                    }
                }

                last_speed_request = now;
            }

            if (now - last_throttle_feedback_request >=
                throttle_feedback_interval)
            {
                try
                {
                    const auto feedback =
                        open_rails.get_cab_controls();

                    if (last_throttle_feedback_error.has_value())
                    {
                        std::cout
                            << "Cab controls API recovered.\n";
                        last_throttle_feedback_error.reset();
                    }

                    if (feedback.throttle_percentage.has_value())
                    {
                        open_rails_throttle =
                            static_cast<int>(
                                *feedback.throttle_percentage
                            );

                        const bool command_settling =
                            last_throttle_command.has_value() &&
                            now - *last_throttle_command <
                                std::chrono::milliseconds(750);

                        if (throttle_pickup_active &&
                            last_throttle.has_value() &&
                            !command_settling &&
                            std::abs(*open_rails_throttle -
                                     *last_throttle) >
                                THROTTLE_PICKUP_TOLERANCE)
                        {
                            throttle_pickup_active = false;

                            std::cout
                                << "Throttle pickup waiting: Open Rails="
                                << *open_rails_throttle
                                << "%, physical="
                                << (physical_throttle.has_value()
                                        ? std::to_string(*physical_throttle) +
                                              "%"
                                        : std::string("unknown"))
                                << '\n';
                        }

                        if (!throttle_pickup_active &&
                            physical_throttle.has_value() &&
                            std::abs(*physical_throttle -
                                     *open_rails_throttle) <=
                                THROTTLE_PICKUP_TOLERANCE)
                        {
                            throttle_pickup_active = true;
                            last_throttle = *physical_throttle;

                            std::cout
                                << "Throttle pickup acquired at "
                                << *physical_throttle
                                << "%.\n";
                        }
                    }

                    if (feedback.train_brake_percentage.has_value())
                    {
                        train_brake_control_missing_reported = false;
                        open_rails_train_brake =
                            static_cast<int>(
                                *feedback.train_brake_percentage
                            );

                        const bool brake_command_settling =
                            last_train_brake_command.has_value() &&
                            now - *last_train_brake_command <
                                std::chrono::milliseconds(750);

                        if (train_brake_pickup_active &&
                            last_train_brake.has_value() &&
                            !brake_command_settling &&
                            std::abs(*open_rails_train_brake -
                                     *last_train_brake) >
                                TRAIN_BRAKE_PICKUP_TOLERANCE)
                        {
                            train_brake_pickup_active = false;

                            std::cout
                                << "Train brake pickup waiting: Open Rails="
                                << *open_rails_train_brake
                                << "%, physical="
                                << (physical_train_brake.has_value()
                                        ? std::to_string(
                                              *physical_train_brake
                                          ) + "%"
                                        : std::string("unknown"))
                                << '\n';
                        }

                        if (!train_brake_pickup_active &&
                            physical_train_brake.has_value() &&
                            std::abs(*physical_train_brake -
                                     *open_rails_train_brake) <=
                                TRAIN_BRAKE_PICKUP_TOLERANCE)
                        {
                            train_brake_pickup_active = true;
                            last_train_brake = *physical_train_brake;

                            std::cout
                                << "Train brake pickup acquired at "
                                << *physical_train_brake
                                << "%.\n";
                        }
                    }
                    else if (!train_brake_control_missing_reported)
                    {
                        std::cerr
                            << "TRAIN_BRAKE control was not found in "
                            << "Open Rails CABCONTROLS.\n";
                        train_brake_control_missing_reported = true;
                    }
                }
                catch (const std::exception& error)
                {
                    const std::string error_message = error.what();

                    if (!last_throttle_feedback_error.has_value() ||
                        error_message != *last_throttle_feedback_error)
                    {
                        std::cerr
                            << "Cab controls API error: "
                            << error_message
                            << " Retrying later.\n";
                        last_throttle_feedback_error = error_message;
                    }
                }

                last_throttle_feedback_request = now;
            }

            if (now - last_signal_request >= signal_request_interval)
            {
                try
                {
                    const auto signal =
                        open_rails.get_next_signal();

                    if (last_signal_error.has_value())
                    {
                        std::cout << "Signal API recovered.\n";
                        last_signal_error.reset();
                    }

                    signal_request_interval =
                        std::chrono::milliseconds(3000);

                    if (signal.has_value())
                    {
                        const bool aspect_changed =
                            !last_signal_aspect.has_value() ||
                            signal->aspect != *last_signal_aspect;

                        if (aspect_changed)
                        {
                            last_signal_limit.reset();
                        }

                        const bool definite_limit_changed =
                            signal->speed_limit_kmh.has_value() &&
                            (!last_signal_limit.has_value() ||
                             *signal->speed_limit_kmh !=
                                 *last_signal_limit);

                        const bool state_changed =
                            !signal_state_initialized ||
                            !last_signal_was_present ||
                            aspect_changed ||
                            definite_limit_changed;

                        if (signal->speed_limit_kmh.has_value())
                        {
                            last_signal_limit =
                                signal->speed_limit_kmh;
                        }

                        last_signal_aspect = signal->aspect;
                        last_signal_was_present = true;
                        signal_state_initialized = true;

                        if (state_changed)
                        {
                            std::cout
                                << "Next signal: "
                                << train_controller::bridge::to_string(
                                       signal->aspect
                                   );

                            if (last_signal_limit.has_value())
                            {
                                std::cout
                                    << ", limit: "
                                    << *last_signal_limit
                                    << " km/h";
                            }
                            else
                            {
                                std::cout
                                    << ", limit: not provided";
                            }

                            if (signal->distance_km.has_value())
                            {
                                std::cout
                                    << ", distance: "
                                    << std::fixed
                                    << std::setprecision(2)
                                    << *signal->distance_km
                                    << " km";
                            }

                            std::cout << '\n';
                        }

                        auto evm_signal = *signal;
                        evm_signal.speed_limit_kmh =
                            last_signal_limit;

                        const auto evm_state =
                            train_controller::bridge::map_to_evm(
                                evm_signal
                            );

                        if (!last_evm_state.has_value() ||
                            evm_state != *last_evm_state)
                        {
                            const std::string evm_message =
                                "EVM=" + std::string(
                                    train_controller::bridge::to_string(
                                        evm_state
                                    )
                                );

                            if (send_to_pico(evm_message))
                            {
                                std::cout
                                    << evm_message
                                    << " -> Pico\n";

                                last_evm_state = evm_state;
                            }
                        }
                    }
                    else if (!signal_state_initialized ||
                             last_signal_was_present)
                    {
                        std::cout
                            << "Next signal: not found\n";

                        last_signal_aspect.reset();
                        last_signal_limit.reset();
                        last_signal_was_present = false;
                        signal_state_initialized = true;

                        const auto evm_state =
                            train_controller::bridge::EvmState::NoSignal;

                        if (!last_evm_state.has_value() ||
                            evm_state != *last_evm_state)
                        {
                            if (send_to_pico("EVM=NO_SIGNAL"))
                            {
                                std::cout
                                    << "EVM=NO_SIGNAL -> Pico\n";
                                last_evm_state = evm_state;
                            }
                        }
                    }
                }
                catch (const std::exception& error)
                {
                    const std::string error_message = error.what();

                    if (!last_signal_error.has_value() ||
                        error_message != *last_signal_error)
                    {
                        std::cerr
                            << "Signal API error: "
                            << error_message
                            << " Retrying later.\n";

                        last_signal_error = error_message;
                    }

                    signal_request_interval *= 2;

                    if (signal_request_interval >
                        std::chrono::seconds(15))
                    {
                        signal_request_interval =
                            std::chrono::seconds(15);
                    }
                }

                last_signal_request = now;
            }

            char character = '\0';

            if (!serial_port.read_character(character))
            {
                continue;
            }

            if (character == '\r')
            {
                continue;
            }

            if (character != '\n')
            {
                line += character;
                continue;
            }

            if (line.empty())
            {
                continue;
            }

            if (line == "RECEIVED_EVM=OK")
            {
                line.clear();
                continue;
            }

            const auto message =
                train_controller::bridge::MessageParser::parse(line);

            if (!message.has_value())
            {
                std::cerr
                    << "Invalid Pico message: "
                    << line
                    << '\n';

                line.clear();
                continue;
            }

            if (message->key == "THROTTLE")
            {
                if (message->value < 0 || message->value > 100)
                {
                    std::cerr
                        << "Invalid throttle value: "
                        << message->value
                        << '\n';
                }
                else
                {
                    const auto previous_physical_throttle =
                        physical_throttle;
                    physical_throttle = message->value;

                    const bool crossed_throttle_target =
                        previous_physical_throttle.has_value() &&
                        open_rails_throttle.has_value() &&
                        ((*previous_physical_throttle <=
                              *open_rails_throttle &&
                          *physical_throttle >=
                              *open_rails_throttle) ||
                         (*previous_physical_throttle >=
                              *open_rails_throttle &&
                          *physical_throttle <=
                              *open_rails_throttle));

                    if (!throttle_pickup_active &&
                        open_rails_throttle.has_value() &&
                        (std::abs(*physical_throttle -
                                  *open_rails_throttle) <=
                             THROTTLE_PICKUP_TOLERANCE ||
                         crossed_throttle_target))
                    {
                        throttle_pickup_active = true;
                        last_throttle.reset();

                        std::cout
                            << "Throttle pickup acquired at "
                            << *physical_throttle
                            << "%.\n";
                    }

                    if (throttle_pickup_active &&
                        (!last_throttle.has_value() ||
                         message->value != *last_throttle))
                    {
                        try
                        {
                            open_rails.set_throttle(
                                static_cast<unsigned>(message->value)
                            );

                            if (last_throttle_error.has_value())
                            {
                                std::cout
                                    << "Throttle API recovered.\n";
                                last_throttle_error.reset();
                            }

                            last_throttle = message->value;
                            last_throttle_command =
                                std::chrono::steady_clock::now();
                        }
                        catch (const std::exception& error)
                        {
                            const std::string error_message = error.what();

                            if (!last_throttle_error.has_value() ||
                                error_message != *last_throttle_error)
                            {
                                std::cerr
                                    << "Throttle API error: "
                                    << error_message
                                    << " Retrying when input is received.\n";
                                last_throttle_error = error_message;
                            }
                        }
                    }
                    else if (!throttle_pickup_active &&
                             open_rails_throttle.has_value())
                    {
                        static std::optional<int>
                            last_reported_physical_throttle;

                        if (!last_reported_physical_throttle.has_value() ||
                            message->value !=
                                *last_reported_physical_throttle)
                        {
                            std::cout
                                << "Throttle pickup waiting: Open Rails="
                                << *open_rails_throttle
                                << "%, physical="
                                << message->value
                                << "%\n";
                            last_reported_physical_throttle =
                                message->value;
                        }
                    }
                }
            }

            if (message->key == "TRAIN_BRAKE")
            {
                if (message->value < 0 || message->value > 100)
                {
                    std::cerr
                        << "Invalid train brake value: "
                        << message->value
                        << '\n';
                }
                else
                {
                    const auto previous_physical_train_brake =
                        physical_train_brake;
                    physical_train_brake = message->value;

                    const bool crossed_train_brake_target =
                        previous_physical_train_brake.has_value() &&
                        open_rails_train_brake.has_value() &&
                        ((*previous_physical_train_brake <=
                              *open_rails_train_brake &&
                          *physical_train_brake >=
                              *open_rails_train_brake) ||
                         (*previous_physical_train_brake >=
                              *open_rails_train_brake &&
                          *physical_train_brake <=
                              *open_rails_train_brake));

                    if (!train_brake_pickup_active &&
                        open_rails_train_brake.has_value() &&
                        (std::abs(*physical_train_brake -
                                  *open_rails_train_brake) <=
                             TRAIN_BRAKE_PICKUP_TOLERANCE ||
                         crossed_train_brake_target))
                    {
                        train_brake_pickup_active = true;
                        last_train_brake.reset();

                        std::cout
                            << "Train brake pickup acquired at "
                            << *physical_train_brake
                            << "%.\n";
                    }

                    if (train_brake_pickup_active &&
                        (!last_train_brake.has_value() ||
                         message->value != *last_train_brake))
                    {
                        try
                        {
                            open_rails.set_train_brake(
                                static_cast<unsigned>(message->value)
                            );

                            if (last_train_brake_error.has_value())
                            {
                                std::cout
                                    << "Train brake API recovered.\n";
                                last_train_brake_error.reset();
                            }

                            last_train_brake = message->value;
                            last_train_brake_command =
                                std::chrono::steady_clock::now();
                        }
                        catch (const std::exception& error)
                        {
                            const std::string error_message = error.what();

                            if (!last_train_brake_error.has_value() ||
                                error_message != *last_train_brake_error)
                            {
                                std::cerr
                                    << "Train brake API error: "
                                    << error_message
                                    << " Retrying when input is received.\n";
                                last_train_brake_error = error_message;
                            }
                        }
                    }
                    else if (!train_brake_pickup_active &&
                             open_rails_train_brake.has_value())
                    {
                        static std::optional<int>
                            last_reported_physical_train_brake;

                        if (!last_reported_physical_train_brake.has_value() ||
                            message->value !=
                                *last_reported_physical_train_brake)
                        {
                            std::cout
                                << "Train brake pickup waiting: Open Rails="
                                << *open_rails_train_brake
                                << "%, physical="
                                << message->value
                                << "%\n";
                            last_reported_physical_train_brake =
                                message->value;
                        }
                    }
                }
            }

            // RECEIVED_SPEED and future valid messages are intentionally
            // ignored here unless they require bridge-side handling.
            line.clear();
        }
    }
    catch (const std::exception& error)
    {
        std::cerr
            << "Error: "
            << error.what()
            << '\n';

        return 1;
    }
}
