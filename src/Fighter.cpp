#include "unmatched/Fighter.hpp"
#include <algorithm>

namespace unmatched {

Fighter::Fighter(FighterDefinition definition)
    : GameEntity(definition.id(), definition.displayName())
    , definition_(std::move(definition))
    , health_(definition_.maxHealth())
    , spaceId_(-1)
    , defeated_(false) {
}

const FighterDefinition& Fighter::definition() const {
    return definition_;
}

const std::string& Fighter::id() const {
    return definition_.id();
}

const std::string& Fighter::displayName() const {
    return definition_.displayName();
}

Character Fighter::cardOwner() const {
    return definition_.cardOwner();
}

bool Fighter::isHero() const {
    return definition_.isHero();
}

int Fighter::maxHealth() const {
    return definition_.maxHealth();
}

int Fighter::health() const {
    return health_;
}

int Fighter::move() const {
    return definition_.move();
}

AttackRange Fighter::range() const {
    return definition_.range();
}

int Fighter::spaceId() const {
    return spaceId_;
}

bool Fighter::defeated() const {
    return defeated_;
}

void Fighter::setHealth(int health) {
    health_ = health;
    if (health_ <= 0) {
        defeated_ = true;
        spaceId_ = -1;
    } else {
        defeated_ = false;
    }
}

void Fighter::setSpaceId(int spaceId) {
    spaceId_ = spaceId;
}

void Fighter::setDefeated(bool defeated) {
    defeated_ = defeated;
    if (defeated_) {
        spaceId_ = -1;
    }
}

void Fighter::placeAt(int spaceId) {
    if (defeated_) {
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
        defeated_ = true;
        spaceId_ = -1;
    }
}

void Fighter::heal(int amount) {
    if (amount < 0) {
        throw RuleViolation("Healing cannot be negative.");
    }
    if (defeated_) {
        return;
    }
    health_ = std::min(maxHealth(), health_ + amount);
}

void Fighter::reviveAt(int spaceId) {
    if (!defeated_) {
        return;
    }
    health_ = 1;
    defeated_ = false;
    spaceId_ = spaceId;
}

void Fighter::removeFromBoard() {
    spaceId_ = -1;
}

std::string Fighter::getSpecialAbility() const {
    return "None";
}

bool Fighter::canPlayCard(const Card& card) const {
    return card.getOwner() == Character::Any || card.getOwner() == cardOwner();
}

void Fighter::onCombatEffect(const Card&, bool, Fighter&) {
}

void Fighter::onSchemeEffect(const Card&) {
}

void Fighter::onTurnStart() {
}

std::vector<Card> Fighter::createDeck() const {
    return {}; 
}

std::vector<std::unique_ptr<Fighter>> Fighter::createSidekicks() const {
    return {};
}

} // namespace unmatched
