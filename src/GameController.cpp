#include "unmatched/GameController.hpp"
#include "unmatched/GameExceptions.hpp"
#include <algorithm>
#include <numeric>
#include <random>
#include <utility>

namespace unmatched {

GameController::GameController()
    : board_(BoardFactory().createBaskervilleManor()),
      currentPlayerIndex_(0),
      actionsRemaining_(2),
      turnNumber_(1),
      started_(false),
      gameOver_(false),
      draculaAbilityUsed_(false),
      random_(std::random_device{}()),
      pendingMovementPoints_(0),
      maxMovementPoints_(0) {}


void GameController::startNewGame(int playerOneAge, int playerTwoAge, HeroKind youngerHero, int youngerStartSlot) {
    if (playerOneAge <= 0 || playerTwoAge <= 0) {
        throw InvalidSetup("Ages must be positive numbers.");
    }
    if (youngerStartSlot != 1 && youngerStartSlot != 2) {
        throw InvalidSetup("Start slot must be 1 or 2.");
    }

    HeroKind olderHero = (youngerHero == HeroKind::Dracula) ? HeroKind::Sherlock : HeroKind::Dracula;
    int youngerIndex = (playerOneAge <= playerTwoAge) ? 0 : 1;
    HeroKind playerOneHero = (youngerIndex == 0) ? youngerHero : olderHero;
    HeroKind playerTwoHero = (youngerIndex == 1) ? youngerHero : olderHero;

    players_.clear();
    players_.emplace_back(0, "Player 1", playerOneAge, playerOneHero);
    players_.emplace_back(1, "Player 2", playerTwoAge, playerTwoHero);

    FighterFactory fighterFactory;
    DeckFactory deckFactory;
    for (auto& player : players_) {
        player.fighters() = fighterFactory.createFighters(player.hero());
        player.deck() = deckFactory.createDeck(player.hero());
        shuffleDeck(player);
    }

    int olderStartSlot = (youngerStartSlot == 1) ? 2 : 1;
    int youngerSpace = board_.startSpaceForSlot(youngerStartSlot);
    int olderSpace = board_.startSpaceForSlot(olderStartSlot);
    players_[youngerIndex].heroFighter().placeAt(youngerSpace);
    players_[1 - youngerIndex].heroFighter().placeAt(olderSpace);

    currentPlayerIndex_ = youngerIndex;
    placeSidekicks(currentPlayer());
    placeSidekicks(opponentPlayer());

    for (int i = 0; i < 5; ++i) {
        drawCard(players_[0]);
        drawCard(players_[1]);
    }

    actionsRemaining_ = 2;
    turnNumber_ = 1;
    started_ = true;
    gameOver_ = false;
    draculaAbilityUsed_ = false;
    winnerName_.clear();
    pendingMovementPoints_ = 0;
    movedThisManeuver_.clear();
    pendingOptionalMovements_.clear();
}


bool GameController::started() const { return started_; }
bool GameController::gameOver() const { return gameOver_; }
const std::string& GameController::winnerName() const { return winnerName_; }
const Board& GameController::board() const { return board_; }

Player& GameController::currentPlayer() { return playerByIndex(currentPlayerIndex_); }
const Player& GameController::currentPlayer() const { return playerByIndex(currentPlayerIndex_); }
Player& GameController::opponentPlayer() { return playerByIndex(opponentPlayerIndex()); }
const Player& GameController::opponentPlayer() const { return playerByIndex(opponentPlayerIndex()); }
const std::vector<Player>& GameController::players() const { return players_; }
int GameController::currentPlayerIndex() const { return currentPlayerIndex_; }
int GameController::opponentPlayerIndex() const { return (currentPlayerIndex_ == 0) ? 1 : 0; }
int GameController::actionsRemaining() const { return actionsRemaining_; }
int GameController::turnNumber() const { return turnNumber_; }

bool GameController::draculaAbilityAvailable() const {
    if (!started_ || gameOver_ || draculaAbilityUsed_ || currentPlayer().hero() != HeroKind::Dracula) {
        return false;
    }
    const Fighter& dracula = currentPlayer().heroFighter();
    if (dracula.defeated()) return false;
    for (const auto& player : players_) {
        for (const auto& fighter : player.fighters()) {
            if (!fighter.defeated() && fighter.id() != dracula.id() &&
                board_.areAdjacentForCombat(dracula.spaceId(), fighter.spaceId())) {
                return true;
            }
        }
    }
    return false;
}

Player& GameController::playerByIndex(int index) {
    if (index < 0 || index >= static_cast<int>(players_.size())) {
        throw RuleViolation("Invalid player index.");
    }
    return players_[index];
}

const Player& GameController::playerByIndex(int index) const {
    if (index < 0 || index >= static_cast<int>(players_.size())) {
        throw RuleViolation("Invalid player index.");
    }
    return players_[index];
}

bool GameController::isSpaceOccupied(int spaceId) const {
    for (const auto& player : players_) {
        for (const auto& fighter : player.fighters()) {
            if (!fighter.defeated() && fighter.spaceId() == spaceId) {
                return true;
            }
        }
    }
    return false;
}

bool GameController::isSpaceOccupiedByEnemy(int spaceId) const {
    for (const auto& fighter : opponentPlayer().fighters()) {
        if (!fighter.defeated() && fighter.spaceId() == spaceId) return true;
    }
    return false;
}

bool GameController::isSpaceOccupiedByAlly(int spaceId, const std::string& excludeFighterId) const {
    for (const auto& fighter : currentPlayer().fighters()) {
        if (fighter.id() == excludeFighterId) continue;
        if (!fighter.defeated() && fighter.spaceId() == spaceId) return true;
    }
    return false;
}

bool GameController::isSpaceOccupiedByAny(int spaceId) const {
    return isSpaceOccupied(spaceId);
}

bool GameController::isSpaceOccupiedByCurrentEnemy(int spaceId) const {
    for (const auto& fighter : opponentPlayer().fighters()) {
        if (!fighter.defeated() && fighter.spaceId() == spaceId) return true;
    }
    return false;
}

void GameController::drawCard(Player& player) {
    if (player.deck().empty()) {
        fatigue(player);
        return;
    }
    Card drawn = player.deck().back();
    player.deck().pop_back();
    player.addToHand(std::move(drawn));
}

void GameController::fatigue(Player& player) {
    for (auto& fighter : player.fighters()) {
        if (!fighter.defeated()) {
            fighter.damage(2);
        }
    }
    checkDefeatedFighters();
    checkWinner();
}

void GameController::placeSidekicks(Player& player) {
    std::vector<int> free = freeSpacesSharingHeroZone(player);
    for (auto& fighter : player.fighters()) {
        if (fighter.isHero()) continue;
        if (free.empty()) {
            throw InvalidSetup("Not enough free spaces for sidekick placement.");
        }
        int destination = free.front();
        free.erase(free.begin());
        fighter.placeAt(destination);
    }
}

void GameController::shuffleDeck(Player& player) {
    std::shuffle(player.deck().begin(), player.deck().end(), random_);
}

void GameController::checkDefeatedFighters() {
    for (auto& player : players_) {
        for (auto& fighter : player.fighters()) {
            if (fighter.defeated()) {
                fighter.removeFromBoard();
            }
        }
    }
}

void GameController::checkWinner() {
    if (gameOver_) return;
    for (const auto& player : players_) {
        if (player.heroFighter().defeated()) {
            const Player& winner = opponentOf(player);
            gameOver_ = true;
            winnerName_ = winner.name() + " (" + heroKindName(winner.hero()) + ")";
            return;
        }
    }
}

const Fighter* GameController::findFighterById(const std::string& fighterId) const {
    for (const auto& player : players_) {
        for (const auto& fighter : player.fighters()) {
            if (fighter.id() == fighterId) return &fighter;
        }
    }
    return nullptr;
}

Fighter* GameController::findFighterById(const std::string& fighterId) {
    for (auto& player : players_) {
        for (auto& fighter : player.fighters()) {
            if (fighter.id() == fighterId) return &fighter;
        }
    }
    return nullptr;
}

const Player* GameController::ownerOfFighter(const std::string& fighterId) const {
    for (const auto& player : players_) {
        for (const auto& fighter : player.fighters()) {
            if (fighter.id() == fighterId) return &player;
        }
    }
    return nullptr;
}

Player& GameController::ownerOfFighterMutable(const std::string& fighterId) {
    for (auto& player : players_) {
        for (auto& fighter : player.fighters()) {
            if (fighter.id() == fighterId) return player;
        }
    }
    throw RuleViolation("Unknown fighter owner.");
}

Player& GameController::opponentOf(const Player& player) {
    return playerByIndex(player.id() == 0 ? 1 : 0);
}

const Player& GameController::opponentOf(const Player& player) const {
    return playerByIndex(player.id() == 0 ? 1 : 0);
}

bool GameController::isCardPlayableBy(const Card& card, const Fighter& fighter, const Player& player) const {
    if (fighter.defeated()) return false;
    if (card.owner() == Character::Any) return true;
    return fighter.cardOwner() == card.owner() && player.hasLivingCharacter(card.owner());
}

bool GameController::canAttackTarget(const Fighter& attacker, const Fighter& defender) const {
    if (attacker.defeated() || defender.defeated()) return false;
    if (board_.areAdjacentForCombat(attacker.spaceId(), defender.spaceId())) return true;
    return attacker.range() == AttackRange::Ranged && board_.shareZone(attacker.spaceId(), defender.spaceId());
}

void GameController::drawCardForCurrentPlayer() {
    drawCard(currentPlayer());
}

void GameController::decrementActions() { --actionsRemaining_; }

void GameController::advanceTurn() {
    currentPlayerIndex_ = opponentPlayerIndex();
    actionsRemaining_ = 2;
    pendingMovementPoints_ = 0;
    movedThisManeuver_.clear();
    pendingOptionalMovements_.clear();
    draculaAbilityUsed_ = false;
    ++turnNumber_;
}

int GameController::countLivingSistersInZoneWith(int spaceId) const {
    int count = 0;
    for (const auto& player : players_) {
        if (player.hero() != HeroKind::Dracula) continue;
        for (const auto& fighter : player.fighters()) {
            if (!fighter.defeated() && fighter.cardOwner() == Character::Sister &&
                board_.shareZone(fighter.spaceId(), spaceId)) {
                ++count;
            }
        }
    }
    return count;
}

void GameController::moveFighterIgnoringDistance(Fighter& fighter, int destinationSpace) {
    if (!board_.contains(destinationSpace)) {
        throw RuleViolation("Destination does not exist.");
    }
    if (isSpaceOccupied(destinationSpace)) {
        throw RuleViolation("Destination is occupied.");
    }
    fighter.placeAt(destinationSpace);
}

std::vector<int> GameController::freeSpacesSharingHeroZone(const Player& player) const {
    std::vector<int> result;
    int heroSpace = player.heroFighter().spaceId();
    for (int candidate : board_.spacesSharingAnyZone(heroSpace)) {
        if (!isSpaceOccupied(candidate)) {
            result.push_back(candidate);
        }
    }
    return result;
}

std::vector<int> GameController::valuesOnCard(const Card& card) const {
    std::vector<int> values;
    if (card.attack() >= 0) values.push_back(card.attack());
    if (card.defense() >= 0) values.push_back(card.defense());
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    return values;
}

bool GameController::cardEffectsProtectedBySherlock(const Card& card, const Player& owner) const {
    if (owner.hero() != HeroKind::Sherlock) return false;
    return card.owner() == Character::Sherlock || card.owner() == Character::Watson;
}

bool GameController::isFighterFinished(const std::string& fighterId) const {
    return std::find(finishedFighters_.begin(), finishedFighters_.end(), fighterId) != finishedFighters_.end();
}

void GameController::queueOptionalMovement(int playerIndex, const std::string& fighterId,
                                           int maxSteps, const std::string& source) {
    if (maxSteps <= 0) return;
    const Player& player = playerByIndex(playerIndex);
    const Fighter& fighter = player.fighterById(fighterId);
    if (fighter.defeated()) return;
    PendingMovementChoice choice{playerIndex, fighterId, maxSteps, source};
    pendingOptionalMovements_.push_back(std::move(choice));
}


void GameController::beginManeuver(int boostHandIndex) {
    if (actionsRemaining_ <= 0) throw RuleViolation("No actions remain.");
    if (!pendingOptionalMovements_.empty()) throw RuleViolation("Resolve pending movement.");

    Player& player = currentPlayer();
    drawCard(player);
    if (gameOver_) { remainingMovementPoints_.clear(); return; }

    int boost = 0;
    if (boostHandIndex != -1) {
        Card boosted = player.removeCardFromHand(boostHandIndex);
        boost = boosted.boost();
        player.addToDiscard(std::move(boosted));
    }

    remainingMovementPoints_.clear();
    for (auto& fighter : player.fighters()) {
        if (!fighter.defeated()) {
            int move = fighter.move() + boost;
            remainingMovementPoints_[fighter.id()] = move;
        }
    }

    movedThisManeuver_.clear();
    finishedFighters_.clear();
}

std::vector<std::string> GameController::movableCurrentFighterIds() const {
    std::vector<std::string> result;
    for (const auto& kv : remainingMovementPoints_) {
        const std::string& id = kv.first;
        int remaining = kv.second;
        if (remaining <= 0) continue;
        if (isFighterFinished(id)) continue;
        const Fighter* fighter = findFighterById(id);
        if (!fighter || fighter->defeated()) continue;
        auto destinations = reachableDestinationsFor(id);
        bool hasValid = false;
        for (int d : destinations) {
            if (d != fighter->spaceId()) { hasValid = true; break; }
        }
        if (hasValid) result.push_back(id);
    }
    return result;
}

std::vector<int> GameController::reachableDestinationsFor(const std::string& fighterId) const {
    const Fighter* fighter = findFighterById(fighterId);
    if (!fighter || fighter->defeated()) return {};
    auto it = remainingMovementPoints_.find(fighterId);
    if (it == remainingMovementPoints_.end() || it->second <= 0) return {};
    int maxSteps = it->second;

    auto costMap = computeReachableWithCost(fighter->spaceId(), maxSteps, fighterId);
    std::vector<int> result;
    for (const auto& pair : costMap) {
        int space = pair.first;
        if (space == fighter->spaceId()) continue;
        if (isSpaceOccupiedByAlly(space, fighterId)) continue;
        result.push_back(space);
    }
    std::sort(result.begin(), result.end());
    return result;
}

void GameController::moveCurrentFighter(const std::string& fighterId, int destinationSpace) {
    Fighter& fighter = currentPlayer().fighterById(fighterId);
    if (isFighterFinished(fighterId)) {
        throw RuleViolation("This fighter has finished its movement.");
    }
    auto it = remainingMovementPoints_.find(fighterId);
    if (it == remainingMovementPoints_.end() || it->second <= 0) {
        throw RuleViolation("No movement points for this fighter.");
    }
    int cost = getMovementCost(fighterId, destinationSpace);
    if (cost <= 0 || cost > it->second) {
        throw RuleViolation("Destination is not reachable or invalid.");
    }
    fighter.placeAt(destinationSpace);
    it->second -= cost;
}

void GameController::finishManeuver() {
    remainingMovementPoints_.clear();
    movedThisManeuver_.clear();
    finishedFighters_.clear();
    --actionsRemaining_;
    endTurnIfNeeded();
}

void GameController::finishCurrentFighter(const std::string& fighterId) {
    finishedFighters_.push_back(fighterId);
}

int GameController::getMovementCost(const std::string& fighterId, int destinationSpace) const {
    const Fighter* fighter = findFighterById(fighterId);
    if (!fighter || fighter->defeated()) return -1;
    auto it = remainingMovementPoints_.find(fighterId);
    if (it == remainingMovementPoints_.end() || it->second <= 0) return -1;
    if (destinationSpace == fighter->spaceId()) return 0;
    auto costMap = computeReachableWithCost(fighter->spaceId(), it->second, fighterId);
    if (costMap.find(destinationSpace) == costMap.end()) return -1;
    if (isSpaceOccupiedByAlly(destinationSpace, fighterId)) return -1;
    return costMap[destinationSpace];
}

int GameController::remainingMovementForFighter(const std::string& fighterId) const {
    auto it = remainingMovementPoints_.find(fighterId);
    return (it != remainingMovementPoints_.end()) ? it->second : -1;
}

std::map<int, int> GameController::computeReachableWithCost(int start, int maxSteps,
                                                            const std::string& fighterId) const {
    std::map<int, int> cost;
    std::deque<int> dq;
    cost[start] = 0;
    dq.push_back(start);

    while (!dq.empty()) {
        int current = dq.front();
        dq.pop_front();
        int currentCost = cost[current];
        if (currentCost >= maxSteps) continue;

        for (int neighbor : board_.directNeighbors(current)) {
            if (isSpaceOccupiedByEnemy(neighbor)) continue;
            int moveCost = 1;
            if (isSpaceOccupiedByAlly(neighbor, fighterId)) {
                moveCost = 2;
            }
            int newCost = currentCost + moveCost;
            if (newCost > maxSteps) continue;
            if (cost.find(neighbor) != cost.end() && cost[neighbor] <= newCost) continue;
            cost[neighbor] = newCost;
            if (moveCost == 1) dq.push_front(neighbor);
            else dq.push_back(neighbor);
        }
    }
    return cost;
}

int GameController::pendingMovementPoints() const {
    int total = 0;
    for (const auto& kv : remainingMovementPoints_) {
        if (kv.second > 0) total += kv.second;
    }
    return total;
}

int GameController::maxMovementPoints() const {
    int total = 0;
    for (const auto& fighter : currentPlayer().fighters()) {
        if (!fighter.defeated()) {
            total += fighter.move();
        }
    }
    return total;
}
} // namespace unmatched
