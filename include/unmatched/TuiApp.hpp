#pragma once

#include "unmatched/GameController.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <optional>
#include <string>
#include <vector>

namespace unmatched {

class TuiApp : public ftxui::ComponentBase {
public:
    explicit TuiApp(ftxui::ScreenInteractive& screen);

    ftxui::Element Render() override;
    bool OnEvent(ftxui::Event event) override;

private:
    enum class ScreenState {
        MainMenu,
        Help,
        Settings,
        SetupAge,
        FighterSelect,
        StartSelect,
        Game,
        ManeuverBoost,
        ManeuverFighter,
        ManeuverDestination,
        AttackAttacker,
        AttackTarget,
        AttackCard,
        AttackBeastBoost,
        DefenseCard,
        DefenseElementaryPrediction,
        SchemeCard,
        SchemeChoice,
        ConfirmSuspicionChoice, 
        DiscardCard,
        DiscardToLimit,
        DraculaAbilityTarget,
        OptionalMovementDestination,
        GameOver,
    };

    ftxui::Element renderMainMenu() const;
    ftxui::Element renderHelp() const;
    ftxui::Element renderSetupAge() const;
    ftxui::Element renderFighterSelect() const;
    ftxui::Element renderStartSelect() const;
    ftxui::Element renderGame() const;
    ftxui::Element renderGameOver() const;
    ftxui::Element renderPlayerPanel(const Player& player, bool active) const;
    ftxui::Element renderHandPanel(const Player& player, bool active) const;
    ftxui::Element renderActionPanel() const;
    ftxui::Element renderMapPanel() const;
    ftxui::Element renderMenuLines(const std::vector<std::string>& entries, const std::string& title) const;

    bool handleEvent(ftxui::Event event);
    void handleEnter();
    void handleDigit(char digit);
    void handleBackspace();
    void moveSelection(int delta);
    std::vector<std::string> currentMenuEntries() const;
    void resetSelection();
    void showError(const std::string& message);
    void openGameScreen();
    void startGameFromSetup();
    void beginAttackFlow();
    void beginSchemeFlow();
    void beginManeuverFlow();
    void chooseSchemeCard(int selectedMenuIndex);
    void completeSchemeChoice();

    std::string fighterMenuLabel(const std::string& fighterId) const;
    std::string cardMenuLabel(const Player& player, int handIndex) const;
    std::string spaceMenuLabel(int spaceId) const;
    ftxui::Element menuLine(const std::string& label, bool selected) const;
    ftxui::Element cardElement(const Card& card, int index, bool highlighted, bool activeOwner) const;
    ftxui::Element fighterLine(const Fighter& fighter) const;
    ftxui::Color heroColor(HeroKind hero) const;

    ftxui::ScreenInteractive& screen_;
    GameController controller_;
    ScreenState state_;
    int selected_;
    std::string ageInput_;
    int ageStep_;
    int playerOneAge_;
    int playerTwoAge_;
    HeroKind selectedHero_;
    int selectedStartSlot_;
    std::string errorMessage_;
    bool asciiOnlyMode_;

    std::vector<std::string> pendingFighterIds_;
    std::vector<int> pendingCardIndexes_;
    std::vector<int> pendingSpaces_;
    std::vector<int> pendingValues_;
    std::string selectedAttackerId_;
    std::string selectedTargetId_;
    int selectedNamedValue_ = -1;
    int selectedAttackCardIndex_;
    int selectedDefenseCardIndex_ = -1;
    int selectedSchemeCardIndex_;
    std::vector<int> selectedBeastFormBoostIndexes_;
    SchemeChoice schemeChoice_;
};

}  // namespace unmatched
