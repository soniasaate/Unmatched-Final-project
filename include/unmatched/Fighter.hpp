#pragma once

#include "unmatched/FighterDefinition.hpp"
#include "unmatched/GameExceptions.hpp"
#include "unmatched/GameEntity.hpp" 

namespace unmatched {

class Fighter : public GameEntity {
public:
    explicit Fighter(FighterDefinition definition);
    virtual ~Fighter() = default;

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

    virtual std::string getSpecialAbility() const;

    void setHealth(int health);
    void setSpaceId(int spaceId);
    void setDefeated(bool defeated);

private:
    FighterDefinition definition_;
    int health_;
    int spaceId_;
    bool defeated_;
};

} // namespace unmatched
