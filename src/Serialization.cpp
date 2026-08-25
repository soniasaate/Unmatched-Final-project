#include "unmatched/Serialization.hpp"
#include "unmatched/Dracula.hpp"
#include "unmatched/Sherlock.hpp"
#include "unmatched/InvisibleMan.hpp"
#include <fstream>
#include <filesystem>

namespace unmatched {

namespace fs = std::filesystem;

json cardToJson(const Card& card) {
    json j;
    j["id"] = card.getId();
    j["title"] = card.getTitle();
    j["owner"] = static_cast<int>(card.getOwner());
    j["type"] = static_cast<int>(card.getType());
    j["attack"] = card.getAttack();
    j["defense"] = card.getDefense();
    j["boost"] = card.getBoost();
    j["timing"] = static_cast<int>(card.getTiming());
    return j;
}

Card jsonToCard(const json& j) {
    return Card(
        j["id"].get<std::string>(),
        j["title"].get<std::string>(),
        static_cast<Character>(j["owner"].get<int>()),
        static_cast<CardType>(j["type"].get<int>()),
        j["attack"].get<int>(),
        j["defense"].get<int>(),
        j["boost"].get<int>(),
        static_cast<Timing>(j["timing"].get<int>()),
        nullptr
    );
}

json cardsToJson(const std::vector<Card>& cards) {
    json arr = json::array();
    for (const auto& card : cards) {
        arr.push_back(cardToJson(card));
    }
    return arr;
}

std::vector<Card> jsonToCards(const json& j) {
    std::vector<Card> cards;
    for (const auto& item : j) {
        cards.push_back(jsonToCard(item));
    }
    return cards;
}

json fighterToJson(const Fighter& fighter) {
    json j;
    j["id"] = fighter.id();
    j["displayName"] = fighter.displayName();
    j["cardOwner"] = static_cast<int>(fighter.cardOwner());
    j["range"] = static_cast<int>(fighter.range());
    j["health"] = fighter.health();
    j["maxHealth"] = fighter.maxHealth();
    j["move"] = fighter.move();
    j["spaceId"] = fighter.spaceId();
    j["defeated"] = fighter.defeated();
    j["isHero"] = fighter.isHero();
    
    if (dynamic_cast<const Dracula*>(&fighter)) {
        j["heroType"] = "Dracula";
    } else if (dynamic_cast<const Sherlock*>(&fighter)) {
        j["heroType"] = "Sherlock";
    } else if (dynamic_cast<const InvisibleMan*>(&fighter)) {
        j["heroType"] = "InvisibleMan";
        const auto* inv = dynamic_cast<const InvisibleMan*>(&fighter);
        json fogTokens = json::array();
        for (int t : inv->getFogTokens()) {
            fogTokens.push_back(t);
        }
        j["fogTokens"] = fogTokens;
        j["startTurnSpace"] = inv->getStartTurnSpace();
    } else {
        j["heroType"] = "Sidekick";
    }
    return j;
}

std::unique_ptr<Fighter> jsonToFighter(const json& j) {
    std::string heroType = j["heroType"].get<std::string>();
    std::unique_ptr<Fighter> fighter;
    
    if (heroType == "Dracula") {
        fighter = std::make_unique<Dracula>();
    } else if (heroType == "Sherlock") {
        fighter = std::make_unique<Sherlock>();
    } else if (heroType == "InvisibleMan") {
        fighter = std::make_unique<InvisibleMan>();
        auto* inv = dynamic_cast<InvisibleMan*>(fighter.get());
        if (j.contains("fogTokens")) {
            std::vector<int> tokens;
            for (const auto& t : j["fogTokens"]) {
                tokens.push_back(t.get<int>());
            }
            inv->placeFogTokens(tokens);
        }
    } else {
        std::string id = j["id"].get<std::string>();
        std::string displayName = j.value("displayName", id);
        Character cardOwner = static_cast<Character>(j.value("cardOwner", static_cast<int>(Character::Any)));
        AttackRange range = static_cast<AttackRange>(j.value("range", static_cast<int>(AttackRange::Melee)));
        int maxHealth = j["maxHealth"].get<int>();
        int move = j.value("move", 2);

        fighter = std::make_unique<Fighter>(
            FighterDefinition(
                id,
                displayName,
                cardOwner,
                false,
                maxHealth,
                move,
                range,
                "Sidekick"
            )
        );
    }
    
    fighter->setHealth(j["health"].get<int>());
    fighter->setSpaceId(j["spaceId"].get<int>());
    fighter->setDefeated(j["defeated"].get<bool>());
    return fighter;
}

void rotateSaveFiles() {
    std::error_code ec;

    // شیفت اسلات ۲ به اسلات ۳
    if (fs::exists("save2.json", ec)) {
        if (fs::exists("save3.json", ec)) {
            fs::remove("save3.json", ec);
        }
        fs::rename("save2.json", "save3.json", ec);
    }
    if (fs::exists("tui_state2.json", ec)) {
        if (fs::exists("tui_state3.json", ec)) {
            fs::remove("tui_state3.json", ec);
        }
        fs::rename("tui_state2.json", "tui_state3.json", ec);
    }

    // شیفت اسلات ۱ به اسلات ۲
    if (fs::exists("save1.json", ec)) {
        if (fs::exists("save2.json", ec)) {
            fs::remove("save2.json", ec);
        }
        fs::rename("save1.json", "save2.json", ec);
    }
    if (fs::exists("tui_state1.json", ec)) {
        if (fs::exists("tui_state2.json", ec)) {
            fs::remove("tui_state2.json", ec);
        }
        fs::rename("tui_state1.json", "tui_state2.json", ec);
    }
}

int findEmptySlot() {
    return 1;
}

} // namespace unmatched
