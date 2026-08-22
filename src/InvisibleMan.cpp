#include "unmatched/InvisibleMan.hpp"
#include "unmatched/Factories.hpp"

namespace unmatched {

InvisibleMan::InvisibleMan()
    : Fighter(FighterDefinition(
          "invisible_man",
          "Invisible Man",
          Character::InvisibleMan,
          true,
          15,
          2,
          AttackRange::Melee,
          "Stealth"))
    , fogTokens_(3, -1)
    , startTurnSpace_(-1) {
}

std::string InvisibleMan::getSpecialAbility() const {
    return "+1 defense when on fog. Fog tokens act as portals.";
}

bool InvisibleMan::canPlayCard(const Card& card) const {
    return card.getOwner() == Character::Any || card.getOwner() == Character::InvisibleMan;
}

std::vector<Card> InvisibleMan::createDeck() const {
    return DeckFactory().createDeckForInvisibleMan();
}

std::vector<std::unique_ptr<Fighter>> InvisibleMan::createSidekicks() const {
    return {};
}

std::vector<int>& InvisibleMan::getFogTokens() {
    return fogTokens_;
}

const std::vector<int>& InvisibleMan::getFogTokens() const {
    return fogTokens_;
}

void InvisibleMan::moveFogToken(int index, int spaceId) {
    if (index < 0 || index >= static_cast<int>(fogTokens_.size())) {
        throw RuleViolation("Invalid fog token index.");
    }
    if (spaceId == -1) {
        fogTokens_[index] = -1;
        return;
    }
    for (int i = 0; i < static_cast<int>(fogTokens_.size()); ++i) {
        if (i != index && fogTokens_[i] == spaceId) {
            throw RuleViolation("Another fog token is already at this space.");
        }
    }
    fogTokens_[index] = spaceId;
}

void InvisibleMan::placeFogToken(int index, int spaceId) {
    if (index < 0 || index >= static_cast<int>(fogTokens_.size())) {
        throw RuleViolation("Invalid fog token index.");
    }
    if (spaceId == -1) {
        fogTokens_[index] = -1;
        return;
    }
    for (int i = 0; i < static_cast<int>(fogTokens_.size()); ++i) {
        if (i != index && fogTokens_[i] == spaceId) {
            throw RuleViolation("Another fog token is already at this space.");
        }
    }
    fogTokens_[index] = spaceId;
}

void InvisibleMan::placeFogTokens(const std::vector<int>& spaces) {
    for (size_t i = 0; i < fogTokens_.size() && i < spaces.size(); ++i) {
        fogTokens_[i] = spaces[i];
    }
    for (int i = 0; i < static_cast<int>(fogTokens_.size()); ++i) {
        for (int j = i + 1; j < static_cast<int>(fogTokens_.size()); ++j) {
            if (fogTokens_[i] != -1 && fogTokens_[i] == fogTokens_[j]) {
                throw InvalidSetup("Fog tokens cannot be placed on the same space.");
            }
        }
    }
}

bool InvisibleMan::isOnFog(int spaceId) const {
    for (int token : fogTokens_) {
        if (token == spaceId) {
            return true;
        }
    }
    return false;
}

bool InvisibleMan::hasFogAt(int spaceId) const {
    return isOnFog(spaceId);
}

std::vector<int> InvisibleMan::getFogSpaces() const {
    std::vector<int> result;
    for (int token : fogTokens_) {
        if (token != -1) {
            result.push_back(token);
        }
    }
    return result;
}

void InvisibleMan::resetFogTokens() {
    for (int& token : fogTokens_) {
        token = -1;
    }
}

void InvisibleMan::onTurnStart() {
    startTurnSpace_ = spaceId();
}


} // namespace unmatched
