#include "unmatched/Space.hpp"
#include <utility>

namespace unmatched {

Space::Space(int id, int row, int column, std::vector<char> zones, std::vector<int> adjacent,
             bool secretPassage, int startSlot)
    : id_(id), row_(row), column_(column), zones_(std::move(zones)),
      adjacent_(std::move(adjacent)), secretPassage_(secretPassage), startSlot_(startSlot) {}

int Space::id() const { return id_; }
int Space::row() const { return row_; }
int Space::column() const { return column_; }
const std::vector<char>& Space::zones() const { return zones_; }
const std::vector<int>& Space::adjacent() const { return adjacent_; }
bool Space::hasSecretPassage() const { return secretPassage_; }
int Space::startSlot() const { return startSlot_; }

} // namespace unmatched