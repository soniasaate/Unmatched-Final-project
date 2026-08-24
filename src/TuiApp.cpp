#include "unmatched/TuiApp.hpp"
#include "unmatched/GameExceptions.hpp"
#include "unmatched/Dracula.hpp"
#include "unmatched/Sherlock.hpp"
#include "unmatched/InvisibleMan.hpp"
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <optional>
#include <sstream>
#include <fstream>
#include <cstdio>

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

std::string cardTypeToString(CardType type) {
    switch (type) {
        case CardType::Attack: return "Attack";
        case CardType::Defense: return "Defense";
        case CardType::Versatile: return "Versatile";
        case CardType::Scheme: return "Scheme";
        default: return "Unknown";
    }
}

std::string characterToString(Character owner) {
    switch (owner) {
        case Character::Any: return "Any";
        case Character::Dracula: return "Dracula";
        case Character::Sister: return "Sister";
        case Character::Sherlock: return "Sherlock";
        case Character::Watson: return "Watson";
        case Character::InvisibleMan: return "Invisible Man";
        default: return "Unknown";
    }
}

std::string fighterTypeName(const Fighter& fighter) {
    if (dynamic_cast<const Dracula*>(&fighter)) {
        return "Dracula";
    } else if (dynamic_cast<const Sherlock*>(&fighter)) {
        return "Sherlock Holmes";
    } else if (dynamic_cast<const InvisibleMan*>(&fighter)) {
        return "Invisible Man";
    }
    return "Unknown";
}

ftxui::Color fighterColor(const Fighter& fighter) {
    if (dynamic_cast<const Dracula*>(&fighter)) {
        return ftxui::Color::Red;
    } else if (dynamic_cast<const Sherlock*>(&fighter)) {
        return ftxui::Color::Blue;
    } else if (dynamic_cast<const InvisibleMan*>(&fighter)) {
        return ftxui::Color::Cyan;
    }
    return ftxui::Color::White;
}

}  // namespace

using namespace ftxui;

TuiApp::TuiApp(ScreenInteractive& screen)
    : screen_(screen),
      state_(ScreenState::MainMenu),
      selected_(0),
      ageStep_(0),
      playerOneAge_(0),
      playerTwoAge_(0),
      selectedHero_(nullptr),
      selectedStartSlot_(1),
      asciiOnlyMode_(true),
      waitingForDestination_(false),
      selectedAttackCardIndex_(-1),
      selectedSchemeCardIndex_(-1) {}

Element TuiApp::Render() {
    switch (state_) {
        case ScreenState::MainMenu:
            return renderMainMenu();
        case ScreenState::Help:
            return renderHelp();
        case ScreenState::LoadGame:
            return renderLoadGameView();
        case ScreenState::SetupAge:
            return renderSetupAge();
        case ScreenState::FighterSelect:
            return renderFighterSelect();
        case ScreenState::StartSelect:
            return renderStartSelect();
        case ScreenState::GameOver:
            return renderGameOver();
        case ScreenState::StudyMethodsView:
            return renderStudyMethodsView();
        case ScreenState::ConfoundChoice:
            return renderConfoundChoiceView();
        case ScreenState::CodedNotesSelect:
            return renderCodedNotesView();
        case ScreenState::LurkingChoice:
            return renderLurkingChoiceView();
        case ScreenState::StepLightlyTarget:
            return renderStepLightlyTargetView();
        case ScreenState::ConfirmSuspicionNoMatchView:
            return renderConfirmSuspicionNoMatchView();
        default:
            return renderGame();
    }
}

Element TuiApp::renderLoadGameView() const {
    Elements body;
    body.push_back(text("Load Game") | bold | color(Color::Yellow) | center);
    body.push_back(separator());
    auto entries = currentMenuEntries();
    if (entries.empty()) {
        body.push_back(text("No save files available.") | color(Color::Red) | center);
    } else {
        for (size_t i = 0; i < entries.size(); ++i) {
            body.push_back(menuLine(entries[i], static_cast<int>(i) == selected_));
        }
    }
    return vbox(std::move(body)) | border | size(WIDTH, GREATER_THAN, 72) | center;
}

Element TuiApp::renderStepLightlyTargetView() const {
    Elements body;
    body.push_back(text("STEP LIGHTLY") | bold | color(Color::Yellow) | center);
    body.push_back(separator());
    body.push_back(text("Choose a target fighter:") | center);
    for (size_t i = 0; i < pendingFighterIds_.size(); ++i) {
        body.push_back(menuLine(fighterMenuLabel(pendingFighterIds_[i]), static_cast<int>(i) == selected_));
    }
    if (!errorMessage_.empty()) {
        body.push_back(text(errorMessage_) | color(Color::Red));
    }
    return vbox(std::move(body)) | border | center | size(WIDTH, GREATER_THAN, 60);
}

Element TuiApp::renderConfoundChoiceView() const {
    Elements body;
    body.push_back(text("CONFOUND") | bold | color(Color::Yellow) | center);
    body.push_back(separator());
    body.push_back(text("Opponent may discard 1 card from hand. Do you want to discard a card?") | center);
    body.push_back(separator());
    body.push_back(menuLine("[1] Yes, discard a card", selected_ == 0));
    body.push_back(menuLine("[2] No, skip discarding", selected_ == 1));
    body.push_back(separator());
    body.push_back(text("Use Arrow keys and Enter to choose.") | dim | center);
    if (!errorMessage_.empty()) {
        body.push_back(text(errorMessage_) | color(Color::Red));
    }
    return vbox(std::move(body)) | border | center | size(WIDTH, GREATER_THAN, 60);
}

Element TuiApp::renderStudyMethodsView() const {
    Elements body;
    body.push_back(text("STUDY METHODS") | bold | color(Color::Yellow) | center);
    body.push_back(separator());
    body.push_back(text("You won the combat. You may look at the opponent's hand:") | center);
    body.push_back(text(controller_.getStudyMethodsHandInfo()) | color(Color::Cyan) | center);
    body.push_back(separator());
    body.push_back(text("[Press Enter to continue]") | dim | center);
    return vbox(std::move(body)) | border | center | size(WIDTH, GREATER_THAN, 60);
}

Element TuiApp::renderConfirmSuspicionNoMatchView() const {
    Elements body;
    body.push_back(text("CONFIRM SUSPICION") | bold | color(Color::Yellow) | center);
    body.push_back(separator());
    body.push_back(text("Opponent has no matching card. Hand is revealed:") | center);
    body.push_back(text(controller_.getConfirmSuspicionHandInfo()) | color(Color::Cyan) | center);
    body.push_back(separator());
    body.push_back(text("[Press Enter to continue]") | dim | center);
    return vbox(std::move(body)) | border | center | size(WIDTH, GREATER_THAN, 60);
}

Element TuiApp::renderCodedNotesView() const {
    Elements body;
    body.push_back(text("CODED NOTES") | bold | color(Color::Yellow) | center);
    body.push_back(separator());
    body.push_back(text("Draw 3 cards, then select 2 cards to put on top of your deck.") | center);
    body.push_back(text("Select 2 cards from your hand (use Arrow keys and Enter):") | center);
    body.push_back(separator());

    const Player* player = nullptr;
    if (controller_.hasPendingCodedNotes()) {
        const auto& pending = controller_.pendingCodedNotes();
        player = &controller_.players()[pending.playerIndex];
    } else {
        player = &controller_.currentPlayer();
    }

    int displayIndex = 0;
    if (!player || player->hand().empty()) {
        body.push_back(text("(no cards in hand)") | color(Color::Red) | center);
    } else {
        for (size_t i = 0; i < player->hand().size(); ++i) {
            if (codedNotesCardIndex_ != -1 && static_cast<int>(i) == codedNotesCardIndex_) continue;
            bool isSelected = (std::find(selectedCodedNotesIndices_.begin(), selectedCodedNotesIndices_.end(), static_cast<int>(i)) != selectedCodedNotesIndices_.end());
            std::string prefix = isSelected ? "[X] " : "[ ] ";
            body.push_back(menuLine(prefix + player->hand()[i].getTitle(), displayIndex == selected_));
            ++displayIndex;
        }
    }

    body.push_back(separator());
    std::string confirmLabel = "Confirm selection (" + std::to_string(selectedCodedNotesIndices_.size()) + "/2 selected)";
    body.push_back(menuLine(confirmLabel, selected_ == displayIndex));
    body.push_back(menuLine("Cancel", selected_ == displayIndex + 1));

    return vbox(std::move(body)) | border | center | size(WIDTH, GREATER_THAN, 60);
}

