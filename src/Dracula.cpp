#include "unmatched/Dracula.hpp"
#include "unmatched/Factories.hpp"

namespace unmatched {

Dracula::Dracula()
    : Fighter(FighterDefinition(
          "dracula",
          "Dracula",
          Character::Dracula,
          true,
          13,
          2,
          AttackRange::Melee,
          "Aggro"))
    , mistFormActive_(false) {
    for (int i = 1; i <= 3; ++i) {
        sisters_.emplace_back(FighterDefinition(
            "sister" + std::to_string(i),
            "Sister " + std::to_string(i),
            Character::Sister,
            false,
            1,
            2,
            AttackRange::Melee,
            "Sidekick"));
    }
}

std::string Dracula::getSpecialAbility() const {
    return "At start of turn, deal 1 damage to an adjacent fighter and draw a card.";
}

bool Dracula::canPlayCard(const Card& card) const {
    if (card.getOwner() == Character::Dracula || card.getOwner() == Character::Any) {
        return true;
    }
    if (card.getOwner() == Character::Sister) {
        for (const auto& sister : getSisters()) {
            if (!sister.defeated()) return true;
        }
        return false;
    }
    return false;
}

std::vector<Card> Dracula::createDeck() const {
    return DeckFactory().createDeckForDracula();
}

std::vector<std::unique_ptr<Fighter>> Dracula::createSidekicks() const {
    std::vector<std::unique_ptr<Fighter>> sidekicks;
    sidekicks.push_back(std::make_unique<Fighter>(
        FighterDefinition("sister1", "Sister I", Character::Sister, false, 1, 2, AttackRange::Melee, "Sidekick")
    ));
    sidekicks.push_back(std::make_unique<Fighter>(
        FighterDefinition("sister2", "Sister II", Character::Sister, false, 1, 2, AttackRange::Melee, "Sidekick")
    ));
    sidekicks.push_back(std::make_unique<Fighter>(
        FighterDefinition("sister3", "Sister III", Character::Sister, false, 1, 2, AttackRange::Melee, "Sidekick")
    ));
    return sidekicks;
}

std::vector<Fighter>& Dracula::getSisters() {
    return sisters_;
}

const std::vector<Fighter>& Dracula::getSisters() const {
    return sisters_;
}

void Dracula::useBloodStrike(int) {
}

void Dracula::activateMistForm() {
    mistFormActive_ = true;
}

void Dracula::deactivateMistForm() {
    mistFormActive_ = false;
}

bool Dracula::isMistFormActive() const {
    return mistFormActive_;
}

} // namespace unmatched
