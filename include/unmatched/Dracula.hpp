#pragma once

#include "unmatched/Fighter.hpp"
#include <vector>

namespace unmatched {

class Dracula : public Fighter {
public:
    Dracula();

    std::string getSpecialAbility() const override;
    bool canPlayCard(const Card& card) const override;

    std::vector<Fighter>& getSisters();
    const std::vector<Fighter>& getSisters() const;

    void useBloodStrike(int bonus);
    void activateMistForm();
    void deactivateMistForm();
    bool isMistFormActive() const;

private:
    std::vector<Fighter> sisters_;
    bool mistFormActive_;
};

} // namespace unmatched
