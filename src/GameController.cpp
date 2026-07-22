#include "unmatched/GameController.hpp"

#include "unmatched/GameExceptions.hpp"

#include <algorithm>
#include <functional>
#include <iterator>
#include <numeric>
#include <set>
#include <sstream>
#include <utility>
#include <queue>

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

    HeroKind olderHero = youngerHero == HeroKind::Dracula ? HeroKind::Sherlock : HeroKind::Dracula;
    int youngerIndex = playerOneAge <= playerTwoAge ? 0 : 1;
    HeroKind playerOneHero = youngerIndex == 0 ? youngerHero : olderHero;
    HeroKind playerTwoHero = youngerIndex == 1 ? youngerHero : olderHero;

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

    int olderStartSlot = youngerStartSlot == 1 ? 2 : 1;
    int youngerSpace = board_.startSpaceForSlot(youngerStartSlot);
    int olderSpace = board_.startSpaceForSlot(olderStartSlot);
    players_.at(static_cast<std::size_t>(youngerIndex)).heroFighter().placeAt(youngerSpace);
    players_.at(static_cast<std::size_t>(1 - youngerIndex)).heroFighter().placeAt(olderSpace);

    currentPlayerIndex_ = youngerIndex;
    placeSidekicks(currentPlayer());
    placeSidekicks(opponentPlayer());

    for (int i = 0; i < 5; ++i) {
        drawCard(players_.at(0));
        drawCard(players_.at(1));
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

bool GameController::started() const {
    return started_;
}

bool GameController::gameOver() const {
    return gameOver_;
}

const std::string& GameController::winnerName() const {
    return winnerName_;
}

const Board& GameController::board() const {
    return board_;
}

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

const std::vector<Player>& GameController::players() const {
    return players_;
}

int GameController::currentPlayerIndex() const {
    return currentPlayerIndex_;
}

int GameController::opponentPlayerIndex() const {
    return currentPlayerIndex_ == 0 ? 1 : 0;
}

int GameController::actionsRemaining() const {
    return actionsRemaining_;
}

int GameController::turnNumber() const {
    return turnNumber_;
}

bool GameController::draculaAbilityAvailable() const {
    if (!started_ || gameOver_ || draculaAbilityUsed_ || currentPlayer().hero() != HeroKind::Dracula) {
        return false;
    }
    const Fighter& dracula = currentPlayer().heroFighter();
    if (dracula.defeated()) {
        return false;
    }
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

void GameController::drawCardForCurrentPlayer() {
    drawCard(currentPlayer());
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

bool GameController::isFighterFinished(const std::string& fighterId) const 
{
    return std::find(finishedFighters_.begin(), finishedFighters_.end(), fighterId) 
           != finishedFighters_.end();
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
        if (card.canAttack() && isCardPlayableBy(card, attacker, currentPlayer())) {
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
        if (card.canDefend() && isCardPlayableBy(card, defender, opponentPlayer())) {
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
    if (!beastFormBoostCardIndexes.empty() && attackCardPreview.effect() != EffectId::DraculaBeastForm) {
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


    int attackValue = std::max(0, attackCard.attack());
    int defenseValue = defenseCard.has_value() ? std::max(0, defenseCard->defense()) : 0;
    bool attackEffectsCanceled = false;
    bool defenseEffectsCanceled = false;

    if (attackCard.effect() == EffectId::Feint && defenseCard.has_value() &&
        !cardEffectsProtectedBySherlock(*defenseCard, defenderPlayer)) {
        defenseEffectsCanceled = true;
    
    }
    if (defenseCard.has_value() && defenseCard->effect() == EffectId::Feint &&
        !cardEffectsProtectedBySherlock(attackCard, attackerPlayer)) {
        attackEffectsCanceled = true;
        
    }

    if (!attackEffectsCanceled && attackCard.effect() == EffectId::DraculaAmbush && !defenderPlayer.hand().empty()) {
        std::uniform_int_distribution<int> distribution(0, static_cast<int>(defenderPlayer.hand().size()) - 1);
        int discardedIndex = distribution(random_);
        Card discarded = defenderPlayer.removeCardFromHand(discardedIndex);
        attackValue += discarded.boost();
    
        defenderPlayer.addToDiscard(std::move(discarded));
    }

    if (defenseCard.has_value() && !defenseEffectsCanceled) {
        if (defenseCard->effect() == EffectId::DraculaLookIntoMyEyes) {
            defenseValue += std::max(0, attackCard.boost());
            
        }
        if (defenseCard->effect() == EffectId::SherlockStrategicDeduction) {
            attackValue = std::max(0, attackCard.boost());
         
        }
        if (defenseCard->effect() == EffectId::SherlockElementary) {
            if (!cardEffectsProtectedBySherlock(attackCard, attackerPlayer)) {
                if (predictedElementaryValue != -1 && attackCard.attack() == predictedElementaryValue) {
                    attackValue = 0;
                    attackEffectsCanceled = true;
                }
            }
        }
    }

    if (!attackEffectsCanceled) {
        if (attackCard.effect() == EffectId::DraculaBloodStrike) {
            int bonus = countLivingSistersInZoneWith(defender.spaceId());
            attackValue += bonus;
          
        }
        if (attackCard.effect() == EffectId::SherlockStrategicDeduction && defenseCard.has_value()) {
            defenseValue = std::max(0, defenseCard->boost());
           
        }
        if (attackCard.effect() == EffectId::DraculaBeastForm) {
            attackValue += beastFormBonus;
          
        }
    }

    int directDamage = std::max(0, attackValue - defenseValue);
    if (directDamage > 0) 
    {
        defender.damage(directDamage);
    }
 
    bool attackerWon = directDamage > 0;

    if (defenseCard.has_value() && !defenseEffectsCanceled) {
        resolveCombatEffectAfterDamage(*defenseCard, defenderPlayer, defender, attackerPlayer, attacker, !attackerWon, directDamage);
    }
    if (!attackEffectsCanceled) {
        resolveCombatEffectAfterDamage(attackCard, attackerPlayer, attacker, defenderPlayer, defender, attackerWon, directDamage);
    }

    if (defenseCard.has_value()) {
        defenderPlayer.addToDiscard(std::move(*defenseCard));
    }
    attackerPlayer.addToDiscard(std::move(attackCard));
    checkDefeatedFighters();
    checkWinner();
    --actionsRemaining_;
    endTurnIfNeeded();
}

std::vector<int> GameController::legalSchemeCards() const {
    std::vector<int> result;
    const auto& hand = currentPlayer().hand();
    for (int i = 0; i < static_cast<int>(hand.size()); ++i) {
        const Card& card = hand.at(static_cast<std::size_t>(i));
        if (!card.isScheme()) {
            continue;
        }
        if (card.owner() == Character::Any || currentPlayer().hasLivingCharacter(card.owner())) {
            result.push_back(i);
        }
    }
    return result;
}

SchemeChoiceKind GameController::requiredChoiceForScheme(int handIndex) const {
    const Card& card = currentPlayer().hand().at(static_cast<std::size_t>(handIndex));
    switch (card.effect()) {
        case EffectId::DraculaMistForm:
        case EffectId::WatsonAid:
            return SchemeChoiceKind::Destination;
        case EffectId::SherlockConfirmSuspicion:
            return SchemeChoiceKind::NamedValue;
        case EffectId::SherlockEliminateImpossible:
            return SchemeChoiceKind::OpponentHandCard;
        case EffectId::SherlockMasterOfDisguise:
            return SchemeChoiceKind::TargetFighter;
        case EffectId::SisterRaveningSeduction:
            return SchemeChoiceKind::TargetAndDestination;
        default:
            return SchemeChoiceKind::None;
    }
}

std::vector<int> GameController::destinationChoicesForScheme(int handIndex, const SchemeChoice& partialChoice) const {
    const Card& card = currentPlayer().hand().at(static_cast<std::size_t>(handIndex));
    std::vector<int> result;
    if (card.effect() == EffectId::DraculaMistForm) {
        for (const auto& candidate : board_.spaces()) {
            if (!isSpaceOccupied(candidate.id())) {
                result.push_back(candidate.id());
            }
        }
        return result;
    }
    if (card.effect() == EffectId::WatsonAid) {
        const Fighter& holmes = currentPlayer().heroFighter();
        return board_.freeAdjacentSpaces(holmes.spaceId(), [&](int spaceId) {
            return isSpaceOccupied(spaceId);
        });
    }
    if (card.effect() == EffectId::SisterRaveningSeduction && !partialChoice.targetFighterId.empty()) {
        const Fighter* target = findFighterById(partialChoice.targetFighterId);
        const Player* owner = ownerOfFighter(partialChoice.targetFighterId);
        if (target == nullptr || owner == nullptr) {
            return result;
        }
        int ownerIndex = owner->id();
        auto occupiedByEnemy = [&](int spaceId) {
            for (const auto& player : players_) {
                if (player.id() == ownerIndex) {
                    continue;
                }
                for (const auto& fighter : player.fighters()) {
                    if (!fighter.defeated() && fighter.spaceId() == spaceId) {
                        return true;
                    }
                }
            }
            return false;
        };
        auto occupiedByAny = [&](int spaceId) {
            for (const auto& player : players_) {
                for (const auto& fighter : player.fighters()) {
                    if (!fighter.defeated() && fighter.id() != target->id() && fighter.spaceId() == spaceId) {
                        return true;
                    }
                }
            }
            return false;
        };
        return board_.reachableSpaces(target->spaceId(), 2, occupiedByEnemy, occupiedByAny);
    }
    return result;
}

std::vector<std::string> GameController::targetChoicesForScheme(int handIndex) const {
    const Card& card = currentPlayer().hand().at(static_cast<std::size_t>(handIndex));
    std::vector<std::string> result;
    if (card.effect() == EffectId::SherlockMasterOfDisguise) {
        for (const auto* fighter : opponentPlayer().aliveFighters()) {
            result.push_back(fighter->id());
        }
    } else if (card.effect() == EffectId::SisterRaveningSeduction) {
        for (const auto& player : players_) {
            for (const auto* fighter : player.aliveFighters()) {
                result.push_back(fighter->id());
            }
        }
    }
    return result;
}

std::vector<int> GameController::namedValueChoicesForScheme(int handIndex) const {
    const Card& card = currentPlayer().hand().at(static_cast<std::size_t>(handIndex));
    if (card.effect() != EffectId::SherlockConfirmSuspicion) {
        return {};
    }
    return {0, 1, 2, 3, 4, 5, 6};
}

std::vector<int> GameController::opponentHandChoicesForScheme(int handIndex) const {
    const Card& card = currentPlayer().hand().at(static_cast<std::size_t>(handIndex));
    if (card.effect() != EffectId::SherlockEliminateImpossible) {
        return {};
    }
    std::vector<int> result(opponentPlayer().hand().size());
    std::iota(result.begin(), result.end(), 0);
    return result;
}

void GameController::playScheme(int handIndex, const SchemeChoice& choice) {
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

    const Card& cardPreview = currentPlayer().hand().at(static_cast<std::size_t>(handIndex));
    SchemeChoiceKind requiredChoice = requiredChoiceForScheme(handIndex);
    std::vector<int> preparedDestinations;
    if (requiredChoice == SchemeChoiceKind::Destination || requiredChoice == SchemeChoiceKind::TargetAndDestination) {
        preparedDestinations = destinationChoicesForScheme(handIndex, choice);
    }

    Player& player = currentPlayer();
    Player& opponent = opponentPlayer();

    auto destinationWasPrepared = [&](int destination) {
        return std::find(preparedDestinations.begin(), preparedDestinations.end(), destination) != preparedDestinations.end();
    };

    switch (cardPreview.effect()) {
        case EffectId::DraculaMistForm:
            if (!destinationWasPrepared(choice.destinationSpace)) {
                throw RuleViolation("Mist Form needs an empty destination.");
            }
            break;
        case EffectId::WatsonAid:
            if (!destinationWasPrepared(choice.destinationSpace)) {
                throw RuleViolation("Watson needs a free space adjacent to Holmes.");
            }
            if (player.fighterById("watson").defeated()) {
                throw RuleViolation("Watson is defeated and cannot be placed by Aid.");
            }
            break;
        case EffectId::SherlockConfirmSuspicion:
            if (choice.namedValue < 0) {
                throw RuleViolation("Confirm Suspicion needs a named value.");
            }
            break;
        case EffectId::SherlockEliminateImpossible:
            if (choice.opponentHandIndex < 0 || choice.opponentHandIndex >= static_cast<int>(opponent.hand().size())) {
                throw RuleViolation("Choose a valid opponent card.");
            }
            break;
        case EffectId::SherlockMasterOfDisguise: {
            auto targets = targetChoicesForScheme(handIndex);
            if (std::find(targets.begin(), targets.end(), choice.targetFighterId) == targets.end()) {
                throw RuleViolation("Choose a valid opposing fighter.");
            }
            break;
        }
        case EffectId::SisterRaveningSeduction: {
            auto targets = targetChoicesForScheme(handIndex);
            if (std::find(targets.begin(), targets.end(), choice.targetFighterId) == targets.end()) {
                throw RuleViolation("Choose a valid fighter for Ravening Seduction.");
            }
            if (!destinationWasPrepared(choice.destinationSpace)) {
                throw RuleViolation("Destination is not reachable for Ravening Seduction.");
            }
            break;
        }
        default:
            break;
    }

    Card card = player.removeCardFromHand(handIndex);
    int extraActions = 0;
    

    switch (card.effect()) {
        case EffectId::DraculaMistForm: {
            if (!board_.contains(choice.destinationSpace) || isSpaceOccupied(choice.destinationSpace)) {
                throw RuleViolation("Mist Form needs an empty destination.");
            }
            Fighter& dracula = player.heroFighter();
            moveFighterIgnoringDistance(dracula, choice.destinationSpace);
            extraActions = 1;
            
            break;
        }
        case EffectId::DraculaBloodBath: {
            Fighter& dracula = player.heroFighter();
            dracula.heal(2);
         
            for (auto& sister : player.fighters()) {
                if (sister.cardOwner() == Character::Sister && sister.defeated()) {
                    auto spaces = freeSpacesSharingHeroZone(player);
                    if (!spaces.empty()) {
                        sister.reviveAt(spaces.front());
                        
                    }
                    break;
                }
            }
            break;
        }
        case EffectId::DraculaHunt: {
            Fighter& dracula = player.heroFighter();
            int healed = 0;
            for (auto& target : opponent.fighters()) {
                if (!target.defeated() && board_.areAdjacentForCombat(dracula.spaceId(), target.spaceId())) {
                    target.damage(1);
                    ++healed;
                    
                }
            }
            dracula.heal(healed);
           
            break;
        }
        case EffectId::WatsonAid: {
            Fighter& holmes = player.heroFighter();
            Fighter& watson = player.fighterById("watson");
            if (std::find(preparedDestinations.begin(), preparedDestinations.end(), choice.destinationSpace) == preparedDestinations.end()) {
                throw RuleViolation("Watson needs a free space adjacent to Holmes.");
            }
            if (watson.defeated()) {
                throw RuleViolation("Watson is defeated and cannot be placed by Aid.");
            }
            watson.placeAt(choice.destinationSpace);
            holmes.heal(1);
            drawCard(player);
            break;
        }
        case EffectId::SherlockConfirmSuspicion: {
            if (choice.namedValue < 0) {
                throw RuleViolation("Confirm Suspicion needs a named value.");
            }
           
            break;
        }
        case EffectId::SherlockEliminateImpossible: {
            if (choice.opponentHandIndex < 0 || choice.opponentHandIndex >= static_cast<int>(opponent.hand().size())) {
                throw RuleViolation("Choose a valid opponent card.");
            }
            Card burned = opponent.removeCardFromHand(choice.opponentHandIndex);
           
            opponent.addToDiscard(std::move(burned));
            break;
        }
        case EffectId::SherlockMasterOfDisguise: {
            Fighter& holmes = player.heroFighter();
            Fighter& target = opponent.fighterById(choice.targetFighterId);
            int holmesSpace = holmes.spaceId();
            int targetSpace = target.spaceId();
            holmes.placeAt(targetSpace);
            target.placeAt(holmesSpace);
            target.damage(1);
          
            break;
        }
        case EffectId::SisterRaveningSeduction: {
            Fighter* target = findFighterById(choice.targetFighterId);
            if (target == nullptr) {
                throw RuleViolation("Choose a valid fighter for Ravening Seduction.");
            }
            if (std::find(preparedDestinations.begin(), preparedDestinations.end(), choice.destinationSpace) == preparedDestinations.end()) {
                throw RuleViolation("Destination is not reachable for Ravening Seduction.");
            }
            target->placeAt(choice.destinationSpace);
            int damage = 0;
            for (const auto& sister : player.fighters()) {
                if (!sister.defeated() && sister.cardOwner() == Character::Sister &&
                    board_.areAdjacentForCombat(sister.spaceId(), target->spaceId())) {
                    ++damage;
                }
            }
            target->damage(damage);
            break;
        }
        default:
            break;
    }

    player.addToDiscard(std::move(card));
    checkDefeatedFighters();
    checkWinner();
    --actionsRemaining_;
    if (extraActions > 0) 
    {
        actionsRemaining_ += extraActions;
    }
    endTurnIfNeeded();
}

std::vector<int> GameController::legalBoostCardIndexes() const {
    std::vector<int> result(currentPlayer().hand().size());
    std::iota(result.begin(), result.end(), 0);
    return result;
}

void GameController::discardCurrentPlayerCard(int handIndex) {
    if (!pendingOptionalMovements_.empty()) {
        throw RuleViolation("Resolve the pending card movement before discarding.");
    }
    Player& player = currentPlayer();
    Card discarded = player.removeCardFromHand(handIndex);
    player.addToDiscard(std::move(discarded));
    endTurnIfNeeded();
}

void GameController::useDraculaStartAbility(const std::string& targetFighterId) {
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
}

void GameController::endTurnIfNeeded() {
    if (gameOver_) {
        return;
    }
    if (!pendingOptionalMovements_.empty()) {
        return;
    }
    if (actionsRemaining_ == 0 && currentPlayerMustDiscardToLimit()) {
        return;
    }
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
}

std::map<int, std::string> GameController::occupantTokens() const {
    std::map<int, std::string> result;
    for (const auto& player : players_) {
        for (const auto& fighter : player.fighters()) {
            if (fighter.defeated()) {
                continue;
            }
            std::string token;
            if (fighter.id() == "dracula") {
                token = "D";
            } else if (fighter.id().find("sister") == 0) {
                token = "Si";
            } else if (fighter.id() == "sherlock") {
                token = "H";
            } else if (fighter.id() == "watson") {
                token = "W";
            } else {
                token = "?";
            }
            result[fighter.spaceId()] = token;
        }
    }
    return result;
}

const Fighter* GameController::findFighterById(const std::string& fighterId) const {
    for (const auto& player : players_) {
        for (const auto& fighter : player.fighters()) {
            if (fighter.id() == fighterId) {
                return &fighter;
            }
        }
    }
    return nullptr;
}

Fighter* GameController::findFighterById(const std::string& fighterId) {
    for (auto& player : players_) {
        for (auto& fighter : player.fighters()) {
            if (fighter.id() == fighterId) {
                return &fighter;
            }
        }
    }
    return nullptr;
}

const Player* GameController::ownerOfFighter(const std::string& fighterId) const {
    for (const auto& player : players_) {
        for (const auto& fighter : player.fighters()) {
            if (fighter.id() == fighterId) {
                return &player;
            }
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
        for (const auto& fighter : player.fighters()) {
            if (fighter.id() == fighterId) {
                return player;
            }
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
    if (fighter.defeated()) {
        return false;
    }
    if (card.owner() == Character::Any) {
        return true;
    }
    return fighter.cardOwner() == card.owner() && player.hasLivingCharacter(card.owner());
}

bool GameController::canAttackTarget(const Fighter& attacker, const Fighter& defender) const {
    if (attacker.defeated() || defender.defeated()) {
        return false;
    }
    if (board_.areAdjacentForCombat(attacker.spaceId(), defender.spaceId())) {
        return true;
    }
    return attacker.range() == AttackRange::Ranged && board_.shareZone(attacker.spaceId(), defender.spaceId());
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

bool GameController::isSpaceOccupiedByCurrentEnemy(int spaceId) const {
    for (const auto& fighter : opponentPlayer().fighters()) {
        if (!fighter.defeated() && fighter.spaceId() == spaceId) {
            return true;
        }
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
}

void GameController::drawCard(Player& player) {
    if (player.deck().empty()) {
        fatigue(player);
        return;
    }
    Card drawn = player.deck().back();
    player.deck().pop_back();
    std::string title = drawn.title();
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
        if (fighter.isHero()) {
            continue;
        }
        if (free.empty()) {
            throw InvalidSetup("Not enough free spaces for sidekick placement.");
        }
        int destination = free.front();
        free.erase(free.begin());
        fighter.placeAt(destination);
    }
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
    if (gameOver_) {
        return;
    }
    for (const auto& player : players_) {
        if (player.heroFighter().defeated()) {
            const Player& winner = opponentOf(player);
            gameOver_ = true;
            winnerName_ = winner.name() + " (" + heroKindName(winner.hero()) + ")";
          
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
        if (player.hero() != HeroKind::Dracula) {
            continue;
        }
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

std::vector<int> GameController::reachableForPlayerFighter(int playerIndex,
                                                           const std::string& fighterId,
                                                           int maxSteps) const {
    const Player& owner = playerByIndex(playerIndex);
    const Fighter& fighter = owner.fighterById(fighterId);
    if (fighter.defeated()) {
        return {};
    }

    auto occupiedByEnemy = [&](int spaceId) {
        for (const auto& player : players_) {
            if (player.id() == owner.id()) {
                continue;
            }
            for (const auto& other : player.fighters()) {
                if (!other.defeated() && other.spaceId() == spaceId) {
                    return true;
                }
            }
        }
        return false;
    };

    auto occupiedByAny = [&](int spaceId) {
        for (const auto& player : players_) {
            for (const auto& other : player.fighters()) {
                if (!other.defeated() && other.id() != fighter.id() && other.spaceId() == spaceId) {
                    return true;
                }
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
    if (maxSteps <= 0) {
        return;
    }
    const Player& player = playerByIndex(playerIndex);
    const Fighter& fighter = player.fighterById(fighterId);
    if (fighter.defeated()) {
        return;
    }

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
        if (!isSpaceOccupied(candidate)) {
            result.push_back(candidate);
        }
    }
    return result;
}

std::vector<int> GameController::valuesOnCard(const Card& card) const {
    std::vector<int> values;
    if (card.attack() >= 0) {
        values.push_back(card.attack());
    }
    if (card.defense() >= 0) {
        values.push_back(card.defense());
    }
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    return values;
}

bool GameController::cardEffectsProtectedBySherlock(const Card& card, const Player& owner) const {
    if (owner.hero() != HeroKind::Sherlock) {
        return false;
    }
    return card.owner() == Character::Sherlock || card.owner() == Character::Watson;
}

void GameController::resolveCombatEffectAfterDamage(const Card& card,
                                                    Player& cardPlayer,
                                                    Fighter& cardFighter,
                                                    Player& opposingPlayer,
                                                    Fighter& opposingFighter,
                                                    bool cardPlayerWon,
                                                    int directDamage) {
    (void)directDamage;
    switch (card.effect()) {
        case EffectId::Dash:
            queueOptionalMovement(cardPlayer.id(), cardFighter.id(), 3, card.title());
            break;
        case EffectId::Exploit:
            drawCard(cardPlayer);
            break;
        case EffectId::SherlockCounterpunch:
            if (!cardFighter.defeated() && !opposingFighter.defeated() &&
                board_.areAdjacentForCombat(cardFighter.spaceId(), opposingFighter.spaceId())) {
                opposingFighter.damage(2);
             
            }
            break;
        case EffectId::EducationNeverEnds:
            if (cardPlayerWon) {
                drawCard(opposingPlayer);
            } else {
                drawCard(cardPlayer);
                drawCard(cardPlayer);
            }
            break;
        case EffectId::SherlockFixedPoint: {
            Fighter& holmes = cardPlayer.heroFighter();
            Fighter& watson = cardPlayer.fighterById("watson");
            if (!holmes.defeated() && !watson.defeated() &&
                board_.areAdjacentForCombat(holmes.spaceId(), watson.spaceId())) {
                holmes.heal(1);
                watson.heal(1);
              
            }
            break;
        }
        case EffectId::SherlockStudyMethods:
            if (cardPlayerWon) {
                std::ostringstream handInfo;
                handInfo << opposingPlayer.name() << " hand: ";
                for (const auto& opponentCard : opposingPlayer.hand()) {
                    handInfo << opponentCard.title() << "; ";
                }
                
            }
            break;
        case EffectId::SherlockGameAfoot:
            queueOptionalMovement(cardPlayer.id(), cardFighter.id(), 3, card.title());
            break;
        case EffectId::SisterThirstForSustenance:
            if (cardPlayerWon && !opposingFighter.defeated()) {
                Fighter& dracula = cardPlayer.heroFighter();
                auto adjacent = board_.freeAdjacentSpaces(opposingFighter.spaceId(), [&](int spaceId) {
                    return isSpaceOccupied(spaceId);
                });
                if (!adjacent.empty() && !dracula.defeated()) {
                    dracula.placeAt(adjacent.front());
                    
                }
            }
            break;
        default:
            break;
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
        if (!fighter.defeated()) {
            total += fighter.move();
        }
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
    // توقف روی هم‌رزم ممنوع
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
    Player& opponent = opponentPlayer();
    if (chosenIndex < 0 || chosenIndex >= static_cast<int>(opponent.hand().size())) {
        throw RuleViolation("Invalid chosen card index.");
    }
    Card burned = opponent.removeCardFromHand(chosenIndex);
    int damage = std::max(0, burned.boost());
    opponent.heroFighter().damage(damage);
    opponent.addToDiscard(std::move(burned));
    checkDefeatedFighters();
    checkWinner();
}

}  // namespace unmatched
