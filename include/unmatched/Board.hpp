#pragma once

#include "unmatched/Space.hpp"
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace unmatched {

class Board {
public:
    explicit Board(std::vector<Space> spaces);

    const std::vector<Space>& spaces() const;
    const Space& space(int id) const;
    bool contains(int id) const;
    bool areAdjacentForCombat(int left, int right) const;
    bool shareZone(int left, int right) const;
    int startSpaceForSlot(int slot) const;
    std::vector<int> secretPassageSpaces() const;
    std::vector<int> movementNeighbors(int id) const;
    std::vector<int> directNeighbors(int id) const;
    std::vector<int> spacesSharingAnyZone(int id) const;
    std::vector<int> freeAdjacentSpaces(int id, const std::function<bool(int)>& occupied) const;
    std::vector<int> reachableSpaces(int start, int maxSteps,
                                     const std::function<bool(int)>& occupiedByEnemy,
                                     const std::function<bool(int)>& occupiedByAny) const;
    std::vector<std::string> renderLines(const std::map<int, std::string>& occupants) const;

private:
    void validate() const;
    const Space* findSpace(int id) const;
    void drawEdge(std::vector<std::string>& canvas, const Space& from, const Space& to) const;
    void putToken(std::vector<std::string>& canvas, const Space& space, const std::string& token) const;

    std::vector<Space> spaces_;
};

} // namespace unmatched
