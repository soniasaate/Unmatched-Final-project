#pragma once

#include "unmatched/Fighter.hpp"
#include <string>

namespace unmatched {

class Sherlock : public Fighter {
public:
    Sherlock();

    std::string getSpecialAbility() const override;
    bool canPlayCard(const Card& card) const override;

    Fighter& getWatson();
    const Fighter& getWatson() const;

private:
    Fighter watson_;
};

} // namespace unmatched
