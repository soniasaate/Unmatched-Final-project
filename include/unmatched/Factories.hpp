#pragma once

#include "unmatched/Board.hpp"
#include "unmatched/Card.hpp"
#include "unmatched/Fighter.hpp"
#include <memory>
#include <vector>

namespace unmatched {

class BoardFactory {
public:
    Board createBaskervilleManor() const;
};

class DeckFactory {
public:
    std::vector<Card> createDeckForDracula() const;
    std::vector<Card> createDeckForSherlock() const;
    std::vector<Card> createDeckForInvisibleMan() const;
};

class FighterFactory {
public:
    std::vector<std::unique_ptr<Fighter>> createDraculaFighters() const;
    std::vector<std::unique_ptr<Fighter>> createSherlockFighters() const;
    std::vector<std::unique_ptr<Fighter>> createInvisibleManFighters() const;
};

}  // namespace unmatched
