#pragma once

#include <stdexcept>
#include <string>

namespace unmatched {

class GameException : public std::runtime_error {
public:
    explicit GameException(const std::string& message) : std::runtime_error(message) {}
};

} // namespace unmatched
