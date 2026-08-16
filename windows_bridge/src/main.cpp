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
        std::optional<int> last_sent_speed;
        std::optional<train_controller::bridge::SignalAspect>
            last_signal_aspect;
        std::optional<int> last_signal_limit;
        std::optional<std::string> last_signal_error;
        std::optional<train_controller::bridge::EvmState>
            last_evm_state;
        bool signal_state_initialized = false;
        bool last_signal_was_present = false;

        auto last_speed_request =
            std::chrono::steady_clock::now();

        auto last_signal_request =
            std::chrono::steady_clock::now();

        while (true)
        {
            const auto now =
                std::chrono::steady_clock::now();

            if (now - last_speed_request >=
                std::chrono::milliseconds(500))
            {
                const double speed =
                    open_rails.get_speed_kmh();

                const int rounded_speed =
                    static_cast<int>(
                        std::lround(std::abs(speed))
                    );

                if (!last_sent_speed.has_value() ||
                    rounded_speed != *last_sent_speed)
                {
                    const std::string speed_message =
                        "SPEED=" + std::to_string(rounded_speed);

                    serial_port.write_line(speed_message);

                    last_sent_speed = rounded_speed;
                }

                last_speed_request = now;
            }

            if (now - last_signal_request >=
                std::chrono::seconds(2))
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
                            std::cout
                                << "EVM="
                                << train_controller::bridge::to_string(
                                       evm_state
                                   )
                                << '\n';

                            last_evm_state = evm_state;
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
                            std::cout << "EVM=NO_SIGNAL\n";
                            last_evm_state = evm_state;
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
                else if (!last_throttle.has_value() ||
                         message->value != *last_throttle)
                {
                    open_rails.set_throttle(
                        static_cast<unsigned>(message->value)
                    );

                    last_throttle = message->value;
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
