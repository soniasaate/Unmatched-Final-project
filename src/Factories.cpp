#include "unmatched/Factories.hpp"
#include <utility>

namespace unmatched {

Board BoardFactory::createBaskervilleManor() const {
    std::vector<Space> spaces;

    auto addSpace = [&](int id, int row, int column, std::vector<char> zones,
                        std::vector<int> adjacent, bool secret = false, int startSlot = 0) {
        spaces.emplace_back(id, row, column, std::move(zones), std::move(adjacent), secret, startSlot);
    };

    addSpace(1, 1, 4, {'b'}, {2, 10}, true, 0);
    addSpace(2, 1, 14, {'b'}, {1, 3}, false, 0);
    addSpace(3, 4, 26, {'b','r'}, {2, 4, 12}, false, 2);
    addSpace(4, 4, 36, {'r'}, {3, 5, 6}, false, 0);
    addSpace(5, 4, 46, {'r'}, {4, 6, 7}, false, 0);
    addSpace(6, 5, 38, {'r'}, {4, 5}, false, 0);
    addSpace(7, 4, 62, {'r','p','g'}, {5, 8, 20}, false, 0);
    addSpace(8, 4, 74, {'y'}, {9, 7, 13}, false, 0);
    addSpace(9, 4, 84, {'y'}, {8}, true, 0);
    addSpace(10, 6, 2, {'b'}, {1, 11, 14}, false, 0);
    addSpace(11, 6, 14, {'b'}, {10, 12}, false, 0);
    addSpace(12, 6, 26, {'b','d'}, {3, 11, 15}, false, 0);
    addSpace(13, 6, 76, {'y'}, {8, 21}, false, 0);
    addSpace(14, 9, 2, {'d'}, {10, 15, 16}, false, 0);
    addSpace(15, 9, 18, {'d'}, {12, 14}, false, 0);
    addSpace(16, 12, 2, {'e','d'}, {14, 17, 23}, false, 0);
    addSpace(17, 12, 18, {'d','g'}, {16, 18}, false, 0);
    addSpace(18, 12, 30, {'g'}, {17, 19, 26}, false, 0);
    addSpace(19, 12, 46, {'g'}, {18, 20}, true, 0);
    addSpace(20, 14, 54, {'g','p'}, {7, 21, 32, 29, 28, 19}, false, 0);
    addSpace(21, 14, 72, {'p'}, {13, 22, 20}, false, 0);
    addSpace(22, 14, 86, {'p'}, {21, 32, 31}, false, 0);
    addSpace(23, 16, 2, {'e'}, {16, 24}, true, 0);
    addSpace(24, 16, 14, {'e'}, {23, 25}, false, 0);
    addSpace(25, 16, 26, {'e'}, {24, 27, 26}, false, 0);
    addSpace(26, 16, 40, {'g','e'}, {18, 28, 25}, false, 0);
    addSpace(27, 18, 20, {'e'}, {28, 25}, false, 0);
    addSpace(28, 18, 34, {'e'}, {27, 26, 20, 29}, false, 0);
    addSpace(29, 18, 48, {'e'}, {30, 20, 28}, false, 0);
    addSpace(30, 18, 62, {'e'}, {29, 31}, false, 0);
    addSpace(31, 18, 76, {'p','e'}, {22, 30}, false, 0);
    addSpace(32, 16, 70, {'p'}, {22, 20}, false, 1);

    return Board(std::move(spaces));
}

} // namespace unmatched