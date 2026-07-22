#include "unmatched/Board.hpp"
#include <algorithm>
#include <set>
#include <queue>
#include <cmath>

namespace unmatched {

Board::Board(std::vector<Space> spaces) : spaces_(std::move(spaces)) {
}

const std::vector<Space>& Board::spaces() const { return spaces_; }

const Space* Board::space(int id) const {
    return findSpace(id);
}

bool Board::contains(int id) const {
    return findSpace(id) != nullptr;
}

bool Board::areAdjacentForCombat(int left, int right) const {
    const Space* l = findSpace(left);
    const Space* r = findSpace(right);
    if (!l || !r) return false;
    const auto& adj = l->adjacent();
    return std::find(adj.begin(), adj.end(), right) != adj.end();
}

bool Board::shareZone(int left, int right) const {
    const Space* l = findSpace(left);
    const Space* r = findSpace(right);
    if (!l || !r) return false;
    const auto& lz = l->zones();
    const auto& rz = r->zones();
    for (char c : lz) {
        if (std::find(rz.begin(), rz.end(), c) != rz.end()) return true;
    }
    return false;
}

int Board::startSpaceForSlot(int slot) const {
    for (const auto& s : spaces_) {
        if (s.startSlot() == slot) return s.id();
    }
    return -1; 
}

std::vector<int> Board::secretPassageSpaces() const {
    std::vector<int> result;
    for (const auto& s : spaces_) {
        if (s.hasSecretPassage()) result.push_back(s.id());
    }
    return result;
}

const Space* Board::findSpace(int id) const {
    auto it = std::find_if(spaces_.begin(), spaces_.end(),
                           [id](const Space& s) { return s.id() == id; });
    return (it != spaces_.end()) ? &(*it) : nullptr;
}



} // namespace unmatched