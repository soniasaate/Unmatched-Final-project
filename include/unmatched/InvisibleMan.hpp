#pragma once

#include "unmatched/Fighter.hpp"
#include <vector>

namespace unmatched {

class InvisibleMan : public Fighter {
public:
    InvisibleMan();

    std::string getSpecialAbility() const override;
    bool canPlayCard(const Card& card) const override;
    std::vector<Card> createDeck() const override;
    std::vector<std::unique_ptr<Fighter>> createSidekicks() const override;

    std::vector<int>& getFogTokens();
    const std::vector<int>& getFogTokens() const;
    void placeFogToken(int index, int spaceId);
    void moveFogToken(int index, int spaceId);
    void placeFogTokens(const std::vector<int>& spaces);
    bool isOnFog(int spaceId) const;
    bool hasFogAt(int spaceId) const;
    std::vector<int> getFogSpaces() const;
    void resetFogTokens();
    int getDefenseBonus() const { return 1; }

private:
    std::vector<int> fogTokens_;
};

} // namespace unmatched
