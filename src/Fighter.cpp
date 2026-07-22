#include "unmatched/Fighter.hpp"
#include <algorithm>
#include <utility>

namespace unmatched {

Fighter::Fighter(FighterDefinition definition)
    : definition_(std::move(definition)),
      health_(definition_.maxHealth()),
      spaceId_(-1) {}

const FighterDefinition& Fighter::definition() const { return definition_; }
const std::string& Fighter::id() const { return definition_.id(); }
const std::string& Fighter::displayName() const { return definition_.displayName(); }
Character Fighter::cardOwner() const { return definition_.cardOwner(); }
bool Fighter::isHero() const { return definition_.isHero(); }
int Fighter::maxHealth() const { return definition_.maxHealth(); }
int Fighter::health() const { return health_; }
int Fighter::move() const { return definition_.move(); }
AttackRange Fighter::range() const { return definition_.range(); }
int Fighter::spaceId() const { return spaceId_; }
bool Fighter::defeated() const { return health_ <= 0; }

void Fighter::placeAt(int spaceId) {
    if (defeated()) {
        throw RuleViolation("A defeated fighter cannot be placed without revival.");
    }
    spaceId_ = spaceId;
}

void Fighter::damage(int amount) {
    if (amount < 0) {
        throw RuleViolation("Damage cannot be negative.");
    }
    health_ = std::max(0, health_ - amount);
    if (health_ == 0) {
        spaceId_ = -1;
    }
}

void Fighter::heal(int amount) {
    if (amount < 0) {
        throw RuleViolation("Healing cannot be negative.");
    }
    if (defeated()) {
        return; 
    }
    health_ = std::min(maxHealth(), health_ + amount);
}

void Fighter::reviveAt(int spaceId) {
    health_ = std::max(1, health_);
    if (health_ == 0) {
        health_ = 1;
    }
    spaceId_ = spaceId;
}

void Fighter::removeFromBoard() {
    spaceId_ = -1;
}

} // namespace unmatched