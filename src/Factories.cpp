#include "unmatched/Factories.hpp"
#include "unmatched/Dracula.hpp"
#include "unmatched/Sherlock.hpp"
#include "unmatched/InvisibleMan.hpp"
#include "unmatched/GameController.hpp"
#include <utility>
#include <random>

namespace unmatched {

Board BoardFactory::createBaskervilleManor() const {
    std::vector<Space> spaces;

    auto addSpace = [&](int id,
                        int row,
                        int column,
                        std::vector<char> zones,
                        std::vector<int> adjacent,
                        bool secret = false,
                        int startSlot = 0) {
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

std::vector<Card> DeckFactory::createDeckForDracula() const {
    std::vector<Card> deck;
    
    auto addCopies = [&](int copies,
                         const std::string& title,
                         Character owner,
                         CardType type,
                         int attack,
                         int defense,
                         int boost,
                         Timing timing,
                         std::function<void(Fighter&, Fighter&, GameController&, int&, int&)> effect) {
        for (int i = 0; i < copies; ++i) {
            deck.emplace_back(title, title, owner, type, attack, defense, boost, timing, effect);
        }
    };


    auto realLivingSisters = [](GameController& controller, Player& owner) {
        std::vector<Fighter*> result;
        for (auto& fighter : owner.fighters()) {
            if (!fighter->isHero() && fighter->cardOwner() == Character::Sister && !fighter->defeated()) {
                result.push_back(fighter.get());
            }
        }
        (void)controller;
        return result;
    };

    addCopies(2, "FEEDING FRENZY", Character::Dracula, CardType::Attack, 2, -1, 3, Timing::DuringCombat,
        [realLivingSisters](Fighter& cardFighter, Fighter& defender, GameController& controller, int& attackValue, int&) {
            Player& owner = controller.mutableOwnerOfFighter(cardFighter.id());
            int bonus = 0;
            for (Fighter* sister : realLivingSisters(controller, owner)) {
                if (controller.board().shareZone(sister->spaceId(), defender.spaceId())) {
                    ++bonus;
                }
            }
            attackValue += bonus;
        });

    addCopies(2, "MISTFORM", Character::Dracula, CardType::Scheme, -1, -1, 2, Timing::None,
        [](Fighter& attacker, Fighter&, GameController& controller, int&, int&) {
            auto* dracula = dynamic_cast<Dracula*>(&attacker);
            if (!dracula) return;
            int destination = controller.currentSchemeChoice().destinationSpace;
            if (destination != -1 && controller.board().contains(destination) &&
                !controller.isSpaceOccupied(destination)) {
                dracula->placeAt(destination);
                controller.addAction(1);
                return;
            }
            for (const auto& space : controller.board().spaces()) {
                if (!controller.isSpaceOccupied(space.id())) {
                    dracula->placeAt(space.id());
                    controller.addAction(1);
                    break;
                }
            }
        });

    addCopies(2, "AMBUSH", Character::Any, CardType::Attack, 2, -1, 3, Timing::Immediately,
        [](Fighter&, Fighter&, GameController& controller, int& attackValue, int&) {
            Player& opponent = controller.opponentPlayer();
            if (opponent.hand().empty()) return;
            int idx = controller.getRandomInt(0, static_cast<int>(opponent.hand().size()) - 1);
            Card discarded = opponent.removeCardFromHand(idx);
            attackValue += discarded.getBoost();
            opponent.addToDiscard(std::move(discarded));
        });

    addCopies(2, "BAPTISM OF BLOOD", Character::Dracula, CardType::Scheme, -1, -1, 2, Timing::None,
        [](Fighter& attacker, Fighter&, GameController& controller, int&, int&) {
            auto* dracula = dynamic_cast<Dracula*>(&attacker);
            if (!dracula) return;
            dracula->heal(2);
            for (auto& fighterPtr : controller.currentPlayer().fighters()) {
                if (fighterPtr->isHero() || fighterPtr->cardOwner() != Character::Sister) continue;
                if (!fighterPtr->defeated()) continue;
                int heroSpace = dracula->spaceId();
                for (const auto& space : controller.board().spaces()) {
                    if (controller.board().shareZone(heroSpace, space.id()) &&
                        !controller.isSpaceOccupied(space.id())) {
                        fighterPtr->reviveAt(space.id());
                        break;
                    }
                }
                break;
            }
        });

    addCopies(2, "BEASTFORM", Character::Dracula, CardType::Attack, 6, -1, 4, Timing::DuringCombat,
        [](Fighter&, Fighter&, GameController&, int&, int&) {
        });

    addCopies(3, "DASH", Character::Any, CardType::Versatile, 3, 3, 1, Timing::AfterCombat,
        [](Fighter& attacker, Fighter&, GameController& controller, int&, int&) {
            const Player* owner = controller.ownerOfFighter(attacker.id());
            if (owner) {
                controller.queueOptionalMovement(owner->id(), attacker.id(), 3, "DASH");
            }
        });

    addCopies(3, "EXPLOIT", Character::Any, CardType::Versatile, 4, 4, 1, Timing::AfterCombat,
        [](Fighter& cardFighter, Fighter&, GameController& controller, int&, int&) {
            controller.drawCard(controller.mutableOwnerOfFighter(cardFighter.id()));
        });

    addCopies(3, "LOOK INTO MY EYES", Character::Dracula, CardType::Defense, -1, 1, 2, Timing::DuringCombat,
        [](Fighter&, Fighter&, GameController&, int&, int& defenseValue) {
            defenseValue += 2;
        });

    addCopies(2, "PREY UPON", Character::Dracula, CardType::Scheme, -1, -1, 4, Timing::None,
        [](Fighter& attacker, Fighter&, GameController& controller, int&, int&) {
            auto* dracula = dynamic_cast<Dracula*>(&attacker);
            if (!dracula) return;
            int healed = 0;
            for (auto& fighter : controller.opponentPlayer().fighters()) {
                if (!fighter->defeated() && 
                    controller.board().areAdjacentForCombat(dracula->spaceId(), fighter->spaceId())) {
                    fighter->damage(1);
                    ++healed;
                }
            }
            dracula->heal(healed);
        });

    addCopies(3, "RAVENING SEDUCTION", Character::Sister, CardType::Scheme, -1, -1, 2, Timing::None,
        [realLivingSisters](Fighter& attacker, Fighter&, GameController& controller, int&, int&) {
            const SchemeChoice& choice = controller.currentSchemeChoice();
            if (choice.targetFighterId.empty() || choice.destinationSpace == -1) return;

            Fighter* target = controller.findFighterById(choice.targetFighterId);
            if (!target || target->defeated()) return;
            if (controller.isSpaceOccupied(choice.destinationSpace)) return;

            target->placeAt(choice.destinationSpace);

            Player& owner = controller.mutableOwnerOfFighter(attacker.id());
            int damage = 0;
            for (Fighter* sister : realLivingSisters(controller, owner)) {
                if (controller.board().areAdjacentForCombat(sister->spaceId(), choice.destinationSpace)) {
                    ++damage;
                }
            }
            if (damage > 0) target->damage(damage);
        });

    addCopies(3, "THIRST FOR SUSTENANCE", Character::Sister, CardType::Attack, 3, -1, 3, Timing::AfterCombat,
        [](Fighter& attacker, Fighter& defender, GameController& controller, int&, int&) {
            auto* dracula = dynamic_cast<Dracula*>(&attacker);
            if (!dracula) return;
            auto adjacent = controller.board().freeAdjacentSpaces(defender.spaceId(), 
                [&](int spaceId) { return controller.isSpaceOccupied(spaceId); });
            if (!adjacent.empty() && !dracula->defeated()) {
                dracula->placeAt(adjacent.front());
            }
        });

    addCopies(3, "FEINT", Character::Any, CardType::Versatile, 2, 2, 2, Timing::Immediately,
        [](Fighter&, Fighter&, GameController&, int&, int&) {
        });

    return deck;
}

std::vector<Card> DeckFactory::createDeckForSherlock() const {
    std::vector<Card> deck;
    
    auto addCopies = [&](int copies,
                         const std::string& title,
                         Character owner,
                         CardType type,
                         int attack,
                         int defense,
                         int boost,
                         Timing timing,
                         std::function<void(Fighter&, Fighter&, GameController&, int&, int&)> effect) {
        for (int i = 0; i < copies; ++i) {
            deck.emplace_back(title, title, owner, type, attack, defense, boost, timing, effect);
        }
    };

    addCopies(2, "ADMINISTER AID", Character::Watson, CardType::Scheme, -1, -1, 2, Timing::None,
        [](Fighter& attacker, Fighter&, GameController& controller, int&, int&) {
            auto* sherlock = dynamic_cast<Sherlock*>(&attacker);
            if (!sherlock) return;
            Fighter& watson = sherlock->getWatson();
            if (watson.defeated()) return;

            int destination = controller.currentSchemeChoice().destinationSpace;
            bool destinationValid = destination != -1 &&
                controller.board().areAdjacentForCombat(sherlock->spaceId(), destination) &&
                !controller.isSpaceOccupied(destination);

            if (!destinationValid) {
                auto adjacent = controller.board().freeAdjacentSpaces(sherlock->spaceId(),
                    [&](int spaceId) { return controller.isSpaceOccupied(spaceId); });
                if (adjacent.empty()) return;
                destination = adjacent.front();
            }

            watson.placeAt(destination);
            sherlock->heal(1);
            controller.drawCard(controller.currentPlayer());
        });

    addCopies(3, "CONFIRM SUSPICION", Character::Sherlock, CardType::Scheme, -1, -1, 1, Timing::None,
        [](Fighter&, Fighter&, GameController&, int&, int&) {
        
        });

    addCopies(3, "COUNTERPUNCH", Character::Sherlock, CardType::Versatile, 3, 3, 1, Timing::AfterCombat,
        [](Fighter& attacker, Fighter& defender, GameController& controller, int&, int&) {
            if (!attacker.defeated() && !defender.defeated() &&
                controller.board().areAdjacentForCombat(attacker.spaceId(), defender.spaceId())) {
                defender.damage(2);
            }
        });

    addCopies(3, "DEDUCE STRATEGY", Character::Sherlock, CardType::Versatile, 3, 3, 1, Timing::DuringCombat,
        [](Fighter&, Fighter&, GameController&, int& attackValue, int&) {
            attackValue = 1;
        });

    addCopies(2, "EDUCATION NEVER ENDS", Character::Any, CardType::Versatile, 3, 3, 1, Timing::AfterCombat,
        [](Fighter&, Fighter&, GameController&, int&, int&) {
        });

    addCopies(2, "ELEMENTARY", Character::Sherlock, CardType::Defense, -1, 3, 3, Timing::DuringCombat,
        [](Fighter&, Fighter&, GameController&, int&, int&) {
        });

    addCopies(2, "ELIMINATE THE IMPOSSIBLE", Character::Sherlock, CardType::Scheme, -1, -1, 2, Timing::None,
        [](Fighter&, Fighter&, GameController& controller, int&, int&) {
            Player& opponent = controller.opponentPlayer();
            if (opponent.hand().empty()) return;
            int idx = controller.getRandomInt(0, static_cast<int>(opponent.hand().size()) - 1);
            Card discarded = opponent.removeCardFromHand(idx);
            opponent.addToDiscard(std::move(discarded));
        });

    addCopies(3, "FEINT", Character::Any, CardType::Versatile, 2, 2, 1, Timing::Immediately,
        [](Fighter&, Fighter&, GameController&, int&, int&) {
        });

    addCopies(2, "FIXED POINT IN A CHANGING AGE", Character::Watson, CardType::Versatile, 3, 3, 1, Timing::AfterCombat,
        [](Fighter& attacker, Fighter&, GameController& controller, int&, int&) {
            auto* sherlock = dynamic_cast<Sherlock*>(&attacker);
            if (!sherlock) return;
            Fighter& watson = sherlock->getWatson();
            if (!sherlock->defeated() && !watson.defeated() &&
                controller.board().areAdjacentForCombat(sherlock->spaceId(), watson.spaceId())) {
                sherlock->heal(1);
                watson.heal(1);
            }
        });

    addCopies(2, "MASTER OF DISGUISE", Character::Sherlock, CardType::Scheme, -1, -1, 2, Timing::None,
        [](Fighter& attacker, Fighter&, GameController& controller, int&, int&) {
            auto* sherlock = dynamic_cast<Sherlock*>(&attacker);
            if (!sherlock) return;

            Fighter* target = nullptr;
            const std::string& chosenId = controller.currentSchemeChoice().targetFighterId;
            if (!chosenId.empty()) {
                target = controller.findFighterById(chosenId);
                if (target && target->defeated()) target = nullptr;
            }
            if (!target) {
                for (auto& fighter : controller.opponentPlayer().fighters()) {
                    if (!fighter->defeated()) { target = fighter.get(); break; }
                }
            }
            if (!target) return;

            int sherlockSpace = sherlock->spaceId();
            int targetSpace = target->spaceId();
            sherlock->placeAt(targetSpace);
            target->placeAt(sherlockSpace);
            target->damage(1);
        });

    addCopies(2, "THE GAME IS AFOOT", Character::Sherlock, CardType::Attack, 5, -1, 2, Timing::AfterCombat,
        [](Fighter& attacker, Fighter&, GameController& controller, int&, int&) {
            const Player* owner = controller.ownerOfFighter(attacker.id());
            if (owner) {
                controller.queueOptionalMovement(owner->id(), attacker.id(), 3, "THE GAME IS AFOOT");
            }
        });

    addCopies(2, "SERVICE REVOLVER", Character::Watson, CardType::Attack, 5, -1, 3, Timing::None,
        nullptr);

    addCopies(2, "STUDY METHODS", Character::Any, CardType::Versatile, 3, 3, 2, Timing::AfterCombat,
        [](Fighter&, Fighter&, GameController&, int&, int&) {
        });

    return deck;
}

std::vector<Card> DeckFactory::createDeckForInvisibleMan() const {
    std::vector<Card> deck;
    
    auto addCopies = [&](int copies,
                         const std::string& title,
                         Character owner,
                         CardType type,
                         int attack,
                         int defense,
                         int boost,
                         Timing timing,
                         std::function<void(Fighter&, Fighter&, GameController&, int&, int&)> effect) {
        for (int i = 0; i < copies; ++i) {
            deck.emplace_back(title, title, owner, type, attack, defense, boost, timing, effect);
        }
    };

    
    addCopies(2, "CODED NOTES", Character::InvisibleMan, CardType::Defense, -1, 3, 2, Timing::AfterCombat,
        [](Fighter& cardFighter, Fighter&, GameController& controller, int&, int&) {
            Player& owner = controller.mutableOwnerOfFighter(cardFighter.id());
            controller.drawCard(owner);
            controller.drawCard(owner);
            controller.drawCard(owner);
            controller.queueDeckTopSelection(owner.id(), 2);
        });

    
    addCopies(2, "CONFOUND", Character::InvisibleMan, CardType::Versatile, 3, 3, 2, Timing::AfterCombat,
        [](Fighter& cardFighter, Fighter&, GameController& controller, int&, int&) {
            const Player* owner = controller.ownerOfFighter(cardFighter.id());
            if (!owner) return;
            controller.queueConfoundDecision(owner->id());
        });

    
    addCopies(3, "COVERT PREPARATION", Character::InvisibleMan, CardType::Versatile, 2, 2, 1, Timing::AfterCombat,
        [](Fighter& cardFighter, Fighter&, GameController& controller, int&, int&) {
            Player& owner = controller.mutableOwnerOfFighter(cardFighter.id());
            controller.drawCard(owner);
            auto* invisible = dynamic_cast<InvisibleMan*>(&cardFighter);
            if (!invisible || invisible->getFogSpaces().empty()) return;
            controller.queuePendingFogChoice(owner.id(), cardFighter.id(), {}, 2);
        });

    addCopies(2, "DREAMING OF REVENGE", Character::InvisibleMan, CardType::Versatile, 3, 3, 1, Timing::AfterCombat,
        [](Fighter& cardFighter, Fighter&, GameController& controller, int&, int&) {
            auto* invisible = dynamic_cast<InvisibleMan*>(&cardFighter);
            if (!invisible) return;
            if (!invisible->isOnFog(cardFighter.spaceId())) return;
        
            Player& enemy = controller.mutableOpponentOfFighter(cardFighter.id());
            for (auto& fighter : enemy.fighters()) {
                if (!fighter->defeated() && invisible->isOnFog(fighter->spaceId())) {
                    fighter->damage(1);
                }
            }
        });

    
    addCopies(2, "EMERGE FROM MIST", Character::InvisibleMan, CardType::Attack, 3, -1, 2, Timing::DuringCombat,
        [](Fighter&, Fighter&, GameController& controller, int& attackValue, int&) {
            if (controller.currentTurnStartedOnFog()) {
                attackValue = 5;
            }
        });

    addCopies(2, "IMPOSSIBLE TO SEE", Character::InvisibleMan, CardType::Versatile, 2, 2, 2, Timing::Immediately,
        [](Fighter&, Fighter&, GameController&, int& attackValue, int& defenseValue) {
            attackValue = 0;
            defenseValue = 0;
        });


    addCopies(2, "INTO THIN AIR", Character::InvisibleMan, CardType::Defense, -1, 4, 1, Timing::AfterCombat,
        [](Fighter& cardFighter, Fighter&, GameController& controller, int&, int&) {
            auto* invisible = dynamic_cast<InvisibleMan*>(&cardFighter);
            if (!invisible) return;
            Player& owner = controller.mutableOwnerOfFighter(cardFighter.id());
            controller.queueOptionalMovement(owner.id(), cardFighter.id(), 1, "INTO THIN AIR");
            if (!invisible->getFogSpaces().empty()) {
                controller.queuePendingFogChoice(owner.id(), cardFighter.id(), {}, 3);
            }
        });


    addCopies(2, "LURKING", Character::InvisibleMan, CardType::Defense, -1, 2, 2, Timing::AfterCombat,
        [](Fighter& cardFighter, Fighter&, GameController& controller, int&, int&) {
            Player& owner = controller.mutableOwnerOfFighter(cardFighter.id());
            controller.drawCard(owner);
            auto* invisible = dynamic_cast<InvisibleMan*>(&cardFighter);
            if (!invisible || invisible->getFogSpaces().empty()) return;
            controller.queueFogTeleport(owner.id(), cardFighter.id());
        });

    addCopies(2, "REIGN OF TERROR", Character::InvisibleMan, CardType::Scheme, -1, -1, 1, Timing::None,
        [](Fighter& attacker, Fighter&, GameController& controller, int&, int&) {
            auto* invisible = dynamic_cast<InvisibleMan*>(&attacker);
            if (!invisible) return;
            if (!invisible->isOnFog(attacker.spaceId())) return;
            for (auto& fighter : controller.opponentPlayer().fighters()) {
                if (!fighter->defeated()) {
                    fighter->damage(2);
                }
            }
        });

    addCopies(2, "ROLLING FOG", Character::InvisibleMan, CardType::Scheme, -1, -1, 1, Timing::None,
        [](Fighter& attacker, Fighter&, GameController& controller, int&, int&) {
            auto* invisible = dynamic_cast<InvisibleMan*>(&attacker);
            if (!invisible || invisible->getFogSpaces().empty()) return;
            controller.addAction(1);
            const Player* owner = controller.ownerOfFighter(attacker.id());
            if (owner) {
                controller.queuePendingFogChoice(owner->id(), attacker.id(), {}, 999);
            }
        });

    addCopies(3, "SLIP AWAY", Character::InvisibleMan, CardType::Attack, 3, -1, 2, Timing::AfterCombat,
        [](Fighter& attacker, Fighter&, GameController& controller, int&, int&) {
            auto* invisible = dynamic_cast<InvisibleMan*>(&attacker);
            if (!invisible) return;
            for (int i = 0; i < 3; ++i) {
                if (invisible->getFogTokens()[i] == attacker.spaceId()) {
                    for (const auto& space : controller.board().spaces()) {
                        if (!controller.isSpaceOccupied(space.id()) && space.id() != attacker.spaceId()) {
                            invisible->moveFogToken(i, space.id());
                            attacker.placeAt(space.id());
                            return;
                        }
                    }
                }
            }
        });

    
    addCopies(2, "STEP LIGHTLY", Character::InvisibleMan, CardType::Scheme, -1, -1, 1, Timing::None,
        [](Fighter& attacker, Fighter&, GameController& controller, int&, int&) {
            auto* invisible = dynamic_cast<InvisibleMan*>(&attacker);
            if (!invisible) return;
            int damage = invisible->isOnFog(attacker.spaceId()) ? 3 : 1;
            for (auto& fighter : controller.opponentPlayer().fighters()) {
                if (!fighter->defeated() &&
                    controller.board().areAdjacentForCombat(attacker.spaceId(), fighter->spaceId())) {
                    fighter->damage(damage);
                    break;
                }
            }
            const Player* owner = controller.ownerOfFighter(attacker.id());
            if (owner && !invisible->getFogSpaces().empty()) {
                int opponentIdx = (owner->id() == 0) ? 1 : 0;
                controller.queuePendingFogChoice(opponentIdx, attacker.id(), {}, 2);
            }
        });

    
    addCopies(2, "VANISH", Character::InvisibleMan, CardType::Scheme, -1, -1, 3, Timing::None,
        [](Fighter& attacker, Fighter&, GameController& controller, int&, int&) {
            attacker.heal(1);
            Player& owner = controller.mutableOwnerOfFighter(attacker.id());
            attacker.removeFromBoard();
            controller.setPendingVanishedPlacement(true, owner.id());
            if (controller.actionsRemaining() == 2) {
                controller.forceEndActionsThisTurn();
            }
        });

    addCopies(2, "DASH", Character::Any, CardType::Versatile, 3, 3, 1, Timing::AfterCombat,
        [](Fighter& attacker, Fighter&, GameController& controller, int&, int&) {
            const Player* owner = controller.ownerOfFighter(attacker.id());
            if (owner) {
                controller.queueOptionalMovement(owner->id(), attacker.id(), 3, "DASH");
            }
        });

    addCopies(3, "EXPLOIT", Character::Any, CardType::Versatile, 4, 4, 1, Timing::AfterCombat,
        [](Fighter& cardFighter, Fighter&, GameController& controller, int&, int&) {
            controller.drawCard(controller.mutableOwnerOfFighter(cardFighter.id()));
        });

    addCopies(3, "FEINT", Character::Any, CardType::Versatile, 2, 2, 2, Timing::Immediately,
        [](Fighter&, Fighter&, GameController&, int&, int&) {
        });

    return deck;
}

std::vector<std::unique_ptr<Fighter>> FighterFactory::createInvisibleManFighters() const {
    std::vector<std::unique_ptr<Fighter>> fighters;
    fighters.push_back(std::make_unique<InvisibleMan>());
    return fighters;
}

std::vector<std::unique_ptr<Fighter>> FighterFactory::createDraculaFighters() const {
    std::vector<std::unique_ptr<Fighter>> fighters;
    fighters.push_back(std::make_unique<Dracula>());
    fighters.push_back(std::make_unique<Fighter>(
        FighterDefinition("sister1", "Sister I", Character::Sister, false, 1, 2, AttackRange::Melee, "Sidekick")
    ));
    fighters.push_back(std::make_unique<Fighter>(
        FighterDefinition("sister2", "Sister II", Character::Sister, false, 1, 2, AttackRange::Melee, "Sidekick")
    ));
    fighters.push_back(std::make_unique<Fighter>(
        FighterDefinition("sister3", "Sister III", Character::Sister, false, 1, 2, AttackRange::Melee, "Sidekick")
    ));
    return fighters;
}

std::vector<std::unique_ptr<Fighter>> FighterFactory::createSherlockFighters() const {
    std::vector<std::unique_ptr<Fighter>> fighters;
    fighters.push_back(std::make_unique<Sherlock>());
    fighters.push_back(std::make_unique<Fighter>(
        FighterDefinition("watson", "Dr. Watson", Character::Watson, false, 8, 2, AttackRange::Ranged, "Support")
    ));
    return fighters;
}

} // namespace unmatched
