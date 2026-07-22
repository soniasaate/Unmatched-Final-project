#pragma once

#include <vector>
#include <string>

namespace unmatched {

class Space {
public:
    Space(int id, int row, int column, std::vector<char> zones, std::vector<int> adjacent,
          bool secretPassage, int startSlot);

    int id() const;
    int row() const;
    int column() const;
    const std::vector<char>& zones() const;
    const std::vector<int>& adjacent() const;
    bool hasSecretPassage() const;
    int startSlot() const;

private:
    int id_;
    int row_;
    int column_;
    std::vector<char> zones_;
    std::vector<int> adjacent_;
    bool secretPassage_;
    int startSlot_;
};

} // namespace unmatched