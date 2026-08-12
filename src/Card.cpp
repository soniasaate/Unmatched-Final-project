#include "unmatched/Card.hpp"
#include "unmatched/Fighter.hpp"
#include "unmatched/GameController.hpp"
#include <utility>

namespace unmatched {

Card::Card(std::string id, std::string name, Character owner,
           CardType type, int attack, int defense, int boost, Timing timing,
           std::function<void(Fighter&, Fighter&, GameController&)> effect)
    : GameEntity(std::move(id), std::move(name))
    , owner_(owner)
    , type_(type)
    , attack_(attack)
    , defense_(defense)
    , boost_(boost)
    , timing_(timing)
    , effect_(std::move(effect)) {
}

Character Card::getOwner() const { return owner_; }
CardType Card::getType() const { return type_; }
int Card::getAttack() const { return attack_; }
int Card::getDefense() const { return defense_; }
int Card::getBoost() const { return boost_; }
Timing Card::getTiming() const { return timing_; }

void Card::setOwner(Character owner) { owner_ = owner; }
void Card::setType(CardType type) { type_ = type; }
void Card::setAttack(int attack) { attack_ = attack; }
void Card::setDefense(int defense) { defense_ = defense; }
void Card::setBoost(int boost) { boost_ = boost; }
void Card::setTiming(Timing timing) { timing_ = timing; }

bool Card::canAttack() const {
    return type_ == CardType::Attack || type_ == CardType::Versatile;
}

bool Card::canDefend() const {
    return type_ == CardType::Defense || type_ == CardType::Versatile;
}

bool Card::isScheme() const {
    return type_ == CardType::Scheme;
}

void Card::applyEffect(Fighter& attacker, Fighter& defender,
                       GameController& controller,
                       int& attackValue, int& defenseValue) const {
    if (effect_) {
        effect_(attacker, defender, controller, attackValue, defenseValue);
    }
}

} // namespace unmatched
