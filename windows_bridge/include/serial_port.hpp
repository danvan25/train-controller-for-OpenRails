#pragma once

#include <windows.h>
#include <string>
#include <string_view>


namespace train_controller::bridge {

class SerialPort {
public:
    explicit SerialPort(const std::string& port_name);
    ~SerialPort();

    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;
    SerialPort(SerialPort&&) = delete;
    SerialPort& operator=(SerialPort&&) = delete;
    void write_line(std::string_view line);

    bool read_character(char& character);

private:
    void configure();
    void configure_timeouts();

    HANDLE handle_ = INVALID_HANDLE_VALUE;
};

}  // namespace train_controller::bridge