#include "message_parser.hpp"

#include <charconv>

namespace train_controller::bridge {

std::optional<Message> MessageParser::parse(std::string_view input) {
    const std::size_t separator_position = input.find('=');

    if (separator_position == std::string_view::npos) {
        return std::nullopt;
    }

    const std::string_view key = input.substr(0, separator_position);
    const std::string_view value_text =
        input.substr(separator_position + 1);

    if (key.empty() || value_text.empty()) {
        return std::nullopt;
    }

    int value = 0;

    const char* begin = value_text.data();
    const char* end = value_text.data() + value_text.size();

    const auto [conversion_end, error] =
        std::from_chars(begin, end, value);

    if (error != std::errc{} || conversion_end != end) {
        return std::nullopt;
    }

    return Message{
        std::string(key),
        value
    };
}

}  // namespace train_controller::bridge