#include "unmatched/Factories.hpp"
#include <utility>

namespace unmatched {

Board BoardFactory::createBaskervilleManor() const {
    std::vector<Space> spaces;

    auto addSpace = [&](int id, int row, int column, std::vector<char> zones,
                        std::vector<int> adjacent, bool secret = false, int startSlot = 0) {
        spaces.emplace_back(id, row, column, std::move(zones), std::move(adjacent), secret, startSlot);
    };

    addSpace(1, 1, 4, {'b'}, {2, 10}, true, 0);
    addSpace(2, 1, 14, {'b'}, {1, 3}, false, 0);
    addSpace(3, 4, 26, {'b','r'}, {2, 4, 12}, false, 2);
    addSpace(4, 4, 36, {'r'}, {3, 5, 6}, false, 0);
    addSpace(5, 4, 46, {'r'}, {4, 6, 7}, false, 0);
    addSpace(6, 5, 38, {'r'}, {4, 5}, false, 0);
    addSpace(7, 4, 62, {'r','p','g'}, {5, 8, 20}, false, 0);
    addSpace(8, 4, 74, {'y'}, {9, 7, 13}, false, 0);
    addSpace(9, 4, 84, {'y'}, {8}, true, 0);
    addSpace(10, 6, 2, {'b'}, {1, 11, 14}, false, 0);
    addSpace(11, 6, 14, {'b'}, {10, 12}, false, 0);
    addSpace(12, 6, 26, {'b','d'}, {3, 11, 15}, false, 0);
    addSpace(13, 6, 76, {'y'}, {8, 21}, false, 0);
    addSpace(14, 9, 2, {'d'}, {10, 15, 16}, false, 0);
    addSpace(15, 9, 18, {'d'}, {12, 14}, false, 0);
    addSpace(16, 12, 2, {'e','d'}, {14, 17, 23}, false, 0);
    addSpace(17, 12, 18, {'d','g'}, {16, 18}, false, 0);
    addSpace(18, 12, 30, {'g'}, {17, 19, 26}, false, 0);
    addSpace(19, 12, 46, {'g'}, {18, 20}, true, 0);
    addSpace(20, 14, 54, {'g','p'}, {7, 21, 32, 29, 28, 19}, false, 0);
    addSpace(21, 14, 72, {'p'}, {13, 22, 20}, false, 0);
    addSpace(22, 14, 86, {'p'}, {21, 32, 31}, false, 0);
    addSpace(23, 16, 2, {'e'}, {16, 24}, true, 0);
    addSpace(24, 16, 14, {'e'}, {23, 25}, false, 0);
    addSpace(25, 16, 26, {'e'}, {24, 27, 26}, false, 0);
    addSpace(26, 16, 40, {'g','e'}, {18, 28, 25}, false, 0);
    addSpace(27, 18, 20, {'e'}, {28, 25}, false, 0);
    addSpace(28, 18, 34, {'e'}, {27, 26, 20, 29}, false, 0);
    addSpace(29, 18, 48, {'e'}, {30, 20, 28}, false, 0);
    addSpace(30, 18, 62, {'e'}, {29, 31}, false, 0);
    addSpace(31, 18, 76, {'p','e'}, {22, 30}, false, 0);
    addSpace(32, 16, 70, {'p'}, {22, 20}, false, 1);

    return Board(std::move(spaces));
}

std::vector<Card> DeckFactory::createDeck(HeroKind hero) const {
    std::vector<Card> deck;
    auto addCopies = [&](int copies,
                         const std::string& title,
                         Character owner,
                         CardType type,
                         int attack,
                         int defense,
                         int boost,
                         Timing timing,
                         EffectId effect,
                         const std::string& text) {
        for (int i = 0; i < copies; ++i) {
            deck.emplace_back(title, owner, type, attack, defense, boost, timing, effect, text);
        }
    };

    if (hero == HeroKind::Dracula) {
        addCopies(2, "Blood Hunger", Character::Dracula, CardType::Attack, 2, -1, 3, Timing::DuringCombat,
                  EffectId::DraculaBloodStrike, "+1 attack for each Sister in the defender's zone.");
        addCopies(2, "Mist Form", Character::Dracula, CardType::Scheme, -1, -1, 2, Timing::None,
                  EffectId::DraculaMistForm, "Place Dracula in any empty space and gain 1 action.");
        addCopies(2, "Ambush", Character::Any, CardType::Attack, 2, -1, 3, Timing::Immediately,
                  EffectId::DraculaAmbush, "Opponent discards a random card; add its boost to this attack.");
        addCopies(2, "Blood Bath", Character::Dracula, CardType::Scheme, -1, -1, 2, Timing::None,
                  EffectId::DraculaBloodBath, "Recover 2 health. Revive one defeated Sister in Dracula's zone.");
        addCopies(2, "Beast Form", Character::Dracula, CardType::Attack, 6, -1, 4, Timing::DuringCombat,
                  EffectId::DraculaBeastForm, "May discard cards for +1 attack each.");
        addCopies(3, "Dash", Character::Any, CardType::Versatile, 3, 3, 1, Timing::AfterCombat,
                  EffectId::Dash, "Move your fighter up to 3 spaces after combat.");
        addCopies(3, "Exploit", Character::Any, CardType::Versatile, 4, 4, 1, Timing::AfterCombat,
                  EffectId::Exploit, "Draw 1 card after combat.");
        addCopies(3, "Look Into My Eyes", Character::Dracula, CardType::Defense, -1, 1, 2, Timing::DuringCombat,
                  EffectId::DraculaLookIntoMyEyes, "Add the attack card boost value to this defense.");
        addCopies(2, "Hunt", Character::Dracula, CardType::Scheme, -1, -1, 4, Timing::None,
                  EffectId::DraculaHunt, "Deal 1 damage to each adjacent opposing fighter and heal that much.");
        addCopies(3, "Ravening Seduction", Character::Sister, CardType::Scheme, -1, -1, 2, Timing::None,
                  EffectId::SisterRaveningSeduction, "Move any fighter up to 2 spaces, then damage for adjacent Sisters.");
        addCopies(3, "Thirst for Sustenance", Character::Sister, CardType::Attack, 3, -1, 3, Timing::AfterCombat,
                  EffectId::SisterThirstForSustenance, "If you won, place Dracula adjacent to the opposing fighter.");
        addCopies(3, "Feint", Character::Any, CardType::Versatile, 2, 2, 2, Timing::Immediately,
                  EffectId::Feint, "Cancel all effects on the opponent's card.");
        return deck;
    }

    addCopies(2, "Aid", Character::Watson, CardType::Scheme, -1, -1, 2, Timing::None,
              EffectId::WatsonAid, "Place Watson adjacent to Holmes, heal Holmes 1, and draw 1 card.");
    addCopies(3, "Confirm Suspicion", Character::Sherlock, CardType::Scheme, -1, -1, 1, Timing::None,
              EffectId::SherlockConfirmSuspicion, "Name a value; opponent discards a matching card or reveals hand.");
    addCopies(3, "Counterpunch", Character::Sherlock, CardType::Versatile, 3, 3, 1, Timing::AfterCombat,
              EffectId::SherlockCounterpunch, "If Holmes is adjacent to the opposing fighter, deal 2 damage.");
    addCopies(3, "Strategic Deduction", Character::Sherlock, CardType::Versatile, 3, 3, 1, Timing::DuringCombat,
              EffectId::SherlockStrategicDeduction, "Change the printed value on the opponent's card to its boost.");
    addCopies(2, "Education Never Ends", Character::Any, CardType::Versatile, 3, 3, 1, Timing::AfterCombat,
              EffectId::EducationNeverEnds, "If you won, opponent draws 1. If you lost, draw 2.");
    addCopies(2, "Elementary", Character::Sherlock, CardType::Defense, -1, 3, 3, Timing::DuringCombat,
              EffectId::SherlockElementary, "Predict and cancel the attack value. Implemented as a revealed defense read.");
    addCopies(2, "Eliminate Impossible", Character::Sherlock, CardType::Scheme, -1, -1, 2, Timing::None,
              EffectId::SherlockEliminateImpossible, "Look at opponent hand and burn 1 card.");
    addCopies(3, "Feint", Character::Any, CardType::Versatile, 2, 2, 1, Timing::Immediately,
              EffectId::Feint, "Cancel all effects on the opponent's card.");
    addCopies(2, "Fixed Point", Character::Watson, CardType::Versatile, 3, 3, 1, Timing::AfterCombat,
              EffectId::SherlockFixedPoint, "If Watson is adjacent to Holmes, heal both by 1.");
    addCopies(2, "Master of Disguise", Character::Sherlock, CardType::Scheme, -1, -1, 2, Timing::None,
              EffectId::SherlockMasterOfDisguise, "Swap Holmes with an opposing fighter and deal it 1 damage.");
    addCopies(2, "The Game Is Afoot", Character::Sherlock, CardType::Attack, 5, -1, 2, Timing::AfterCombat,
              EffectId::SherlockGameAfoot, "Move Holmes up to 3 spaces after combat.");
    addCopies(2, "Service Revolver", Character::Watson, CardType::Attack, 5, -1, 3, Timing::None,
              EffectId::WatsonPistol, "No effect.");
    addCopies(2, "Study Methods", Character::Any, CardType::Versatile, 3, 3, 2, Timing::AfterCombat,
              EffectId::SherlockStudyMethods, "If you won, inspect the opponent's hand.");
    return deck;
}


} // namespace unmatched