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

} // namespace unmatched
