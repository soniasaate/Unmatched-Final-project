#include "unmatched/Dracula.hpp"

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
    return card.getOwner() == Character::Dracula || card.getOwner() == Character::Any;
}

std::vector<Fighter>& Dracula::getSisters() {
    return sisters_;
}

const std::vector<Fighter>& Dracula::getSisters() const {
    return sisters_;
}

void Dracula::useBloodStrike(int) {
    //بعدا
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
