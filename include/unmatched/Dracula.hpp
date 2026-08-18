#pragma once

#include "unmatched/Fighter.hpp"
#include <vector>

namespace unmatched {

class Dracula : public Fighter {
public:
    Dracula();

    std::string getSpecialAbility() const override;
    bool canPlayCard(const Card& card) const override;

    std::vector<Card> createDeck() const override;
    std::vector<std::unique_ptr<Fighter>> createSidekicks() const override;

    void useBloodStrike(int bonus);
    void activateMistForm();
    void deactivateMistForm();
    bool isMistFormActive() const;

private:
    bool mistFormActive_;
};

} // namespace unmatched
