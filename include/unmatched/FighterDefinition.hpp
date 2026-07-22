#pragma once

#include "unmatched/Card.hpp"
#include <string>

namespace unmatched {

enum class AttackRange {
    Melee,
    Ranged,
};

class FighterDefinition {
public:
    FighterDefinition(std::string id,
                      std::string displayName,
                      Character cardOwner,
                      bool hero,
                      int maxHealth,
                      int move,
                      AttackRange range,
                      std::string style);

    const std::string& id() const;
    const std::string& displayName() const;
    Character cardOwner() const;
    bool isHero() const;
    int maxHealth() const;
    int move() const;
    AttackRange range() const;
    const std::string& style() const;
    std::string rangeLabel() const;

private:
    std::string id_;
    std::string displayName_;
    Character cardOwner_;
    bool hero_;
    int maxHealth_;
    int move_;
    AttackRange range_;
    std::string style_;
};

} // namespace unmatched
