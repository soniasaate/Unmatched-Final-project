#include "unmatched/GameController.hpp"
#include "unmatched/GameExceptions.hpp"
#include "unmatched/Dracula.hpp"
#include "unmatched/Sherlock.hpp"
#include "unmatched/InvisibleMan.hpp"
#include "unmatched/Serialization.hpp"

#include <algorithm>
#include <functional>
#include <iterator>
#include <numeric>
#include <set>
#include <sstream>
#include <utility>
#include <queue>
#include <fstream>

namespace unmatched {

json pendingMovementChoiceToJson(const PendingMovementChoice& choice) {
    json j;
    j["playerIndex"] = choice.playerIndex;
    j["fighterId"] = choice.fighterId;
    j["maxSteps"] = choice.maxSteps;
    j["source"] = choice.source;
    return j;
}

PendingMovementChoice jsonToPendingMovementChoice(const json& j) {
    PendingMovementChoice choice;
    choice.playerIndex = j["playerIndex"].get<int>();
    choice.fighterId = j["fighterId"].get<std::string>();
    choice.maxSteps = j["maxSteps"].get<int>();
    choice.source = j["source"].get<std::string>();
    return choice;
}

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
      maxMovementPoints_(0),
      pendingVanishedPlacement_(false) {}

void GameController::startNewGame(int playerOneAge, int playerTwoAge,
                                  std::unique_ptr<Fighter> hero1,
                                  std::unique_ptr<Fighter> hero2,
                                  int youngerStartSlot) {
    if (playerOneAge <= 0 || playerTwoAge <= 0) {
        throw InvalidSetup("Ages must be positive numbers.");
    }
    if (youngerStartSlot != 1 && youngerStartSlot != 2) {
        throw InvalidSetup("Start slot must be 1 or 2.");
    }

    int youngerIndex;
    if (playerOneAge == playerTwoAge) {
        std::uniform_int_distribution<int> dist(0, 1);
        youngerIndex = dist(random_);
    } else {
        youngerIndex = playerOneAge < playerTwoAge ? 0 : 1;
    }

    players_.clear();
    players_.emplace_back(0, "Player 1", playerOneAge);
    players_.emplace_back(1, "Player 2", playerTwoAge);

    auto& player1 = players_.at(0);
    player1.fighters().push_back(std::move(hero1));
    for (auto& sidekick : player1.fighters()[0]->createSidekicks()) {
        player1.fighters().push_back(std::move(sidekick));
    }
    player1.deck() = player1.fighters()[0]->createDeck();

    auto& player2 = players_.at(1);
    player2.fighters().push_back(std::move(hero2));
    for (auto& sidekick : player2.fighters()[0]->createSidekicks()) {
        player2.fighters().push_back(std::move(sidekick));
    }
    player2.deck() = player2.fighters()[0]->createDeck();

    for (auto* player : {&player1, &player2}) {
        if (auto* sherlock = dynamic_cast<Sherlock*>((*player).fighters()[0].get())) {
            for (auto& f : (*player).fighters()) {
                if (f->id() == "watson") {
                    sherlock->setWatson(f.get());
                    break;
                }
            }
        }
    }

    shuffleDeck(player1);
    shuffleDeck(player2);
    int olderStartSlot = youngerStartSlot == 1 ? 2 : 1;
    int youngerSpace = board_.startSpaceForSlot(youngerStartSlot);
    int olderSpace = board_.startSpaceForSlot(olderStartSlot);

    players_.at(static_cast<std::size_t>(youngerIndex)).heroFighter().placeAt(youngerSpace);
    players_.at(static_cast<std::size_t>(1 - youngerIndex)).heroFighter().placeAt(olderSpace);

    if (auto* invisible = dynamic_cast<InvisibleMan*>(players_.at(0).fighters()[0].get())) {
        int heroSpace = invisible->spaceId();
        std::vector<int> fogSpaces;
        for (const auto& space : board_.spaces()) {
            if (board_.shareZone(heroSpace, space.id()) &&
                !isSpaceOccupied(space.id()) &&
                space.id() != heroSpace) {
                fogSpaces.push_back(space.id());
            }
        }
        if (fogSpaces.size() >= 3) {
            invisible->placeFogTokens({fogSpaces[0], fogSpaces[1], fogSpaces[2]});
        }
    }

    if (auto* invisible = dynamic_cast<InvisibleMan*>(players_.at(1).fighters()[0].get())) {
        int heroSpace = invisible->spaceId();
        std::vector<int> fogSpaces;
        for (const auto& space : board_.spaces()) {
            if (board_.shareZone(heroSpace, space.id()) &&
                !isSpaceOccupied(space.id()) &&
                space.id() != heroSpace) {
                fogSpaces.push_back(space.id());
            }
        }
        if (fogSpaces.size() >= 3) {
            invisible->placeFogTokens({fogSpaces[0], fogSpaces[1], fogSpaces[2]});
        }
    }

    currentPlayerIndex_ = youngerIndex;
    for (auto& player : players_) {
        for (auto& fighter : player.fighters()) {
            if (!fighter->defeated()) {
                fighter->onTurnStart();
            }
        }
    }
    placeSidekicks(currentPlayer());
    placeSidekicks(opponentPlayer());

    for (int i = 0; i < 5; ++i) {
        drawCard(players_.at(0));
        drawCard(players_.at(1));
    }

    actionsRemaining_ = 2;
    turnNumber_ = 1;
    clearUndoStack();
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

Player& GameController::currentPlayer() {
    return playerByIndex(currentPlayerIndex_);
}
const Player& GameController::currentPlayer() const {
    return playerByIndex(currentPlayerIndex_);
}
Player& GameController::opponentPlayer() {
    return playerByIndex(opponentPlayerIndex());
}
const Player& GameController::opponentPlayer() const {
    return playerByIndex(opponentPlayerIndex());
}
const std::vector<Player>& GameController::players() const { return players_; }
int GameController::currentPlayerIndex() const { return currentPlayerIndex_; }
int GameController::opponentPlayerIndex() const {
    return currentPlayerIndex_ == 0 ? 1 : 0;
}
int GameController::actionsRemaining() const { return actionsRemaining_; }
int GameController::turnNumber() const { return turnNumber_; }

bool GameController::draculaAbilityAvailable() const {
    if (!started_ || gameOver_ || draculaAbilityUsed_) return false;
    const Fighter& hero = currentPlayer().heroFighter();
    if (dynamic_cast<const Dracula*>(&hero) == nullptr) return false;
    if (hero.defeated()) return false;
    for (const auto& player : players_) {
        for (const auto& fighter : player.fighters()) {
            if (!fighter->defeated() && fighter->id() != hero.id() &&
                board_.areAdjacentForCombat(hero.spaceId(), fighter->spaceId())) {
                return true;
            }
        }
    }
    return false;
}

bool GameController::isSpaceOccupiedByEnemy(int spaceId) const {
    for (const auto& fighter : opponentPlayer().fighters()) {
        if (!fighter->defeated() && fighter->spaceId() == spaceId) return true;
    }
    return false;
}

bool GameController::isSpaceOccupiedByAlly(int spaceId, const std::string& excludeFighterId) const {
    for (const auto& fighter : currentPlayer().fighters()) {
        if (fighter->id() == excludeFighterId) continue;
        if (!fighter->defeated() && fighter->spaceId() == spaceId) return true;
    }
    return false;
}

bool GameController::isSpaceOccupiedByAny(int spaceId) const {
    return isSpaceOccupied(spaceId);
}

void GameController::drawCardForCurrentPlayer() {
    drawCard(currentPlayer());
}

void GameController::beginManeuver(int boostHandIndex) {
    pushUndoState();
    try {
        if (actionsRemaining_ <= 0) throw RuleViolation("No actions remain.");
        if (!pendingOptionalMovements_.empty()) throw RuleViolation("Resolve pending movement.");

        Player& player = currentPlayer();
        drawCard(player);
        if (gameOver_) { remainingMovementPoints_.clear(); return; }

        int boost = 0;
        if (boostHandIndex != -1) {
            Card boosted = player.removeCardFromHand(boostHandIndex);
            boost = boosted.getBoost();
            player.addToDiscard(std::move(boosted));
        }

        remainingMovementPoints_.clear();
        for (auto& fighter : player.fighters()) {
            if (!fighter->defeated()) {
                int move = fighter->move() + boost;
                remainingMovementPoints_[fighter->id()] = move;
            }
        }

        movedThisManeuver_.clear();
        finishedFighters_.clear();
    } catch (...) {
        if (!undoStack_.empty()) undoStack_.pop_back();
        throw;
    }
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
            if (d != fighter->spaceId()) {
                hasValid = true;
                break;
            }
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

bool GameController::isFighterFinished(const std::string& fighterId) const {
    return std::find(finishedFighters_.begin(), finishedFighters_.end(), fighterId)
           != finishedFighters_.end();
}

void GameController::moveCurrentFighter(const std::string& fighterId, int destinationSpace) {
    pushUndoState();
    try {
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
    } catch (...) {
        if (!undoStack_.empty()) undoStack_.pop_back();
        throw;
    }
}

void GameController::finishManeuver() {
    remainingMovementPoints_.clear();
    movedThisManeuver_.clear();
    finishedFighters_.clear();
    --actionsRemaining_;
    endTurnIfNeeded();
}

std::vector<std::string> GameController::legalAttackers() const {
    std::vector<std::string> result;
    for (const auto* fighter : currentPlayer().aliveFighters()) {
        if (!legalAttackCardsFor(fighter->id()).empty() && !legalTargetsFor(fighter->id()).empty()) {
            result.push_back(fighter->id());
        }
    }
    return result;
}

std::vector<std::string> GameController::legalTargetsFor(const std::string& attackerId) const {
    const Fighter& attacker = currentPlayer().fighterById(attackerId);
    std::vector<std::string> result;
    for (const auto* target : opponentPlayer().aliveFighters()) {
        if (canAttackTarget(attacker, *target)) {
            result.push_back(target->id());
        }
    }
    return result;
}

std::vector<int> GameController::legalAttackCardsFor(const std::string& attackerId) const {
    const Fighter& attacker = currentPlayer().fighterById(attackerId);
    std::vector<int> result;
    const auto& hand = currentPlayer().hand();
    for (int i = 0; i < static_cast<int>(hand.size()); ++i) {
        const Card& card = hand.at(static_cast<std::size_t>(i));
        if (card.canAttack() && isCardPlayableBy(card, attacker)) {
            result.push_back(i);
        }
    }
    return result;
}

std::vector<int> GameController::legalDefenseCardsFor(const std::string& defenderId) const {
    const Fighter& defender = opponentPlayer().fighterById(defenderId);
    std::vector<int> result;
    const auto& hand = opponentPlayer().hand();
    for (int i = 0; i < static_cast<int>(hand.size()); ++i) {
        const Card& card = hand.at(static_cast<std::size_t>(i));
        if (card.canDefend() && isCardPlayableBy(card, defender)) {
            result.push_back(i);
        }
    }
    return result;
}

void GameController::resolveAttack(const std::string& attackerId,
                                   const std::string& defenderId,
                                   int attackCardIndex,
                                   int defenseCardIndex,
                                   const std::vector<int>& beastFormBoostCardIndexes,
                                   int predictedElementaryValue) {
    pushUndoState();
    try {
        if (actionsRemaining_ <= 0) {
            throw RuleViolation("No actions remain this turn.");
        }
        if (!pendingOptionalMovements_.empty()) {
            throw RuleViolation("Resolve the pending card movement before starting another action.");
        }

        Player& attackerPlayer = currentPlayer();
        Player& defenderPlayer = opponentPlayer();
        Fighter& attacker = attackerPlayer.fighterById(attackerId);
        Fighter& defender = defenderPlayer.fighterById(defenderId);

        if (!canAttackTarget(attacker, defender)) {
            throw RuleViolation("Target is not in range.");
        }

        auto legalAttackCards = legalAttackCardsFor(attackerId);
        if (std::find(legalAttackCards.begin(), legalAttackCards.end(), attackCardIndex) == legalAttackCards.end()) {
            throw RuleViolation("Selected attack card is not legal for this fighter.");
        }

        if (defenseCardIndex != -1) {
            auto legalDefenseCards = legalDefenseCardsFor(defenderId);
            if (std::find(legalDefenseCards.begin(), legalDefenseCards.end(), defenseCardIndex) == legalDefenseCards.end()) {
                throw RuleViolation("Selected defense card is not legal for this fighter.");
            }
        }

        const Card& attackCardPreview = attackerPlayer.hand().at(static_cast<std::size_t>(attackCardIndex));
        if (!beastFormBoostCardIndexes.empty() && attackCardPreview.getTitle() != "BEASTFORM") {
            throw RuleViolation("Only Beast Form can discard cards for extra attack.");
        }

        std::set<int> uniqueBeastBoosts;
        for (int index : beastFormBoostCardIndexes) {
            if (index < 0 || index >= static_cast<int>(attackerPlayer.hand().size())) {
                throw RuleViolation("Invalid Beast Form discard index.");
            }
            if (index == attackCardIndex) {
                throw RuleViolation("Beast Form cannot discard the attack card itself.");
            }
            if (!uniqueBeastBoosts.insert(index).second) {
                throw RuleViolation("A Beast Form discard card was selected more than once.");
            }
        }

        Card attackCard = attackerPlayer.removeCardFromHand(attackCardIndex);
        std::vector<int> sortedBeastBoosts(beastFormBoostCardIndexes.begin(), beastFormBoostCardIndexes.end());
        std::sort(sortedBeastBoosts.begin(), sortedBeastBoosts.end(), std::greater<int>());
        int beastFormBonus = 0;
        for (int originalIndex : sortedBeastBoosts) {
            int adjustedIndex = originalIndex > attackCardIndex ? originalIndex - 1 : originalIndex;
            Card burned = attackerPlayer.removeCardFromHand(adjustedIndex);
            ++beastFormBonus;
            attackerPlayer.addToDiscard(std::move(burned));
        }

        std::optional<Card> defenseCard;
        if (defenseCardIndex != -1) {
            defenseCard = defenderPlayer.removeCardFromHand(defenseCardIndex);
        }

        int attackValue = std::max(0, attackCard.getAttack()) + beastFormBonus;
        int defenseValue = defenseCard.has_value() ? std::max(0, defenseCard->getDefense()) : 0;

        if (defenseCard.has_value()) {
            auto* invisible = dynamic_cast<InvisibleMan*>(&defender);
            if (invisible && invisible->isOnFog(defender.spaceId())) {
                defenseValue += 1;
            }
        }
        if (attackCard.getTitle() == "DEDUCE STRATEGY") {
            defenseValue = attackCard.getBoost();
        }
        if (defenseCard.has_value() && defenseCard->getTitle() == "DEDUCE STRATEGY") {
            attackValue = defenseCard->getBoost();
        }

        bool attackEffectsCanceled = false;
        bool defenseEffectsCanceled = false;

        if (attackCard.getTitle() == "FEINT" && defenseCard.has_value() &&
            !cardEffectsProtectedBySherlock(*defenseCard, defenderPlayer)) {
            defenseEffectsCanceled = true;
        }
        if (defenseCard.has_value() && defenseCard->getTitle() == "FEINT" &&
            !cardEffectsProtectedBySherlock(attackCard, attackerPlayer)) {
            attackEffectsCanceled = true;
        }

        if (defenseCard.has_value() && defenseCard->getTitle() == "LOOK INTO MY EYES") {
            defenseValue += std::max(0, attackCard.getBoost());
        }

        if (defenseCard.has_value()) {
            defenseCard->applyEffect(defender, attacker, *this, defenseValue, attackValue);
        }

        attackCard.applyEffect(attacker, defender, *this, attackValue, defenseValue);

        if (defenseCard.has_value() && !defenseEffectsCanceled) {
            if (defenseCard->getTitle() == "ELEMENTARY") {
                if (!cardEffectsProtectedBySherlock(attackCard, attackerPlayer)) {
                    if (predictedElementaryValue != -1 && attackCard.getAttack() == predictedElementaryValue) {
                        attackValue = 0;
                        attackEffectsCanceled = true;
                    }
                }
            }
        }

        int directDamage = std::max(0, attackValue - defenseValue);
        if (directDamage > 0) {
            defender.damage(directDamage);
        }

        bool attackerWon = directDamage > 0;

        if (defenseCard.has_value() && !defenseEffectsCanceled) {
            resolveCombatEffectAfterDamage(*defenseCard, defenderPlayer, defender,
                                           attackerPlayer, attacker, !attackerWon, directDamage);
        }
        if (!attackEffectsCanceled) {
            resolveCombatEffectAfterDamage(attackCard, attackerPlayer, attacker,
                                           defenderPlayer, defender, attackerWon, directDamage);
        }

        if (defenseCard.has_value()) {
            defenderPlayer.addToDiscard(std::move(*defenseCard));
        }
        attackerPlayer.addToDiscard(std::move(attackCard));
        checkDefeatedFighters();
        checkWinner();
        --actionsRemaining_;
        endTurnIfNeeded();
    } catch (...) {
        if (!undoStack_.empty()) undoStack_.pop_back();
        throw;
    }
}

std::vector<int> GameController::legalSchemeCards() const {
    std::vector<int> result;
    const auto& hand = currentPlayer().hand();
    for (int i = 0; i < static_cast<int>(hand.size()); ++i) {
        const Card& card = hand.at(static_cast<std::size_t>(i));
        if (!card.isScheme()) continue;
        if (card.getOwner() == Character::Any || currentPlayer().hasLivingCharacter(card.getOwner())) {
            result.push_back(i);
        }
    }
    return result;
}

SchemeChoiceKind GameController::requiredChoiceForScheme(int handIndex) const {
    const Card& card = currentPlayer().hand().at(static_cast<std::size_t>(handIndex));
    const std::string& title = card.getTitle();

    if (title == "MISTFORM") return SchemeChoiceKind::Destination;
    if (title == "ADMINISTER AID") return SchemeChoiceKind::Destination;
    if (title == "CONFIRM SUSPICION") return SchemeChoiceKind::NamedValue;
    if (title == "ELIMINATE THE IMPOSSIBLE") return SchemeChoiceKind::OpponentHandCard;
    if (title == "MASTER OF DISGUISE") return SchemeChoiceKind::None;
    if (title == "RAVENING SEDUCTION") return SchemeChoiceKind::TargetAndDestination;
    return SchemeChoiceKind::None;
}

std::vector<int> GameController::destinationChoicesForScheme(int handIndex, const SchemeChoice& partialChoice) const {
    const Card& card = currentPlayer().hand().at(static_cast<std::size_t>(handIndex));
    const std::string& title = card.getTitle();
    std::vector<int> result;

    if (title == "MISTFORM") {
        for (const auto& candidate : board_.spaces()) {
            if (!isSpaceOccupied(candidate.id())) result.push_back(candidate.id());
        }
        return result;
    }
    if (title == "ADMINISTER AID") {
        const Fighter& holmes = currentPlayer().heroFighter();
        return board_.freeAdjacentSpaces(holmes.spaceId(), [&](int spaceId) {
            return isSpaceOccupied(spaceId);
        });
    }
    if (title == "RAVENING SEDUCTION" && !partialChoice.targetFighterId.empty()) {
        const Fighter* target = findFighterById(partialChoice.targetFighterId);
        const Player* owner = ownerOfFighter(partialChoice.targetFighterId);
        if (target == nullptr || owner == nullptr) return result;
        int ownerIndex = owner->id();
        auto occupiedByEnemy = [&](int spaceId) {
            for (const auto& player : players_) {
                if (player.id() == ownerIndex) continue;
                for (const auto& fighter : player.fighters()) {
                    if (!fighter->defeated() && fighter->spaceId() == spaceId) return true;
                }
            }
            return false;
        };
        auto occupiedByAny = [&](int spaceId) {
            for (const auto& player : players_) {
                for (const auto& fighter : player.fighters()) {
                    if (!fighter->defeated() && fighter->id() != target->id() && fighter->spaceId() == spaceId) return true;
                }
            }
            return false;
        };
        auto reachable = board_.reachableSpaces(target->spaceId(), 2, occupiedByEnemy, occupiedByAny);
        reachable.erase(std::remove(reachable.begin(), reachable.end(), target->spaceId()), reachable.end());
        return reachable;
    }
    return result;
}

std::vector<std::string> GameController::targetChoicesForScheme(int handIndex) const {
    const Card& card = currentPlayer().hand().at(static_cast<std::size_t>(handIndex));
    const std::string& title = card.getTitle();
    std::vector<std::string> result;
    if (title == "MASTER OF DISGUISE") {
        for (const auto* fighter : opponentPlayer().aliveFighters()) result.push_back(fighter->id());
    } else if (title == "RAVENING SEDUCTION") {
        for (const auto& player : players_) {
            for (const auto* fighter : player.aliveFighters()) result.push_back(fighter->id());
        }
    }
    return result;
}

std::vector<int> GameController::namedValueChoicesForScheme(int handIndex) const {
    const Card& card = currentPlayer().hand().at(static_cast<std::size_t>(handIndex));
    if (card.getTitle() != "CONFIRM SUSPICION") return {};
    return {0, 1, 2, 3, 4, 5, 6};
}

std::vector<int> GameController::opponentHandChoicesForScheme(int handIndex) const {
    const Card& card = currentPlayer().hand().at(static_cast<std::size_t>(handIndex));
    if (card.getTitle() != "ELIMINATE THE IMPOSSIBLE") return {};
    std::vector<int> result(opponentPlayer().hand().size());
    std::iota(result.begin(), result.end(), 0);
    return result;
}

void GameController::playScheme(int handIndex, const SchemeChoice& choice) {
    pushUndoState();
    try {
        if (actionsRemaining_ <= 0) {
            throw RuleViolation("No actions remain this turn.");
        }
        if (!pendingOptionalMovements_.empty()) {
            throw RuleViolation("Resolve the pending card movement before starting another action.");
        }

        auto legal = legalSchemeCards();
        if (std::find(legal.begin(), legal.end(), handIndex) == legal.end()) {
            throw RuleViolation("Selected scheme card is not legal.");
        }

        Player& player = currentPlayer();
        Player& opponent = opponentPlayer();

        Card card = player.removeCardFromHand(handIndex);

        if (card.getTitle() == "RAVENING SEDUCTION") {
            PendingRaveningChoice choice;
            choice.handIndex = handIndex;
            pendingRaveningChoice_ = choice;
            return;
        }

        int dummyAttack = 0;
        int dummyDefense = 0;
        card.applyEffect(player.heroFighter(), opponent.heroFighter(), *this, dummyAttack, dummyDefense);

        if (card.getTitle() == "MISTFORM") {
            if (choice.destinationSpace != -1 && board_.contains(choice.destinationSpace) &&
                !isSpaceOccupied(choice.destinationSpace)) {
                auto* dracula = dynamic_cast<Dracula*>(&player.heroFighter());
                if (dracula) {
                    dracula->placeAt(choice.destinationSpace);
                    addAction(1);
                }
            }
        }

        if (card.getTitle() == "ADMINISTER AID") {
            auto* sherlock = dynamic_cast<Sherlock*>(&player.heroFighter());
            if (sherlock) {
                Fighter& watson = sherlock->getWatson();
                if (!watson.defeated() && choice.destinationSpace != -1 && board_.contains(choice.destinationSpace) &&
                    !isSpaceOccupied(choice.destinationSpace) &&
                    board_.areAdjacentForCombat(sherlock->spaceId(), choice.destinationSpace)) {
                    watson.placeAt(choice.destinationSpace);
                    sherlock->heal(1);
                    drawCard(player);
                }
            }
        }

        if (card.getTitle() == "ELIMINATE THE IMPOSSIBLE") {
            if (choice.opponentHandIndex < 0 ||
                choice.opponentHandIndex >= static_cast<int>(opponent.hand().size())) {
                throw RuleViolation("Invalid card selection for Eliminate the Impossible.");
            }
            Card discarded = opponent.removeCardFromHand(choice.opponentHandIndex);
            opponent.addToDiscard(std::move(discarded));
        }

        player.addToDiscard(std::move(card));
        checkDefeatedFighters();
        checkWinner();
        --actionsRemaining_;
        endTurnIfNeeded();
    } catch (...) {
        if (!undoStack_.empty()) undoStack_.pop_back();
        throw;
    }
}

std::vector<int> GameController::legalBoostCardIndexes() const {
    std::vector<int> result(currentPlayer().hand().size());
    std::iota(result.begin(), result.end(), 0);
    return result;
}

void GameController::discardCurrentPlayerCard(int handIndex) {
    pushUndoState();
    try {
        if (!pendingOptionalMovements_.empty()) {
            throw RuleViolation("Resolve the pending card movement before discarding.");
        }
        Player& player = currentPlayer();
        Card discarded = player.removeCardFromHand(handIndex);
        player.addToDiscard(std::move(discarded));
        endTurnIfNeeded();
    } catch (...) {
        if (!undoStack_.empty()) undoStack_.pop_back();
        throw;
    }
}

void GameController::useDraculaStartAbility(const std::string& targetFighterId) {
    pushUndoState();
    try {
        if (!draculaAbilityAvailable()) {
            throw RuleViolation("Dracula's start ability is not available now.");
        }
        Fighter& dracula = currentPlayer().heroFighter();
        Fighter* target = findFighterById(targetFighterId);
        if (target == nullptr || target->defeated() || target->id() == dracula.id()) {
            throw RuleViolation("Choose a living fighter adjacent to Dracula.");
        }
        if (!board_.areAdjacentForCombat(dracula.spaceId(), target->spaceId())) {
            throw RuleViolation("Dracula's ability target must be adjacent to Dracula.");
        }
        target->damage(1);
        drawCard(currentPlayer());
        draculaAbilityUsed_ = true;
        checkDefeatedFighters();
        checkWinner();
    } catch (...) {
        if (!undoStack_.empty()) undoStack_.pop_back();
        throw;
    }
}

void GameController::endTurnIfNeeded() {
    if (gameOver_) return;
    if (!pendingOptionalMovements_.empty()) return;
    if (actionsRemaining_ == 0 && currentPlayerMustDiscardToLimit()) return;
    if (!pendingFogChoices_.empty()) return;
    if (actionsRemaining_ == 0) {
        advanceTurn();
    }
}

bool GameController::currentPlayerMustDiscardToLimit() const {
    return actionsRemaining_ == 0 && currentPlayer().hand().size() > 7;
}

bool GameController::hasPendingOptionalMovement() const {
    return !pendingOptionalMovements_.empty();
}

const PendingMovementChoice& GameController::pendingOptionalMovement() const {
    if (pendingOptionalMovements_.empty()) {
        throw RuleViolation("There is no pending optional movement.");
    }
    return pendingOptionalMovements_.front();
}

std::vector<int> GameController::pendingOptionalMovementDestinations() const {
    const auto& pending = pendingOptionalMovement();
    return reachableForPlayerFighter(pending.playerIndex, pending.fighterId, pending.maxSteps);
}

void GameController::resolvePendingOptionalMovement(int destinationSpace) {
    pushUndoState();
    try {
        if (pendingOptionalMovements_.empty()) {
            throw RuleViolation("There is no pending optional movement.");
        }

        PendingMovementChoice pending = pendingOptionalMovements_.front();
        pendingOptionalMovements_.pop_front();
        Player& player = playerByIndex(pending.playerIndex);
        Fighter& fighter = player.fighterById(pending.fighterId);

        if (destinationSpace == -1) {
            endTurnIfNeeded();
            return;
        }
        if (fighter.defeated()) {
            endTurnIfNeeded();
            return;
        }

        std::vector<int> destinations = reachableForPlayerFighter(pending.playerIndex, pending.fighterId, pending.maxSteps);
        if (std::find(destinations.begin(), destinations.end(), destinationSpace) == destinations.end()) {
            throw RuleViolation("Selected destination is not reachable for the pending movement.");
        }

        fighter.placeAt(destinationSpace);
        endTurnIfNeeded();
    } catch (...) {
        if (!undoStack_.empty()) undoStack_.pop_back();
        throw;
    }
}

std::map<int, std::string> GameController::occupantTokens() const {
    std::map<int, std::string> result;
    for (const auto& player : players_) {
        for (const auto& fighter : player.fighters()) {
            if (fighter->defeated()) continue;
            std::string token;
            if (fighter->id() == "dracula") token = "D";
            else if (fighter->id().find("sister") == 0) token = "Si";
            else if (fighter->id() == "sherlock") token = "H";
            else if (fighter->id() == "watson") token = "W";
            else if (fighter->id() == "invisible_man") token = "I";
            else token = "?";
            result[fighter->spaceId()] = token;
        }
    }
    for (const auto& player : players_) {
        const Fighter& hero = player.heroFighter();
        if (auto* invisible = dynamic_cast<const InvisibleMan*>(&hero)) {
            for (int fogSpace : invisible->getFogTokens()) {
                if (fogSpace == -1) continue;
                auto it = result.find(fogSpace);
                if (it != result.end()) it->second += "+F";
                else result[fogSpace] = "F";
            }
        }
    }
    return result;
}

const Fighter* GameController::findFighterById(const std::string& fighterId) const {
    for (const auto& player : players_) {
        for (const auto& fighter : player.fighters()) {
            if (fighter->id() == fighterId) return fighter.get();
        }
    }
    return nullptr;
}

Fighter* GameController::findFighterById(const std::string& fighterId) {
    for (auto& player : players_) {
        for (auto& fighter : player.fighters()) {
            if (fighter->id() == fighterId) return fighter.get();
        }
    }
    return nullptr;
}

const Player* GameController::ownerOfFighter(const std::string& fighterId) const {
    for (const auto& player : players_) {
        for (const auto& fighter : player.fighters()) {
            if (fighter->id() == fighterId) return &player;
        }
    }
    return nullptr;
}

Player& GameController::playerByIndex(int index) {
    if (index < 0 || index >= static_cast<int>(players_.size())) {
        throw RuleViolation("Invalid player index.");
    }
    return players_.at(static_cast<std::size_t>(index));
}

const Player& GameController::playerByIndex(int index) const {
    if (index < 0 || index >= static_cast<int>(players_.size())) {
        throw RuleViolation("Invalid player index.");
    }
    return players_.at(static_cast<std::size_t>(index));
}

Player& GameController::ownerOfFighterMutable(const std::string& fighterId) {
    for (auto& player : players_) {
        for (auto& fighter : player.fighters()) {
            if (fighter->id() == fighterId) return player;
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

bool GameController::isCardPlayableBy(const Card& card, const Fighter& fighter) const {
    if (fighter.defeated()) return false;
    if (card.getOwner() == Character::Sister && dynamic_cast<const Dracula*>(&fighter)) {
        return hasLivingSisterForDracula(fighter);
    }
    return fighter.canPlayCard(card);
}

bool GameController::canAttackTarget(const Fighter& attacker, const Fighter& defender) const {
    if (attacker.defeated() || defender.defeated()) return false;
    if (board_.areAdjacentForCombat(attacker.spaceId(), defender.spaceId())) return true;
    return attacker.range() == AttackRange::Ranged && board_.shareZone(attacker.spaceId(), defender.spaceId());
}

bool GameController::isSpaceOccupied(int spaceId) const {
    for (const auto& player : players_) {
        for (const auto& fighter : player.fighters()) {
            if (!fighter->defeated() && fighter->spaceId() == spaceId) return true;
        }
    }
    return false;
}

bool GameController::isSpaceOccupiedByCurrentEnemy(int spaceId) const {
    for (const auto& fighter : opponentPlayer().fighters()) {
        if (!fighter->defeated() && fighter->spaceId() == spaceId) return true;
    }
    return false;
}

void GameController::advanceTurn() {
    currentPlayerIndex_ = opponentPlayerIndex();
    actionsRemaining_ = 2;
    pendingMovementPoints_ = 0;
    movedThisManeuver_.clear();
    pendingOptionalMovements_.clear();
    draculaAbilityUsed_ = false;
    ++turnNumber_;
    pendingFogChoices_.clear();

    for (auto& player : players_) {
        for (auto& fighter : player.fighters()) {
            if (!fighter->defeated()) {
                fighter->onTurnStart();
            }
        }
    }
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
        if (!fighter->defeated()) fighter->damage(2);
    }
    checkDefeatedFighters();
    checkWinner();
}

void GameController::placeSidekicks(Player& player) {
    std::vector<int> free = freeSpacesSharingHeroZone(player);
    for (auto& fighter : player.fighters()) {
        if (fighter->isHero()) continue;
        if (free.empty()) {
            throw InvalidSetup("Not enough free spaces for sidekick placement.");
        }
        int destination = free.front();
        free.erase(free.begin());
        fighter->placeAt(destination);
    }
}

void GameController::checkDefeatedFighters() {
    for (auto& player : players_) {
        for (auto& fighter : player.fighters()) {
            if (fighter->defeated()) fighter->removeFromBoard();
        }
    }
}

void GameController::checkWinner() {
    if (gameOver_) return;
    for (const auto& player : players_) {
        if (player.heroFighter().defeated()) {
            const Player& winner = opponentOf(player);
            gameOver_ = true;
            winnerName_ = winner.name() + " (" + winner.heroFighter().displayName() + ")";
            return;
        }
    }
}

void GameController::shuffleDeck(Player& player) {
    std::shuffle(player.deck().begin(), player.deck().end(), random_);
}

int GameController::countLivingSistersInZoneWith(int spaceId) const {
    int count = 0;
    for (const auto& player : players_) {
        const Fighter& hero = player.heroFighter();
        if (dynamic_cast<const Dracula*>(&hero) == nullptr) continue;
        for (const auto& fighter : player.fighters()) {
            if (!fighter->defeated() && fighter->cardOwner() == Character::Sister &&
                board_.shareZone(fighter->spaceId(), spaceId)) {
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

std::vector<int> GameController::reachableForPlayerFighter(int playerIndex,
                                                           const std::string& fighterId,
                                                           int maxSteps) const {
    const Player& owner = playerByIndex(playerIndex);
    const Fighter& fighter = owner.fighterById(fighterId);
    if (fighter.defeated()) return {};

    auto occupiedByEnemy = [&](int spaceId) {
        for (const auto& player : players_) {
            if (player.id() == owner.id()) continue;
            for (const auto& other : player.fighters()) {
                if (!other->defeated() && other->spaceId() == spaceId) return true;
            }
        }
        return false;
    };

    auto occupiedByAny = [&](int spaceId) {
        for (const auto& player : players_) {
            for (const auto& other : player.fighters()) {
                if (!other->defeated() && other->id() != fighter.id() && other->spaceId() == spaceId) return true;
            }
        }
        return false;
    };

    return board_.reachableSpaces(fighter.spaceId(), maxSteps, occupiedByEnemy, occupiedByAny);
}

void GameController::queueOptionalMovement(int playerIndex,
                                           const std::string& fighterId,
                                           int maxSteps,
                                           const std::string& source) {
    if (maxSteps <= 0) return;
    const Player& player = playerByIndex(playerIndex);
    const Fighter& fighter = player.fighterById(fighterId);
    if (fighter.defeated()) return;

    PendingMovementChoice choice;
    choice.playerIndex = playerIndex;
    choice.fighterId = fighterId;
    choice.maxSteps = maxSteps;
    choice.source = source;
    pendingOptionalMovements_.push_back(std::move(choice));
}

std::vector<int> GameController::freeSpacesSharingHeroZone(const Player& player) const {
    std::vector<int> result;
    int heroSpace = player.heroFighter().spaceId();
    for (int candidate : board_.spacesSharingAnyZone(heroSpace)) {
        if (!isSpaceOccupied(candidate)) result.push_back(candidate);
    }
    return result;
}

std::vector<int> GameController::valuesOnCard(const Card& card) const {
    std::vector<int> values;
    if (card.getAttack() >= 0) values.push_back(card.getAttack());
    if (card.getDefense() >= 0) values.push_back(card.getDefense());
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    return values;
}

bool GameController::cardEffectsProtectedBySherlock(const Card& card, const Player& owner) const {
    const Fighter& hero = owner.heroFighter();
    if (dynamic_cast<const Sherlock*>(&hero) == nullptr) return false;
    return card.getOwner() == Character::Sherlock || card.getOwner() == Character::Watson;
}

void GameController::resolveCombatEffectAfterDamage(const Card& card,
                                                    Player& cardPlayer,
                                                    Fighter& cardFighter,
                                                    Player& opposingPlayer,
                                                    Fighter& opposingFighter,
                                                    bool cardPlayerWon,
                                                    int directDamage) {
    (void)directDamage;
    const std::string& title = card.getTitle();

    if (title == "DASH") {
        queueOptionalMovement(cardPlayer.id(), cardFighter.id(), 3, "DASH");
        return;
    }
    if (title == "EXPLOIT") {
        drawCard(cardPlayer);
        return;
    }
    if (title == "COUNTERPUNCH") {
        if (!cardFighter.defeated() && !opposingFighter.defeated() &&
            board_.areAdjacentForCombat(cardFighter.spaceId(), opposingFighter.spaceId())) {
            opposingFighter.damage(2);
        }
        return;
    }
    if (title == "EDUCATION NEVER ENDS") {
        if (cardPlayerWon) drawCard(opposingPlayer);
        else { drawCard(cardPlayer); drawCard(cardPlayer); }
        return;
    }
    if (title == "FIXED POINT IN A CHANGING AGE" || title == "FIXED POINT") {
        Fighter& holmes = cardPlayer.heroFighter();
        try {
            Fighter& watson = cardPlayer.fighterById("watson");
            if (!holmes.defeated() && !watson.defeated() &&
                board_.areAdjacentForCombat(holmes.spaceId(), watson.spaceId())) {
                holmes.heal(1);
                watson.heal(1);
            }
        } catch (...) {}
        return;
    }
    if (title == "STUDY METHODS") {
        if (cardPlayerWon) {
            std::string handInfo = "Opponent hand: ";
            const auto& hand = opposingPlayer.hand();
            if (hand.empty()) handInfo += "(empty)";
            else {
                for (size_t i = 0; i < hand.size(); ++i) {
                    handInfo += hand[i].getTitle();
                    if (i < hand.size() - 1) handInfo += ", ";
                }
            }
            studyMethodsHandInfo_ = handInfo;
        }
        return;
    }
    if (title == "THE GAME IS AFOOT") {
        queueOptionalMovement(cardPlayer.id(), cardFighter.id(), 3, "THE GAME IS AFOOT");
        return;
    }
    if (title == "CONFOUND") {
        if (dynamic_cast<InvisibleMan*>(&cardPlayer.heroFighter())) {
            PendingConfoundChoice choice;
            choice.playerIndex = opposingPlayer.id();
            choice.opponentDecided = false;
            pendingConfoundChoice_ = choice;
        }
        return;
    }
    if (title == "THIRST FOR SUSTENANCE") {
        if (cardPlayerWon && !opposingFighter.defeated()) {
            Fighter& dracula = cardPlayer.heroFighter();
            auto adjacent = board_.freeAdjacentSpaces(opposingFighter.spaceId(), [&](int spaceId) {
                return isSpaceOccupied(spaceId);
            });
            if (!adjacent.empty() && !dracula.defeated()) {
                dracula.placeAt(adjacent.front());
            }
        }
        return;
    }
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
        if (!fighter->defeated()) total += fighter->move();
    }
    return total;
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

std::map<int, int> GameController::computeReachableWithCost(int start, int maxSteps, const std::string& fighterId) const {
    std::map<int, int> cost;
    std::deque<int> dq;
    cost[start] = 0;
    dq.push_back(start);

    const Fighter* fighter = findFighterById(fighterId);
    const InvisibleMan* invisible = nullptr;
    std::vector<int> fogSpaces;
    if (fighter) {
        invisible = dynamic_cast<const InvisibleMan*>(fighter);
        if (invisible) fogSpaces = invisible->getFogSpaces();
    }

    while (!dq.empty()) {
        int current = dq.front();
        dq.pop_front();
        int currentCost = cost[current];
        if (currentCost >= maxSteps) continue;

        for (int neighbor : board_.directNeighbors(current)) {
            if (isSpaceOccupiedByEnemy(neighbor)) continue;
            int moveCost = 1;
            if (isSpaceOccupiedByAlly(neighbor, fighterId)) moveCost = 2;
            int newCost = currentCost + moveCost;
            if (newCost > maxSteps) continue;
            if (cost.find(neighbor) != cost.end() && cost[neighbor] <= newCost) continue;

            cost[neighbor] = newCost;
            if (moveCost == 1) dq.push_front(neighbor);
            else dq.push_back(neighbor);
        }

        if (invisible && invisible->isOnFog(current)) {
            for (int fogSpace : fogSpaces) {
                if (fogSpace == current || fogSpace == -1) continue;
                if (isSpaceOccupiedByEnemy(fogSpace)) continue;
                int moveCost = 1;
                if (isSpaceOccupiedByAlly(fogSpace, fighterId)) moveCost = 2;
                int newCost = currentCost + moveCost;
                if (newCost > maxSteps) continue;
                if (cost.find(fogSpace) != cost.end() && cost[fogSpace] <= newCost) continue;

                cost[fogSpace] = newCost;
                if (moveCost == 1) dq.push_front(fogSpace);
                else dq.push_back(fogSpace);
            }
        }
    }
    return cost;
}

std::vector<int> GameController::getMatchingCardIndicesForConfirmSuspicion(int namedValue) const {
    std::vector<int> result;
    const auto& hand = opponentPlayer().hand();
    for (int i = 0; i < static_cast<int>(hand.size()); ++i) {
        const Card& card = hand.at(i);
        auto values = valuesOnCard(card);
        if (std::find(values.begin(), values.end(), namedValue) != values.end()) {
            result.push_back(i);
        }
    }
    return result;
}

void GameController::applyConfirmSuspicion(int chosenIndex) {
    pushUndoState();
    try {
        Player& opponent = opponentPlayer();
        if (chosenIndex < 0 || chosenIndex >= static_cast<int>(opponent.hand().size())) {
            throw RuleViolation("Invalid chosen card index.");
        }
        Card burned = opponent.removeCardFromHand(chosenIndex);
        int damage = std::max(0, burned.getBoost());
        opponent.heroFighter().damage(damage);
        opponent.addToDiscard(std::move(burned));
        checkDefeatedFighters();
        checkWinner();
    } catch (...) {
        if (!undoStack_.empty()) undoStack_.pop_back();
        throw;
    }
}

void GameController::setConfirmSuspicionHandInfo(const std::string& info) {
    confirmSuspicionHandInfo_ = info;
}

void GameController::addAction(int count) {
    actionsRemaining_ += count;
}

void GameController::placeVanishedInvisibleMan(int spaceId) {
    pushUndoState();
    try {
        if (!pendingVanishedPlacement_) {
            throw RuleViolation("Invisible Man is not waiting to be placed.");
        }
        if (!board_.contains(spaceId)) {
            throw RuleViolation("Invalid space.");
        }
        if (isSpaceOccupied(spaceId)) {
            throw RuleViolation("Space is occupied.");
        }
        Player& player = currentPlayer();
        Fighter& invisible = player.heroFighter();
        invisible.placeAt(spaceId);
        pendingVanishedPlacement_ = false;
        vanishedDestination_ = -1;
    } catch (...) {
        if (!undoStack_.empty()) undoStack_.pop_back();
        throw;
    }
}

std::vector<int> GameController::getValidPlacementSpacesForVanished() const {
    std::vector<int> result;
    for (const auto& space : board_.spaces()) {
        if (!isSpaceOccupied(space.id())) result.push_back(space.id());
    }
    return result;
}

void GameController::saveGame() {
    int slot = findEmptySlot();
    std::string filename = "save" + std::to_string(slot) + ".json";

    json j;
    j["version"] = 2;
    j["timestamp"] = std::time(nullptr);
    j["turnNumber"] = turnNumber_;
    j["currentPlayerIndex"] = currentPlayerIndex_;
    j["actionsRemaining"] = actionsRemaining_;
    j["draculaAbilityUsed"] = draculaAbilityUsed_;
    j["started"] = started_;
    j["gameOver"] = gameOver_;
    j["winnerName"] = winnerName_;

    j["pendingVanishedPlacement"] = pendingVanishedPlacement_;
    j["vanishedDestination"] = vanishedDestination_;
    j["studyMethodsHandInfo"] = studyMethodsHandInfo_;
    j["confoundSchemeCardIndex"] = confoundSchemeCardIndex_;
    j["confirmSuspicionHandInfo"] = confirmSuspicionHandInfo_;

    json remainingMovementJson = json::array();
    for (const auto& [fighterId, points] : remainingMovementPoints_) {
        json entry;
        entry["fighterId"] = fighterId;
        entry["points"] = points;
        remainingMovementJson.push_back(entry);
    }
    j["remainingMovementPoints"] = remainingMovementJson;

    json movedThisManeuverJson = json::array();
    for (const auto& [fighterId, moved] : movedThisManeuver_) {
        json entry;
        entry["fighterId"] = fighterId;
        entry["moved"] = moved;
        movedThisManeuverJson.push_back(entry);
    }
    j["movedThisManeuver"] = movedThisManeuverJson;

    json finishedFightersJson = json::array();
    for (const auto& fighterId : finishedFighters_) {
        finishedFightersJson.push_back(fighterId);
    }
    j["finishedFighters"] = finishedFightersJson;

    json pendingMovementsJson = json::array();
    for (const auto& movement : pendingOptionalMovements_) {
        pendingMovementsJson.push_back(pendingMovementChoiceToJson(movement));
    }
    j["pendingOptionalMovements"] = pendingMovementsJson;

    json pendingFogChoicesJson = json::array();
    for (const auto& fogChoice : pendingFogChoices_) {
        json entry;
        entry["chooserPlayerIndex"] = fogChoice.chooserPlayerIndex;
        entry["fighterOwnerIndex"] = fogChoice.fighterOwnerIndex;
        entry["fighterId"] = fogChoice.fighterId;
        entry["maxSteps"] = fogChoice.maxSteps;
        pendingFogChoicesJson.push_back(entry);
    }
    j["pendingFogChoices"] = pendingFogChoicesJson;

    if (pendingConfoundChoice_.has_value()) {
        json pc;
        pc["playerIndex"] = pendingConfoundChoice_->playerIndex;
        pc["opponentDecided"] = pendingConfoundChoice_->opponentDecided;
        pc["opponentWantsToDiscard"] = pendingConfoundChoice_->opponentWantsToDiscard;
        j["pendingConfoundChoice"] = pc;
    }

    if (pendingRaveningChoice_.has_value()) {
        json pr;
        pr["handIndex"] = pendingRaveningChoice_->handIndex;
        pr["movedFighters"] = pendingRaveningChoice_->movedFighters;
        json pos;
        for (const auto& [id, space] : pendingRaveningChoice_->newPositions) {
            pos[id] = space;
        }
        pr["newPositions"] = pos;
        j["pendingRaveningChoice"] = pr;
    }

    json playersJson = json::array();
    for (const auto& player : players_) {
        json p;
        p["name"] = player.name();
        p["age"] = player.age();

        json fightersJson = json::array();
        for (const auto& fighter : player.fighters()) {
            fightersJson.push_back(fighterToJson(*fighter));
        }
        p["fighters"] = fightersJson;
        p["deck"] = cardsToJson(player.deck());
        p["hand"] = cardsToJson(player.hand());
        p["discardPile"] = cardsToJson(player.discardPile());
        playersJson.push_back(p);
    }
    j["players"] = playersJson;

    std::ofstream file(filename);
    file << j.dump(4);
}

void GameController::loadGame(int slot) {
    std::string filename = "save" + std::to_string(slot) + ".json";
    if (!std::ifstream(filename).good()) {
        throw RuleViolation("Save file does not exist.");
    }

    std::ifstream file(filename);
    json j;
    file >> j;

    players_.clear();
    remainingMovementPoints_.clear();
    movedThisManeuver_.clear();
    finishedFighters_.clear();
    pendingOptionalMovements_.clear();
    pendingFogChoices_.clear();
    pendingConfoundChoice_.reset();
    pendingRaveningChoice_.reset();

    turnNumber_ = j["turnNumber"].get<int>();
    currentPlayerIndex_ = j["currentPlayerIndex"].get<int>();
    actionsRemaining_ = j["actionsRemaining"].get<int>();
    draculaAbilityUsed_ = j["draculaAbilityUsed"].get<bool>();
    started_ = j["started"].get<bool>();
    gameOver_ = j["gameOver"].get<bool>();
    winnerName_ = j["winnerName"].get<std::string>();

    pendingVanishedPlacement_ = j.value("pendingVanishedPlacement", false);
    vanishedDestination_ = j.value("vanishedDestination", -1);
    studyMethodsHandInfo_ = j.value("studyMethodsHandInfo", "");
    confoundSchemeCardIndex_ = j.value("confoundSchemeCardIndex", -1);
    confirmSuspicionHandInfo_ = j.value("confirmSuspicionHandInfo", "");

    if (j.contains("remainingMovementPoints")) {
        for (const auto& entry : j["remainingMovementPoints"]) {
            std::string fighterId = entry["fighterId"].get<std::string>();
            int points = entry["points"].get<int>();
            remainingMovementPoints_[fighterId] = points;
        }
    }

    if (j.contains("movedThisManeuver")) {
        for (const auto& entry : j["movedThisManeuver"]) {
            std::string fighterId = entry["fighterId"].get<std::string>();
            int moved = entry["moved"].get<int>();
            movedThisManeuver_[fighterId] = moved;
        }
    }

    if (j.contains("finishedFighters")) {
        for (const auto& fighterId : j["finishedFighters"]) {
            finishedFighters_.push_back(fighterId.get<std::string>());
        }
    }

    if (j.contains("pendingOptionalMovements")) {
        for (const auto& movementJson : j["pendingOptionalMovements"]) {
            pendingOptionalMovements_.push_back(jsonToPendingMovementChoice(movementJson));
        }
    }

    if (j.contains("pendingFogChoices")) {
        for (const auto& entry : j["pendingFogChoices"]) {
            PendingFogChoice choice;
            choice.chooserPlayerIndex = entry["chooserPlayerIndex"].get<int>();
            choice.fighterOwnerIndex = entry["fighterOwnerIndex"].get<int>();
            choice.fighterId = entry["fighterId"].get<std::string>();
            choice.maxSteps = entry["maxSteps"].get<int>();
            pendingFogChoices_.push_back(choice);
        }
    }

    if (j.contains("pendingConfoundChoice")) {
        const auto& pc = j["pendingConfoundChoice"];
        PendingConfoundChoice choice;
        choice.playerIndex = pc["playerIndex"].get<int>();
        choice.opponentDecided = pc["opponentDecided"].get<bool>();
        choice.opponentWantsToDiscard = pc["opponentWantsToDiscard"].get<bool>();
        pendingConfoundChoice_ = choice;
    }

    if (j.contains("pendingRaveningChoice")) {
        const auto& pr = j["pendingRaveningChoice"];
        PendingRaveningChoice choice;
        choice.handIndex = pr["handIndex"].get<int>();
        choice.movedFighters = pr["movedFighters"].get<std::vector<std::string>>();
        for (const auto& [key, value] : pr["newPositions"].items()) {
            choice.newPositions[key] = value.get<int>();
        }
        pendingRaveningChoice_ = choice;
    }

    for (const auto& pJson : j["players"]) {
        std::string name = pJson["name"].get<std::string>();
        int age = pJson["age"].get<int>();
        Player player(players_.size(), name, age);

        for (const auto& fJson : pJson["fighters"]) {
            auto fighter = jsonToFighter(fJson);
            player.fighters().push_back(std::move(fighter));
        }

        player.deck() = jsonToCards(pJson["deck"]);
        player.hand() = jsonToCards(pJson["hand"]);
        player.discardPile() = jsonToCards(pJson["discardPile"]);

        players_.push_back(std::move(player));
    }

    for (auto& player : players_) {
        if (auto* sherlock = dynamic_cast<Sherlock*>(player.fighters()[0].get())) {
            for (auto& f : player.fighters()) {
                if (f->id() == "watson") {
                    sherlock->setWatson(f.get());
                    break;
                }
            }
        }
    }
}

std::vector<std::pair<int, std::string>> GameController::getSaveSlots() const {
    std::vector<std::pair<int, std::string>> result;
    for (int i = 1; i <= 3; ++i) {
        std::string filename = "save" + std::to_string(i) + ".json";
        if (std::ifstream(filename).good()) {
            std::ifstream file(filename);
            json j;
            file >> j;
            std::string info = "Slot " + std::to_string(i) + ": ";
            if (j.contains("players") && j["players"].size() > 0) {
                info += j["players"][0]["name"].get<std::string>() + " - Turn " + std::to_string(j["turnNumber"].get<int>());
            }
            result.push_back({i, info});
        }
    }
    return result;
}

bool GameController::hasLivingSisterForDracula(const Fighter& dracula) const {
    const Player* owner = ownerOfFighter(dracula.id());
    if (!owner) return false;
    for (const auto& fighter : owner->fighters()) {
        if (!fighter->defeated() && fighter->cardOwner() == Character::Sister) return true;
    }
    return false;
}

void GameController::handleConfound(int handIndex, bool opponentDiscards) {
    pushUndoState();
    try {
        Card card = currentPlayer().removeCardFromHand(handIndex);
        if (opponentDiscards) {
            if (!opponentPlayer().hand().empty()) {
                int idx = getRandomInt(0, static_cast<int>(opponentPlayer().hand().size()) - 1);
                Card discarded = opponentPlayer().removeCardFromHand(idx);
                opponentPlayer().addToDiscard(std::move(discarded));
            }
        } else {
            auto* invisible = dynamic_cast<InvisibleMan*>(&currentPlayer().heroFighter());
            if (invisible) {
                for (int i = 0; i < 3; ++i) {
                    if (invisible->getFogTokens()[i] != -1) {
                        int current = invisible->getFogTokens()[i];
                        auto occupiedByEnemy = [&](int spaceId) {
                            for (const auto& fighter : opponentPlayer().fighters()) {
                                if (!fighter->defeated() && fighter->spaceId() == spaceId) return true;
                            }
                            return false;
                        };
                        auto occupiedByAny = [&](int spaceId) {
                            return isSpaceOccupied(spaceId);
                        };
                        auto reachable = board_.reachableSpaces(current, 99, occupiedByEnemy, occupiedByAny);
                        if (!reachable.empty()) {
                            invisible->moveFogToken(i, reachable.front());
                            break;
                        }
                    }
                }
            }
        }
        currentPlayer().addToDiscard(std::move(card));
        --actionsRemaining_;
        endTurnIfNeeded();
    } catch (...) {
        if (!undoStack_.empty()) undoStack_.pop_back();
        throw;
    }
}

void GameController::handleVanish(int handIndex, int destinationSpace) {
    pushUndoState();
    try {
        Card card = currentPlayer().removeCardFromHand(handIndex);
        Fighter& hero = currentPlayer().heroFighter();
        hero.heal(1);
        hero.removeFromBoard();
        vanishedDestination_ = destinationSpace;
        pendingVanishedPlacement_ = true;
        currentPlayer().addToDiscard(std::move(card));

        if (actionsRemaining_ == 2) {
            actionsRemaining_ = 0;
        } else {
            --actionsRemaining_;
        }
        endTurnIfNeeded();
    } catch (...) {
        if (!undoStack_.empty()) undoStack_.pop_back();
        throw;
    }
}

void GameController::handleStepLightly(int handIndex, const std::string& targetFighterId) {
    pushUndoState();
    try {
        Card card = currentPlayer().removeCardFromHand(handIndex);
        auto* invisible = dynamic_cast<InvisibleMan*>(&currentPlayer().heroFighter());
        if (invisible) {
            int damage = invisible->isOnFog(invisible->spaceId()) ? 3 : 1;
            Fighter* target = findFighterById(targetFighterId);
            if (target && !target->defeated()) {
                target->damage(damage);
            }

            auto* currentInvisible = dynamic_cast<InvisibleMan*>(&currentPlayer().heroFighter());
            if (currentInvisible) {
                bool hasFog = false;
                for (int token : currentInvisible->getFogTokens()) {
                    if (token != -1) { hasFog = true; break; }
                }
                if (hasFog) {
                    queueFogChoice(
                        opponentPlayer().id(),
                        currentInvisible->id(),
                        2
                    );
                }
            }
        }
        currentPlayer().addToDiscard(std::move(card));
        --actionsRemaining_;
        endTurnIfNeeded();
    } catch (...) {
        if (!undoStack_.empty()) undoStack_.pop_back();
        throw;
    }
}

void GameController::handleLurking(int handIndex, int choice) {
    pushUndoState();
    try {
        Card card = currentPlayer().removeCardFromHand(handIndex);
        drawCard(currentPlayer());
        auto* invisible = dynamic_cast<InvisibleMan*>(&currentPlayer().heroFighter());
        if (invisible) {
            if (choice == 0) {
                for (int fog : invisible->getFogTokens()) {
                    if (fog != -1 && !isSpaceOccupied(fog)) {
                        invisible->placeAt(fog);
                        break;
                    }
                }
            } else if (choice == 1) {
                for (int i = 0; i < 3; ++i) {
                    if (invisible->getFogTokens()[i] != -1) {
                        int current = invisible->getFogTokens()[i];
                        auto occupiedByEnemy = [&](int spaceId) {
                            for (const auto& fighter : opponentPlayer().fighters()) {
                                if (!fighter->defeated() && fighter->spaceId() == spaceId) return true;
                            }
                            return false;
                        };
                        auto occupiedByAny = [&](int spaceId) {
                            return isSpaceOccupied(spaceId);
                        };
                        auto reachable = board_.reachableSpaces(current, 3, occupiedByEnemy, occupiedByAny);
                        if (!reachable.empty()) {
                            invisible->moveFogToken(i, reachable.front());
                            break;
                        }
                    }
                }
            }
        }
        currentPlayer().addToDiscard(std::move(card));
        --actionsRemaining_;
        endTurnIfNeeded();
    } catch (...) {
        if (!undoStack_.empty()) undoStack_.pop_back();
        throw;
    }
}

void GameController::playConfirmSuspicion(int handIndex, int namedValue) {
    pushUndoState();
    try {
        Card card = currentPlayer().removeCardFromHand(handIndex);
        auto matching = getMatchingCardIndicesForConfirmSuspicion(namedValue);
        if (matching.empty()) {
            std::string handInfo = "Opponent hand (no matching card): ";
            const auto& hand = opponentPlayer().hand();
            if (hand.empty()) {
                handInfo += "(empty)";
            } else {
                for (size_t i = 0; i < hand.size(); ++i) {
                    handInfo += hand[i].getTitle();
                    if (i < hand.size() - 1) handInfo += ", ";
                }
            }
            setConfirmSuspicionHandInfo(handInfo);
            currentPlayer().addToDiscard(std::move(card));
            --actionsRemaining_;
            endTurnIfNeeded();
        } else {
            currentPlayer().addToDiscard(std::move(card));
            --actionsRemaining_;
            endTurnIfNeeded();
        }
    } catch (...) {
        if (!undoStack_.empty()) undoStack_.pop_back();
        throw;
    }
}

void GameController::handleStudyMethods(int cardIndex, bool won) {
    (void)cardIndex;
    if (!won) return;
    std::string handInfo = "Opponent hand: ";
    const auto& hand = opponentPlayer().hand();
    if (hand.empty()) {
        handInfo += "(empty)";
    } else {
        for (size_t i = 0; i < hand.size(); ++i) {
            handInfo += hand[i].getTitle();
            if (i < hand.size() - 1) handInfo += ", ";
        }
    }
    setStudyMethodsHandInfo(handInfo);
}

void GameController::handleCodedNotes(int handIndex, const std::vector<int>& selectedIndices) {
    pushUndoState();
    try {
        if (selectedIndices.size() != 2) {
            throw RuleViolation("You must select exactly 2 cards.");
        }

        Card card = currentPlayer().removeCardFromHand(handIndex);

        drawCard(currentPlayer());
        drawCard(currentPlayer());
        drawCard(currentPlayer());

        std::vector<Card> cardsToTop;
        std::set<int> sortedIndices(selectedIndices.begin(), selectedIndices.end());

        for (auto it = sortedIndices.rbegin(); it != sortedIndices.rend(); ++it) {
            int idx = *it;
            if (idx < 0 || idx >= static_cast<int>(currentPlayer().hand().size())) {
                throw RuleViolation("Invalid card index.");
            }
            Card removed = currentPlayer().removeCardFromHand(idx);
            cardsToTop.push_back(std::move(removed));
        }

        for (auto& c : cardsToTop) {
            currentPlayer().deck().push_back(std::move(c));
        }

        currentPlayer().addToDiscard(std::move(card));
        --actionsRemaining_;
        endTurnIfNeeded();
    } catch (...) {
        if (!undoStack_.empty()) undoStack_.pop_back();
        throw;
    }
}

void GameController::handleConfoundDiscard(int handIndex, int opponentCardIndex) {
    pushUndoState();
    try {
        Card card = currentPlayer().removeCardFromHand(handIndex);
        Card discarded = opponentPlayer().removeCardFromHand(opponentCardIndex);
        opponentPlayer().addToDiscard(std::move(discarded));
        currentPlayer().addToDiscard(std::move(card));
        --actionsRemaining_;
        endTurnIfNeeded();
    } catch (...) {
        if (!undoStack_.empty()) undoStack_.pop_back();
        throw;
    }
}

void GameController::handleConfoundFogMove(int handIndex) {
    pushUndoState();
    try {
        Card card = currentPlayer().removeCardFromHand(handIndex);
        auto* invisible = dynamic_cast<InvisibleMan*>(&currentPlayer().heroFighter());
        if (invisible) {
            for (int i = 0; i < 3; ++i) {
                if (invisible->getFogTokens()[i] != -1) {
                    int current = invisible->getFogTokens()[i];
                    auto occupiedByEnemy = [&](int spaceId) {
                        for (const auto& fighter : opponentPlayer().fighters()) {
                            if (!fighter->defeated() && fighter->spaceId() == spaceId) return true;
                        }
                        return false;
                    };
                    auto occupiedByAny = [&](int spaceId) {
                        return isSpaceOccupied(spaceId);
                    };
                    auto reachable = board_.reachableSpaces(current, 99, occupiedByEnemy, occupiedByAny);
                    if (!reachable.empty()) {
                        invisible->moveFogToken(i, reachable.front());
                        break;
                    }
                }
            }
        }
        currentPlayer().addToDiscard(std::move(card));
        --actionsRemaining_;
        endTurnIfNeeded();
    } catch (...) {
        if (!undoStack_.empty()) undoStack_.pop_back();
        throw;
    }
}

bool GameController::hasPendingFogChoice() const {
    return !pendingFogChoices_.empty();
}

const PendingFogChoice& GameController::pendingFogChoice() const {
    if (pendingFogChoices_.empty()) {
        throw RuleViolation("There is no pending fog choice.");
    }
    return pendingFogChoices_.front();
}

std::vector<int> GameController::pendingFogChoices() const {
    const auto& pending = pendingFogChoice();
    const Player& owner = playerByIndex(pending.fighterOwnerIndex);
    const Fighter& fighter = owner.fighterById(pending.fighterId);
    const auto* invisible = dynamic_cast<const InvisibleMan*>(&fighter);
    if (!invisible) return {};

    std::vector<int> result;
    const auto& tokens = invisible->getFogTokens();
    for (int i = 0; i < static_cast<int>(tokens.size()); ++i) {
        if (tokens[i] != -1) result.push_back(i);
    }
    return result;
}

void GameController::resolvePendingFogChoice(int fogIndex) {
    if (pendingFogChoices_.empty()) {
        throw RuleViolation("There is no pending fog choice.");
    }

    PendingFogChoice pending = pendingFogChoices_.front();
    pendingFogChoices_.pop_front();

    Player& owner = playerByIndex(pending.fighterOwnerIndex);
    Fighter& fighter = owner.fighterById(pending.fighterId);
    auto* invisible = dynamic_cast<InvisibleMan*>(&fighter);
    if (!invisible) return;

    const auto& tokens = invisible->getFogTokens();
    if (fogIndex < 0 || fogIndex >= static_cast<int>(tokens.size())) {
        throw RuleViolation("Invalid fog token index.");
    }

    int current = tokens[fogIndex];
    auto occupiedByEnemy = [&](int spaceId) {
        for (const auto& f : opponentOf(owner).fighters()) {
            if (!f->defeated() && f->spaceId() == spaceId) return true;
        }
        return false;
    };
    auto occupiedByAny = [&](int spaceId) {
        return isSpaceOccupied(spaceId);
    };

    auto reachable = board_.reachableSpaces(current, pending.maxSteps, occupiedByEnemy, occupiedByAny);
    if (!reachable.empty()) {
        invisible->moveFogToken(fogIndex, reachable.front());
    }
}

void GameController::queueFogChoice(int chooserPlayerIndex, const std::string& fighterId, int maxSteps) {
    if (maxSteps <= 0) return;

    int fighterOwnerIndex = -1;
    for (int i = 0; i < static_cast<int>(players_.size()); ++i) {
        for (const auto& f : players_[i].fighters()) {
            if (f->id() == fighterId) {
                fighterOwnerIndex = i;
                break;
            }
        }
        if (fighterOwnerIndex != -1) break;
    }
    if (fighterOwnerIndex == -1) return;

    PendingFogChoice choice;
    choice.chooserPlayerIndex = chooserPlayerIndex;
    choice.fighterOwnerIndex = fighterOwnerIndex;
    choice.fighterId = fighterId;
    choice.maxSteps = maxSteps;
    pendingFogChoices_.push_back(std::move(choice));
}

json GameController::createFullSnapshot() const {
    json j;

    j["turnNumber"] = turnNumber_;
    j["currentPlayerIndex"] = currentPlayerIndex_;
    j["actionsRemaining"] = actionsRemaining_;
    j["draculaAbilityUsed"] = draculaAbilityUsed_;
    j["started"] = started_;
    j["gameOver"] = gameOver_;
    j["winnerName"] = winnerName_;
    j["pendingMovementPoints"] = pendingMovementPoints_;
    j["maxMovementPoints"] = maxMovementPoints_;

    j["pendingVanishedPlacement"] = pendingVanishedPlacement_;
    j["vanishedDestination"] = vanishedDestination_;
    j["studyMethodsHandInfo"] = studyMethodsHandInfo_;
    j["confoundSchemeCardIndex"] = confoundSchemeCardIndex_;
    j["confirmSuspicionHandInfo"] = confirmSuspicionHandInfo_;

    json remainingMovementJson = json::array();
    for (const auto& [fighterId, points] : remainingMovementPoints_) {
        json entry;
        entry["fighterId"] = fighterId;
        entry["points"] = points;
        remainingMovementJson.push_back(entry);
    }
    j["remainingMovementPoints"] = remainingMovementJson;

    json movedThisManeuverJson = json::array();
    for (const auto& [fighterId, moved] : movedThisManeuver_) {
        json entry;
        entry["fighterId"] = fighterId;
        entry["moved"] = moved;
        movedThisManeuverJson.push_back(entry);
    }
    j["movedThisManeuver"] = movedThisManeuverJson;

    json finishedFightersJson = json::array();
    for (const auto& fighterId : finishedFighters_) {
        finishedFightersJson.push_back(fighterId);
    }
    j["finishedFighters"] = finishedFightersJson;

    json pendingMovementsJson = json::array();
    for (const auto& movement : pendingOptionalMovements_) {
        json entry;
        entry["playerIndex"] = movement.playerIndex;
        entry["fighterId"] = movement.fighterId;
        entry["maxSteps"] = movement.maxSteps;
        entry["source"] = movement.source;
        pendingMovementsJson.push_back(entry);
    }
    j["pendingOptionalMovements"] = pendingMovementsJson;

    json pendingFogChoicesJson = json::array();
    for (const auto& fogChoice : pendingFogChoices_) {
        json entry;
        entry["chooserPlayerIndex"] = fogChoice.chooserPlayerIndex;
        entry["fighterOwnerIndex"] = fogChoice.fighterOwnerIndex;
        entry["fighterId"] = fogChoice.fighterId;
        entry["maxSteps"] = fogChoice.maxSteps;
        pendingFogChoicesJson.push_back(entry);
    }
    j["pendingFogChoices"] = pendingFogChoicesJson;

    if (pendingConfoundChoice_.has_value()) {
        json pc;
        pc["playerIndex"] = pendingConfoundChoice_->playerIndex;
        pc["opponentDecided"] = pendingConfoundChoice_->opponentDecided;
        pc["opponentWantsToDiscard"] = pendingConfoundChoice_->opponentWantsToDiscard;
        j["pendingConfoundChoice"] = pc;
    }

    if (pendingRaveningChoice_.has_value()) {
        json pr;
        pr["handIndex"] = pendingRaveningChoice_->handIndex;
        pr["movedFighters"] = pendingRaveningChoice_->movedFighters;
        json pos;
        for (const auto& [id, space] : pendingRaveningChoice_->newPositions) {
            pos[id] = space;
        }
        pr["newPositions"] = pos;
        j["pendingRaveningChoice"] = pr;
    }

    json playersJson = json::array();
    for (const auto& player : players_) {
        json p;
        p["name"] = player.name();
        p["age"] = player.age();

        json fightersJson = json::array();
        for (const auto& fighter : player.fighters()) {
            fightersJson.push_back(fighterToJson(*fighter));
        }
        p["fighters"] = fightersJson;
        p["deck"] = cardsToJson(player.deck());
        p["hand"] = cardsToJson(player.hand());
        p["discardPile"] = cardsToJson(player.discardPile());
        playersJson.push_back(p);
    }
    j["players"] = playersJson;

    return j;
}

void GameController::restoreFullSnapshot(const json& j) {
    players_.clear();
    remainingMovementPoints_.clear();
    movedThisManeuver_.clear();
    finishedFighters_.clear();
    pendingOptionalMovements_.clear();
    pendingFogChoices_.clear();
    pendingConfoundChoice_.reset();
    pendingRaveningChoice_.reset();

    turnNumber_ = j["turnNumber"].get<int>();
    currentPlayerIndex_ = j["currentPlayerIndex"].get<int>();
    actionsRemaining_ = j["actionsRemaining"].get<int>();
    draculaAbilityUsed_ = j["draculaAbilityUsed"].get<bool>();
    started_ = j["started"].get<bool>();
    gameOver_ = j["gameOver"].get<bool>();
    winnerName_ = j["winnerName"].get<std::string>();
    pendingMovementPoints_ = j.value("pendingMovementPoints", 0);
    maxMovementPoints_ = j.value("maxMovementPoints", 0);

    pendingVanishedPlacement_ = j.value("pendingVanishedPlacement", false);
    vanishedDestination_ = j.value("vanishedDestination", -1);
    studyMethodsHandInfo_ = j.value("studyMethodsHandInfo", "");
    confoundSchemeCardIndex_ = j.value("confoundSchemeCardIndex", -1);
    confirmSuspicionHandInfo_ = j.value("confirmSuspicionHandInfo", "");

    if (j.contains("remainingMovementPoints")) {
        for (const auto& entry : j["remainingMovementPoints"]) {
            std::string fighterId = entry["fighterId"].get<std::string>();
            int points = entry["points"].get<int>();
            remainingMovementPoints_[fighterId] = points;
        }
    }

    if (j.contains("movedThisManeuver")) {
        for (const auto& entry : j["movedThisManeuver"]) {
            std::string fighterId = entry["fighterId"].get<std::string>();
            int moved = entry["moved"].get<int>();
            movedThisManeuver_[fighterId] = moved;
        }
    }

    if (j.contains("finishedFighters")) {
        for (const auto& fighterId : j["finishedFighters"]) {
            finishedFighters_.push_back(fighterId.get<std::string>());
        }
    }

    if (j.contains("pendingOptionalMovements")) {
        for (const auto& entry : j["pendingOptionalMovements"]) {
            PendingMovementChoice choice;
            choice.playerIndex = entry["playerIndex"].get<int>();
            choice.fighterId = entry["fighterId"].get<std::string>();
            choice.maxSteps = entry["maxSteps"].get<int>();
            choice.source = entry.value("source", "");
            pendingOptionalMovements_.push_back(choice);
        }
    }

    if (j.contains("pendingFogChoices")) {
        for (const auto& entry : j["pendingFogChoices"]) {
            PendingFogChoice choice;
            choice.chooserPlayerIndex = entry["chooserPlayerIndex"].get<int>();
            choice.fighterOwnerIndex = entry["fighterOwnerIndex"].get<int>();
            choice.fighterId = entry["fighterId"].get<std::string>();
            choice.maxSteps = entry["maxSteps"].get<int>();
            pendingFogChoices_.push_back(choice);
        }
    }

    if (j.contains("pendingConfoundChoice")) {
        const auto& pc = j["pendingConfoundChoice"];
        PendingConfoundChoice choice;
        choice.playerIndex = pc["playerIndex"].get<int>();
        choice.opponentDecided = pc["opponentDecided"].get<bool>();
        choice.opponentWantsToDiscard = pc["opponentWantsToDiscard"].get<bool>();
        pendingConfoundChoice_ = choice;
    }

    if (j.contains("pendingRaveningChoice")) {
        const auto& pr = j["pendingRaveningChoice"];
        PendingRaveningChoice choice;
        choice.handIndex = pr["handIndex"].get<int>();
        choice.movedFighters = pr["movedFighters"].get<std::vector<std::string>>();
        for (const auto& [key, value] : pr["newPositions"].items()) {
            choice.newPositions[key] = value.get<int>();
        }
        pendingRaveningChoice_ = choice;
    }

    for (const auto& pJson : j["players"]) {
        std::string name = pJson["name"].get<std::string>();
        int age = pJson["age"].get<int>();
        Player player(players_.size(), name, age);

        for (const auto& fJson : pJson["fighters"]) {
            auto fighter = jsonToFighter(fJson);
            player.fighters().push_back(std::move(fighter));
        }

        player.deck() = jsonToCards(pJson["deck"]);
        player.hand() = jsonToCards(pJson["hand"]);
        player.discardPile() = jsonToCards(pJson["discardPile"]);

        players_.push_back(std::move(player));
    }

    for (auto& player : players_) {
        if (auto* sherlock = dynamic_cast<Sherlock*>(player.fighters()[0].get())) {
            for (auto& f : player.fighters()) {
                if (f->id() == "watson") {
                    sherlock->setWatson(f.get());
                    break;
                }
            }
        }
    }
}

void GameController::pushUndoState() {
    json snapshot = createFullSnapshot();
    undoStack_.push_back(snapshot);
    if (undoStack_.size() > MAX_UNDO_STATES) {
        undoStack_.erase(undoStack_.begin());
    }
}

bool GameController::canUndo() const {
    return !undoStack_.empty();
}

void GameController::undoLastAction() {
    if (!canUndo()) {
        throw RuleViolation("No action to undo.");
    }
    json snapshot = undoStack_.back();
    undoStack_.pop_back();
    restoreFullSnapshot(snapshot);
}

void GameController::clearUndoStack() {
    undoStack_.clear();
}

bool GameController::hasPendingConfoundChoice() const {
    return pendingConfoundChoice_.has_value();
}

void GameController::resolveConfoundChoice(bool opponentWantsToDiscard) {
    if (!pendingConfoundChoice_.has_value()) {
        throw RuleViolation("No pending confound choice.");
    }
    pendingConfoundChoice_->opponentWantsToDiscard = opponentWantsToDiscard;
    pendingConfoundChoice_->opponentDecided = true;
}

void GameController::resolveConfoundDiscard(int cardIndex) {
    if (!pendingConfoundChoice_.has_value() || !pendingConfoundChoice_->opponentDecided || !pendingConfoundChoice_->opponentWantsToDiscard) {
        throw RuleViolation("Invalid confound discard state.");
    }
    Player& opponent = playerByIndex(pendingConfoundChoice_->playerIndex);
    if (cardIndex < 0 || cardIndex >= static_cast<int>(opponent.hand().size())) {
        throw RuleViolation("Invalid card index.");
    }
    Card discarded = opponent.removeCardFromHand(cardIndex);
    opponent.addToDiscard(std::move(discarded));
    pendingConfoundChoice_.reset();
}

void GameController::resolveConfoundFogMove(int fogIndex, int destinationSpace) {
    if (!pendingConfoundChoice_.has_value() || !pendingConfoundChoice_->opponentDecided || pendingConfoundChoice_->opponentWantsToDiscard) {
        throw RuleViolation("Invalid confound fog move state.");
    }
    Fighter& hero = currentPlayer().heroFighter();
    auto* invisible = dynamic_cast<InvisibleMan*>(&hero);
    if (!invisible) {
        throw RuleViolation("Current player is not Invisible Man.");
    }
    if (fogIndex < 0 || fogIndex >= 3 || invisible->getFogTokens()[fogIndex] == -1) {
        throw RuleViolation("Invalid fog token.");
    }
    if (!board_.contains(destinationSpace)) {
        throw RuleViolation("Invalid destination space.");
    }
    invisible->moveFogToken(fogIndex, destinationSpace);
    pendingConfoundChoice_.reset();
}

bool GameController::hasPendingRaveningChoice() const {
    return pendingRaveningChoice_.has_value();
}

std::vector<std::string> GameController::getRaveningTargets() const {
    if (!pendingRaveningChoice_.has_value()) return {};
    std::vector<std::string> result;
    for (const auto& player : players_) {
        for (const auto& fighter : player.aliveFighters()) {
            if (std::find(pendingRaveningChoice_->movedFighters.begin(),
                          pendingRaveningChoice_->movedFighters.end(),
                          fighter->id()) == pendingRaveningChoice_->movedFighters.end()) {
                result.push_back(fighter->id());
            }
        }
    }
    return result;
}

std::vector<int> GameController::getRaveningDestinations(const std::string& fighterId) const {
    if (!pendingRaveningChoice_.has_value()) return {};
    const Fighter* fighter = findFighterById(fighterId);
    if (!fighter || fighter->defeated()) return {};

    auto occupiedByEnemy = [&](int spaceId) {
        for (const auto& f : opponentPlayer().fighters()) {
            if (!f->defeated() && f->spaceId() == spaceId) return true;
        }
        return false;
    };
    auto occupiedByAny = [&](int spaceId) {
        return isSpaceOccupied(spaceId);
    };
    auto reachable = board_.reachableSpaces(fighter->spaceId(), 2, occupiedByEnemy, occupiedByAny);
    reachable.erase(std::remove(reachable.begin(), reachable.end(), fighter->spaceId()), reachable.end());
    return reachable;
}

void GameController::applyRaveningMove(const std::string& fighterId, int destinationSpace) {
    if (!pendingRaveningChoice_.has_value()) {
        throw RuleViolation("No pending ravening choice.");
    }
    if (!board_.contains(destinationSpace)) {
        throw RuleViolation("Invalid destination.");
    }
    if (std::find(pendingRaveningChoice_->movedFighters.begin(),
                  pendingRaveningChoice_->movedFighters.end(),
                  fighterId) != pendingRaveningChoice_->movedFighters.end()) {
        throw RuleViolation("This fighter has already been moved.");
    }
    const Fighter* fighter = findFighterById(fighterId);
    if (!fighter || fighter->defeated()) throw RuleViolation("Invalid fighter.");

    pendingRaveningChoice_->newPositions[fighterId] = destinationSpace;
    pendingRaveningChoice_->movedFighters.push_back(fighterId);
}

void GameController::finishRaveningScheme() {
    if (!pendingRaveningChoice_.has_value()) {
        throw RuleViolation("No pending ravening choice.");
    }

    Player& player = currentPlayer();

    for (const auto& [fighterId, newSpace] : pendingRaveningChoice_->newPositions) {
        Fighter* fighter = findFighterById(fighterId);
        if (fighter && !fighter->defeated()) {
            fighter->placeAt(newSpace);
        }
    }

    for (const auto& [fighterId, newSpace] : pendingRaveningChoice_->newPositions) {
        Fighter* fighter = findFighterById(fighterId);
        if (!fighter || fighter->defeated()) continue;

        int damage = 0;
        for (const auto& sister : player.fighters()) {
            if (!sister->defeated() && sister->cardOwner() == Character::Sister &&
                board_.areAdjacentForCombat(sister->spaceId(), fighter->spaceId())) {
                ++damage;
            }
        }
        if (damage > 0) {
            fighter->damage(damage);
        }
    }

    Card card = player.removeCardFromHand(pendingRaveningChoice_->handIndex);
    player.addToDiscard(std::move(card));
    --actionsRemaining_;

    pendingRaveningChoice_.reset();
    checkDefeatedFighters();
    checkWinner();
    endTurnIfNeeded();
}

void GameController::cancelRaveningScheme() {
    if (pendingRaveningChoice_.has_value()) {
        pendingRaveningChoice_.reset();
    }
}

std::vector<int> GameController::getReachableFogDestinations(int fogIndex) const {
    if (!hasPendingFogChoice()) return {};
    const auto& pending = pendingFogChoice();
    const Fighter* fighter = findFighterById(pending.fighterId);
    if (!fighter) return {};
    const auto* invisible = dynamic_cast<const InvisibleMan*>(fighter);
    if (!invisible) return {};
    const auto& tokens = invisible->getFogTokens();
    if (fogIndex < 0 || fogIndex >= static_cast<int>(tokens.size()) || tokens[fogIndex] == -1) return {};
    
    const Player* owner = ownerOfFighter(pending.fighterId);
    if (!owner) return {};
    
    auto occupiedByEnemy = [&](int spaceId) {
        for (const auto& f : opponentOf(*owner).fighters()) {
            if (!f->defeated() && f->spaceId() == spaceId) return true;
        }
        return false;
    };
    auto occupiedByAny = [&](int spaceId) {
        return isSpaceOccupied(spaceId);
    };
    return board_.reachableSpaces(tokens[fogIndex], pending.maxSteps, occupiedByEnemy, occupiedByAny);
}

void GameController::moveFogToken(int fogIndex, int destinationSpace) {
    if (!hasPendingFogChoice()) {
        throw RuleViolation("No pending fog choice.");
    }
    PendingFogChoice pending = pendingFogChoices_.front();
    pendingFogChoices_.pop_front();

    Player& owner = playerByIndex(pending.fighterOwnerIndex);
    Fighter& fighter = owner.fighterById(pending.fighterId);
    auto* invisible = dynamic_cast<InvisibleMan*>(&fighter);
    if (!invisible) return;

    const auto& tokens = invisible->getFogTokens();
    if (fogIndex < 0 || fogIndex >= static_cast<int>(tokens.size()) || tokens[fogIndex] == -1) {
        throw RuleViolation("Invalid fog token.");
    }
    if (!board_.contains(destinationSpace)) {
        throw RuleViolation("Invalid destination space.");
    }
    invisible->moveFogToken(fogIndex, destinationSpace);
}

} // namespace unmatched
