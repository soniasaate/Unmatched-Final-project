#pragma once

#include "unmatched/FighterDefinition.hpp"
#include "unmatched/GameExceptions.hpp"

namespace unmatched {

class Fighter {
public:
    explicit Fighter(FighterDefinition definition);

    const FighterDefinition& definition() const;
    const std::string& id() const;
    const std::string& displayName() const;
    Character cardOwner() const;
    bool isHero() const;
    int maxHealth() const;
    int health() const;
    int move() const;
    AttackRange range() const;
    int spaceId() const;
    bool defeated() const;

    void placeAt(int spaceId);
    void damage(int amount);
    void heal(int amount);
    void reviveAt(int spaceId);
    void removeFromBoard();

private:
    FighterDefinition definition_;
    int health_;
    int spaceId_;
};

} // namespace unmatched
