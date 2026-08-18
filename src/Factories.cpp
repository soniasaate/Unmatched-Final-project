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

    addCopies(2, "FEEDING FRENZY", Character::Dracula, CardType::Attack, 2, -1, 3, Timing::DuringCombat,
        [](Fighter& attacker, Fighter& defender, GameController& controller, int& attackValue, int&) {
            const Player* owner = controller.ownerOfFighter(attacker.id());
            if (!owner) return;
            int bonus = 0;
            for (const auto& fighter : owner->fighters()) {
                if (!fighter->defeated() && fighter->cardOwner() == Character::Sister &&
                    controller.board().shareZone(fighter->spaceId(), defender.spaceId())) {
                    ++bonus;
                }
            }
            attackValue += bonus;
        });

    addCopies(2, "MISTFORM", Character::Dracula, CardType::Scheme, -1, -1, 2, Timing::None,
        [](Fighter& attacker, Fighter&, GameController& controller, int&, int&) {
            auto* dracula = dynamic_cast<Dracula*>(&attacker);
            if (!dracula) return;
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
            const Player* owner = controller.ownerOfFighter(attacker.id());
            if (!owner) return;
            for (auto& fighter : owner->fighters()) {
                if (fighter->cardOwner() == Character::Sister && fighter->defeated()) {
                    std::vector<int> freeSpaces = controller.freeSpacesSharingHeroZone(*owner);
                    if (!freeSpaces.empty()) {
                        fighter->reviveAt(freeSpaces.front());
                        break;
                    }
                }
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
        [](Fighter&, Fighter&, GameController& controller, int&, int&) {
            controller.drawCard(controller.currentPlayer());
        });

    addCopies(3, "LOOK INTO MY EYES", Character::Dracula, CardType::Defense, -1, 1, 2, Timing::DuringCombat,
        [](Fighter&, Fighter&, GameController&, int&, int&) {
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
        [](Fighter& attacker, Fighter&, GameController& controller, int&, int&) {
            const Player* owner = controller.ownerOfFighter(attacker.id());
            if (!owner) return;
            for (auto& fighter : controller.opponentPlayer().fighters()) {
                if (!fighter->defeated()) {
                    int current = fighter->spaceId();
                    for (int neighbor : controller.board().movementNeighbors(current)) {
                        if (!controller.isSpaceOccupied(neighbor)) {
                            fighter->placeAt(neighbor);
                            int damage = 0;
                            for (const auto& sister : owner->fighters()) {
                                if (!sister->defeated() && sister->cardOwner() == Character::Sister &&
                                    controller.board().areAdjacentForCombat(sister->spaceId(), neighbor)) {
                                    ++damage;
                                }
                            }
                            if (damage > 0) fighter->damage(damage);
                            return;
                        }
                    }
                    break;
                }
            }
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
            auto adjacent = controller.board().freeAdjacentSpaces(sherlock->spaceId(), 
                [&](int spaceId) { return controller.isSpaceOccupied(spaceId); });
            if (!adjacent.empty()) {
                watson.placeAt(adjacent.front());
                sherlock->heal(1);
                controller.drawCard(controller.currentPlayer());
            }
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
            for (auto& fighter : controller.opponentPlayer().fighters()) {
                if (!fighter->defeated()) {
                    int sherlockSpace = sherlock->spaceId();
                    int targetSpace = fighter->spaceId();
                    sherlock->placeAt(targetSpace);
                    fighter->placeAt(sherlockSpace);
                    fighter->damage(1);
                    break;
                }
            }
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
        [](Fighter&, Fighter&, GameController&, int&, int&) {
        });

    addCopies(2, "CONFOUND", Character::InvisibleMan, CardType::Versatile, 3, 3, 2, Timing::AfterCombat,
        [](Fighter&, Fighter&, GameController&, int&, int&) {
        });
            
    addCopies(3, "COVERT PREPARATION", Character::InvisibleMan, CardType::Versatile, 2, 2, 1, Timing::AfterCombat,
        [](Fighter& attacker, Fighter&, GameController& controller, int&, int&) {
            controller.drawCard(controller.currentPlayer());
            auto* invisible = dynamic_cast<InvisibleMan*>(&attacker);
            if (!invisible) return;
            for (int i = 0; i < 3; ++i) {
                if (invisible->getFogTokens()[i] != -1) {
                    int current = invisible->getFogTokens()[i];
                    auto occupiedByEnemy = [&](int spaceId) {
                        for (const auto& fighter : controller.opponentPlayer().fighters()) {
                            if (!fighter->defeated() && fighter->spaceId() == spaceId) return true;
                        }
                        return false;
                    };
                    auto occupiedByAny = [&](int spaceId) {
                        return controller.isSpaceOccupied(spaceId);
                    };
                    auto reachable = controller.board().reachableSpaces(current, 2, occupiedByEnemy, occupiedByAny);
                    if (!reachable.empty()) {
                        invisible->moveFogToken(i, reachable.front());
                        return;
                    }
                }
            }
        });

    addCopies(2, "DREAMING OF REVENGE", Character::InvisibleMan, CardType::Versatile, 3, 3, 1, Timing::AfterCombat,
        [](Fighter& attacker, Fighter&, GameController& controller, int&, int&) {
            auto* invisible = dynamic_cast<InvisibleMan*>(&attacker);
            if (!invisible) return;
            if (!invisible->isOnFog(attacker.spaceId())) return;
            for (auto& fighter : controller.opponentPlayer().fighters()) {
                if (!fighter->defeated() && invisible->isOnFog(fighter->spaceId())) {
                    fighter->damage(1);
                }
            }
        });

    addCopies(2, "EMERGE FROM MIST", Character::InvisibleMan, CardType::Attack, 3, -1, 2, Timing::DuringCombat,
        [](Fighter& attacker, Fighter&, GameController&, int& attackValue, int&) {
            auto* invisible = dynamic_cast<InvisibleMan*>(&attacker);
            if (invisible && invisible->isOnFog(attacker.spaceId())) {
                attackValue = 5;
            }
        });

    addCopies(2, "IMPOSSIBLE TO SEE", Character::InvisibleMan, CardType::Versatile, 2, 2, 2, Timing::Immediately,
        [](Fighter&, Fighter&, GameController&, int& attackValue, int& defenseValue) {
            attackValue = 0;
            defenseValue = 0;
        });

   addCopies(2, "INTO THIN AIR", Character::InvisibleMan, CardType::Defense, -1, 4, 1, Timing::AfterCombat,
        [](Fighter& attacker, Fighter&, GameController& controller, int&, int&) {
            auto* invisible = dynamic_cast<InvisibleMan*>(&attacker);
            if (!invisible) return;
            const Player* owner = controller.ownerOfFighter(attacker.id());
            if (owner) {
                controller.queueOptionalMovement(owner->id(), attacker.id(), 1, "INTO THIN AIR");
            }
            for (int i = 0; i < 3; ++i) {
                if (invisible->getFogTokens()[i] != -1) {
                    int current = invisible->getFogTokens()[i];
                    auto occupiedByEnemy = [&](int spaceId) {
                        for (const auto& fighter : controller.opponentPlayer().fighters()) {
                            if (!fighter->defeated() && fighter->spaceId() == spaceId) return true;
                        }
                        return false;
                    };
                    auto occupiedByAny = [&](int spaceId) {
                        return controller.isSpaceOccupied(spaceId);
                    };
                    auto reachable = controller.board().reachableSpaces(current, 3, occupiedByEnemy, occupiedByAny);
                    if (!reachable.empty()) {
                        invisible->moveFogToken(i, reachable.front());
                        return;
                    }
                }
            }
        });

    addCopies(2, "LURKING", Character::InvisibleMan, CardType::Defense, -1, 2, 2, Timing::AfterCombat,
        [](Fighter& attacker, Fighter&, GameController& controller, int&, int&) {
            controller.drawCard(controller.currentPlayer());
            auto* invisible = dynamic_cast<InvisibleMan*>(&attacker);
            if (!invisible) return;
            for (int i = 0; i < 3; ++i) {
                if (invisible->getFogTokens()[i] != -1) {
                    int current = invisible->getFogTokens()[i];
                    for (const auto& space : controller.board().spaces()) {
                        if (space.id() != current) {
                            invisible->moveFogToken(i, space.id());
                            return;
                        }
                    }
                }
            }
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
            if (!invisible) return;
            for (int i = 0; i < 3; ++i) {
                if (invisible->getFogTokens()[i] != -1) {
                    int current = invisible->getFogTokens()[i];
                    for (const auto& space : controller.board().spaces()) {
                        if (space.id() != current) {
                            invisible->moveFogToken(i, space.id());
                            controller.addAction(1);
                            return;
                        }
                    }
                }
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
        });
        
    addCopies(2, "VANISH", Character::InvisibleMan, CardType::Scheme, -1, -1, 3, Timing::None,
        [](Fighter& attacker, Fighter&, GameController& controller, int&, int&) {
            attacker.heal(1);
            attacker.removeFromBoard();
            controller.setPendingVanishedPlacement(true);
            if (controller.actionsRemaining() == 2) {
                controller.decrementActions();
                controller.endTurnIfNeeded();
            }
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
