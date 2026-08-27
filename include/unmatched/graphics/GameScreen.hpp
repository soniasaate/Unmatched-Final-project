#pragma once

#include "Application.hpp"
#include "Screen.hpp"
#include "unmatched/GameController.hpp"

#include <SFML/Graphics.hpp>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace unmatched::gfx {


class GameScreen : public Screen {
public:
    GameScreen(Application& app, int playerOneAge, int playerTwoAge,
               int fighter1Index, int fighter2Index, int youngerStartSlot);
    
    GameScreen(Application& app, int loadSlot);
    ~GameScreen() override = default;

    void handleEvent(const sf::Event& event) override;
    void update(float deltaSeconds) override;
    void render(sf::RenderWindow& window) override;

private:
    enum class Mode {
        Idle,
        ManeuverSelectBoost,
        ManeuverSelectFighter,
        ManeuverSelectDestination,
        AttackSelectAttacker,
        AttackSelectCard,
        AttackBeastBoost,
        AttackSelectTarget,
        AttackSelectDefenseCard,
        AttackElementaryPrediction,
        SchemeSelectCard,
        SchemeSelectTarget,
        SchemeSelectDestination,
        SchemeSelectNamedValue,
        SchemeSelectOpponentCard,
        SchemeStepLightlyTarget,
        RaveningTarget,
        RaveningDestination,
        RaveningContinue,
        ConfoundYesNo,
        ConfoundDiscardCard,
        ConfoundFogSelect,
        ConfoundFogDestination,
        CodedNotesSelectCards,
        LurkingChoice,
        LurkingFogToken,
        LurkingDestination,
        SlipAwayFogToken,
        SlipAwayDestination,
        RollingFogFogToken,
        RollingFogDestination,
        FogTokenSelect,
        FogTokenDestination,
        VanishedPlacement,
        InfoPopup,
        DraculaAbilityTarget,
        DiscardSelectCard,
        OptionalMovementDestination,
        LoadGame,
        GameOver,
    };

    struct HandCardView {
        int playerIndex;
        int handIndex;
        sf::FloatRect bounds;
    };

    struct ActiveHandSelection {
        int playerIndex;
        std::vector<int> legalIndices;
        std::function<void(int)> onChosen;
    };

    struct UiButton {
        sf::Text text;
        sf::RectangleShape background;
        sf::FloatRect bounds;
        std::function<void()> onClick;
        sf::Color accent = sf::Color(150, 145, 135);
        bool hovered = false;

        UiButton(sf::Font& font, const std::string& label, sf::Vector2f position, sf::Vector2f size);
        void handleEvent(const sf::Event& event);
        void render(sf::RenderWindow& window);
    };


    std::unique_ptr<Fighter> createFighterByIndex(int index) const;
    void computeSpacePositions();
    sf::Color accentColorFor(const Fighter& hero) const;
    void setupCommonVisuals();

    void renderTopBar(sf::RenderWindow& window);
    void renderBoard(sf::RenderWindow& window);
    void renderFighterTokens(sf::RenderWindow& window);
    void renderFogMarkersOnBoard(sf::RenderWindow& window);
    void renderSidePanel(sf::RenderWindow& window, int playerIndex, sf::Vector2f origin);
    void renderCardBackIcon(sf::RenderWindow& window, int playerIndex, sf::Vector2f origin);
    void renderHandOverlay(sf::RenderWindow& window, int playerIndex);
    void renderDiscardOverlay(sf::RenderWindow& window, int playerIndex);
    void renderControlRow(sf::RenderWindow& window);
    void renderStatus(sf::RenderWindow& window);
    void renderGameOver(sf::RenderWindow& window);
    void renderInfoPopup(sf::RenderWindow& window);

    void enterMode(Mode mode);
    void rebuildActionButtons();
    
    void rebuildChipButtons(const std::vector<std::string>& labels, std::function<void(int)> onSelect,
                            bool vertical = false);
    void clearInteractiveWidgets();

    void onManeuverClicked();
    void onAttackClicked();
    void onSchemeClicked();
    void onDiscardClicked();
    void onEndTurnClicked();
    void onDraculaAbilityClicked();
    void onUndoClicked();
    void onSaveClicked();
    void onLoadClicked();
    void onLoadSlotChosen(int slot);
    void onHelpClicked();
    void onMainMenuClicked();

    void onBoostCardChosen(int handIndex);
    void onManeuverFighterChosen(int index, const std::vector<std::string>& ids);
    void onDestinationSpaceClicked(int spaceId);
    void onFinishManeuverClicked();

    void onAttackerChosen(int index, const std::vector<std::string>& ids);
    void onAttackCardChosen(int handIndex);
    void onBeastBoostChosen(int handIndex);
    void onBeastBoostDone();
    void onTargetChosen(int index, const std::vector<std::string>& ids);
    void onDefenseCardChosen(int handIndex);
    void onElementaryPredictionChosen(int value);
    void onDraculaAbilityTargetChosen(int index, const std::vector<std::string>& ids);

