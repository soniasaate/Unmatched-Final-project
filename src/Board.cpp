#include "unmatched/Board.hpp"
#include "unmatched/GameExceptions.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <queue>
#include <set>
#include <sstream>

namespace unmatched {

Board::Board(std::vector<Space> spaces) : spaces_(std::move(spaces)) {
    validate();
}

const std::vector<Space>& Board::spaces() const { return spaces_; }

const Space& Board::space(int id) const {
    const Space* found = findSpace(id);
    if (!found) {
        throw GameException("Unknown board space: " + std::to_string(id));
    }
    return *found;
}

bool Board::contains(int id) const {
    return findSpace(id) != nullptr;
}

bool Board::areAdjacentForCombat(int left, int right) const {
    const auto& adj = space(left).adjacent();
    return std::find(adj.begin(), adj.end(), right) != adj.end();
}

bool Board::shareZone(int left, int right) const {
    const auto& lz = space(left).zones();
    const auto& rz = space(right).zones();
    for (char c : lz) {
        if (std::find(rz.begin(), rz.end(), c) != rz.end()) return true;
    }
    return false;
}

int Board::startSpaceForSlot(int slot) const {
    for (const auto& s : spaces_) {
        if (s.startSlot() == slot) return s.id();
    }
    throw GameException("Start slot not available: " + std::to_string(slot));
}

std::vector<int> Board::secretPassageSpaces() const {
    std::vector<int> result;
    for (const auto& s : spaces_) {
        if (s.hasSecretPassage()) result.push_back(s.id());
    }
    return result;
}

std::vector<int> Board::movementNeighbors(int id) const {
    std::set<int> result;
    const auto& current = space(id);
    result.insert(current.adjacent().begin(), current.adjacent().end());
    if (current.hasSecretPassage()) {
        for (int secret : secretPassageSpaces()) {
            if (secret != id) result.insert(secret);
        }
    }
    return {result.begin(), result.end()};
}

std::vector<int> Board::directNeighbors(int id) const {
    return movementNeighbors(id);
}

std::vector<int> Board::spacesSharingAnyZone(int id) const {
    std::vector<int> result;
    for (const auto& candidate : spaces_) {
        if (candidate.id() != id && shareZone(id, candidate.id())) {
            result.push_back(candidate.id());
        }
    }
    return result;
}

std::vector<int> Board::freeAdjacentSpaces(int id, const std::function<bool(int)>& occupied) const {
    std::vector<int> result;
    for (int adj : space(id).adjacent()) {
        if (!occupied(adj)) result.push_back(adj);
    }
    return result;
}

std::vector<int> Board::reachableSpaces(int start, int maxSteps,
                                        const std::function<bool(int)>& occupiedByEnemy,
                                        const std::function<bool(int)>& occupiedByAny) const {
    if (maxSteps < 0) {
        throw GameException("Movement cannot be negative.");
    }

    int maxId = 0;
    for (const auto& s : spaces_) {
        if (s.id() > maxId) maxId = s.id();
    }
    std::vector<int> distance(maxId + 1, -1);
    std::queue<int> q;
    distance[start] = 0;
    q.push(start);

    while (!q.empty()) {
        int current = q.front();
        q.pop();
        int nextDist = distance[current] + 1;
        if (nextDist > maxSteps) continue;
        for (int next : movementNeighbors(current)) {
            if (occupiedByEnemy(next)) continue;
            if (distance[next] != -1) continue;
            distance[next] = nextDist;
            q.push(next);
        }
    }

    std::vector<int> result;
    for (int i = 1; i <= maxId; ++i) {
        if (distance[i] == -1) continue;
        if (i == start) {
            result.push_back(i);
            continue;
        }
        if (!occupiedByAny(i)) result.push_back(i);
    }
    return result;
}

std::vector<std::string> Board::renderLines(const std::map<int, std::string>& occupants) const {
    std::vector<std::string> canvas(25, std::string(100, ' '));
    for (const auto& from : spaces_) {
        for (int toId : from.adjacent()) {
            if (from.id() < toId) {
                drawEdge(canvas, from, space(toId));
            }
        }
    }

    for (const auto& s : spaces_) {
        std::string token;
        auto it = occupants.find(s.id());
        if (it != occupants.end()) {
            token = "[" + it->second + "]";
        } else if (s.startSlot() > 0) {
            token = "<S" + std::to_string(s.startSlot()) + ">";
        } else if (s.hasSecretPassage()) {
            token = "(~" + std::to_string(s.id()) + ")";
        } else {
            token = "(" + std::to_string(s.id()) + ")";
        }
        putToken(canvas, s, token);
    }

    return canvas;
}


void Board::validate() const {
    if (spaces_.empty()) {
        throw InvalidSetup("Board cannot be empty.");
    }

    std::set<int> ids;
    std::set<std::pair<int, int>> coordinates;
    std::map<int, int> startSlotCounts;
    int secretPassageCount = 0;

    for (const auto& s : spaces_) {
        if (s.id() <= 0) {
            throw InvalidSetup("Board spaces must use positive ids.");
        }
        if (!ids.insert(s.id()).second) {
            throw InvalidSetup("Duplicate board space id: " + std::to_string(s.id()));
        }
        if (!coordinates.insert({s.row(), s.column()}).second) {
            throw InvalidSetup("Overlapping board coordinates at space " + std::to_string(s.id()));
        }
        if (s.zones().empty()) {
            throw InvalidSetup("Board space " + std::to_string(s.id()) + " has no zone.");
        }
        if (s.startSlot() < 0 || s.startSlot() > 2) {
            throw InvalidSetup("Invalid start slot on space " + std::to_string(s.id()));
        }
        if (s.startSlot() > 0) {
            ++startSlotCounts[s.startSlot()];
        }
        if (s.hasSecretPassage()) {
            ++secretPassageCount;
        }
    }

    for (int slot : {1, 2}) {
        if (startSlotCounts[slot] != 1) {
            throw InvalidSetup("Board must define exactly one start slot " + std::to_string(slot) + ".");
        }
    }
    if (secretPassageCount == 1) {
        throw InvalidSetup("A board with secret passages must define at least two passage spaces.");
    }

    for (const auto& s : spaces_) {
        std::set<int> uniqueAdjacent;
        for (int adjId : s.adjacent()) {
            if (adjId == s.id()) {
                throw InvalidSetup("Board space " + std::to_string(s.id()) + " cannot be adjacent to itself.");
            }
            if (!uniqueAdjacent.insert(adjId).second) {
                throw InvalidSetup("Duplicate adjacency on board space " + std::to_string(s.id()));
            }
            const Space* adj = findSpace(adjId);
            if (!adj) {
                throw InvalidSetup("Board space " + std::to_string(s.id()) +
                                   " references missing space " + std::to_string(adjId));
            }
            const auto& rev = adj->adjacent();
            if (std::find(rev.begin(), rev.end(), s.id()) == rev.end()) {
                throw InvalidSetup("Board adjacency must be symmetric between spaces " +
                                   std::to_string(s.id()) + " and " + std::to_string(adjId));
            }
        }
    }
}

const Space* Board::findSpace(int id) const {
    auto it = std::find_if(spaces_.begin(), spaces_.end(),
                           [id](const Space& s) { return s.id() == id; });
    return (it != spaces_.end()) ? &(*it) : nullptr;
}

void Board::drawEdge(std::vector<std::string>& canvas, const Space& from, const Space& to) const {
    int r1 = from.row(), c1 = from.column() + 2;
    int r2 = to.row(), c2 = to.column() + 2;
    int dr = r2 - r1, dc = c2 - c1;
    int steps = std::max(std::abs(dr), std::abs(dc));
    if (steps <= 1) return;

    char glyph = '-';
    if (std::abs(dr) > std::abs(dc) * 2) {
        glyph = '|';
    } else if (dr * dc > 0) {
        glyph = '\\';
    } else if (dr * dc < 0) {
        glyph = '/';
    }

    for (int i = 1; i < steps; ++i) {
        int r = r1 + dr * i / steps;
        int c = c1 + dc * i / steps;
        if (r >= 0 && r < static_cast<int>(canvas.size()) && c >= 0 && c < static_cast<int>(canvas[r].size())) {
            if (canvas[r][c] == ' ') {
                canvas[r][c] = glyph;
            }
        }
    }
}

void Board::putToken(std::vector<std::string>& canvas, const Space& space, const std::string& token) const {
    int r = space.row(), c = space.column();
    if (r < 0 || r >= static_cast<int>(canvas.size())) return;
    for (std::size_t i = 0; i < token.size(); ++i) {
        int tc = c + static_cast<int>(i);
        if (tc >= 0 && tc < static_cast<int>(canvas[r].size())) {
            canvas[r][tc] = token[i];
        }
    }
}

} // namespace unmatched
