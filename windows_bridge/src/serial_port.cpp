#include "serial_port.hpp"

#include <stdexcept>
#include <string>

namespace {

std::runtime_error windows_error(const std::string& message) {
    return std::runtime_error(
        message + " Windows error: " + std::to_string(GetLastError()));
}

}  // namespace

namespace train_controller::bridge {

SerialPort::SerialPort(const std::string& port_name) {
    const std::string device_name = R"(\\.\)" + port_name;

    handle_ = CreateFileA(
        device_name.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr);

    if (handle_ == INVALID_HANDLE_VALUE) {
        throw windows_error("Could not open serial port.");
    }

    try {
        configure();
        configure_timeouts();

        if (!PurgeComm(handle_, PURGE_RXCLEAR | PURGE_TXCLEAR)) {
            throw windows_error("Could not clear serial buffers.");
        }
    } catch (...) {
        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
        throw;
    }
}

SerialPort::~SerialPort() {
    if (handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(handle_);
    }
}

void SerialPort::configure() {
    DCB configuration{};
    configuration.DCBlength = sizeof(configuration);

    if (!GetCommState(handle_, &configuration)) {
        throw windows_error("Could not read serial port configuration.");
    }

    configuration.BaudRate = CBR_115200;
    configuration.ByteSize = 8;
    configuration.StopBits = ONESTOPBIT;
    configuration.Parity = NOPARITY;
    configuration.fDtrControl = DTR_CONTROL_ENABLE;
    configuration.fRtsControl = RTS_CONTROL_ENABLE;

    if (!SetCommState(handle_, &configuration)) {
        throw windows_error("Could not configure serial port.");
    }
}

void SerialPort::configure_timeouts() {
    COMMTIMEOUTS timeouts{};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 500;
    timeouts.WriteTotalTimeoutMultiplier = 0;

    if (!SetCommTimeouts(handle_, &timeouts)) {
        throw windows_error("Could not configure serial port timeouts.");
    }
}

bool SerialPort::read_character(char& character) {
    DWORD bytes_read = 0;

    if (!ReadFile(handle_, &character, 1, &bytes_read, nullptr)) {
        throw windows_error("Could not read from serial port.");
    }

    return bytes_read == 1;
}

bool SerialPort::write_line(std::string_view line) {
    std::string data(line);
    data += '\n';

    constexpr unsigned MAXIMUM_ATTEMPTS = 3;
    last_write_error_ = ERROR_SUCCESS;
    std::size_t total_bytes_written = 0;

    for (unsigned attempt = 0; attempt < MAXIMUM_ATTEMPTS; ++attempt) {
        while (total_bytes_written < data.size()) {
            DWORD bytes_written = 0;

            const BOOL succeeded = WriteFile(
                handle_,
                data.data() + total_bytes_written,
                static_cast<DWORD>(
                    data.size() - total_bytes_written
                ),
                &bytes_written,
                nullptr
            );

            if (!succeeded) {
                last_write_error_ = GetLastError();
                break;
            }

            if (bytes_written == 0) {
                last_write_error_ = ERROR_SEM_TIMEOUT;
                break;
            }

            total_bytes_written += bytes_written;
        }

        if (total_bytes_written == data.size()) {
            last_write_error_ = ERROR_SUCCESS;
            return true;
        }

        DWORD communication_errors = 0;
        ClearCommError(handle_, &communication_errors, nullptr);
        Sleep(20);
    }

    return false;
}

DWORD SerialPort::last_write_error() const {
    return last_write_error_;
}

}  // namespace train_controller::bridge
