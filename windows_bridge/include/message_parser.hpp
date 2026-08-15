#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace train_controller::bridge {

struct Message {
    std::string key;
    int value;
};

class MessageParser {
public:
    static std::optional<Message> parse(std::string_view input);
};

}  // namespace train_controller::bridge