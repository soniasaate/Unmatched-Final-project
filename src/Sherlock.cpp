#include "unmatched/Sherlock.hpp"
#include "unmatched/Factories.hpp"
#include "unmatched/GameExceptions.hpp"

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
    , watson_(nullptr) {
}

std::string Sherlock::getSpecialAbility() const {
    return "Card effects cannot cancel Sherlock or Watson cards.";
}

bool Sherlock::canPlayCard(const Card& card) const {
    if (card.getOwner() == Character::Sherlock || card.getOwner() == Character::Any) {
        return true;
    }
    if (card.getOwner() == Character::Watson) {
        if (!watson_) return false;
        return !watson_->defeated();
    }
    return false;
}

Fighter& Sherlock::getWatson() {
    if (!watson_) {
        throw RuleViolation("Watson has not been initialized.");
    }
    return *watson_;
}

const Fighter& Sherlock::getWatson() const {
    if (!watson_) {
        throw RuleViolation("Watson has not been initialized.");
    }
    return *watson_;
}

void Sherlock::setWatson(Fighter* watson) {
    watson_ = watson;
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
