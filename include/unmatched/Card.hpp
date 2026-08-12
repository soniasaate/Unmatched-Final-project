#pragma once

#include "unmatched/GameEntity.hpp"
#include <string>
#include <functional>

namespace unmatched {

enum class HeroKind {
    Dracula,
    Sherlock,
};

enum class Character {
    Any,
    Dracula,
    Sister,
    Sherlock,
    Watson,
};

enum class CardType {
    Attack,
    Defense,
    Versatile,
    Scheme,
};

enum class Timing {
    None,
    Immediately,
    DuringCombat,
    AfterCombat,
};

class GameController;
class Fighter;

class Card : public GameEntity {
public:
    Card(std::string id, std::string name, Character owner,
         CardType type, int attack, int defense, int boost, Timing timing,
         std::function<void(Fighter&, Fighter&, GameController&, int&, int&)> effect = nullptr);

    virtual ~Card() = default;

    Character getOwner() const;
    CardType getType() const;
    int getAttack() const;
    int getDefense() const;
    int getBoost() const;
    Timing getTiming() const;
    std::string getTitle() const;

    void setOwner(Character owner);
    void setType(CardType type);
    void setAttack(int attack);
    void setDefense(int defense);
    void setBoost(int boost);
    void setTiming(Timing timing);

    bool canAttack() const;
    bool canDefend() const;
    bool isScheme() const;

    void applyEffect(Fighter& attacker, Fighter& defender,
                 GameController& controller,
                 int& attackValue, int& defenseValue) const;

private:
    Character owner_;
    CardType type_;
    int attack_;
    int defense_;
    int boost_;
    Timing timing_;
    std::function<void(Fighter&, Fighter&, GameController&, int&, int&)> effect_;
};

} // namespace unmatched
