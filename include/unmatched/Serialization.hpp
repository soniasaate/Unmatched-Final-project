#pragma once

#include <nlohmann/json.hpp>
#include <vector>
#include <string>
#include <memory>
#include "unmatched/Card.hpp"
#include "unmatched/Fighter.hpp"

namespace unmatched {

using json = nlohmann::json;

json cardToJson(const Card& card);
Card jsonToCard(const json& j);
json cardsToJson(const std::vector<Card>& cards);
std::vector<Card> jsonToCards(const json& j);

json fighterToJson(const Fighter& fighter);
std::unique_ptr<Fighter> jsonToFighter(const json& j);

int findEmptySlot();

} // namespace unmatched
