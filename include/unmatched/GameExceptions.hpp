#pragma once

#include <stdexcept>
#include <string>

namespace unmatched {

class GameException : public std::runtime_error {
public:
    explicit GameException(const std::string& message) : std::runtime_error(message) {}
};

class RuleViolation : public GameException {
public:
    explicit RuleViolation(const std::string& message) : GameException(message) {}
};

class InvalidSetup : public GameException {
public:
    explicit InvalidSetup(const std::string& message) : GameException(message) {}
};

} // namespace unmatched