    void onSchemeCardChosen(int handIndex);
    void onSchemeTargetChosen(int index, const std::vector<std::string>& ids);
    void onSchemeNamedValueChosen(int index, const std::vector<int>& values);
    void onSchemeOpponentCardChosen(int index, const std::vector<int>& indexes);
    void onSchemeStepLightlyTargetChosen(int index, const std::vector<std::string>& ids);
    void finalizeSchemeIfReady();
    void onRaveningTargetChosen(int index, const std::vector<std::string>& ids);
    void onRaveningDestinationClicked(int spaceId);
    void onRaveningContinue();
    void onRaveningFinish();

    void onConfoundYes();
    void onConfoundNo();
    void onConfoundCardChosen(int handIndex);
    void onConfoundFogTokenChosen(int fogIndex);
    void onConfoundFogDestinationClicked(int spaceId);

    void onCodedNotesToggleCard(int handIndex);
    void onCodedNotesConfirm();

    void onLurkingChoiceChosen(int choice);
    void onLurkingFogTokenChosen(int fogIndex);
    void onLurkingDestinationClicked(int spaceId);
    void onLurkingSkip();

    void onSlipAwayFogTokenChosen(int fogIndex);
    void onSlipAwayDestinationClicked(int spaceId);
    void onSlipAwaySkip();

    void onRollingFogFogTokenChosen(int fogIndex);
    void onRollingFogDestinationClicked(int spaceId);
    void onRollingFogSkip();

    void onFogTokenChosen(int tokenIndex);
    void onFogTokenDestinationClicked(int spaceId);

    void onVanishedSpaceClicked(int spaceId);

    void onDiscardCardChosen(int handIndex);
    void onOptionalMovementDestinationClicked(int spaceId);
    void onSkipOptionalMovementClicked();
    void onInfoPopupAcknowledged();
    std::optional<std::string> fighterIdAtPoint(sf::Vector2f point) const;
    bool isSelectableFighter(const std::string& fighterId) const;
    bool handleBoardClick(sf::Vector2f point);

    void checkAutoPrompts();
    void setStatus(const std::string& message);
    void setError(const std::string& message);

    std::string cardStatsLine(const Card& card) const;
    
    std::string cardListLabel(const Card& card) const;
    std::string cardImagePath(const Card& card) const;
    std::string characterImagePath(const Fighter& fighter) const;
    std::string fighterLabel(const std::string& fighterId) const;
    sf::Texture* tryGetTexture(const std::string& path);
    int indexOfPlayer(const Player& player) const;

    GameController controller_;
    Mode mode_ = Mode::Idle;

    sf::Sprite background_;
    std::unique_ptr<sf::Sprite> boardSprite_;
    sf::FloatRect boardBounds_;
    sf::Text titleText_;
    sf::Text statusText_;
    sf::Text errorText_;
    sf::Text gameOverText_;
    std::string infoPopupMessage_;
    std::string infoPopupTitle_;

    std::map<int, sf::Vector2f> spacePositions_;
    float nodeRadius_ = 13.f;
    std::vector<int> highlightedSpaces_;
    std::vector<std::string> selectableFighterIds_;

    std::vector<UiButton> actionButtons_;
    std::vector<UiButton> chipButtons_;
    bool chipListVertical_ = false;
    std::optional<UiButton> backToMenuButton_;
    std::optional<UiButton> infoOkButton_;

    std::vector<HandCardView> handCardViews_;
    std::optional<HandCardView> hoveredCard_;
    sf::Vector2f lastMousePos_;
    std::optional<ActiveHandSelection> activeHandSelection_;

    sf::FloatRect cardBackBoundsLeft_;
    sf::FloatRect cardBackBoundsRight_;
    bool hoveringCardBackLeft_ = false;
    bool hoveringCardBackRight_ = false;
    std::optional<int> openedHandPlayer_;

    
    sf::FloatRect discardBoundsLeft_;
    sf::FloatRect discardBoundsRight_;
    std::optional<int> openedDiscardPlayer_;

    std::set<std::string> missingTexturePaths_;

    int pendingBoostIndex_ = -1;
    int pendingFogTokenIndex_ = -1;
    std::string selectedFighterId_;
    
    bool maneuverBegun_ = false;
    int selectedAttackCardIndex_ = -1;
    int selectedDefenseCardIndex_ = -1;
    std::string selectedTargetId_;
    std::string selectedRaveningFighterId_;
    std::vector<int> selectedBeastFormBoostIndexes_;
    int pendingSchemeHandIndex_ = -1;
    SchemeChoice pendingSchemeChoice_;
    SchemeChoiceKind pendingSchemeKind_ = SchemeChoiceKind::None;

    
    int pendingConfoundFogIndex_ = -1;
    
    std::vector<int> codedNotesSelection_;
};

} // namespace unmatched::gfx
