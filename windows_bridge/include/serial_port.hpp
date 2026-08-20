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
    bool write_line(std::string_view line);
    DWORD last_write_error() const;

    bool read_character(char& character);

private:
    void configure();
    void configure_timeouts();

    HANDLE handle_ = INVALID_HANDLE_VALUE;
    DWORD last_write_error_ = ERROR_SUCCESS;
};

}  // namespace train_controller::bridge
