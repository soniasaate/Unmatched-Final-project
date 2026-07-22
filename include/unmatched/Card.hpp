#pragma once

#include <string>
#include <vector>

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

enum class EffectId {
    None,
    DraculaBloodStrike,
    DraculaMistForm,
    DraculaAmbush,
    DraculaBloodBath,
    DraculaBeastForm,
    Dash,
    Exploit,
    DraculaLookIntoMyEyes,
    DraculaHunt,
    SisterRaveningSeduction,
    SisterThirstForSustenance,
    Feint,
    WatsonAid,
    SherlockConfirmSuspicion,
    SherlockCounterpunch,
    SherlockStrategicDeduction,
    EducationNeverEnds,
    SherlockElementary,
    SherlockEliminateImpossible,
    SherlockFixedPoint,
    SherlockMasterOfDisguise,
    SherlockGameAfoot,
    WatsonPistol,
    SherlockStudyMethods,
};

class Card {
public:
    Card(std::string title,
         Character owner,
         CardType type,
         int attack,
         int defense,
         int boost,
         Timing timing,
         EffectId effect,
         std::string effectText);

    const std::string& title() const;
    Character owner() const;
    CardType type() const;
    int attack() const;
    int defense() const;
    int boost() const;
    Timing timing() const;
    EffectId effect() const;
    const std::string& effectText() const;

    bool canAttack() const;
    bool canDefend() const;
    bool isScheme() const;
    bool hasCombatEffect() const;
    std::string typeLabel() const;
    std::string ownerLabel() const;
    std::string timingLabel() const;

private:
    std::string title_;
    Character owner_;
    CardType type_;
    int attack_;
    int defense_;
    int boost_;
    Timing timing_;
    EffectId effect_;
    std::string effectText_;
};

std::string heroKindName(HeroKind hero);
std::string characterName(Character character);

}  // namespace unmatched