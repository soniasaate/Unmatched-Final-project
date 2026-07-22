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

} // namespace unmatched