Element TuiApp::renderLurkingChoiceView() const {
    Elements body;
    body.push_back(text("LURKING") | bold | color(Color::Yellow) | center);
    body.push_back(separator());
    body.push_back(text("Choose an effect:") | center);
    body.push_back(menuLine("[1] Move Invisible Man to a space with Fog", selected_ == 0));
    body.push_back(menuLine("[2] Move a Fog Token up to 3 spaces", selected_ == 1));
    if (!errorMessage_.empty()) {
        body.push_back(text(errorMessage_) | color(Color::Red));
    }
    return vbox(std::move(body)) | border | center | size(WIDTH, GREATER_THAN, 60);
}

bool TuiApp::OnEvent(Event event) {
    try {
        if (event == Event::Character("s") || event == Event::Character("S")) {
            if (controller_.started() && !controller_.gameOver()) {
                controller_.saveGame();
                saveTuiState();
                showError("Game saved successfully!");
                return true;
            }
            return true;
        }
        if (event == Event::Character("z") || event == Event::Character("Z")) {
            if (controller_.canUndo()) {
                try {
                    controller_.undoLastAction();
                    selectedSchemeCardIndex_ = -1;
                    openGameScreen();
                    showError("Undo successful.");
                } catch (const std::exception& e) {
                    showError(std::string("Undo failed: ") + e.what());
                }
            } else {
                showError("Nothing to undo.");
            }
            return true;
        }
        if (event == Event::Character("l") || event == Event::Character("L")) {
            if (!controller_.started() || controller_.gameOver()) {
                auto slots = controller_.getSaveSlots();
                if (!slots.empty()) {
                    pendingSlots_ = slots;
                    state_ = ScreenState::LoadGame;
                    resetSelection();
                    return true;
                }
                showError("No save files found.");
                return true;
            }
            return true;
        }
        return handleEvent(event);
    } catch (const GameException& exception) {
        showError(exception.what());
        return true;
    } catch (const std::exception& exception) {
        showError(std::string("Unexpected error: ") + exception.what());
        return true;
    }
}

Element TuiApp::renderMainMenu() const {
    Elements body;
    body.push_back(text("UNMATCHED TUI - Dracula vs Sherlock Holmes") | bold | center);
    body.push_back(separator());
    body.push_back(text("Baskerville Manor duel simulator") | center);
    body.push_back(text("Use Arrow keys and Enter. Main menu requires no typed commands.") | dim | center);
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
    rules.push_back(text("1. The younger player chooses a fighter; the other fighter is assigned automatically."));
    rules.push_back(text("2. The younger player chooses start space 1 or 2; the opponent starts on the remaining space."));
    rules.push_back(text("3. Each player shuffles a 30-card deck and draws 5 cards."));
    rules.push_back(text("4. On your turn you must take exactly 2 actions: Maneuver, Attack, or Scheme."));
    rules.push_back(text("5. Maneuver draws 1 card, then may boost and move fighters through connected spaces."));
    rules.push_back(text("6. Melee attacks adjacent enemies. Ranged attacks adjacent enemies or enemies sharing a zone."));
    rules.push_back(text("7. Secret passages count as movement neighbors only; they are not combat adjacency."));
    rules.push_back(text("8. At end of turn discard down to 7 cards. Reduce the enemy hero to 0 HP to win."));
    rules.push_back(separator());
    rules.push_back(renderMenuLines(currentMenuEntries(), controller_.started() ? "Return" : "Return"));
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
    body.push_back(text("The younger player chooses first. The remaining fighter is assigned automatically."));
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
    body.push_back(text("Choose the younger player's start space. The other hero takes the remaining start."));
    body.push_back(renderMenuLines(currentMenuEntries(), "Start Space"));
    if (!errorMessage_.empty()) {
        body.push_back(separator());
        body.push_back(text(errorMessage_) | color(Color::Red));
    }
    return vbox(std::move(body)) | border | size(WIDTH, GREATER_THAN, 76) | center;
}

