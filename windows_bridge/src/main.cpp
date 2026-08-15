#include "message_parser.hpp"
#include "openrails_client.hpp"
#include "serial_port.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <cmath>

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
            << "Waiting for Pico messages...\n"
            << "Press Ctrl+C to stop.\n\n";

        std::string line;
        std::optional<int> last_throttle;
        std::optional<int> last_sent_speed;

        auto last_speed_request =
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

    const int rounded_speed = static_cast<int>(std::lround(std::abs(speed)));

    std::cout
        << "Open Rails speed: "
        << std::fixed
        << std::setprecision(1)
        << speed
        << " km/h\n";

    if (!last_sent_speed.has_value() ||
        rounded_speed != *last_sent_speed)
    {
        const std::string speed_message =
            "SPEED=" + std::to_string(rounded_speed);

        serial_port.write_line(speed_message);

        std::cout
            << "Bridge -> Pico: "
            << speed_message
            << '\n';

        last_sent_speed = rounded_speed;
    }

    last_speed_request = now;
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

            std::cout << "Pico -> " << line << '\n';

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
