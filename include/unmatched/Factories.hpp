#pragma once

#include "unmatched/Board.hpp"
#include "unmatched/Card.hpp"
#include "unmatched/Fighter.hpp"
#include <vector>

namespace unmatched {

class BoardFactory {
public:
    Board createBaskervilleManor() const;
};

class DeckFactory {
public:
    std::vector<Card> createDeck(HeroKind hero) const;
};



} // namespace unmatched
