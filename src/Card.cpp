#include "unmatched/Card.hpp"

#include <utility>

namespace unmatched {

Card::Card(std::string title,
           Character owner,
           CardType type,
           int attack,
           int defense,
           int boost,
           Timing timing,
           EffectId effect,
           std::string effectText)
    : title_(std::move(title)),
      owner_(owner),
      type_(type),
      attack_(attack),
      defense_(defense),
      boost_(boost),
      timing_(timing),
      effect_(effect),
      effectText_(std::move(effectText)) {}

const std::string& Card::title() const {
    return title_;
}

Character Card::owner() const {
    return owner_;
}

CardType Card::type() const {
    return type_;
}

int Card::attack() const {
    return attack_;
}

int Card::defense() const {
    return defense_;
}

int Card::boost() const {
    return boost_;
}

Timing Card::timing() const {
    return timing_;
}

EffectId Card::effect() const {
    return effect_;
}

const std::string& Card::effectText() const {
    return effectText_;
}

bool Card::canAttack() const {
    return type_ == CardType::Attack || type_ == CardType::Versatile;
}

bool Card::canDefend() const {
    return type_ == CardType::Defense || type_ == CardType::Versatile;
}

bool Card::isScheme() const {
    return type_ == CardType::Scheme;
}

bool Card::hasCombatEffect() const {
    return effect_ != EffectId::None && type_ != CardType::Scheme;
}

std::string Card::typeLabel() const {
    switch (type_) {
        case CardType::Attack:
            return "Attack";
        case CardType::Defense:
            return "Defense";
        case CardType::Versatile:
            return "Versatile";
        case CardType::Scheme:
            return "Scheme";
    }
    return "Unknown";
}

std::string Card::ownerLabel() const {
    return characterName(owner_);
}

std::string Card::timingLabel() const {
    switch (timing_) {
        case Timing::None:
            return "-";
        case Timing::Immediately:
            return "Immediately";
        case Timing::DuringCombat:
            return "During combat";
        case Timing::AfterCombat:
            return "After combat";
    }
    return "-";
}

std::string heroKindName(HeroKind hero) {
    switch (hero) {
        case HeroKind::Dracula:
            return "Dracula";
        case HeroKind::Sherlock:
            return "Sherlock Holmes";
    }
    return "Unknown";
}

std::string characterName(Character character) {
    switch (character) {
        case Character::Any:
            return "Any fighter";
        case Character::Dracula:
            return "Dracula";
        case Character::Sister:
            return "Sister";
        case Character::Sherlock:
            return "Sherlock";
        case Character::Watson:
            return "Watson";
    }
    return "Unknown";
}

}  // namespace unmatched