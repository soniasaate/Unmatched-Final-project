#include "unmatched/TuiApp.hpp"
#include "unmatched/GameExceptions.hpp"
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace unmatched {
namespace {

std::string repeat(char value, int count) {
    return std::string(static_cast<std::size_t>(std::max(0, count)), value);
}

std::string fixedWidth(const std::string& value, int width) {
    if (static_cast<int>(value.size()) >= width) {
        return value.substr(0, static_cast<std::size_t>(width));
    }
    return value + repeat(' ', width - static_cast<int>(value.size()));
}

std::string hpBar(int current, int maximum) {
    int width = 14;
    int filled = maximum <= 0 ? 0 : current * width / maximum;
    return "[" + repeat('#', filled) + repeat('.', width - filled) + "]";
}

} // namespace

using namespace ftxui;

TuiApp::TuiApp(ScreenInteractive& screen)
    : screen_(screen),
      state_(ScreenState::MainMenu),
      selected_(0),
      ageStep_(0),
      playerOneAge_(0),
      playerTwoAge_(0),
      selectedHero_(HeroKind::Dracula),
      selectedStartSlot_(1),
      asciiOnlyMode_(true),
      selectedAttackCardIndex_(-1),
      selectedSchemeCardIndex_(-1) {}

Element TuiApp::Render() {
    switch (state_) {
        case ScreenState::MainMenu:
            return renderMainMenu();
        case ScreenState::Help:
            return renderHelp();
        case ScreenState::SetupAge:
            return renderSetupAge();
        case ScreenState::FighterSelect:
            return renderFighterSelect();
        case ScreenState::StartSelect:
            return renderStartSelect();
        case ScreenState::GameOver:
            return renderGameOver();
        default:
            return renderGame();
    }
}

Element TuiApp::renderMainMenu() const {
    Elements body;
    body.push_back(text("UNMATCHED TUI - Dracula vs Sherlock Holmes") | bold | center);
    body.push_back(separator());
    body.push_back(text("Baskerville Manor duel simulator") | center);
    body.push_back(text("Use Arrow keys and Enter.") | dim | center);
    body.push_back(separator());
    body.push_back(renderMenuLines(currentMenuEntries(), "Main Menu"));
    if (!errorMessage_.empty()) {
        body.push_back(separator());
        body.push_back(text(errorMessage_) | color(Color::Red));
    }
    return vbox(std::move(body)) | border | size(WIDTH, GREATER_THAN, 72) | center;
}

Element TuiApp::renderHelp() const {
    Elements rules;
    rules.push_back(text("How to play") | bold | color(Color::Yellow));
    rules.push_back(text("1. The younger player chooses a fighter."));
    rules.push_back(text("2. The younger player chooses start space 1 or 2."));
    rules.push_back(text("3. Each player shuffles a 30-card deck and draws 5 cards."));
    rules.push_back(text("4. On your turn you must take exactly 2 actions."));
    rules.push_back(text("5. Maneuver draws 1 card, then may boost and move."));
    rules.push_back(text("6. Melee attacks adjacent enemies."));
    rules.push_back(text("7. Secret passages count as movement neighbors only."));
    rules.push_back(text("8. At end of turn discard down to 7 cards."));
    rules.push_back(separator());
    rules.push_back(renderMenuLines(currentMenuEntries(), "Return"));
    if (!errorMessage_.empty()) {
        rules.push_back(text(errorMessage_) | color(Color::Red));
    }
    return vbox(std::move(rules)) | border | size(WIDTH, GREATER_THAN, 92) | center;
}

Element TuiApp::renderSetupAge() const {
    std::string prompt = ageStep_ == 0 ? "Enter Player 1 age" : "Enter Player 2 age";
    Elements body;
    body.push_back(text("New Game Setup") | bold | color(Color::Yellow));
    body.push_back(separator());
    body.push_back(text(prompt));
    body.push_back(text("> " + ageInput_) | color(Color::Cyan));
    body.push_back(text("Digits, Backspace, Enter. Escape returns to main menu.") | dim);
    if (!errorMessage_.empty()) {
        body.push_back(separator());
        body.push_back(text(errorMessage_) | color(Color::Red));
    }
    return vbox(std::move(body)) | border | size(WIDTH, GREATER_THAN, 70) | center;
}

Element TuiApp::renderFighterSelect() const {
    Elements body;
    body.push_back(text("Fighter Selection") | bold | color(Color::Yellow));
    body.push_back(separator());
    body.push_back(text("The younger player chooses first."));
    body.push_back(renderMenuLines(currentMenuEntries(), "Choose Fighter"));
    if (!errorMessage_.empty()) {
        body.push_back(separator());
        body.push_back(text(errorMessage_) | color(Color::Red));
    }
    return vbox(std::move(body)) | border | size(WIDTH, GREATER_THAN, 76) | center;
}

Element TuiApp::renderStartSelect() const {
    Elements body;
    body.push_back(text("Initial Placement") | bold | color(Color::Yellow));
    body.push_back(separator());
    body.push_back(text("Choose the younger player's start space."));
    body.push_back(renderMenuLines(currentMenuEntries(), "Start Space"));
    if (!errorMessage_.empty()) {
        body.push_back(separator());
        body.push_back(text(errorMessage_) | color(Color::Red));
    }
    return vbox(std::move(body)) | border | size(WIDTH, GREATER_THAN, 76) | center;
}

Element TuiApp::renderGame() const {
    if (!controller_.started()) {
        return renderMainMenu();
    }

    const auto& players = controller_.players();
    Elements page;
    page.push_back(text("UNMATCHED TUI - Dracula vs Sherlock Holmes") | bold | center);
    page.push_back(text("Turn " + std::to_string(controller_.turnNumber()) + " - " +
                    controller_.currentPlayer().name() + " / " +
                    heroKindName(controller_.currentPlayer().hero()) +
                    " - Actions: " + std::to_string(controller_.actionsRemaining()) +
                    " - Moves: " + std::to_string(controller_.pendingMovementPoints()) + "/" +
                    std::to_string(controller_.maxMovementPoints())) |
               color(heroColor(controller_.currentPlayer().hero())) | center);
    page.push_back(separator());
    page.push_back(hbox({
        renderPlayerPanel(players.at(0), controller_.currentPlayerIndex() == 0) | size(WIDTH, EQUAL, 34),
        renderMapPanel() | flex,
        renderPlayerPanel(players.at(1), controller_.currentPlayerIndex() == 1) | size(WIDTH, EQUAL, 34),
    }));
    page.push_back(separator());
    page.push_back(hbox({
        renderHandPanel(players.at(0), controller_.currentPlayerIndex() == 0) | size(WIDTH, EQUAL, 42),
        renderActionPanel() | flex,
        renderHandPanel(players.at(1), controller_.currentPlayerIndex() == 1) | size(WIDTH, EQUAL, 42),
    }));
    page.push_back(separator());
    page.push_back(text("Zones: b=blue  r=brown  p=purple  y=yellow  g=green  d=dark-blue  e=grey") | dim | center);
    if (!errorMessage_.empty()) {
        page.push_back(separator());
        page.push_back(text("Error: " + errorMessage_) | color(Color::Red) | bold | center);
    }
    return vbox(std::move(page));
}

Element TuiApp::renderGameOver() const {
    Elements body;
    body.push_back(text("Game Over") | bold | color(Color::Yellow) | center);
    body.push_back(separator());
    body.push_back(text("Winner: " + controller_.winnerName()) | color(Color::Green) | center);
    body.push_back(separator());
    body.push_back(renderMenuLines(currentMenuEntries(), "Next"));
    return vbox(std::move(body)) | border | center | size(WIDTH, GREATER_THAN, 70);
}

Element TuiApp::renderPlayerPanel(const Player& player, bool active) const {
    Elements rows;
    rows.push_back(text(player.name() + " - " + heroKindName(player.hero())) | bold | color(heroColor(player.hero())));
    rows.push_back(text(active ? "ACTIVE TURN" : "Waiting") | color(active ? Color::Green : Color::GrayDark));
    for (const auto& fighter : player.fighters()) {
        rows.push_back(fighterLine(fighter));
    }
    rows.push_back(text("Deck: " + std::to_string(player.deck().size()) +
                        "   Hand: " + std::to_string(player.hand().size()) +
                        "   Discard: " + std::to_string(player.discardPile().size())));
    return vbox(std::move(rows));
}

Element TuiApp::renderHandPanel(const Player& player, bool active) const {
    Elements rows;
    rows.push_back(text(player.name() + " Hand") | bold | color(active ? heroColor(player.hero()) : Color::White));
    rows.push_back(separator());
    if (player.hand().empty()) {
        rows.push_back(text("(empty)") | dim);
    } else {
        int maxCards = std::min(8, static_cast<int>(player.hand().size()));
        for (int i = 0; i < maxCards; ++i) {
            const Card& card = player.hand().at(static_cast<std::size_t>(i));
            rows.push_back(cardElement(card, i, false, active));
        }
        if (static_cast<int>(player.hand().size()) > maxCards) {
            rows.push_back(text("... +" + std::to_string(player.hand().size() - maxCards) + " more") | dim);
        }
    }
    return window(text(active ? "Current Player Cards" : "Opponent Cards"), vbox(std::move(rows)));
}

Element TuiApp::renderActionPanel() const {
    std::string title = "Action Menu";
    switch (state_) {
        case ScreenState::ManeuverBoost: title = "Maneuver - Boost"; break;
        case ScreenState::ManeuverFighter: title = "Maneuver - Move Fighter"; break;
        case ScreenState::ManeuverDestination: title = "Maneuver - Destination"; break;
        case ScreenState::AttackAttacker: title = "Attack - Attacker"; break;
        case ScreenState::AttackTarget: title = "Attack - Target"; break;
        case ScreenState::AttackCard: title = "Attack - Card"; break;
        case ScreenState::AttackBeastBoost: title = "Attack - Beast Form"; break;
        case ScreenState::DefenseCard: title = "Defense - Card"; break;
        case ScreenState::DefenseElementaryPrediction: title = "Defense - Prediction"; break;
        case ScreenState::SchemeCard: title = "Scheme - Card"; break;
        case ScreenState::SchemeChoice: title = "Scheme - Choice"; break;
        case ScreenState::DiscardCard: case ScreenState::DiscardToLimit: title = "Discard Cards"; break;
        case ScreenState::DraculaAbilityTarget: title = "Dracula Ability"; break;
        default: title = "Action Menu"; break;
    }
    return renderMenuLines(currentMenuEntries(), title);
}

Element TuiApp::renderMapPanel() const {
    Elements lines;
    lines.push_back(text("MAP - Baskerville Manor") | bold | color(Color::Green) | center);
    lines.push_back(separator());
    auto mapLines = controller_.board().renderLines(controller_.occupantTokens());
    for (const auto& line : mapLines) {
        lines.push_back(text(line));
    }
    lines.push_back(separator());
    lines.push_back(text("Legend: [D] Dracula  [Si] Sister  [H] Holmes  [W] Watson  (~id) Secret passage  <S1>/<S2> Start") | dim);
    return window(text("Board"), vbox(std::move(lines)));
}

Element TuiApp::renderMenuLines(const std::vector<std::string>& entries, const std::string& title) const {
    Elements rows;
    rows.push_back(text(title) | bold | color(Color::Yellow));
    rows.push_back(separator());
    if (entries.empty()) {
        rows.push_back(text("(no legal options)") | color(Color::Red));
    } else {
        for (int i = 0; i < static_cast<int>(entries.size()); ++i) {
            rows.push_back(menuLine(entries.at(static_cast<std::size_t>(i)), i == selected_));
        }
    }
    return window(text(title), vbox(std::move(rows)));
}


bool TuiApp::OnEvent(Event event) {
    try {
        return handleEvent(event);
    } catch (const GameException& exception) {
        showError(exception.what());
        return true;
    } catch (const std::exception& exception) {
        showError(std::string("Unexpected error: ") + exception.what());
        return true;
    }
}

bool TuiApp::handleEvent(Event event) {
    if (state_ == ScreenState::SetupAge) {
        if (event == Event::Escape) {
            state_ = ScreenState::MainMenu;
            resetSelection();
            return true;
        }
        if (event == Event::Backspace) {
            handleBackspace();
            return true;
        }
        if (event == Event::Return) {
            handleEnter();
            return true;
        }
        if (event.is_character() && event.character().size() == 1 && std::isdigit(static_cast<unsigned char>(event.character()[0]))) {
            handleDigit(event.character()[0]);
            return true;
        }
        return false;
    }

    if (event == Event::Escape) {
        if (state_ == ScreenState::MainMenu) {
            screen_.ExitLoopClosure()();
        } else if (state_ == ScreenState::Help) {
            state_ = controller_.started() ? ScreenState::Game : ScreenState::MainMenu;
        } else if (controller_.started()) {
            state_ = ScreenState::Game;
        } else {
            state_ = ScreenState::MainMenu;
        }
        resetSelection();
        return true;
    }
    if (event == Event::ArrowUp) {
        moveSelection(-1);
        return true;
    }
    if (event == Event::ArrowDown) {
        moveSelection(1);
        return true;
    }
    if (event == Event::Return) {
        handleEnter();
        return true;
    }
    if (event == Event::Character("q") || event == Event::Character("Q")) {
        screen_.ExitLoopClosure()();
        return true;
    }
    return false;
}

void TuiApp::handleEnter() {
    errorMessage_.clear();
    switch (state_) {
        case ScreenState::MainMenu:
            if (selected_ == 0) {
                ageStep_ = 0;
                ageInput_.clear();
                playerOneAge_ = 0;
                playerTwoAge_ = 0;
                state_ = ScreenState::SetupAge;
            } else if (selected_ == 1) {
                state_ = ScreenState::Help;
            } else {
                screen_.ExitLoopClosure()();
            }
            resetSelection();
            break;

        case ScreenState::Help:
            state_ = controller_.started() ? ScreenState::Game : ScreenState::MainMenu;
            resetSelection();
            break;

        case ScreenState::SetupAge: {
            if (ageInput_.empty()) {
                showError("Please enter an age.");
                return;
            }
            int age = std::stoi(ageInput_);
            if (age <= 0) {
                showError("Age must be positive.");
                return;
            }
            if (ageStep_ == 0) {
                playerOneAge_ = age;
                ageInput_.clear();
                ageStep_ = 1;
            } else {
                playerTwoAge_ = age;
                state_ = ScreenState::FighterSelect;
                resetSelection();
            }
            break;
        }

        case ScreenState::FighterSelect:
            selectedHero_ = selected_ == 0 ? HeroKind::Dracula : HeroKind::Sherlock;
            state_ = ScreenState::StartSelect;
            resetSelection();
            break;

        case ScreenState::StartSelect:
            selectedStartSlot_ = selected_ == 0 ? 1 : 2;
            startGameFromSetup();
            break;

        case ScreenState::Game: {
            auto entries = currentMenuEntries();
            if (entries.empty()) {
                showError("No menu entries available.");
                return;
            }
            if (selected_ < 0 || selected_ >= static_cast<int>(entries.size())) {
                showError("Invalid selection.");
                return;
            }
            std::string choice = entries.at(static_cast<std::size_t>(selected_));
            if (choice == "Use Dracula start ability") {
                pendingFighterIds_.clear();
                const Fighter& dracula = controller_.currentPlayer().heroFighter();
                for (const auto& player : controller_.players()) {
                    for (const auto& fighter : player.fighters()) {
                        if (!fighter.defeated() && fighter.id() != dracula.id() &&
                            controller_.board().areAdjacentForCombat(dracula.spaceId(), fighter.spaceId())) {
                            pendingFighterIds_.push_back(fighter.id());
                        }
                    }
                }
                if (pendingFighterIds_.empty()) {
                    showError("No valid targets for Dracula ability.");
                    return;
                }
                state_ = ScreenState::DraculaAbilityTarget;
                resetSelection();
            } else if (choice == "Maneuver") {
                beginManeuverFlow();
            } else if (choice == "Attack") {
                beginAttackFlow();
            } else if (choice == "Scheme") {
                beginSchemeFlow();
            } else if (choice == "Discarding Cards") {
                pendingCardIndexes_.clear();
                for (int i = 0; i < static_cast<int>(controller_.currentPlayer().hand().size()); ++i) {
                    pendingCardIndexes_.push_back(i);
                }
                if (pendingCardIndexes_.empty()) {
                    showError("No cards to discard.");
                    return;
                }
                state_ = ScreenState::DiscardCard;
                resetSelection();
            } else if (choice == "Help") {
                state_ = ScreenState::Help;
                resetSelection();
            } else if (choice == "Back to main menu") {
                state_ = ScreenState::MainMenu;
                resetSelection();
            }
            break;
        }

        case ScreenState::ManeuverBoost: {
            if (selected_ < 0 || selected_ >= static_cast<int>(pendingCardIndexes_.size()) + 1) {
                showError("Invalid boost selection.");
                return;
            }
            std::optional<int> boostIndex;
            if (selected_ > 0) {
                boostIndex = pendingCardIndexes_.at(static_cast<std::size_t>(selected_ - 1));
            }
            controller_.beginManeuver(boostIndex.value_or(-1));
            if (controller_.gameOver()) {
                openGameScreen();
                break;
            }
            pendingFighterIds_ = controller_.movableCurrentFighterIds();
            if (pendingFighterIds_.empty()) {
                controller_.finishManeuver();
                openGameScreen();
                break;
            }
            state_ = ScreenState::ManeuverFighter;
            resetSelection();
            break;
        }

        case ScreenState::ManeuverFighter: {
            if (selected_ == 0) {
                controller_.finishManeuver();
                openGameScreen();
                break;
            }
            int index = selected_ - 1;
            if (index < 0 || index >= static_cast<int>(pendingFighterIds_.size())) {
                showError("Invalid fighter selection.");
                return;
            }
            selectedAttackerId_ = pendingFighterIds_.at(static_cast<std::size_t>(index));
            pendingSpaces_ = controller_.reachableDestinationsFor(selectedAttackerId_);
            state_ = ScreenState::ManeuverDestination;
            resetSelection();
            break;
        }

        case ScreenState::ManeuverDestination: {
            if (selected_ == 0) {
                controller_.finishCurrentFighter(selectedAttackerId_);
                pendingFighterIds_ = controller_.movableCurrentFighterIds();
                if (!pendingFighterIds_.empty()) {
                    state_ = ScreenState::ManeuverFighter;
                } else {
                    controller_.finishManeuver();
                    openGameScreen();
                }
                resetSelection();
                break;
            }
            int index = selected_ - 1;
            if (index < 0 || index >= static_cast<int>(pendingSpaces_.size())) {
                showError("Invalid destination.");
                return;
            }
            int destination = pendingSpaces_.at(static_cast<std::size_t>(index));
            controller_.moveCurrentFighter(selectedAttackerId_, destination);

            if (controller_.remainingMovementForFighter(selectedAttackerId_) > 0) {
                pendingSpaces_ = controller_.reachableDestinationsFor(selectedAttackerId_);
                const Fighter* fighter = controller_.findFighterById(selectedAttackerId_);
                bool hasOther = false;
                if (fighter) {
                    for (int s : pendingSpaces_) {
                        if (s != fighter->spaceId()) { hasOther = true; break; }
                    }
                }
                if (hasOther) {
                    state_ = ScreenState::ManeuverDestination;
                    resetSelection();
                    break;
                }
            }

            controller_.finishCurrentFighter(selectedAttackerId_);
            pendingFighterIds_ = controller_.movableCurrentFighterIds();
            if (!pendingFighterIds_.empty()) {
                state_ = ScreenState::ManeuverFighter;
            } else {
                controller_.finishManeuver();
                openGameScreen();
            }
            resetSelection();
            break;
        }

        default:
            break;
    }
}

void TuiApp::handleDigit(char digit) {
    if (ageInput_.size() < 3) {
        ageInput_.push_back(digit);
        errorMessage_.clear();
    }
}

void TuiApp::handleBackspace() {
    if (!ageInput_.empty()) {
        ageInput_.pop_back();
    }
}

void TuiApp::moveSelection(int delta) {
    auto entries = currentMenuEntries();
    if (entries.empty()) {
        selected_ = 0;
        return;
    }
    int size = static_cast<int>(entries.size());
    selected_ = (selected_ + delta + size) % size;
}

void TuiApp::resetSelection() {
    selected_ = 0;
}

void TuiApp::showError(const std::string& message) {
    errorMessage_ = message;
}
} // namespace unmatched