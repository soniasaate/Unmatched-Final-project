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
#include <random>

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


struct PendingDeckTopSelection {
    int playerIndex = -1;
    int remaining = 0;
};


struct PendingFogChoice {
    int chooserPlayerIndex = -1;
    int invisibleManOwnerIndex = -1;
    std::string invisibleManFighterId;
    std::vector<int> excludedTokenIndexes;
    int maxSteps = 0;
};

struct PendingFogTeleport {
    int fighterOwnerPlayerIndex = -1;
    std::string fighterId;
};

struct PendingConfoundDecision {
    int deciderPlayerIndex = -1;
    int cardOwnerPlayerIndex = -1;
};

class GameController {
public:
    GameController();

    void startNewGame(int playerOneAge, int playerTwoAge, 
                  std::unique_ptr<Fighter> hero1, 
                  std::unique_ptr<Fighter> hero2, 
                  int youngerStartSlot);
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

    
    bool currentTurnStartedOnFog() const { return currentTurnStartedOnFog_; }

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
    const SchemeChoice& currentSchemeChoice() const { return currentSchemeChoice_; }

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
    void forceEndActionsThisTurn(); 
    bool currentPlayerMustDiscardToLimit() const;

    std::map<int, std::string> occupantTokens() const;
    const std::map<std::string, int>& movedThisManeuver() const { return movedThisManeuver_; }
    const Fighter* findFighterById(const std::string& fighterId) const;
    Fighter* findFighterById(const std::string& fighterId);
    const Player* ownerOfFighter(const std::string& fighterId) const;
    Player& mutableOwnerOfFighter(const std::string& fighterId);
    Player& mutableOpponentOfFighter(const std::string& fighterId);
    void finishCurrentFighter(const std::string& fighterId);
    int getMovementCost(const std::string& fighterId, int destinationSpace) const;
    int remainingMovementForFighter(const std::string& fighterId) const;
    void decrementActions() { if (actionsRemaining_ > 0) --actionsRemaining_; }

    bool isSpaceOccupied(int spaceId) const;
    void drawCard(Player& player);
    void queueOptionalMovement(int playerIndex, const std::string& fighterId, int maxSteps, const std::string& source);
    int getRandomInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(random_);
    }
    void addAction(int count = 1);

    
    void placeVanishedInvisibleMan(int spaceId);
    std::vector<int> getValidPlacementSpacesForVanished() const;
    bool hasPendingVanishedPlacement() const { return pendingVanishedPlacement_; }
    void clearPendingVanishedPlacement() { pendingVanishedPlacement_ = false; vanishedPlayerIndex_ = -1; }
    void setPendingVanishedPlacement(bool value, int playerIndex = -1) {
        pendingVanishedPlacement_ = value;
        vanishedPlayerIndex_ = value ? playerIndex : -1;
    }
    int vanishedPlayerIndex() const { return vanishedPlayerIndex_; }

    const std::string& getStudyMethodsHandInfo() const { return studyMethodsHandInfo_; }
    void clearStudyMethodsHandInfo() { studyMethodsHandInfo_.clear(); }
    void setConfoundSchemeCardIndex(int index) { confoundSchemeCardIndex_ = index; }
    int getConfoundSchemeCardIndex() const { return confoundSchemeCardIndex_; }
    void clearConfoundData() { confoundSchemeCardIndex_ = -1; }
    void setConfirmSuspicionHandInfo(const std::string& info);
    const std::string& getConfirmSuspicionHandInfo() const { return confirmSuspicionHandInfo_; }
    void clearConfirmSuspicionHandInfo() { confirmSuspicionHandInfo_.clear(); }

    
    void queueDeckTopSelection(int playerIndex, int count);
    bool hasPendingDeckTopSelection() const { return pendingDeckTopSelection_.has_value(); }
    const PendingDeckTopSelection& pendingDeckTopSelection() const { return *pendingDeckTopSelection_; }
    void selectCardForDeckTop(int handIndex);


    void queuePendingFogChoice(int chooserPlayerIndex, const std::string& invisibleManFighterId,
                               std::vector<int> excludedTokenIndexes, int maxSteps);
    bool hasPendingFogChoice() const { return !pendingFogChoices_.empty(); }
    const PendingFogChoice& pendingFogChoice() const { return pendingFogChoices_.front(); }
    std::vector<int> fogTokenOptionsForPendingChoice() const;
    std::vector<int> fogTokenReachableSpaces(const std::string& fighterId, int tokenIndex, int maxSteps) const;
    void resolvePendingFogChoice(int tokenIndex, int destinationSpace);

    
    void queueFogTeleport(int ownerPlayerIndex, const std::string& fighterId);
    bool hasPendingFogTeleport() const { return pendingFogTeleport_.has_value(); }
    const PendingFogTeleport& pendingFogTeleport() const { return *pendingFogTeleport_; }
    std::vector<int> fogTeleportDestinations() const;
    void resolveFogTeleport(int destinationSpace);


    void queueConfoundDecision(int cardOwnerPlayerIndex);
    bool hasPendingConfoundDecision() const { return pendingConfoundDecision_.has_value(); }
    const PendingConfoundDecision& pendingConfoundDecision() const { return *pendingConfoundDecision_; }
    void resolveConfoundDecision(bool willDiscard);
    bool hasPendingConfoundDiscardSelection() const { return pendingConfoundDiscardPlayerIndex_ != -1; }
    int pendingConfoundDiscardPlayerIndex() const { return pendingConfoundDiscardPlayerIndex_; }
    void resolveConfoundDiscardSelection(int handIndex);

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
    bool isCardPlayableBy(const Card& card, const Fighter& fighter) const;
    bool canAttackTarget(const Fighter& attacker, const Fighter& defender) const;
    bool isSpaceOccupiedByCurrentEnemy(int spaceId) const;
    void advanceTurn();
    void fatigue(Player& player);
    void placeSidekicks(Player& player);
    void checkDefeatedFighters();
    void checkWinner();
    void shuffleDeck(Player& player);
    int countLivingSistersInZoneWith(int spaceId) const;
    void moveFighterIgnoringDistance(Fighter& fighter, int destinationSpace);
    std::vector<int> reachableForPlayerFighter(int playerIndex, const std::string& fighterId, int maxSteps) const;
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
    void refreshTurnStartFogStatus();

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
    bool pendingVanishedPlacement_ = false;
    int vanishedPlayerIndex_ = -1;
    std::string studyMethodsHandInfo_;
    int confoundSchemeCardIndex_ = -1;
    std::string confirmSuspicionHandInfo_;
    SchemeChoice currentSchemeChoice_;
    bool currentTurnStartedOnFog_ = false;

    std::optional<PendingDeckTopSelection> pendingDeckTopSelection_;
    std::deque<PendingFogChoice> pendingFogChoices_;
    std::optional<PendingFogTeleport> pendingFogTeleport_;
    std::optional<PendingConfoundDecision> pendingConfoundDecision_;
    int pendingConfoundDiscardPlayerIndex_ = -1;

    std::map<int, int> computeReachableWithCost(int start, int maxSteps, const std::string& fighterId) const;
};

} // namespace unmatched