Element TuiApp::renderGame() const {
    if (!controller_.started()) return renderMainMenu();
    const auto& players = controller_.players();
    Elements page;
    std::string title = "UNMATCHED TUI - Dracula vs Sherlock Holmes";
    page.push_back(text(title) | bold | center);
    const Fighter& hero = controller_.currentPlayer().heroFighter();
    page.push_back(text("Turn " + std::to_string(controller_.turnNumber()) + " - " +
                    controller_.currentPlayer().name() + " / " +
                    fighterTypeName(hero) +
                    " - Actions: " + std::to_string(controller_.actionsRemaining()) +
                    " - Moves: " + std::to_string(controller_.pendingMovementPoints()) + "/" +
                    std::to_string(controller_.maxMovementPoints())) |
               color(fighterColor(hero)) | center);
    page.push_back(separator());
    page.push_back(hbox({
        renderPlayerPanel(players.at(0), controller_.currentPlayerIndex() == 0) | size(WIDTH, EQUAL, 34 ),
        renderMapPanel() | flex,
        renderPlayerPanel(players.at(1), controller_.currentPlayerIndex() == 1) | size(WIDTH, EQUAL, 34 ),
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
    const Fighter& hero = player.heroFighter();
    Elements rows;
    std::string heroName = fighterTypeName(hero);
    rows.push_back(text(player.name() + " - " + heroName) | bold | color(fighterColor(hero)));
    rows.push_back(text(active ? "ACTIVE TURN" : "Waiting") | color(active ? Color::Green : Color::GrayDark));
    for (const auto& fighter : player.fighters()) {
        rows.push_back(fighterLine(*fighter));
    }
    if (auto* invisible = dynamic_cast<const InvisibleMan*>(&hero)) {
        std::string fogInfo = "Fog Tokens: ";
        bool first = true;
        for (int space : invisible->getFogTokens()) {
            if (!first) fogInfo += ", ";
            if (space == -1) fogInfo += "(not placed)";
            else fogInfo += "Space " + std::to_string(space);
            first = false;
        }
        rows.push_back(text(fogInfo) | dim);
    }
    rows.push_back(text("Deck: " + std::to_string(player.deck().size()) +
                        "   Hand: " + std::to_string(player.hand().size()) +
                        "   Discard: " + std::to_string(player.discardPile().size())));
    return vbox(std::move(rows));
}

Element TuiApp::renderHandPanel(const Player& player, bool active) const {
    const Fighter& hero = player.heroFighter();
    Elements rows;
    rows.push_back(text(player.name() + " Hand") | bold | color(active ? fighterColor(hero) : Color::White));
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
        case ScreenState::ManeuverBoost:
            title = "Maneuver - Boost";
            break;
        case ScreenState::ManeuverFighter:
            title = "Maneuver - Move Fighter";
            break;
        case ScreenState::ManeuverDestination:
            title = "Maneuver - Destination";
            break;
        case ScreenState::AttackAttacker:
            title = "Attack - Attacker";
            break;
        case ScreenState::AttackTarget:
            title = "Attack - Target";
            break;
        case ScreenState::AttackCard:
            title = "Attack - Card";
            break;
        case ScreenState::AttackBeastBoost:
            title = "Attack - Beast Form";
            break;
        case ScreenState::DefenseCard:
            title = "Defense - Card";
            break;
        case ScreenState::SchemeCard:
            title = "Scheme - Card";
            break;
        case ScreenState::SchemeChoice:
            title = "Scheme - Choice";
            break;
        case ScreenState::DiscardCard:
        case ScreenState::DiscardToLimit:
            title = "Discard Cards";
            break;
        case ScreenState::FogChoice: {
            if (controller_.hasPendingFogChoice()) {
                const auto& pending = controller_.pendingFogChoice();
                const auto& players = controller_.players();
                if (pending.chooserPlayerIndex >= 0 &&
                    pending.chooserPlayerIndex < static_cast<int>(players.size())) {
                    title = players[pending.chooserPlayerIndex].name() + " chooses a Fog Token to move";
                } else {
                    title = "Choose a Fog Token to move";
                }
            } else {
                title = "Choose a Fog Token to move";
            }
            break;
        }
        case ScreenState::FogDestination:
            title = "Choose destination for Fog Token";
            break;
        case ScreenState::DraculaAbilityTarget:
            title = "Dracula Ability";
            break;
        default:
            title = "Action Menu";
            break;
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
    lines.push_back(text("Legend: [D] Dracula  [Si] Sister  [H] Holmes  [W] Watson  [I] Invisible Man  [F] Fog Token  (~id) Secret passage  <S1>/<S2> Start") | dim);
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
                auto slots = controller_.getSaveSlots();
                if (slots.empty()) {
                    showError("No save files found.");
                    return;
                }
                pendingSlots_ = slots;
                state_ = ScreenState::LoadGame;
                resetSelection();
            } else if (selected_ == 2) {
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

        case ScreenState::LoadGame: {
            if (selected_ < static_cast<int>(pendingSlots_.size())) {
                int slot = pendingSlots_[selected_].first;
                try {
                    controller_.loadGame(slot);
                    loadTuiState();
                } catch (const std::exception& e) {
                    showError(std::string("Failed to load: ") + e.what());
                    state_ = ScreenState::MainMenu;
                }
            } else {
                state_ = ScreenState::MainMenu;
            }
            resetSelection();
            break;
        }

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
            if (selected_ == 0) {
                selectedHero_ = std::make_unique<Dracula>();
                remainingHeroes_.push_back(std::make_unique<Sherlock>());
                remainingHeroes_.push_back(std::make_unique<InvisibleMan>());
            } else if (selected_ == 1) {
                selectedHero_ = std::make_unique<Sherlock>();
                remainingHeroes_.push_back(std::make_unique<Dracula>());
                remainingHeroes_.push_back(std::make_unique<InvisibleMan>());
            } else {
                selectedHero_ = std::make_unique<InvisibleMan>();
                remainingHeroes_.push_back(std::make_unique<Dracula>());
                remainingHeroes_.push_back(std::make_unique<Sherlock>());
            }
            state_ = ScreenState::FighterSelectSecond;
            resetSelection();
            break;

        case ScreenState::FighterSelectSecond:
            if (selected_ == 0) {
                secondHero_ = std::move(remainingHeroes_[0]);
            } else {
                secondHero_ = std::move(remainingHeroes_[1]);
            }
            state_ = ScreenState::StartSelect;
            resetSelection();
            break;

        case ScreenState::StepLightlyTarget: {
            if (selected_ < 0 || selected_ >= static_cast<int>(pendingFighterIds_.size())) {
                showError("Invalid target selection.");
                return;
            }
            std::string targetId = pendingFighterIds_[selected_];
            controller_.handleStepLightly(stepLightlyCardIndex_, targetId);
            stepLightlyCardIndex_ = -1;
            pendingFighterIds_.clear();
            openGameScreen();
            break;
        }

        case ScreenState::FogChoice: {
            if (selected_ < 0 || selected_ >= static_cast<int>(pendingFogIndices_.size())) {
                showError("Invalid fog token selection.");
                return;
            }
            selectedFogIndex_ = pendingFogIndices_[selected_];
            openGameScreen();
            break;
        }

        case ScreenState::FogDestination: {
            if (selected_ < 0 || selected_ >= static_cast<int>(pendingFogDestinations_.size())) {
                showError("Invalid destination.");
                return;
            }
            int destination = pendingFogDestinations_[selected_];
            controller_.moveFogToken(selectedFogIndex_, destination);
            selectedFogIndex_ = -1;
            pendingFogDestinations_.clear();
            openGameScreen();
            break;
        }

        case ScreenState::StartSelect:
            selectedStartSlot_ = selected_ == 0 ? 1 : 2;
            startGameFromSetup();
            break;

        case ScreenState::StudyMethodsView:
            controller_.clearStudyMethodsHandInfo();
            openGameScreen();
            break;

        case ScreenState::ConfoundChoice:
            if (selected_ == 0) {
                controller_.resolveConfoundChoice(true);
                pendingCardIndexes_.clear();
                for (int i = 0; i < static_cast<int>(controller_.opponentPlayer().hand().size()); ++i) {
                    pendingCardIndexes_.push_back(i);
                }
                if (pendingCardIndexes_.empty()) {
                    controller_.resolveConfoundChoice(false);
                    state_ = ScreenState::ConfoundFogSelect;
                    resetSelection();
                } else {
                    state_ = ScreenState::ConfoundDiscardSelect;
                    resetSelection();
                }
            } else {
                controller_.resolveConfoundChoice(false);
                state_ = ScreenState::ConfoundFogSelect;
                resetSelection();
            }
            break;

        case ScreenState::ConfoundDiscardSelect:
            if (selected_ < 0 || selected_ >= static_cast<int>(pendingCardIndexes_.size())) {
                showError("Invalid card selection.");
                return;
            }
            controller_.resolveConfoundDiscard(pendingCardIndexes_[selected_]);
            openGameScreen();
            break;

        case ScreenState::ConfoundFogSelect:
            if (selected_ < 0 || selected_ >= static_cast<int>(pendingFogIndices_.size())) {
                showError("Invalid fog token selection.");
                return;
            }
            selectedFogIndex_ = pendingFogIndices_[selected_];
            pendingSpaces_.clear();
            for (const auto& space : controller_.board().spaces()) {
                pendingSpaces_.push_back(space.id());
            }
            state_ = ScreenState::ConfoundDestinationSelect;
            resetSelection();
            break;

        case ScreenState::ConfoundDestinationSelect:
            if (selected_ < 0 || selected_ >= static_cast<int>(pendingSpaces_.size())) {
                showError("Invalid destination selection.");
                return;
            }
            controller_.resolveConfoundFogMove(selectedFogIndex_, pendingSpaces_[selected_]);
            openGameScreen();
            break;

        case ScreenState::LurkingChoice: {
            controller_.handleLurking(lurkingCardIndex_, selected_);
            lurkingCardIndex_ = -1;
            openGameScreen();
            break;
        }

        case ScreenState::CodedNotesSelect: {
            const Player* player = nullptr;
            if (controller_.hasPendingCodedNotes()) {
                const auto& pending = controller_.pendingCodedNotes();
                player = &controller_.players()[pending.playerIndex];
            } else {
                player = &controller_.currentPlayer();
            }
            if (!player) {
                showError("Invalid player.");
                return;
            }
            const auto& hand = player->hand();
            size_t handSize = hand.size();
            bool isFromScheme = (codedNotesCardIndex_ != -1);
            size_t entriesSize = isFromScheme ? (handSize - 1) : handSize;

            if (selected_ == static_cast<int>(entriesSize)) {
                if (selectedCodedNotesIndices_.size() != 2) {
                    showError("Please select exactly 2 cards.");
                    return;
                }
                std::vector<int> indices = selectedCodedNotesIndices_;
                if (controller_.hasPendingCodedNotes()) {
                    controller_.finishCodedNotes(indices);
                } else {
                    controller_.handleCodedNotes(codedNotesCardIndex_, indices);
                }
                selectedCodedNotesIndices_.clear();
                codedNotesCardIndex_ = -1;
                openGameScreen();
            } else if (selected_ == static_cast<int>(entriesSize + 1)) {
                if (controller_.hasPendingCodedNotes()) {
                    controller_.cancelCodedNotes();
                } else {
                    Card played = controller_.currentPlayer().removeCardFromHand(codedNotesCardIndex_);
                    controller_.currentPlayer().addToDiscard(std::move(played));
                    controller_.decrementActions();
                }
                selectedCodedNotesIndices_.clear();
                codedNotesCardIndex_ = -1;
                openGameScreen();
            } else {
                int realIdx = -1;
                int current = 0;
                for (size_t i = 0; i < handSize; ++i) {
                    if (isFromScheme && static_cast<int>(i) == codedNotesCardIndex_) continue;
                    if (current == selected_) {
                        realIdx = static_cast<int>(i);
                        break;
                    }
                    ++current;
                }
                if (realIdx == -1) {
                    showError("Invalid card selection.");
                    return;
                }
                auto it = std::find(selectedCodedNotesIndices_.begin(), selectedCodedNotesIndices_.end(), realIdx);
                if (it != selectedCodedNotesIndices_.end()) {
                    selectedCodedNotesIndices_.erase(it);
                } else {
                    if (selectedCodedNotesIndices_.size() < 2) {
                        selectedCodedNotesIndices_.push_back(realIdx);
                    } else {
                        showError("You can only select 2 cards.");
                    }
                }
            }
            break;
        }

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
                        if (!fighter->defeated() && fighter->id() != dracula.id() &&
                            controller_.board().areAdjacentForCombat(dracula.spaceId(), fighter->spaceId())) {
                            pendingFighterIds_.push_back(fighter->id());
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
            } else if (choice == "Drawing Card") {
                showError("In legal play, drawing is performed through Maneuver or card effects.");
            } else if (choice == "Save Game") {
                controller_.saveGame();
                showError("Game saved successfully!");
                openGameScreen();
            } else if (choice == "Help") {
                state_ = ScreenState::Help;
                resetSelection();
            } else if (choice == "Back to main menu") {
                state_ = ScreenState::MainMenu;
                resetSelection();
            } else if (choice == "Undo last action (Z)") {
                if (controller_.canUndo()) {
                    try {
                        controller_.undoLastAction();
                        openGameScreen();
                        showError("Undo successful.");
                    } catch (const std::exception& e) {
                        showError(std::string("Undo failed: ") + e.what());
                    }
                } else {
                    showError("Nothing to undo.");
                }
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

        case ScreenState::AttackAttacker: {
            if (selected_ < 0 || selected_ >= static_cast<int>(pendingFighterIds_.size())) {
                showError("Invalid attacker selection.");
                return;
            }
            selectedAttackerId_ = pendingFighterIds_.at(static_cast<std::size_t>(selected_));
            pendingFighterIds_ = controller_.legalTargetsFor(selectedAttackerId_);
            if (pendingFighterIds_.empty()) {
                showError("No legal targets for attack.");
                return;
            }
            state_ = ScreenState::AttackTarget;
            resetSelection();
            break;
        }

        case ScreenState::AttackTarget: {
            if (selected_ < 0 || selected_ >= static_cast<int>(pendingFighterIds_.size())) {
                showError("Invalid target selection.");
                return;
            }
            selectedTargetId_ = pendingFighterIds_.at(static_cast<std::size_t>(selected_));
            pendingCardIndexes_ = controller_.legalAttackCardsFor(selectedAttackerId_);
            if (pendingCardIndexes_.empty()) {
                showError("No legal attack cards.");
                return;
            }
            state_ = ScreenState::AttackCard;
            resetSelection();
            break;
        }

        case ScreenState::AttackCard: {
            if (selected_ < 0 || selected_ >= static_cast<int>(pendingCardIndexes_.size())) {
                showError("Invalid attack card selection.");
                return;
            }
            selectedAttackCardIndex_ = pendingCardIndexes_.at(static_cast<std::size_t>(selected_));
            selectedBeastFormBoostIndexes_.clear();
            if (controller_.currentPlayer().hand().at(static_cast<std::size_t>(selectedAttackCardIndex_)).getTitle() == "BEASTFORM") {
                pendingCardIndexes_.clear();
                for (int i = 0; i < static_cast<int>(controller_.currentPlayer().hand().size()); ++i) {
                    if (i != selectedAttackCardIndex_) pendingCardIndexes_.push_back(i);
                }
                state_ = ScreenState::AttackBeastBoost;
                resetSelection();
            } else {
                pendingCardIndexes_ = controller_.legalDefenseCardsFor(selectedTargetId_);
                state_ = ScreenState::DefenseCard;
                resetSelection();
            }
            break;
        }

        case ScreenState::AttackBeastBoost: {
            if (selected_ < 0 || selected_ >= static_cast<int>(pendingCardIndexes_.size()) + 1) {
                showError("Invalid Beast Form selection.");
                return;
            }
            if (selected_ == 0) {
                pendingCardIndexes_ = controller_.legalDefenseCardsFor(selectedTargetId_);
                state_ = ScreenState::DefenseCard;
                resetSelection();
            } else {
                int chosenIndex = pendingCardIndexes_.at(static_cast<std::size_t>(selected_ - 1));
                selectedBeastFormBoostIndexes_.push_back(chosenIndex);
                pendingCardIndexes_.erase(std::remove(pendingCardIndexes_.begin(), pendingCardIndexes_.end(), chosenIndex),
                                          pendingCardIndexes_.end());
                resetSelection();
            }
            break;
        }

        case ScreenState::DefenseElementaryPrediction: {
            if (selected_ < 0 || selected_ >= static_cast<int>(pendingValues_.size())) {
                showError("Invalid prediction value.");
                return;
            }
            int predicted = pendingValues_.at(static_cast<std::size_t>(selected_));
            controller_.resolveAttack(selectedAttackerId_, selectedTargetId_, selectedAttackCardIndex_,
                                      selectedDefenseCardIndex_, selectedBeastFormBoostIndexes_, predicted);
            selectedBeastFormBoostIndexes_.clear();
            openGameScreen();
            break;
        }

        case ScreenState::DefenseCard: {
            if (selected_ == 0) {
                controller_.resolveAttack(selectedAttackerId_, selectedTargetId_, selectedAttackCardIndex_,
                                          -1, selectedBeastFormBoostIndexes_, -1);
                selectedBeastFormBoostIndexes_.clear();
                openGameScreen();
                break;
            }
            int index = selected_ - 1;
            if (index < 0 || index >= static_cast<int>(pendingCardIndexes_.size())) {
                showError("Invalid defense card selection.");
                return;
            }
            int defenseIndex = pendingCardIndexes_.at(static_cast<std::size_t>(index));
            const Card& defenseCard = controller_.opponentPlayer().hand().at(defenseIndex);
            if (defenseCard.getTitle() == "ELEMENTARY") {
                selectedDefenseCardIndex_ = defenseIndex;
                pendingValues_.clear();
                for (int v = 0; v <= 6; ++v) pendingValues_.push_back(v);
                state_ = ScreenState::DefenseElementaryPrediction;
                resetSelection();
                break;
            }
            controller_.resolveAttack(selectedAttackerId_, selectedTargetId_, selectedAttackCardIndex_,
                                      defenseIndex, selectedBeastFormBoostIndexes_, -1);
            selectedBeastFormBoostIndexes_.clear();
            openGameScreen();
            break;
        }

        case ScreenState::SchemeCard:
            chooseSchemeCard(selected_);
            break;

        case ScreenState::SchemeChoice:
            completeSchemeChoice();
            break;

        case ScreenState::DiscardCard: {
            if (pendingCardIndexes_.empty()) {
                state_ = ScreenState::Game;
                resetSelection();
                break;
            }
            if (selected_ < 0 || selected_ >= static_cast<int>(pendingCardIndexes_.size())) {
                showError("Invalid card selection.");
                return;
            }
            int cardIndex = controller_.getConfoundSchemeCardIndex();
            int opponentCardIndex = pendingCardIndexes_.at(selected_);
            controller_.handleConfoundDiscard(cardIndex, opponentCardIndex);
            controller_.setConfoundSchemeCardIndex(-1);
            openGameScreen();
            break;
        }

        case ScreenState::DiscardToLimit: {
            if (selected_ < 0 || selected_ >= static_cast<int>(pendingCardIndexes_.size())) {
                showError("Invalid card selection.");
                return;
            }
            controller_.discardCurrentPlayerCard(pendingCardIndexes_.at(static_cast<std::size_t>(selected_)));
            if (controller_.currentPlayerMustDiscardToLimit()) {
                pendingCardIndexes_.clear();
                for (int i = 0; i < static_cast<int>(controller_.currentPlayer().hand().size()); ++i) {
                    pendingCardIndexes_.push_back(i);
                }
                resetSelection();
            } else {
                openGameScreen();
            }
            break;
        }

        case ScreenState::DraculaAbilityTarget: {
            if (selected_ < 0 || selected_ >= static_cast<int>(pendingFighterIds_.size())) {
                showError("Invalid target selection.");
                return;
            }
            controller_.useDraculaStartAbility(pendingFighterIds_.at(static_cast<std::size_t>(selected_)));
            openGameScreen();
            break;
        }

        case ScreenState::OptionalMovementDestination: {
            if (selected_ == 0) {
                controller_.resolvePendingOptionalMovement(-1);
                openGameScreen();
                break;
            }
            int index = selected_ - 1;
            if (index < 0 || index >= static_cast<int>(pendingSpaces_.size())) {
                showError("Invalid destination.");
                return;
            }
            controller_.resolvePendingOptionalMovement(pendingSpaces_.at(static_cast<std::size_t>(index)));
            openGameScreen();
            break;
        }

        case ScreenState::ConfirmSuspicionChoice: {
            if (selected_ < 0 || selected_ >= static_cast<int>(pendingCardIndexes_.size())) {
                showError("Invalid card choice.");
                return;
            }
            int chosenIndex = pendingCardIndexes_.at(static_cast<std::size_t>(selected_));
            controller_.applyConfirmSuspicion(chosenIndex);
            Card played = controller_.currentPlayer().removeCardFromHand(selectedSchemeCardIndex_);
            controller_.currentPlayer().addToDiscard(std::move(played));
            controller_.decrementActions();
            controller_.endTurnIfNeeded();
            openGameScreen();
            break;
        }

        case ScreenState::ConfirmSuspicionNoMatchView:
            controller_.clearConfirmSuspicionHandInfo();
            openGameScreen();
            break;

        case ScreenState::PlaceVanishedInvisibleMan: {
            if (selected_ < 0 || selected_ >= static_cast<int>(pendingSpaces_.size())) {
                showError("Invalid space selection.");
                return;
            }
            int chosenSpace = pendingSpaces_.at(static_cast<std::size_t>(selected_));
            controller_.placeVanishedInvisibleMan(chosenSpace);
            openGameScreen();
            break;
        }

        case ScreenState::GameOver:
            if (selected_ == 0) state_ = ScreenState::MainMenu;
            else screen_.ExitLoopClosure()();
            resetSelection();
            break;

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
    if (!ageInput_.empty()) ageInput_.pop_back();
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

std::vector<std::string> TuiApp::currentMenuEntries() const {
    switch (state_) {
        case ScreenState::MainMenu:
            return {"Play", "Load Game", "Help", "Exit"};
        case ScreenState::Help:
            return {controller_.started() ? "Back to game" : "Back to main menu"};
        case ScreenState::FighterSelect:
            return {"Dracula", "Sherlock Holmes", "Invisible Man"};
        case ScreenState::FighterSelectSecond: {
            std::vector<std::string> entries;
            for (const auto& hero : remainingHeroes_) {
                if (dynamic_cast<Dracula*>(hero.get())) entries.push_back("Dracula");
                else if (dynamic_cast<Sherlock*>(hero.get())) entries.push_back("Sherlock Holmes");
                else if (dynamic_cast<InvisibleMan*>(hero.get())) entries.push_back("Invisible Man");
            }
            return entries;
        }
        case ScreenState::StartSelect:
            return {"Start Space 1", "Start Space 2"};
        case ScreenState::Game: {
            std::vector<std::string> entries;
            if (controller_.draculaAbilityAvailable()) entries.push_back("Use Dracula start ability");
            if (controller_.canUndo()) entries.push_back("Undo last action (Z)");
            entries.push_back("Maneuver");
            entries.push_back("Attack");
            entries.push_back("Scheme");
            entries.push_back("Discarding Cards");
            entries.push_back("Drawing Card");
            entries.push_back("Save Game");
            entries.push_back("Help");
            entries.push_back("Back to main menu");
            return entries;
        }
        case ScreenState::LoadGame: {
            std::vector<std::string> entries;
            for (const auto& slot : pendingSlots_) entries.push_back(slot.second);
            entries.push_back("Back");
            return entries;
        }
        case ScreenState::StepLightlyTarget: {
            std::vector<std::string> entries;
            for (const auto& id : pendingFighterIds_) entries.push_back(fighterMenuLabel(id));
            return entries;
        }
        case ScreenState::ManeuverBoost: {
            std::vector<std::string> entries{"No boost"};
            for (int index : pendingCardIndexes_) entries.push_back(cardMenuLabel(controller_.currentPlayer(), index));
            return entries;
        }
        case ScreenState::ManeuverFighter: {
            std::vector<std::string> entries;
            int totalRemaining = 0;
            for (const auto& id : pendingFighterIds_) {
                int rem = controller_.remainingMovementForFighter(id);
                if (rem > 0) totalRemaining += rem;
            }
            entries.push_back("Finish movement (Total remaining: " + std::to_string(totalRemaining) + " moves)");
            for (const auto& id : pendingFighterIds_) entries.push_back(fighterMenuLabel(id));
            return entries;
        }
        case ScreenState::ManeuverDestination: {
            std::vector<std::string> entries;
            int remaining = controller_.remainingMovementForFighter(selectedAttackerId_);
            if (remaining < 0) remaining = 0;
            entries.push_back("Finish movement (Remaining: " + std::to_string(remaining) + " moves)");
            const Fighter* fighter = controller_.findFighterById(selectedAttackerId_);
            if (fighter) {
                for (int space : pendingSpaces_) {
                    std::string label = spaceMenuLabel(space);
                    if (space == fighter->spaceId()) {
                        label += " (current)";
                    } else {
                        int cost = controller_.getMovementCost(selectedAttackerId_, space);
                        if (cost > 0 && cost <= remaining) {
                            label += " (cost: " + std::to_string(cost) + " move" + (cost > 1 ? "s" : "") + ")";
                        } else {
                            continue;
                        }
                    }
                    entries.push_back(label);
                }
            }
            return entries;
        }
        case ScreenState::OptionalMovementDestination: {
            std::vector<std::string> entries{"Skip movement"};
            const PendingMovementChoice& pending = controller_.pendingOptionalMovement();
            entries.front() += " for " + fighterMenuLabel(pending.fighterId);
            for (int space : pendingSpaces_) entries.push_back(spaceMenuLabel(space));
            return entries;
        }
        case ScreenState::AttackAttacker:
        case ScreenState::AttackTarget:
        case ScreenState::DraculaAbilityTarget: {
            std::vector<std::string> entries;
            for (const auto& id : pendingFighterIds_) entries.push_back(fighterMenuLabel(id));
            return entries;
        }
        case ScreenState::AttackCard: {
            std::vector<std::string> entries;
            for (int index : pendingCardIndexes_) entries.push_back(cardMenuLabel(controller_.currentPlayer(), index));
            return entries;
        }
        case ScreenState::AttackBeastBoost: {
            std::vector<std::string> entries{
                "Done choosing Beast Form discards (+" + std::to_string(selectedBeastFormBoostIndexes_.size()) + " attack)",
            };
            for (int index : pendingCardIndexes_) entries.push_back(cardMenuLabel(controller_.currentPlayer(), index));
            return entries;
        }
        case ScreenState::DefenseCard: {
            std::vector<std::string> entries{"No defense"};
            for (int index : pendingCardIndexes_) entries.push_back(cardMenuLabel(controller_.opponentPlayer(), index));
            return entries;
        }
        case ScreenState::DefenseElementaryPrediction: {
            std::vector<std::string> entries;
            for (int value : pendingValues_) entries.push_back("Predict attack value: " + std::to_string(value));
            return entries;
        }
        case ScreenState::SchemeCard:
        case ScreenState::DiscardCard:
        case ScreenState::DiscardToLimit: {
            std::vector<std::string> entries;
            for (int index : pendingCardIndexes_) entries.push_back(cardMenuLabel(controller_.currentPlayer(), index));
            return entries;
        }
        case ScreenState::SchemeChoice: {
            std::vector<std::string> entries;
            if (waitingForDestination_) {
                for (int space : pendingSpaces_) entries.push_back(spaceMenuLabel(space));
                return entries;
            }
            if (!pendingFighterIds_.empty()) {
                for (const auto& id : pendingFighterIds_) entries.push_back(fighterMenuLabel(id));
                return entries;
            }
            if (!pendingSpaces_.empty()) {
                for (int space : pendingSpaces_) entries.push_back(spaceMenuLabel(space));
                return entries;
            }
            if (!pendingValues_.empty()) {
                for (int value : pendingValues_) entries.push_back("Value " + std::to_string(value));
                return entries;
            }
            for (int index : pendingCardIndexes_) entries.push_back(cardMenuLabel(controller_.opponentPlayer(), index));
            return entries;
        }
        case ScreenState::ConfirmSuspicionChoice: {
            std::vector<std::string> entries;
            for (int index : pendingCardIndexes_) entries.push_back(cardMenuLabel(controller_.opponentPlayer(), index));
            return entries;
        }
        case ScreenState::ConfoundChoice:
            return {"Yes, discard a card", "No, move a fog token"};
        case ScreenState::ConfoundDiscardSelect: {
            std::vector<std::string> entries;
            for (int idx : pendingCardIndexes_) entries.push_back(cardMenuLabel(controller_.opponentPlayer(), idx));
            return entries;
        }
        case ScreenState::ConfoundFogSelect: {
            std::vector<std::string> entries;
            const auto* invisible = dynamic_cast<const InvisibleMan*>(&controller_.currentPlayer().heroFighter());
            if (invisible) {
                const auto& tokens = invisible->getFogTokens();
                for (int idx : pendingFogIndices_) {
                    entries.push_back("Fog token " + std::to_string(idx + 1) + " (space " + std::to_string(tokens[idx]) + ")");
                }
            }
            return entries;
        }
        case ScreenState::ConfoundDestinationSelect: {
            std::vector<std::string> entries;
            for (int space : pendingSpaces_) entries.push_back(spaceMenuLabel(space));
            return entries;
        }
        case ScreenState::FogChoice: {
            std::vector<std::string> entries;
            if (!controller_.hasPendingFogChoice()) return entries;
            const auto& pending = controller_.pendingFogChoice();
            const Player* owner = nullptr;
            for (const auto& p : controller_.players()) {
                for (const auto& f : p.fighters()) {
                    if (f->id() == pending.fighterId) { owner = &p; break; }
                }
                if (owner) break;
            }
            if (!owner) return entries;
            const Fighter* fighter = &owner->fighterById(pending.fighterId);
            const auto* invisible = dynamic_cast<const InvisibleMan*>(fighter);
            if (!invisible) return entries;
            const auto& tokens = invisible->getFogTokens();
            for (int idx : pendingFogIndices_) {
                if (idx >= 0 && idx < static_cast<int>(tokens.size()) && tokens[idx] != -1) {
                    entries.push_back("Fog Token " + std::to_string(idx + 1) + " (space " + std::to_string(tokens[idx]) + ")");
                }
            }
            return entries;
        }
        case ScreenState::FogDestination: {
            std::vector<std::string> entries;
            for (int space : pendingFogDestinations_) entries.push_back(spaceMenuLabel(space));
            return entries;
        }
        case ScreenState::CodedNotesSelect: {
            std::vector<std::string> entries;
            const Player* player = nullptr;
            if (controller_.hasPendingCodedNotes()) {
                const auto& pending = controller_.pendingCodedNotes();
                player = &controller_.players()[pending.playerIndex];
            } else {
                player = &controller_.currentPlayer();
            }
            if (player) {
                for (size_t i = 0; i < player->hand().size(); ++i) {
                    if (codedNotesCardIndex_ != -1 && static_cast<int>(i) == codedNotesCardIndex_) continue;
                    bool isSelected = (std::find(selectedCodedNotesIndices_.begin(), selectedCodedNotesIndices_.end(), static_cast<int>(i)) != selectedCodedNotesIndices_.end());
                    std::string prefix = isSelected ? "[X] " : "[ ] ";
                    entries.push_back(prefix + player->hand()[i].getTitle());
                }
            }
            entries.push_back("Confirm selection (" + std::to_string(selectedCodedNotesIndices_.size()) + "/2 selected)");
            entries.push_back("Cancel");
            return entries;
        }
        case ScreenState::PlaceVanishedInvisibleMan: {
            std::vector<std::string> entries;
            for (int space : pendingSpaces_) entries.push_back(spaceMenuLabel(space));
            return entries;
        }
        case ScreenState::GameOver:
            return {"Back to main menu", "Exit"};
        default:
            return {};
    }
}

void TuiApp::resetSelection() {
    selected_ = 0;
}

void TuiApp::showError(const std::string& message) {
    errorMessage_ = message;
}

void TuiApp::openGameScreen() {
    if (controller_.gameOver()) {
        state_ = ScreenState::GameOver;
    } else if (!controller_.getStudyMethodsHandInfo().empty()) {
        state_ = ScreenState::StudyMethodsView;
    } else if (!controller_.getConfirmSuspicionHandInfo().empty()) {
        state_ = ScreenState::ConfirmSuspicionNoMatchView;
   } else if (controller_.hasPendingCodedNotes()) {
        const auto& pending = controller_.pendingCodedNotes();
        const Player& player = controller_.players()[pending.playerIndex];
        pendingCardIndexes_.clear();
        for (int i = 0; i < static_cast<int>(player.hand().size()); ++i) {
            pendingCardIndexes_.push_back(i);
        }
        selectedCodedNotesIndices_.clear();
        codedNotesCardIndex_ = -1;
        state_ = ScreenState::CodedNotesSelect;
        resetSelection();
    } else if (controller_.getConfoundSchemeCardIndex() != -1) {
        state_ = ScreenState::ConfoundChoice;
    } else if (controller_.hasPendingFogChoice()) {
        if (selectedFogIndex_ == -1) {
            pendingFogIndices_ = controller_.pendingFogChoices();
            state_ = ScreenState::FogChoice;
        } else {
            pendingFogDestinations_ = controller_.getReachableFogDestinations(selectedFogIndex_);
            state_ = ScreenState::FogDestination;
        }
        resetSelection();
    } else if (controller_.hasPendingConfoundChoice()) {
        auto* invisible = dynamic_cast<InvisibleMan*>(&controller_.currentPlayer().heroFighter());
        if (invisible) {
            pendingFogIndices_.clear();
            const auto& tokens = invisible->getFogTokens();
            for (int i = 0; i < static_cast<int>(tokens.size()); ++i) {
                if (tokens[i] != -1) pendingFogIndices_.push_back(i);
            }
        }
        state_ = ScreenState::ConfoundChoice;
        resetSelection();
    } else if (controller_.hasPendingOptionalMovement()) {
        pendingSpaces_ = controller_.pendingOptionalMovementDestinations();
        state_ = ScreenState::OptionalMovementDestination;
    } else if (controller_.currentPlayerMustDiscardToLimit()) {
        pendingCardIndexes_.clear();
        for (int i = 0; i < static_cast<int>(controller_.currentPlayer().hand().size()); ++i) {
            pendingCardIndexes_.push_back(i);
        }
        state_ = ScreenState::DiscardToLimit;
    } else if (controller_.hasPendingVanishedPlacement()) {
        pendingSpaces_ = controller_.getValidPlacementSpacesForVanished();
        state_ = ScreenState::PlaceVanishedInvisibleMan;
    } else {
        state_ = ScreenState::Game;
    }
    resetSelection();
}

void TuiApp::startGameFromSetup() {
    std::unique_ptr<Fighter> hero1;
    std::unique_ptr<Fighter> hero2;
    if (dynamic_cast<Dracula*>(selectedHero_.get())) {
        hero1 = std::make_unique<Dracula>();
        hero2 = std::move(secondHero_);
    } else if (dynamic_cast<Sherlock*>(selectedHero_.get())) {
        hero1 = std::make_unique<Sherlock>();
        hero2 = std::move(secondHero_);
    } else if (dynamic_cast<InvisibleMan*>(selectedHero_.get())) {
        hero1 = std::make_unique<InvisibleMan>();
        hero2 = std::move(secondHero_);
    }
    controller_.startNewGame(playerOneAge_, playerTwoAge_, std::move(hero1), std::move(hero2), selectedStartSlot_);
    state_ = ScreenState::Game;
    selected_ = 0;
    selectedAttackerId_.clear();
    pendingSpaces_.clear();
    pendingFighterIds_.clear();
    pendingCardIndexes_.clear();
    pendingValues_.clear();
    pendingFogIndices_.clear();
    selectedNamedValue_ = -1;
    selectedAttackCardIndex_ = -1;
    selectedDefenseCardIndex_ = -1;
    selectedBeastFormBoostIndexes_.clear();
    waitingForDestination_ = false;
    codedNotesCardIndex_ = -1;
    lurkingCardIndex_ = -1;
    stepLightlyCardIndex_ = -1;
    selectedCodedNotesIndices_.clear();
    schemeChoice_ = SchemeChoice{};
    selectedFogIndex_ = -1;
    pendingFogDestinations_.clear();
    std::remove("tui_state.json");
    openGameScreen();
    resetSelection();
}

void TuiApp::beginAttackFlow() {
    pendingFighterIds_ = controller_.legalAttackers();
    if (pendingFighterIds_.empty()) {
        showError("No legal attacker has both a target and an attack card.");
        return;
    }
    state_ = ScreenState::AttackAttacker;
    resetSelection();
}

void TuiApp::beginSchemeFlow() {
    pendingCardIndexes_ = controller_.legalSchemeCards();
    if (pendingCardIndexes_.empty()) {
        showError("No legal scheme card is available.");
        return;
    }
    state_ = ScreenState::SchemeCard;
    resetSelection();
}

void TuiApp::beginManeuverFlow() {
    pendingCardIndexes_ = controller_.legalBoostCardIndexes();
    state_ = ScreenState::ManeuverBoost;
    resetSelection();
}

void TuiApp::chooseSchemeCard(int selectedMenuIndex) {
    if (selectedMenuIndex < 0 || selectedMenuIndex >= static_cast<int>(pendingCardIndexes_.size())) {
        showError("Invalid scheme card selection.");
        state_ = ScreenState::Game;
        resetSelection();
        return;
    }
    selectedSchemeCardIndex_ = pendingCardIndexes_.at(static_cast<std::size_t>(selectedMenuIndex));
    schemeChoice_ = SchemeChoice{};
    pendingCardIndexes_.clear();
    pendingFighterIds_.clear();
    pendingSpaces_.clear();
    pendingValues_.clear();

    const Card& card = controller_.currentPlayer().hand().at(selectedSchemeCardIndex_);

    if (card.getTitle() == "LURKING") {
        lurkingCardIndex_ = selectedSchemeCardIndex_;
        state_ = ScreenState::LurkingChoice;
        resetSelection();
        return;
    }

    if (card.getTitle() == "STEP LIGHTLY") {
        pendingFighterIds_.clear();
        const Fighter& invisible = controller_.currentPlayer().heroFighter();
        for (auto& fighter : controller_.opponentPlayer().fighters()) {
            if (!fighter->defeated() && controller_.board().areAdjacentForCombat(invisible.spaceId(), fighter->spaceId())) {
                pendingFighterIds_.push_back(fighter->id());
            }
        }
        if (pendingFighterIds_.empty()) {
            showError("No adjacent enemy fighters.");
            Card played = controller_.currentPlayer().removeCardFromHand(selectedSchemeCardIndex_);
            controller_.currentPlayer().addToDiscard(std::move(played));
            controller_.decrementActions();
            openGameScreen();
            return;
        }
        stepLightlyCardIndex_ = selectedSchemeCardIndex_;
        state_ = ScreenState::StepLightlyTarget;
        resetSelection();
        return;
    }

    if (card.getTitle() == "CODED NOTES") {
        codedNotesCardIndex_ = selectedSchemeCardIndex_;
        selectedCodedNotesIndices_.clear();
        state_ = ScreenState::CodedNotesSelect;
        resetSelection();
        return;
    }

    SchemeChoiceKind kind = controller_.requiredChoiceForScheme(selectedSchemeCardIndex_);
    if (kind == SchemeChoiceKind::None) {
        controller_.playScheme(selectedSchemeCardIndex_, schemeChoice_);
        openGameScreen();
        return;
    }
    if (kind == SchemeChoiceKind::Destination) {
        pendingSpaces_ = controller_.destinationChoicesForScheme(selectedSchemeCardIndex_, schemeChoice_);
        if (pendingSpaces_.empty()) {
            showError("No valid destination for this scheme.");
            Card played = controller_.currentPlayer().removeCardFromHand(selectedSchemeCardIndex_);
            controller_.currentPlayer().addToDiscard(std::move(played));
            controller_.decrementActions();
            controller_.endTurnIfNeeded();
            openGameScreen();
            return;
        }
    } else if (kind == SchemeChoiceKind::NamedValue) {
        pendingValues_ = controller_.namedValueChoicesForScheme(selectedSchemeCardIndex_);
    } else if (kind == SchemeChoiceKind::OpponentHandCard) {
        pendingCardIndexes_ = controller_.opponentHandChoicesForScheme(selectedSchemeCardIndex_);
    } else if (kind == SchemeChoiceKind::TargetFighter || kind == SchemeChoiceKind::TargetAndDestination) {
        pendingFighterIds_ = controller_.targetChoicesForScheme(selectedSchemeCardIndex_);
    }

    bool hasChoices = !pendingSpaces_.empty() || !pendingFighterIds_.empty() ||
                      !pendingValues_.empty() || !pendingCardIndexes_.empty();

    if (!hasChoices) {
        showError("This scheme has no legal choice right now.");
        Card played = controller_.currentPlayer().removeCardFromHand(selectedSchemeCardIndex_);
        controller_.currentPlayer().addToDiscard(std::move(played));
        controller_.decrementActions();
        controller_.endTurnIfNeeded();
        openGameScreen();
        return;
    }

    state_ = ScreenState::SchemeChoice;
    resetSelection();
}

std::string TuiApp::fighterMenuLabel(const std::string& fighterId) const {
    const Fighter* fighter = controller_.findFighterById(fighterId);
    if (!fighter) return fighterId;
    std::ostringstream label;
    label << fighter->displayName() << " | HP " << fighter->health() << "/" << fighter->maxHealth();
    if (!fighter->defeated()) {
        label << " | Space " << fighter->spaceId();
        int rem = controller_.remainingMovementForFighter(fighterId);
        if (rem >= 0) label << " | Moves: " << rem;
    }
    return label.str();
}

void TuiApp::completeSchemeChoice() {
    SchemeChoiceKind kind = controller_.requiredChoiceForScheme(selectedSchemeCardIndex_);

    if (kind == SchemeChoiceKind::Destination) {
        if (selected_ < 0 || selected_ >= static_cast<int>(pendingSpaces_.size())) {
            showError("Invalid destination selection.");
            return;
        }
        schemeChoice_.destinationSpace = pendingSpaces_.at(static_cast<std::size_t>(selected_));
        controller_.playScheme(selectedSchemeCardIndex_, schemeChoice_);
        openGameScreen();
        return;
    }

    if (kind == SchemeChoiceKind::NamedValue) {
        if (selected_ < 0 || selected_ >= static_cast<int>(pendingValues_.size())) {
            showError("Invalid value selection.");
            return;
        }
        int namedValue = pendingValues_.at(static_cast<std::size_t>(selected_));
        const Card& card = controller_.currentPlayer().hand().at(selectedSchemeCardIndex_);
        if (card.getTitle() == "CONFIRM SUSPICION") {
            auto matching = controller_.getMatchingCardIndicesForConfirmSuspicion(namedValue);
            if (matching.empty()) {
                controller_.playConfirmSuspicion(selectedSchemeCardIndex_, namedValue);
                openGameScreen();
                return;
            } else {
                selectedNamedValue_ = namedValue;
                pendingCardIndexes_ = matching;
                state_ = ScreenState::ConfirmSuspicionChoice;
                resetSelection();
                return;
            }
        } else {
            schemeChoice_.namedValue = namedValue;
            controller_.playScheme(selectedSchemeCardIndex_, schemeChoice_);
            openGameScreen();
            return;
        }
    }

    if (kind == SchemeChoiceKind::OpponentHandCard) {
        if (selected_ < 0 || selected_ >= static_cast<int>(pendingCardIndexes_.size())) {
            showError("Invalid card selection.");
            return;
        }
        schemeChoice_.opponentHandIndex = pendingCardIndexes_.at(static_cast<std::size_t>(selected_));
        controller_.playScheme(selectedSchemeCardIndex_, schemeChoice_);
        openGameScreen();
        return;
    }

    if (kind == SchemeChoiceKind::TargetFighter) {
        if (selected_ < 0 || selected_ >= static_cast<int>(pendingFighterIds_.size())) {
            showError("Invalid fighter selection.");
            return;
        }
        schemeChoice_.targetFighterId = pendingFighterIds_.at(static_cast<std::size_t>(selected_));
        controller_.playScheme(selectedSchemeCardIndex_, schemeChoice_);
        openGameScreen();
        return;
    }

    if (kind == SchemeChoiceKind::TargetAndDestination) {
        if (!waitingForDestination_) {
            if (selected_ < 0 || selected_ >= static_cast<int>(pendingFighterIds_.size())) {
                showError("Invalid fighter selection.");
                return;
            }
            schemeChoice_.targetFighterId = pendingFighterIds_.at(static_cast<std::size_t>(selected_));
            pendingFighterIds_.clear();
            pendingSpaces_ = controller_.destinationChoicesForScheme(selectedSchemeCardIndex_, schemeChoice_);
            waitingForDestination_ = true;
            resetSelection();
            return;
        }
        if (selected_ < 0 || selected_ >= static_cast<int>(pendingSpaces_.size())) {
            showError("Invalid destination selection.");
            return;
        }
        schemeChoice_.destinationSpace = pendingSpaces_.at(static_cast<std::size_t>(selected_));
        controller_.playScheme(selectedSchemeCardIndex_, schemeChoice_);
        waitingForDestination_ = false;
        openGameScreen();
    }

    showError("Invalid scheme choice.");
    openGameScreen();
}

std::string TuiApp::cardMenuLabel(const Player& player, int handIndex) const {
    if (handIndex < 0 || handIndex >= static_cast<int>(player.hand().size())) {
        return "Invalid card";
    }
    const Card& card = player.hand().at(static_cast<std::size_t>(handIndex));
    std::ostringstream label;
    label << (handIndex + 1) << ". " << card.getTitle()
          << " [" << cardTypeToString(card.getType()) << "]"
          << " owner=" << characterToString(card.getOwner())
          << " boost=" << card.getBoost();
    if (card.getAttack() >= 0) label << " atk=" << card.getAttack();
    if (card.getDefense() >= 0) label << " def=" << card.getDefense();
    return label.str();
}

std::string TuiApp::spaceMenuLabel(int spaceId) const {
    if (!controller_.board().contains(spaceId)) return "Invalid space";
    const Space& space = controller_.board().space(spaceId);
    std::ostringstream label;
    label << "Space " << space.id() << " | Zone " << space.zoneLabel();
    if (space.hasSecretPassage()) label << " | secret";
    if (space.startSlot() > 0) label << " | start " << space.startSlot();
    return label.str();
}

Element TuiApp::menuLine(const std::string& label, bool selected) const {
    auto line = text((selected ? "> " : "  ") + label);
    if (selected) return line | inverted | bold;
    return line;
}

Element TuiApp::cardElement(const Card& card, int index, bool highlighted, bool activeOwner) const {
    std::ostringstream line;
    line << std::setw(2) << (index + 1) << " " << fixedWidth(card.getTitle(), 22)
         << " " << fixedWidth(cardTypeToString(card.getType()), 9)
         << " B" << card.getBoost();
    if (card.getAttack() >= 0) line << " A" << card.getAttack();
    if (card.getDefense() >= 0) line << " D" << card.getDefense();
    auto element = text(line.str());
    if (highlighted) element = element | inverted;
    if (activeOwner) element = element | color(Color::White);
    else element = element | dim;
    return element;
}

Element TuiApp::fighterLine(const Fighter& fighter) const {
    std::ostringstream line;
    line << fixedWidth(fighter.displayName(), 16)
         << " HP " << std::setw(2) << fighter.health() << "/" << std::setw(2) << fighter.maxHealth()
         << " " << hpBar(fighter.health(), fighter.maxHealth());
    if (fighter.defeated()) line << " defeated";
    else line << " space " << fighter.spaceId();
    return text(line.str());
}

json TuiApp::serializeState() const {
    json j;
    j["state"] = static_cast<int>(state_);
    j["selected"] = selected_;
    j["selectedAttackerId"] = selectedAttackerId_;
    j["pendingSpaces"] = pendingSpaces_;
    j["pendingFighterIds"] = pendingFighterIds_;
    j["pendingCardIndexes"] = pendingCardIndexes_;
    j["pendingValues"] = pendingValues_;
    j["pendingFogIndices"] = pendingFogIndices_;
    j["selectedSchemeCardIndex"] = selectedSchemeCardIndex_;
    j["selectedNamedValue"] = selectedNamedValue_;
    j["selectedAttackCardIndex"] = selectedAttackCardIndex_;
    j["selectedDefenseCardIndex"] = selectedDefenseCardIndex_;
    j["selectedBeastFormBoostIndexes"] = selectedBeastFormBoostIndexes_;
    j["waitingForDestination"] = waitingForDestination_;
    j["codedNotesCardIndex"] = codedNotesCardIndex_;
    j["lurkingCardIndex"] = lurkingCardIndex_;
    j["stepLightlyCardIndex"] = stepLightlyCardIndex_;
    j["selectedCodedNotesIndices"] = selectedCodedNotesIndices_;
    json sc;
    sc["destinationSpace"] = schemeChoice_.destinationSpace;
    sc["targetFighterId"] = schemeChoice_.targetFighterId;
    sc["namedValue"] = schemeChoice_.namedValue;
    sc["opponentHandIndex"] = schemeChoice_.opponentHandIndex;
    j["schemeChoice"] = sc;
    return j;
}

void TuiApp::deserializeState(const json& j) {
    state_ = static_cast<ScreenState>(j["state"].get<int>());
    selected_ = j["selected"].get<int>();
    selectedAttackerId_ = j.value("selectedAttackerId", "");
    pendingSpaces_ = j.value("pendingSpaces", std::vector<int>{});
    pendingFighterIds_ = j.value("pendingFighterIds", std::vector<std::string>{});
    pendingCardIndexes_ = j.value("pendingCardIndexes", std::vector<int>{});
    pendingValues_ = j.value("pendingValues", std::vector<int>{});
    pendingFogIndices_ = j.value("pendingFogIndices", std::vector<int>{});
    selectedSchemeCardIndex_ = j.value("selectedSchemeCardIndex", -1);
    selectedNamedValue_ = j.value("selectedNamedValue", -1);
    selectedAttackCardIndex_ = j.value("selectedAttackCardIndex", -1);
    selectedDefenseCardIndex_ = j.value("selectedDefenseCardIndex", -1);
    selectedBeastFormBoostIndexes_ = j.value("selectedBeastFormBoostIndexes", std::vector<int>{});
    waitingForDestination_ = j.value("waitingForDestination", false);
    codedNotesCardIndex_ = j.value("codedNotesCardIndex", -1);
    lurkingCardIndex_ = j.value("lurkingCardIndex", -1);
    stepLightlyCardIndex_ = j.value("stepLightlyCardIndex", -1);
    if (j.contains("selectedCodedNotesIndices")) {
        selectedCodedNotesIndices_ = j["selectedCodedNotesIndices"].get<std::vector<int>>();
    }
    if (j.contains("schemeChoice")) {
        const auto& sc = j["schemeChoice"];
        schemeChoice_.destinationSpace = sc.value("destinationSpace", -1);
        schemeChoice_.targetFighterId = sc.value("targetFighterId", "");
        schemeChoice_.namedValue = sc.value("namedValue", -1);
        schemeChoice_.opponentHandIndex = sc.value("opponentHandIndex", -1);
    }
}

void TuiApp::saveTuiState() const {
    std::string filename = "tui_state.json";
    std::ofstream file(filename);
    file << serializeState().dump(4);
}

void TuiApp::loadTuiState() {
    std::string filename = "tui_state.json";
    if (!std::ifstream(filename).good()) return;
    std::ifstream file(filename);
    json j;
    file >> j;
    deserializeState(j);
    if (state_ == ScreenState::Game) openGameScreen();
    else resetSelection();
}

}  // namespace unmatched
