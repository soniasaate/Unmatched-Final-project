#include "unmatched/Sherlock.hpp"
#include "unmatched/Factories.hpp"

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
    if (card.getOwner() == Character::Sherlock || card.getOwner() == Character::Any) {
        return true;
    }
    if (card.getOwner() == Character::Watson) {
        return !getWatson().defeated();
    }
    return false;
}

Fighter& Sherlock::getWatson() {
    return watson_;
}

const Fighter& Sherlock::getWatson() const {
    return watson_;
}

std::vector<Card> Sherlock::createDeck() const {
    return DeckFactory().createDeckForSherlock();
}

std::vector<std::unique_ptr<Fighter>> Sherlock::createSidekicks() const {
    std::vector<std::unique_ptr<Fighter>> sidekicks;
    sidekicks.push_back(std::make_unique<Fighter>(
        FighterDefinition("watson", "Dr. Watson", Character::Watson, false, 8, 2, AttackRange::Ranged, "Support")
    ));
    return sidekicks;
}

} // namespace unmatched
