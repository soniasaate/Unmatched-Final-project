#pragma once

#include "unmatched/Board.hpp"
#include "unmatched/Card.hpp"
#include "unmatched/Factories.hpp"
#include "unmatched/Player.hpp"

#include <deque>
#include <optional>
#include <random>
#include <string>
#include <vector>
#include <map>
#include <deque>

namespace unmatched {

enum class SchemeChoiceKind {
    None,
    Destination,
    TargetFighter,
    NamedValue,
    OpponentHandCard,
    TargetAndDestination,
};

struct SchemeChoice {
    int destinationSpace = -1;
    std::string targetFighterId;
    int namedValue = -1;
    int opponentHandIndex = -1;
};

struct PendingMovementChoice {
    int playerIndex = -1;
    std::string fighterId;
    int maxSteps = 0;
    std::string source;
};

class GameController {
public:
    GameController();

    void startNewGame(int playerOneAge, int playerTwoAge, HeroKind youngerHero, int youngerStartSlot);
    bool started() const;
    bool gameOver() const;
    const std::string& winnerName() const;

    const Board& board() const;
    Player& currentPlayer();
    const Player& currentPlayer() const;
    Player& opponentPlayer();
    const Player& opponentPlayer() const;
    const std::vector<Player>& players() const;
    int currentPlayerIndex() const;
    int opponentPlayerIndex() const;
    int actionsRemaining() const;
    int turnNumber() const;
    bool draculaAbilityAvailable() const;

    void drawCardForCurrentPlayer();
    void beginManeuver(int boostHandIndex = -1);
    int pendingMovementPoints() const;
    int maxMovementPoints() const;
    std::vector<std::string> movableCurrentFighterIds() const;
    std::vector<int> reachableDestinationsFor(const std::string& fighterId) const;
    void moveCurrentFighter(const std::string& fighterId, int destinationSpace);
    void finishManeuver();

    std::vector<std::string> legalAttackers() const;
    std::vector<std::string> legalTargetsFor(const std::string& attackerId) const;
    std::vector<int> legalAttackCardsFor(const std::string& attackerId) const;
    std::vector<int> legalDefenseCardsFor(const std::string& defenderId) const;
    /*void resolveAttack(const std::string& attackerId,
                       const std::string& defenderId,
                       int attackCardIndex,
                       int defenseCardIndex = -1,
                       const std::vector<int>& beastFormBoostCardIndexes = {});
    */
    void resolveAttack(const std::string& attackerId,
                   const std::string& defenderId,
                   int attackCardIndex,
                   int defenseCardIndex = -1,
                   const std::vector<int>& beastFormBoostCardIndexes = {},
                   int predictedElementaryValue = -1);


    std::vector<int> legalSchemeCards() const;
    SchemeChoiceKind requiredChoiceForScheme(int handIndex) const;
    std::vector<int> destinationChoicesForScheme(int handIndex, const SchemeChoice& partialChoice) const;
    std::vector<std::string> targetChoicesForScheme(int handIndex) const;
    std::vector<int> namedValueChoicesForScheme(int handIndex) const;
    std::vector<int> opponentHandChoicesForScheme(int handIndex) const;
    void playScheme(int handIndex, const SchemeChoice& choice);

    std::vector<int> getMatchingCardIndicesForConfirmSuspicion(int namedValue) const;
    void applyConfirmSuspicion(int chosenIndex);

    std::vector<int> legalBoostCardIndexes() const;
    void discardCurrentPlayerCard(int handIndex);
    void useDraculaStartAbility(const std::string& targetFighterId);
    bool hasPendingOptionalMovement() const;
    const PendingMovementChoice& pendingOptionalMovement() const;
    std::vector<int> pendingOptionalMovementDestinations() const;
    void resolvePendingOptionalMovement(int destinationSpace = -1);
    void endTurnIfNeeded();
    bool currentPlayerMustDiscardToLimit() const;

    std::map<int, std::string> occupantTokens() const;
    const std::map<std::string, int>& movedThisManeuver() const { return movedThisManeuver_; }
    const Fighter* findFighterById(const std::string& fighterId) const;
    Fighter* findFighterById(const std::string& fighterId);
    const Player* ownerOfFighter(const std::string& fighterId) const;
    void finishCurrentFighter(const std::string& fighterId);
    int getMovementCost(const std::string& fighterId, int destinationSpace) const;
    int remainingMovementForFighter(const std::string& fighterId) const;
    void decrementActions() { --actionsRemaining_; }

private:
    std::map<std::string, int> remainingMovementPoints_;
    std::map<std::string, int> movedThisManeuver_;
    std::vector<std::string> finishedFighters_;
    bool isSpaceOccupiedByEnemy(int spaceId) const;
    bool isSpaceOccupiedByAlly(int spaceId, const std::string& excludeFighterId) const;
    bool isSpaceOccupiedByAny(int spaceId) const;
    Player& playerByIndex(int index);
    const Player& playerByIndex(int index) const;
    Player& ownerOfFighterMutable(const std::string& fighterId);
    Player& opponentOf(const Player& player);
    const Player& opponentOf(const Player& player) const;
    bool isCardPlayableBy(const Card& card, const Fighter& fighter, const Player& player) const;
    bool canAttackTarget(const Fighter& attacker, const Fighter& defender) const;
    bool isSpaceOccupied(int spaceId) const;
    bool isSpaceOccupiedByCurrentEnemy(int spaceId) const;
    void advanceTurn();
    void drawCard(Player& player);
    void fatigue(Player& player);
    void placeSidekicks(Player& player);
    void checkDefeatedFighters();
    void checkWinner();
    void shuffleDeck(Player& player);
    int countLivingSistersInZoneWith(int spaceId) const;
    void moveFighterIgnoringDistance(Fighter& fighter, int destinationSpace);
    std::vector<int> reachableForPlayerFighter(int playerIndex, const std::string& fighterId, int maxSteps) const;
    void queueOptionalMovement(int playerIndex, const std::string& fighterId, int maxSteps, const std::string& source);
    std::vector<int> freeSpacesSharingHeroZone(const Player& player) const;
    std::vector<int> valuesOnCard(const Card& card) const;
    bool cardEffectsProtectedBySherlock(const Card& card, const Player& owner) const;
    void resolveCombatEffectAfterDamage(const Card& card,
                                        Player& cardPlayer,
                                        Fighter& cardFighter,
                                        Player& opposingPlayer,
                                        Fighter& opposingFighter,
                                        bool cardPlayerWon,
                                        int directDamage);

    Board board_;
    bool isFighterFinished(const std::string& fighterId) const;
    std::vector<Player> players_;
    int currentPlayerIndex_;
    int actionsRemaining_;
    int turnNumber_;
    bool started_;
    bool gameOver_;
    bool draculaAbilityUsed_;
    std::string winnerName_;
    std::mt19937 random_;
    int pendingMovementPoints_;
    int maxMovementPoints_; 
    std::deque<PendingMovementChoice> pendingOptionalMovements_;
    std::map<int, int> computeReachableWithCost(int start, int maxSteps, const std::string& fighterId) const;
};

}  // namespace unmatched
