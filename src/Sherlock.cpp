#include "unmatched/Sherlock.hpp"

namespace unmatched {

Sherlock::Sherlock()
    : Fighter(FighterDefinition(
          "sherlock",
          "Sherlock Holmes",
          Character::Sherlock,
          true,
          16,
          2,
          AttackRange::Melee,
          "Intel"))
    , watson_(FighterDefinition(
          "watson",
          "Dr. Watson",
          Character::Watson,
          false,
          8,
          2,
          AttackRange::Ranged,
          "Support")) {
}

std::string Sherlock::getSpecialAbility() const {
    return "Card effects cannot cancel Sherlock or Watson cards.";
}

bool Sherlock::canPlayCard(const Card& card) const {
    return card.owner() == Character::Sherlock || card.owner() == Character::Any;
}

Fighter& Sherlock::getWatson() {
    return watson_;
}

const Fighter& Sherlock::getWatson() const {
    return watson_;
}

} // namespace unmatched
