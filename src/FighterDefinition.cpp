#include "unmatched/FighterDefinition.hpp"
#include <utility>

namespace unmatched {

FighterDefinition::FighterDefinition(std::string id,
                                     std::string displayName,
                                     Character cardOwner,
                                     bool hero,
                                     int maxHealth,
                                     int move,
                                     AttackRange range,
                                     std::string style)
    : id_(std::move(id)),
      displayName_(std::move(displayName)),
      cardOwner_(cardOwner),
      hero_(hero),
      maxHealth_(maxHealth),
      move_(move),
      range_(range),
      style_(std::move(style)) {}

const std::string& FighterDefinition::id() const { return id_; }
const std::string& FighterDefinition::displayName() const { return displayName_; }
Character FighterDefinition::cardOwner() const { return cardOwner_; }
bool FighterDefinition::isHero() const { return hero_; }
int FighterDefinition::maxHealth() const { return maxHealth_; }
int FighterDefinition::move() const { return move_; }
AttackRange FighterDefinition::range() const { return range_; }
const std::string& FighterDefinition::style() const { return style_; }

std::string FighterDefinition::rangeLabel() const {
    return range_ == AttackRange::Melee ? "Melee" : "Ranged";
}

} // namespace unmatched