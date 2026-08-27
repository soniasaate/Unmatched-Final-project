#include "unmatched/graphics/GameScreen.hpp"
#include "unmatched/graphics/MainMenuScreen.hpp"

#include "unmatched/Dracula.hpp"
#include "unmatched/Sherlock.hpp"
#include "unmatched/InvisibleMan.hpp"

#include <algorithm>
#include <cctype>
#include <climits>
#include <sstream>
#include <fstream>
#include <unordered_map>

namespace unmatched::gfx {

namespace {

const sf::Color kBgDark(14, 13, 12);
const sf::Color kPanelFill(22, 20, 18, 235);
const sf::Color kMetalOutline(150, 150, 155);
const sf::Color kTextLight(220, 212, 198);
const sf::Color kTextDim(160, 152, 140);

const sf::Color kAccentDracula(150, 24, 24);
const sf::Color kAccentInvisible(30, 60, 120);
const sf::Color kAccentSherlock(190, 160, 30);
const sf::Color kAccentNeutral(95, 90, 85);

sf::Color getZoneColor(char zone) {
    switch (zone) {
        case 'b': return sf::Color(41, 128, 185);
        case 'r': return sf::Color(160, 82, 45);
        case 'p': return sf::Color(142, 68, 173);
        case 'y': return sf::Color(218, 165, 32);
        case 'g': return sf::Color(39, 174, 96);
        case 'd': return sf::Color(30, 45, 90);
        case 'e': return sf::Color(127, 140, 141);
        default:  return sf::Color(80, 80, 80);
    }
}

constexpr float kTopBarH = 46.f;
constexpr float kPanelX_L = 12.f, kPanelX_R = 1018.f, kPanelW = 250.f, kPanelY = 54.f, kPanelH = 700.f;
constexpr float kPortraitH = 200.f;
constexpr float kBoardX = 274.f, kBoardY = 54.f, kBoardW = 732.f, kBoardH = 560.f;
constexpr float kControlRowY = kBoardY + kBoardH + 10.f; 
constexpr float kControlRowH = 38.f;
constexpr float kStatusY = kControlRowY + kControlRowH + 8.f;
constexpr float kCardBackW = 74.f, kCardBackH = 100.f;
}

GameScreen::UiButton::UiButton(sf::Font& font, const std::string& label,
                               sf::Vector2f position, sf::Vector2f size)
    : text(font), background(size), bounds(position, size) {
    text.setString(label);
    text.setCharacterSize(12);
    text.setFillColor(kTextLight);
    background.setPosition(position);
    background.setFillColor(sf::Color(24, 22, 20, 245));
    background.setOutlineThickness(1.5f);
    background.setOutlineColor(accent);
    const auto b = text.getLocalBounds();
    text.setOrigin(sf::Vector2f(b.position.x + b.size.x / 2.f,
                                b.position.y + b.size.y / 2.f));
    text.setPosition(sf::Vector2f(position.x + size.x / 2.f,
                                  position.y + size.y / 2.f));
}

void GameScreen::UiButton::handleEvent(const sf::Event& event) {
    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        hovered = bounds.contains(sf::Vector2f(static_cast<float>(moved->position.x),
                                               static_cast<float>(moved->position.y)));
    }
    if (const auto* released = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (released->button == sf::Mouse::Button::Left &&
            bounds.contains(sf::Vector2f(static_cast<float>(released->position.x),
                                         static_cast<float>(released->position.y)))) {
            if (onClick) onClick();
        }
    }
}

void GameScreen::UiButton::render(sf::RenderWindow& window) {
    background.setFillColor(hovered ? sf::Color(42, 38, 34, 250) : sf::Color(24, 22, 20, 245));
    background.setOutlineColor(hovered ? sf::Color(235, 205, 130) : accent);
    background.setOutlineThickness(hovered ? 2.0f : 1.5f);
    window.draw(background);
    window.draw(text);
}

std::unique_ptr<Fighter> GameScreen::createFighterByIndex(int index) const {
    switch (index) {
        case 0: return std::make_unique<Dracula>();
        case 1: return std::make_unique<Sherlock>();
        case 2: return std::make_unique<InvisibleMan>();
        default: return std::make_unique<Dracula>();
    }
}

sf::Color GameScreen::accentColorFor(const Fighter& hero) const {
    if (dynamic_cast<const Dracula*>(&hero)) return kAccentDracula;
    if (dynamic_cast<const InvisibleMan*>(&hero)) return kAccentInvisible;
    if (dynamic_cast<const Sherlock*>(&hero)) return kAccentSherlock;
    return kAccentNeutral;
}

GameScreen::GameScreen(Application& app, int playerOneAge, int playerTwoAge,
                       int fighter1Index, int fighter2Index, int youngerStartSlot)
    : Screen(app)
    , background_(app_.resources().getTexture("assets/images/menu_background.jpg"))
    , titleText_(app_.resources().getFont("assets/fonts/title_font.ttf"))
    , statusText_(app_.resources().getFont("assets/fonts/title_font.ttf"))
    , errorText_(app_.resources().getFont("assets/fonts/title_font.ttf"))
    , gameOverText_(app_.resources().getFont("assets/fonts/title_font.ttf")) {

    setupCommonVisuals();

    controller_.startNewGame(playerOneAge, playerTwoAge,
                             createFighterByIndex(fighter1Index),
                             createFighterByIndex(fighter2Index),
                             youngerStartSlot);

    enterMode(Mode::Idle);
}

GameScreen::GameScreen(Application& app, int loadSlot)
    : Screen(app)
    , background_(app_.resources().getTexture("assets/images/menu_background.jpg"))
    , titleText_(app_.resources().getFont("assets/fonts/title_font.ttf"))
    , statusText_(app_.resources().getFont("assets/fonts/title_font.ttf"))
    , errorText_(app_.resources().getFont("assets/fonts/title_font.ttf"))
    , gameOverText_(app_.resources().getFont("assets/fonts/title_font.ttf")) {

    setupCommonVisuals();

    try {
        controller_.loadGame(loadSlot);
        setStatus("Saved game loaded.");
    } catch (const std::exception& e) {
        setError(std::string("Could not load saved game: ") + e.what());
    }

    enterMode(Mode::Idle);
}

void GameScreen::setupCommonVisuals() {
    sf::Vector2u textureSize = background_.getTexture().getSize();
    sf::Vector2u windowSize = app_.window().getSize();
    background_.setScale(sf::Vector2f(
        static_cast<float>(windowSize.x) / static_cast<float>(textureSize.x),
        static_cast<float>(windowSize.y) / static_cast<float>(textureSize.y)));
    background_.setColor(sf::Color(255, 255, 255, 35));

    try {
        auto& tex = app_.resources().getTexture("assets/images/board_baskerville.png");
        boardSprite_ = std::make_unique<sf::Sprite>(tex);
        boardBounds_ = sf::FloatRect(sf::Vector2f(kBoardX, kBoardY), sf::Vector2f(kBoardW, kBoardH));
        const auto size = tex.getSize();
        if (size.x > 0 && size.y > 0) {
            boardSprite_->setPosition(boardBounds_.position);
            boardSprite_->setScale(sf::Vector2f(
                boardBounds_.size.x / static_cast<float>(size.x),
                boardBounds_.size.y / static_cast<float>(size.y)));
        }
    } catch (const std::exception&) {
        boardSprite_.reset();
        boardBounds_ = sf::FloatRect(sf::Vector2f(kBoardX, kBoardY), sf::Vector2f(kBoardW, kBoardH));
    }

    computeSpacePositions();

    titleText_.setCharacterSize(20);
    titleText_.setFillColor(kTextLight);

    statusText_.setCharacterSize(13);
    statusText_.setFillColor(sf::Color(210, 190, 100));
    statusText_.setPosition(sf::Vector2f(kBoardX, kStatusY));

    errorText_.setCharacterSize(13);
    errorText_.setFillColor(sf::Color(220, 60, 60));
    errorText_.setPosition(sf::Vector2f(kBoardX, kStatusY + 20.f));

    gameOverText_.setCharacterSize(56);
    gameOverText_.setFillColor(sf::Color(200, 30, 30));
    gameOverText_.setStyle(sf::Text::Bold);
}

void GameScreen::computeSpacePositions() {
    boardBounds_ = sf::FloatRect(sf::Vector2f(kBoardX, kBoardY), sf::Vector2f(kBoardW, kBoardH));

    const auto& spaces = controller_.board().spaces();
    if (spaces.empty()) return;

    int minRow = INT_MAX, maxRow = INT_MIN;
    int minCol = INT_MAX, maxCol = INT_MIN;
    for (const auto& s : spaces) {
        minRow = std::min(minRow, s.row());
        maxRow = std::max(maxRow, s.row());
        minCol = std::min(minCol, s.column());
        maxCol = std::max(maxCol, s.column());
    }

    constexpr float padX = 0.045f;
    constexpr float padY = 0.10f;
    const float colSpan = static_cast<float>(std::max(1, maxCol - minCol));
    const float rowSpan = static_cast<float>(std::max(1, maxRow - minRow));

    for (const auto& s : spaces) {
        const float nx = static_cast<float>(s.column() - minCol) / colSpan;
        const float ny = static_cast<float>(s.row() - minRow) / rowSpan;
        spacePositions_[s.id()] = sf::Vector2f(
            boardBounds_.position.x + boardBounds_.size.x * (padX + nx * (1.f - 2.f * padX)),
            boardBounds_.position.y + boardBounds_.size.y * (padY + ny * (1.f - 2.f * padY)));
    }
}

void GameScreen::handleEvent(const sf::Event& event) {
    if (mode_ == Mode::GameOver) {
        if (backToMenuButton_) backToMenuButton_->handleEvent(event);
        return;
    }
    if (mode_ == Mode::InfoPopup) {
        if (infoOkButton_) infoOkButton_->handleEvent(event);
        return;
    }

    if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>()) {
        if (keyEvent->code == sf::Keyboard::Key::Z) {
            if (mode_ != Mode::Idle && mode_ != Mode::GameOver && mode_ != Mode::InfoPopup) {
                maneuverBegun_ = false;
                pendingSchemeHandIndex_ = -1;
                selectedAttackCardIndex_ = -1;
                selectedFighterId_.clear();
                selectedTargetId_.clear();
                codedNotesSelection_.clear();
                enterMode(Mode::Idle);
                setStatus("Action canceled.");
            } 
            else if (mode_ == Mode::Idle) {
                if (controller_.canUndo()) {
                    try {
                        controller_.undoLastAction();
                        setStatus("Last action undone.");
                        enterMode(Mode::Idle);
                    } catch (const std::exception& e) {
                        setError(std::string("Undo failed: ") + e.what());
                    }
                } else {
                    setError("Cannot undo: start of turn reached or action already resolved.");
                }
            }
            return;
        }
        if (keyEvent->code == sf::Keyboard::Key::S) {
            onSaveClicked();
            return;
        }
        if (keyEvent->code == sf::Keyboard::Key::L) {
            onLoadClicked();
            return;
        }
    }

    try {
        if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
            (void)moved;
            for (auto& b : actionButtons_) b.handleEvent(event);
            for (auto& b : chipButtons_) b.handleEvent(event);
        } else if (const auto* released = event.getIf<sf::Event::MouseButtonReleased>()) {
            if (released->button == sf::Mouse::Button::Left) {
                const sf::Vector2f clickPos(static_cast<float>(released->position.x), static_cast<float>(released->position.y));

                for (auto& b : actionButtons_) {
                    if (b.bounds.contains(clickPos)) {
                        if (b.onClick) b.onClick();
                        return;
                    }
                }
                for (auto& b : chipButtons_) {
                    if (b.bounds.contains(clickPos)) {
                        if (b.onClick) b.onClick();
                        return;
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        setError(std::string("UI error: ") + e.what());
        enterMode(Mode::Idle);
        return;
    }

    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        lastMousePos_ = sf::Vector2f(static_cast<float>(moved->position.x), static_cast<float>(moved->position.y));
        hoveringCardBackLeft_ = cardBackBoundsLeft_.contains(lastMousePos_);
        hoveringCardBackRight_ = cardBackBoundsRight_.contains(lastMousePos_);
        hoveredCard_.reset();
        for (const auto& view : handCardViews_) {
            if (view.bounds.contains(lastMousePos_)) {
                hoveredCard_ = view;
                break;
            }
        }
    }

    const auto* released = event.getIf<sf::Event::MouseButtonReleased>();
    if (!released || released->button != sf::Mouse::Button::Left) return;

    sf::Vector2f pos(static_cast<float>(released->position.x), static_cast<float>(released->position.y));

    if (activeHandSelection_) {
        for (const auto& view : handCardViews_) {
            if (view.playerIndex != activeHandSelection_->playerIndex) continue;
            if (!view.bounds.contains(pos)) continue;
            const bool legal = std::find(activeHandSelection_->legalIndices.begin(), activeHandSelection_->legalIndices.end(), view.handIndex) != activeHandSelection_->legalIndices.end();
            if (legal) {
                auto callback = activeHandSelection_->onChosen;
                callback(view.handIndex);
            } else {
                setError("This card cannot be used here.");
            }
            return;
        }
    }

    if (cardBackBoundsLeft_.contains(pos)) {
        openedHandPlayer_ = (openedHandPlayer_ && *openedHandPlayer_ == 0) ? std::optional<int>{} : std::optional<int>{0};
        openedDiscardPlayer_.reset();
        return;
    }
    if (cardBackBoundsRight_.contains(pos)) {
        openedHandPlayer_ = (openedHandPlayer_ && *openedHandPlayer_ == 1) ? std::optional<int>{} : std::optional<int>{1};
        openedDiscardPlayer_.reset();
        return;
    }
    if (discardBoundsLeft_.contains(pos)) {
        openedDiscardPlayer_ = (openedDiscardPlayer_ && *openedDiscardPlayer_ == 0) ? std::optional<int>{} : std::optional<int>{0};
        openedHandPlayer_.reset();
        return;
    }
    if (discardBoundsRight_.contains(pos)) {
        openedDiscardPlayer_ = (openedDiscardPlayer_ && *openedDiscardPlayer_ == 1) ? std::optional<int>{} : std::optional<int>{1};
        openedHandPlayer_.reset();
        return;
    }

    if (handleBoardClick(pos)) return;
}

void GameScreen::update(float /*deltaSeconds*/) {
    checkAutoPrompts();
}

void GameScreen::checkAutoPrompts() {
    if (controller_.gameOver() && mode_ != Mode::GameOver) {
        enterMode(Mode::GameOver);
        return;
    }
    if (mode_ == Mode::GameOver || mode_ == Mode::InfoPopup) return;

    if (!controller_.getStudyMethodsHandInfo().empty()) {
        infoPopupTitle_ = "STUDY METHODS";
        infoPopupMessage_ = controller_.getStudyMethodsHandInfo();
        enterMode(Mode::InfoPopup);
        return;
    }
    if (!controller_.getConfirmSuspicionHandInfo().empty()) {
        infoPopupTitle_ = "CONFIRM SUSPICION";
        infoPopupMessage_ = controller_.getConfirmSuspicionHandInfo();
        enterMode(Mode::InfoPopup);
        return;
    }
    if (controller_.hasPendingCodedNotes()) {
        if (mode_ != Mode::CodedNotesSelectCards) {
            codedNotesSelection_.clear();
            enterMode(Mode::CodedNotesSelectCards);
        }
        return;
    }
    if (controller_.hasPendingRaveningChoice()) {
        if (mode_ != Mode::RaveningTarget &&
            mode_ != Mode::RaveningDestination &&
            mode_ != Mode::RaveningContinue) {
            enterMode(Mode::RaveningTarget);
        }
        return;
    }
    if (controller_.hasPendingConfoundChoice()) {
        const auto& pending = controller_.pendingConfoundChoice();
        if (!pending.opponentDecided) {
            if (mode_ != Mode::ConfoundYesNo) enterMode(Mode::ConfoundYesNo);
            return;
        }
        if (pending.opponentWantsToDiscard) {
            if (mode_ != Mode::ConfoundDiscardCard) enterMode(Mode::ConfoundDiscardCard);
            return;
        }
        if (!pending.fogMoveDone) {
            if (mode_ != Mode::ConfoundFogSelect && mode_ != Mode::ConfoundFogDestination) {
                pendingConfoundFogIndex_ = -1;
                enterMode(Mode::ConfoundFogSelect);
            }
            return;
        }
    }
    if (controller_.hasPendingLurking()) {
        const auto& pending = controller_.pendingLurking();
        if (pending.choice == -1) {
            if (mode_ != Mode::LurkingChoice) enterMode(Mode::LurkingChoice);
            return;
        }
        if (pending.choice == 1 && pending.selectedFogIndex == -1) {
            if (mode_ != Mode::LurkingFogToken) enterMode(Mode::LurkingFogToken);
            return;
        }
        if (mode_ != Mode::LurkingDestination) enterMode(Mode::LurkingDestination);
        return;
    }
    if (controller_.hasPendingSlipAway()) {
        const auto& pending = controller_.pendingSlipAway();
        if (pending.selectedFogIndex == -1) {
            if (mode_ != Mode::SlipAwayFogToken) enterMode(Mode::SlipAwayFogToken);
        } else {
            if (mode_ != Mode::SlipAwayDestination) enterMode(Mode::SlipAwayDestination);
        }
        return;
    }
    if (controller_.hasPendingRollingFog()) {
        const auto& pending = controller_.pendingRollingFog();
        if (pending.selectedFogIndex == -1) {
            if (mode_ != Mode::RollingFogFogToken) enterMode(Mode::RollingFogFogToken);
        } else {
            if (mode_ != Mode::RollingFogDestination) enterMode(Mode::RollingFogDestination);
        }
        return;
    }
    if (controller_.hasPendingOptionalMovement()) {
        if (mode_ != Mode::OptionalMovementDestination) enterMode(Mode::OptionalMovementDestination);
        return;
    }
    if (controller_.hasPendingFogChoice()) {
        if (mode_ != Mode::FogTokenSelect && mode_ != Mode::FogTokenDestination) enterMode(Mode::FogTokenSelect);
        return;
    }
    if (controller_.hasPendingVanishedPlacement()) {
        if (mode_ != Mode::VanishedPlacement) enterMode(Mode::VanishedPlacement);
        return;
    }
    if (controller_.currentPlayerMustDiscardToLimit()) {
        if (mode_ != Mode::DiscardSelectCard) {
            setStatus("You must discard down to your hand limit.");
            enterMode(Mode::DiscardSelectCard);
        }
    }
}

sf::Texture* GameScreen::tryGetTexture(const std::string& path) {
    if (missingTexturePaths_.count(path)) return nullptr;
    try {
        return &app_.resources().getTexture(path);
    } catch (const std::exception&) {
        missingTexturePaths_.insert(path);
        return nullptr;
    }
}

std::string GameScreen::cardImagePath(const Card& card) const {
    const std::string title = card.getTitle();
    std::string key;
    for (char c : title) key += (c == '_' ? ' ' : c);

    std::string normalized;
    bool space = false;
    for (char c : key) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!space) normalized += ' ';
            space = true;
        } else {
            normalized += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            space = false;
        }
    }
    while (!normalized.empty() && normalized.front() == ' ') normalized.erase(normalized.begin());
    while (!normalized.empty() && normalized.back() == ' ') normalized.pop_back();

    auto exists = [](const std::string& p) {
        std::ifstream f(p, std::ios::binary);
        return static_cast<bool>(f);
    };

    static const std::unordered_map<std::string, std::string> names = {
        {"ambush","ambush.png"},{"baptism of blood","baptism-of-blood.png"},
        {"beastform","beastform.png"},{"dash","dash.png"},{"do my bidding","do-my-bidding.png"},
        {"exploit","exploit.png"},{"feeding frenzy","feeding-frenzy.png"},
        {"look into my eyes","look-into-my-eyes.png"},{"mistform","mistform.png"},
        {"prey upon","prey-upon.png"},{"ravening seduction","ravening-seduction.png"},
        {"thirst for sustenance","thirst-for-sustenance.png"},
        {"coded notes","coded-notes.png"},{"confound","confound.png"},
        {"covert preparation","covert-preparation.png"},{"dreaming of revenge","dreaming-of-revenge.png"},
        {"emerge from mist","emerge-from-mist.png"},{"impossible to see","impossible-to-see.png"},
        {"into thin air","into-thin-air.png"},{"lurking","lurking.png"},
        {"reign of terror","reign-of-terror.png"},{"rolling fog","rolling-fog.png"},
        {"slip away","slip-away.png"},{"step lightly","step-lightly.png"},
        {"administer aid","administer-aid.png"},{"confirm suspicion","confirm-suspicion.png"},
        {"counterpunch","counterpunch.png"},{"deduce strategy","deduce-strategy.png"},
        {"education never ends","education-never-ends.png"},{"elementary","elementary.png"},
        {"eliminate the impossible","eliminate-the-impossible.png"},
        {"fixed point in a changing age","fixed-point-in-a-changing-age.png"},
        {"master of disguise","master-of-disguise.png"},{"service revolver","service-revolver.png"},
        {"study methods","study-methods.png"},{"the game is afoot","the-game-is-afoot.png"}
    };

    if (normalized == "feint") {
        const std::string dracula = "assets/images/cards/dracula/feint (1).png";
        const std::string sherlock = "assets/images/cards/sherlock/feint (2).png";
        if (exists(dracula)) return dracula;
        if (exists(sherlock)) return sherlock;
        return dracula;
    }

    std::string filename;
    auto it = names.find(normalized);
    if (it != names.end()) filename = it->second;
    else {
        filename = normalized;
        for (char& c : filename) if (c == ' ') c = '-';
        filename += ".png";
    }

    const std::array<std::string,3> folders = {
        "dracula", "sherlock", "invisible_man"
    };
    for (const auto& folder : folders) {
        const std::string p = "assets/images/cards/" + folder + "/" + filename;
        if (exists(p)) return p;
    }

    return "assets/images/cards/dracula/" + filename;
}

std::string GameScreen::characterImagePath(const Fighter& fighter) const {
    return "assets/images/characters/" + fighter.getId() + ".png";
}

int GameScreen::indexOfPlayer(const Player& player) const {
    return player.id();
}

void GameScreen::render(sf::RenderWindow& window) {
    window.clear(kBgDark);
    window.draw(background_);

    if (mode_ == Mode::GameOver) {
        renderGameOver(window);
        return;
    }

    handCardViews_.clear();
    renderTopBar(window);
    renderBoard(window);
    renderFogMarkersOnBoard(window);
    renderSidePanel(window, 0, sf::Vector2f(kPanelX_L, kPanelY));
    renderSidePanel(window, 1, sf::Vector2f(kPanelX_R, kPanelY));

    const bool activeHand = activeHandSelection_.has_value();
    if (activeHand) {
        openedHandPlayer_ = activeHandSelection_->playerIndex;
    }
    if (openedHandPlayer_.has_value()) {
        renderHandOverlay(window, *openedHandPlayer_);
    } else if (openedDiscardPlayer_.has_value()) {
        renderDiscardOverlay(window, *openedDiscardPlayer_);
    }

    renderControlRow(window);
    renderStatus(window);

    if (mode_ == Mode::InfoPopup) renderInfoPopup(window);
}

void GameScreen::renderTopBar(sf::RenderWindow& window) {
    sf::RectangleShape bar(sf::Vector2f(static_cast<float>(app_.window().getSize().x), kTopBarH));
    bar.setPosition(sf::Vector2f(0.f, 0.f));
    bar.setFillColor(sf::Color(10, 9, 8, 235));
    bar.setOutlineThickness(2.f);
    bar.setOutlineColor(kMetalOutline);
    window.draw(bar);

    const Fighter& activeHero = controller_.currentPlayer().heroFighter();
    sf::Color accent = accentColorFor(activeHero);

    std::ostringstream title;
    title << "TURN " << controller_.turnNumber() << "   -   "
          << controller_.currentPlayer().name() << " (" << activeHero.displayName() << ")";
    titleText_.setString(title.str());
    titleText_.setFillColor(accent + sf::Color(80, 80, 80, 0)); 
    sf::FloatRect bounds = titleText_.getLocalBounds();
    titleText_.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f));
    titleText_.setPosition(sf::Vector2f(static_cast<float>(app_.window().getSize().x) / 2.f, kTopBarH / 2.f));
    window.draw(titleText_);

    int totalActions = 2;
    int remaining = controller_.actionsRemaining();
    float dotRadius = 8.f;
    float startX = titleText_.getPosition().x + bounds.size.x / 2.f + 30.f;
    for (int i = 0; i < totalActions; ++i) {
        sf::CircleShape dot(dotRadius);
        dot.setOrigin(sf::Vector2f(dotRadius, dotRadius));
        dot.setPosition(sf::Vector2f(startX + static_cast<float>(i) * (dotRadius * 2.f + 8.f), kTopBarH / 2.f));
        dot.setOutlineThickness(1.5f);
        dot.setOutlineColor(kMetalOutline);
        dot.setFillColor(i < remaining ? accent : sf::Color(40, 38, 36, 200));
        window.draw(dot);
    }
}

void GameScreen::renderBoard(sf::RenderWindow& window) {
    if (boardSprite_) {
        window.draw(*boardSprite_);
        sf::RectangleShape frame(boardBounds_.size);
        frame.setPosition(boardBounds_.position);
        frame.setFillColor(sf::Color::Transparent);
        frame.setOutlineThickness(3.f);
        frame.setOutlineColor(kMetalOutline);
        window.draw(frame);
    } else {
        sf::RectangleShape frame(boardBounds_.size);
        frame.setPosition(boardBounds_.position);
        frame.setFillColor(sf::Color(18, 17, 16, 235));
        frame.setOutlineThickness(4.f);
        frame.setOutlineColor(kMetalOutline);
        window.draw(frame);

        const auto& spaces = controller_.board().spaces();
        for (const auto& s : spaces) {
            auto fromIt = spacePositions_.find(s.id());
            if (fromIt == spacePositions_.end()) continue;
            for (int neighborId : s.adjacent()) {
                if (neighborId < s.id()) continue;
                auto toIt = spacePositions_.find(neighborId);
                if (toIt == spacePositions_.end()) continue;
                sf::Vertex line[] = {
                    sf::Vertex{fromIt->second, sf::Color(120, 110, 100, 140)},
                    sf::Vertex{toIt->second, sf::Color(120, 110, 100, 140)}
                };
                window.draw(line, 2, sf::PrimitiveType::Lines);
            }
        }

        for (const auto& s : spaces) {
            auto it = spacePositions_.find(s.id());
            if (it == spacePositions_.end()) continue;

            const auto& zones = s.zones();
            float radius = 13.f;

            if (zones.size() == 1) {
                sf::CircleShape circle(radius);
                circle.setOrigin(sf::Vector2f(radius, radius));
                circle.setPosition(it->second);
                circle.setFillColor(getZoneColor(zones[0]));
                circle.setOutlineThickness(2.f);
                circle.setOutlineColor(s.hasSecretPassage() ? sf::Color::Cyan : (s.startSlot() > 0 ? sf::Color::Yellow : sf::Color(200, 200, 200)));
                window.draw(circle);
            } else {
                sf::VertexArray fan(sf::PrimitiveType::TriangleFan);

                sf::Vertex centerVertex;
                centerVertex.position = it->second;
                centerVertex.color = sf::Color::White;
                fan.append(centerVertex);

                float angleStep = 360.f / static_cast<float>(zones.size());
                for (size_t z = 0; z < zones.size(); ++z) {
                    sf::Color zColor = getZoneColor(zones[z]);
                    for (float a = z * angleStep; a <= (z + 1) * angleStep + 0.1f; a += 5.f) {
                        float rad = a * 3.14159265f / 180.f;
                        sf::Vertex edgeVertex;
                        edgeVertex.position = it->second + sf::Vector2f(std::cos(rad) * radius, std::sin(rad) * radius);
                        edgeVertex.color = zColor;
                        fan.append(edgeVertex);
                    }
                }
                window.draw(fan);

                sf::CircleShape ring(radius);
                ring.setOrigin(sf::Vector2f(radius, radius));
                ring.setPosition(it->second);
                ring.setFillColor(sf::Color::Transparent);
                ring.setOutlineThickness(2.f);
                ring.setOutlineColor(s.hasSecretPassage() ? sf::Color::Cyan : (s.startSlot() > 0 ? sf::Color::Yellow : sf::Color(200, 200, 200)));
                window.draw(ring);
            }

            sf::Text number(app_.resources().getFont("assets/fonts/title_font.ttf"));
            number.setString(std::to_string(s.id()));
            number.setCharacterSize(10);
            number.setFillColor(sf::Color::White);
            number.setStyle(sf::Text::Bold);
            sf::FloatRect b = number.getLocalBounds();
            number.setOrigin(sf::Vector2f(b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f));
            number.setPosition(it->second + sf::Vector2f(0.f, -18.f));
            window.draw(number);
        }
    }

    for (int spaceId : highlightedSpaces_) {
        auto it = spacePositions_.find(spaceId);
        if (it == spacePositions_.end()) continue;
        sf::CircleShape glow(nodeRadius_ + 8.f);
        glow.setOrigin(sf::Vector2f(nodeRadius_ + 8.f, nodeRadius_ + 8.f));
        glow.setPosition(it->second);
        glow.setFillColor(sf::Color(230, 185, 55, 65));
        glow.setOutlineThickness(3.f);
        glow.setOutlineColor(sf::Color(255, 220, 110, 230));
        window.draw(glow);
    }

    renderFighterTokens(window);
}

void GameScreen::renderFighterTokens(sf::RenderWindow& window) {
    sf::Font& font = app_.resources().getFont("assets/fonts/title_font.ttf");
    const auto tokens = controller_.occupantTokens();
    for (const auto& [spaceId, label] : tokens) {
        auto posIt = spacePositions_.find(spaceId);
        if (posIt == spacePositions_.end()) continue;

        sf::CircleShape token(16.f);
        token.setOrigin(sf::Vector2f(16.f, 16.f));
        token.setPosition(posIt->second);
        token.setFillColor(sf::Color(25, 22, 20, 245));
        token.setOutlineThickness(2.f);
        token.setOutlineColor(kMetalOutline);
        window.draw(token);

        sf::Text text(font);
        text.setString(label);
        text.setCharacterSize(11);
        text.setFillColor(kTextLight);
        const sf::FloatRect b = text.getLocalBounds();
        text.setOrigin(sf::Vector2f(b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f));
        text.setPosition(posIt->second);
        window.draw(text);
    }

    for (const auto& player : controller_.players()) {
        for (const auto& fighterPtr : player.fighters()) {
            const Fighter& fighter = *fighterPtr;
            if (fighter.defeated() || fighter.spaceId() <= 0) continue;
            const bool selectable = isSelectableFighter(fighter.id());
            const bool activeFighter = !selectedFighterId_.empty() && selectedFighterId_ == fighter.id();
            if (!selectable && !activeFighter) continue;

            auto posIt = spacePositions_.find(fighter.spaceId());
            if (posIt == spacePositions_.end()) continue;

            sf::CircleShape ring(22.f);
            ring.setOrigin(sf::Vector2f(22.f, 22.f));
            ring.setPosition(posIt->second);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineThickness(3.f);
            ring.setOutlineColor(activeFighter ? sf::Color(255, 235, 130) : sf::Color(255, 210, 80, 220));
            window.draw(ring);
        }
    }
}

void GameScreen::renderFogMarkersOnBoard(sf::RenderWindow& window) {
    (void)window;
}

void GameScreen::renderSidePanel(sf::RenderWindow& window, int playerIndex, sf::Vector2f origin) {
    const Player& player = controller_.players()[static_cast<std::size_t>(playerIndex)];
    const Fighter& hero = player.heroFighter();
    bool active = controller_.currentPlayerIndex() == playerIndex;
    sf::Color accent = accentColorFor(hero);

    sf::RectangleShape panel(sf::Vector2f(kPanelW, kPanelH));
    panel.setPosition(origin);
    panel.setFillColor(kPanelFill);
    panel.setOutlineThickness(active ? 3.f : 2.f);
    panel.setOutlineColor(active ? accent : kMetalOutline);
    window.draw(panel);

    sf::RectangleShape portraitFrame(sf::Vector2f(kPanelW - 20.f, kPortraitH));
    portraitFrame.setPosition(sf::Vector2f(origin.x + 10.f, origin.y + 10.f));
    portraitFrame.setFillColor(sf::Color(10, 9, 8));
    portraitFrame.setOutlineThickness(2.f);
    portraitFrame.setOutlineColor(accent);
    window.draw(portraitFrame);

    sf::Texture* portrait = tryGetTexture(characterImagePath(hero));
    if (portrait) {
        sf::Sprite sprite(*portrait);
        sf::Vector2u size = portrait->getSize();
        float scale = std::min((kPanelW - 20.f) / size.x, kPortraitH / size.y);
        sprite.setScale(sf::Vector2f(scale, scale));
        sf::Vector2f spriteSize(size.x * scale, size.y * scale);
        sprite.setPosition(sf::Vector2f(
            origin.x + 10.f + (kPanelW - 20.f - spriteSize.x) / 2.f,
            origin.y + 10.f + (kPortraitH - spriteSize.y) / 2.f));
        window.draw(sprite);
    } else {
        sf::Text placeholder(app_.resources().getFont("assets/fonts/title_font.ttf"));
        placeholder.setString(hero.displayName());
        placeholder.setCharacterSize(18);
        placeholder.setFillColor(accent);
        sf::FloatRect b = placeholder.getLocalBounds();
        placeholder.setOrigin(sf::Vector2f(b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f));
        placeholder.setPosition(sf::Vector2f(origin.x + kPanelW / 2.f, origin.y + 10.f + kPortraitH / 2.f));
        window.draw(placeholder);
    }

    float infoY = origin.y + 10.f + kPortraitH + 8.f;
    std::ostringstream oss;
    oss << hero.displayName() << (hero.defeated() ? "  [DOWN]" : "") << "\n"
        << "HP " << hero.health() << " / " << hero.maxHealth() << "\n"
        << "Space " << (hero.spaceId() > 0 ? std::to_string(hero.spaceId()) : "-") << "   "
        << (hero.range() == AttackRange::Melee ? "Melee" : "Ranged") << "\n"
        << player.name() << ", age " << player.age()
        << "\nHand: " << player.hand().size()
        << "   Deck: " << player.deck().size()
        << "   Discard: " << player.discardPile().size();

    sf::Text infoText(app_.resources().getFont("assets/fonts/title_font.ttf"));
    infoText.setCharacterSize(12);
    infoText.setFillColor(kTextLight);
    infoText.setString(oss.str());
    infoText.setPosition(sf::Vector2f(origin.x + 10.f, infoY));
    window.draw(infoText);

    float sepY = infoY + 92.f;
    sf::RectangleShape sep(sf::Vector2f(kPanelW - 20.f, 2.f));
    sep.setPosition(sf::Vector2f(origin.x + 10.f, sepY));
    sep.setFillColor(sf::Color(70, 64, 58));
    window.draw(sep);

    float sideY = sepY + 10.f;
    if (const auto* invisible = dynamic_cast<const InvisibleMan*>(&hero)) {
        sf::RectangleShape fogBox(sf::Vector2f(kPanelW - 20.f, 70.f));
        fogBox.setPosition(sf::Vector2f(origin.x + 10.f, sideY));
        fogBox.setFillColor(sf::Color(24, 30, 44, 220));
        fogBox.setOutlineThickness(1.5f);
        fogBox.setOutlineColor(kMetalOutline);
        window.draw(fogBox);

        int placedCount = static_cast<int>(invisible->getFogSpaces().size());
        sf::Texture* fogTexture = tryGetTexture("assets/images/tokens/fog_token.png");
        float iconX = origin.x + 24.f;
        for (int i = 0; i < 3; ++i) {
            sf::Vector2f iconPos(iconX + static_cast<float>(i) * 34.f, sideY + 20.f);
            if (fogTexture) {
                sf::Sprite sprite(*fogTexture);
                sf::Vector2u size = fogTexture->getSize();
                float scale = 26.f / std::max(size.x, size.y);
                sprite.setScale(sf::Vector2f(scale, scale));
                sprite.setOrigin(sf::Vector2f(size.x / 2.f, size.y / 2.f));
                sprite.setPosition(iconPos);
                sprite.setColor(sf::Color(255, 255, 255, i < placedCount ? 255 : 90));
                window.draw(sprite);
            } else {
                sf::CircleShape marker(13.f);
                marker.setOrigin(sf::Vector2f(13.f, 13.f));
                marker.setPosition(iconPos);
                marker.setFillColor(i < placedCount ? sf::Color(150, 160, 200, 220) : sf::Color(70, 74, 90, 120));
                marker.setOutlineThickness(1.f);
                marker.setOutlineColor(sf::Color::White);
                window.draw(marker);
            }
        }
        sf::Text fogLabel(app_.resources().getFont("assets/fonts/title_font.ttf"));
        fogLabel.setString("Fog Tokens: " + std::to_string(placedCount) + "/3");
        fogLabel.setCharacterSize(11);
        fogLabel.setFillColor(kTextDim);
        fogLabel.setPosition(sf::Vector2f(origin.x + 14.f, sideY + 46.f));
        window.draw(fogLabel);
        sideY += 80.f;
    } else {
        for (const auto& f : player.fighters()) {
            if (f->isHero()) continue;
            sf::RectangleShape row(sf::Vector2f(kPanelW - 20.f, 40.f));
            row.setPosition(sf::Vector2f(origin.x + 10.f, sideY));
            row.setFillColor(sf::Color(26, 24, 22, 210));
            row.setOutlineThickness(1.f);
            row.setOutlineColor(f->defeated() ? sf::Color(70, 40, 40) : kMetalOutline);
            window.draw(row);

            sf::Texture* icon = tryGetTexture(characterImagePath(*f));
            float textX = origin.x + 16.f;
            if (icon) {
                sf::Sprite sprite(*icon);
                sf::Vector2u size = icon->getSize();
                float scale = 34.f / std::max(size.x, size.y);
                sprite.setScale(sf::Vector2f(scale, scale));
                sprite.setPosition(sf::Vector2f(origin.x + 14.f, sideY + 3.f));
                window.draw(sprite);
                textX += 40.f;
            }

            sf::Text row_text(app_.resources().getFont("assets/fonts/title_font.ttf"));
            std::ostringstream rowOss;
            rowOss << f->displayName() << "  HP " << f->health() << "/" << f->maxHealth();
            if (f->defeated()) rowOss << " (out)";
            row_text.setString(rowOss.str());
            row_text.setCharacterSize(11);
            row_text.setFillColor(f->defeated() ? kTextDim : kTextLight);
            row_text.setPosition(sf::Vector2f(textX, sideY + 11.f));
            window.draw(row_text);

            sideY += 46.f;
        }
    }

    renderCardBackIcon(window, playerIndex, sf::Vector2f(origin.x + (kPanelW - kCardBackW) / 2.f,
                                                          origin.y + kPanelH - kCardBackH - 10.f));

    {
        sf::Text discardLabel(app_.resources().getFont("assets/fonts/title_font.ttf"));
        discardLabel.setString("[ View Discard (" + std::to_string(player.discardPile().size()) + ") ]");
        discardLabel.setCharacterSize(11);
        discardLabel.setFillColor(sf::Color(200, 190, 150));
        sf::FloatRect labelBounds = discardLabel.getLocalBounds();
        sf::Vector2f labelPos(origin.x + (kPanelW - labelBounds.size.x) / 2.f,
                              origin.y + kPanelH - kCardBackH - 30.f);
        discardLabel.setPosition(labelPos);
        window.draw(discardLabel);

        sf::FloatRect clickBounds(labelPos - sf::Vector2f(6.f, 4.f),
                                  sf::Vector2f(labelBounds.size.x + 12.f, labelBounds.size.y + 12.f));
        if (playerIndex == 0) discardBoundsLeft_ = clickBounds;
        else discardBoundsRight_ = clickBounds;
    }
}

void GameScreen::renderCardBackIcon(sf::RenderWindow& window, int playerIndex, sf::Vector2f origin) {
    sf::FloatRect bounds(origin, sf::Vector2f(kCardBackW, kCardBackH));
    if (playerIndex == 0) cardBackBoundsLeft_ = bounds;
    else cardBackBoundsRight_ = bounds;

    bool hovering = (playerIndex == 0) ? hoveringCardBackLeft_ : hoveringCardBackRight_;
    const Player& player = controller_.players()[static_cast<std::size_t>(playerIndex)];
    const Fighter& hero = player.heroFighter();
    sf::Color accent = accentColorFor(hero);

    sf::RectangleShape back(sf::Vector2f(kCardBackW, kCardBackH));
    back.setPosition(origin);
    back.setFillColor(sf::Color(30, 27, 24, 235));
    back.setOutlineThickness(hovering ? 3.f : 2.f);
    back.setOutlineColor(hovering ? sf::Color(255, 220, 130) : accent);
    window.draw(back);

    sf::CircleShape emblem(kCardBackW * 0.28f);
    emblem.setOrigin(sf::Vector2f(emblem.getRadius(), emblem.getRadius()));
    emblem.setPosition(sf::Vector2f(origin.x + kCardBackW / 2.f, origin.y + kCardBackH / 2.f));
    emblem.setFillColor(sf::Color(0, 0, 0, 0));
    emblem.setOutlineThickness(2.f);
    emblem.setOutlineColor(accent);
    window.draw(emblem);

    sf::Text count(app_.resources().getFont("assets/fonts/title_font.ttf"));
    count.setString(std::to_string(player.hand().size()));
    count.setCharacterSize(13);
    count.setFillColor(kTextLight);
    sf::FloatRect b = count.getLocalBounds();
    count.setOrigin(sf::Vector2f(b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f));
    count.setPosition(sf::Vector2f(origin.x + kCardBackW / 2.f, origin.y + kCardBackH - 14.f));
    window.draw(count);
}

void GameScreen::renderHandOverlay(sf::RenderWindow& window, int playerIndex) {
    sf::RectangleShape dim(sf::Vector2f(static_cast<float>(app_.window().getSize().x),
                                        static_cast<float>(app_.window().getSize().y)));
    dim.setFillColor(sf::Color(0, 0, 0, 155));
    window.draw(dim);

    const Player& player = controller_.players()[static_cast<std::size_t>(playerIndex)];
    const auto& hand = player.hand();
    if (hand.empty()) return;

    const bool selecting = activeHandSelection_ && activeHandSelection_->playerIndex == playerIndex;
    float cardW = 118.f;
    float cardH = 172.f;
    float gap = 9.f;
    const int perRow = 7;
    const int count = static_cast<int>(hand.size());
    const int firstRowCount = std::min(count, perRow);
    const float rowWidth = firstRowCount * cardW + std::max(0, firstRowCount - 1) * gap;
    const float startX = (static_cast<float>(app_.window().getSize().x) - rowWidth) / 2.f;
    const float baseY = 394.f;

    sf::Text label(app_.resources().getFont("assets/fonts/title_font.ttf"));
    label.setString((selecting ? "SELECT: " : "HAND: ") + player.name());
    label.setCharacterSize(14);
    label.setFillColor(selecting ? sf::Color(255, 220, 120) : kTextLight);
    label.setPosition(sf::Vector2f(startX, baseY - 28.f));
    window.draw(label);

    for (std::size_t i = 0; i < hand.size(); ++i) {
        const int row = static_cast<int>(i) / perRow;
        const int col = static_cast<int>(i) % perRow;
        const int rowCount = std::min(perRow, count - row * perRow);
        const float thisRowWidth = rowCount * cardW + std::max(0, rowCount - 1) * gap;
        const float rowStartX = (static_cast<float>(app_.window().getSize().x) - thisRowWidth) / 2.f;
        const sf::Vector2f pos(rowStartX + col * (cardW + gap), baseY + row * (cardH + 12.f));
        const sf::FloatRect bounds(pos, sf::Vector2f(cardW, cardH));
        handCardViews_.push_back(HandCardView{playerIndex, static_cast<int>(i), bounds});

        bool legal = selecting && std::find(activeHandSelection_->legalIndices.begin(),
                                             activeHandSelection_->legalIndices.end(), static_cast<int>(i))
                                             != activeHandSelection_->legalIndices.end();
        bool hovered = hoveredCard_ && hoveredCard_->playerIndex == playerIndex && hoveredCard_->handIndex == static_cast<int>(i);

        sf::RectangleShape rect(sf::Vector2f(cardW, cardH));
        rect.setPosition(pos);
        rect.setFillColor(sf::Color(24, 22, 20, 250));
        rect.setOutlineThickness(legal ? 4.f : (hovered ? 3.f : 2.f));
        rect.setOutlineColor(legal ? sf::Color(255, 220, 100) : (hovered ? sf::Color(230, 200, 130) : kMetalOutline));
        window.draw(rect);

        const Card& card = hand[i];
        sf::Texture* art = tryGetTexture(cardImagePath(card));
        if (art) {
            sf::Sprite sprite(*art);
            const auto size = art->getSize();
            if (size.x > 0 && size.y > 0) {
                sprite.setScale(sf::Vector2f(cardW / static_cast<float>(size.x), cardH / static_cast<float>(size.y)));
                sprite.setPosition(pos);
                window.draw(sprite);
            }
        } else {
            sf::Text text(app_.resources().getFont("assets/fonts/title_font.ttf"));
            text.setCharacterSize(10);
            text.setFillColor(kTextLight);
            text.setString(card.getTitle() + "\n" + cardStatsLine(card));
            text.setPosition(pos + sf::Vector2f(6.f, 6.f));
            window.draw(text);
        }

        if (legal) {
            sf::Text pick(app_.resources().getFont("assets/fonts/title_font.ttf"));
            pick.setString("CLICK");
            pick.setCharacterSize(10);
            pick.setFillColor(sf::Color(255, 235, 150));
            pick.setPosition(pos + sf::Vector2f(6.f, cardH - 20.f));
            window.draw(pick);
        }
    }
}

void GameScreen::renderDiscardOverlay(sf::RenderWindow& window, int playerIndex) {
    sf::RectangleShape dim(sf::Vector2f(static_cast<float>(app_.window().getSize().x),
                                        static_cast<float>(app_.window().getSize().y)));
    dim.setFillColor(sf::Color(0, 0, 0, 155));
    window.draw(dim);

    const Player& player = controller_.players()[static_cast<std::size_t>(playerIndex)];
    const auto& pile = player.discardPile();

    sf::Text label(app_.resources().getFont("assets/fonts/title_font.ttf"));
    label.setString("DISCARD PILE: " + player.name() + " (" + std::to_string(pile.size()) + ")");
    label.setCharacterSize(14);
    label.setFillColor(kTextLight);

    float cardW = 118.f, cardH = 172.f, gap = 9.f;
    const int perRow = 7;
    const int count = static_cast<int>(pile.size());
    const int firstRowCount = std::max(1, std::min(count, perRow));
    const float rowWidth = firstRowCount * cardW + std::max(0, firstRowCount - 1) * gap;
    const float startX = (static_cast<float>(app_.window().getSize().x) - rowWidth) / 2.f;
    const float baseY = 394.f;
    label.setPosition(sf::Vector2f(startX, baseY - 28.f));
    window.draw(label);

    if (pile.empty()) {
        sf::Text empty(app_.resources().getFont("assets/fonts/title_font.ttf"));
        empty.setString("(empty)");
        empty.setCharacterSize(13);
        empty.setFillColor(kTextDim);
        empty.setPosition(sf::Vector2f(startX, baseY));
        window.draw(empty);
        return;
    }

    for (std::size_t i = 0; i < pile.size(); ++i) {
        const int row = static_cast<int>(i) / perRow;
        const int col = static_cast<int>(i) % perRow;
        const int rowCount = std::min(perRow, count - row * perRow);
        const float thisRowWidth = rowCount * cardW + std::max(0, rowCount - 1) * gap;
        const float rowStartX = (static_cast<float>(app_.window().getSize().x) - thisRowWidth) / 2.f;
        const sf::Vector2f pos(rowStartX + col * (cardW + gap), baseY + row * (cardH + 12.f));

        sf::RectangleShape rect(sf::Vector2f(cardW, cardH));
        rect.setPosition(pos);
        rect.setFillColor(sf::Color(24, 22, 20, 250));
        rect.setOutlineThickness(2.f);
        rect.setOutlineColor(kMetalOutline);
        window.draw(rect);

        const Card& card = pile[i];
        sf::Texture* art = tryGetTexture(cardImagePath(card));
        if (art) {
            sf::Sprite sprite(*art);
            const auto size = art->getSize();
            if (size.x > 0 && size.y > 0) {
                sprite.setScale(sf::Vector2f(cardW / static_cast<float>(size.x), cardH / static_cast<float>(size.y)));
                sprite.setPosition(pos);
                sprite.setColor(sf::Color(190, 190, 190));
                window.draw(sprite);
            }
        } else {
            sf::Text text(app_.resources().getFont("assets/fonts/title_font.ttf"));
            text.setCharacterSize(10);
            text.setFillColor(kTextDim);
            text.setString(card.getTitle() + "\n" + cardStatsLine(card));
            text.setPosition(pos + sf::Vector2f(6.f, 6.f));
            window.draw(text);
        }
    }
}

void GameScreen::renderControlRow(sf::RenderWindow& window) {
    if (chipListVertical_ && !chipButtons_.empty()) {
        sf::RectangleShape dim(boardBounds_.size);
        dim.setPosition(boardBounds_.position);
        dim.setFillColor(sf::Color(0, 0, 0, 165));
        window.draw(dim);
    }
    for (auto& b : actionButtons_) b.render(window);
    for (auto& b : chipButtons_) b.render(window);
}

void GameScreen::renderStatus(sf::RenderWindow& window) {
    window.draw(statusText_);
    window.draw(errorText_);
}

void GameScreen::renderGameOver(sf::RenderWindow& window) {
    gameOverText_.setString("WINNER: " + controller_.winnerName());
    sf::FloatRect bounds = gameOverText_.getLocalBounds();
    gameOverText_.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f));
    gameOverText_.setPosition(sf::Vector2f(static_cast<float>(app_.window().getSize().x) / 2.f, 320.f));
    window.draw(gameOverText_);
    if (backToMenuButton_) backToMenuButton_->render(window);
}

void GameScreen::renderInfoPopup(sf::RenderWindow& window) {
    sf::RectangleShape dim(sf::Vector2f(static_cast<float>(app_.window().getSize().x),
                                        static_cast<float>(app_.window().getSize().y)));
    dim.setFillColor(sf::Color(0, 0, 0, 160));
    window.draw(dim);

    float w = 560.f, h = 260.f;
    float x = (static_cast<float>(app_.window().getSize().x) - w) / 2.f;
    float y = (static_cast<float>(app_.window().getSize().y) - h) / 2.f;

    sf::RectangleShape box(sf::Vector2f(w, h));
    box.setPosition(sf::Vector2f(x, y));
    box.setFillColor(sf::Color(20, 18, 16, 250));
    box.setOutlineThickness(3.f);
    box.setOutlineColor(sf::Color(210, 170, 60));
    window.draw(box);

    sf::Text title(app_.resources().getFont("assets/fonts/title_font.ttf"));
    title.setString(infoPopupTitle_);
    title.setCharacterSize(18);
    title.setFillColor(sf::Color(210, 170, 60));
    title.setStyle(sf::Text::Bold);
    title.setPosition(sf::Vector2f(x + 20.f, y + 16.f));
    window.draw(title);

    sf::Text body(app_.resources().getFont("assets/fonts/title_font.ttf"));
    body.setString(infoPopupMessage_);
    body.setCharacterSize(12);
    body.setFillColor(kTextLight);
    body.setPosition(sf::Vector2f(x + 20.f, y + 60.f));
    window.draw(body);

    if (infoOkButton_) infoOkButton_->render(window);
}

void GameScreen::clearInteractiveWidgets() {
    chipButtons_.clear();
    chipListVertical_ = false;
    highlightedSpaces_.clear();
    selectableFighterIds_.clear();
    activeHandSelection_.reset();
}

void GameScreen::rebuildActionButtons() {
    actionButtons_.clear();
    sf::Font& font = app_.resources().getFont("assets/fonts/title_font.ttf");
    const sf::Color accent = accentColorFor(controller_.currentPlayer().heroFighter());
    float totalW = kBoardW;
    int count = 10 + (controller_.draculaAbilityAvailable() ? 1 : 0);
    float w = (totalW - static_cast<float>(count - 1) * 8.f) / static_cast<float>(count);
    float x = kBoardX, y = kControlRowY, h = kControlRowH, gap = 8.f;

    auto addButton = [&](const std::string& label, std::function<void()> onClick, bool highlight) {
        actionButtons_.emplace_back(font, label, sf::Vector2f(x, y), sf::Vector2f(w, h));
        actionButtons_.back().onClick = std::move(onClick);
        actionButtons_.back().accent = highlight ? accent : kMetalOutline;
        x += w + gap;
    };

    addButton("Maneuver", [this]() { onManeuverClicked(); }, true);
    addButton("Attack", [this]() { onAttackClicked(); }, true);
    addButton("Scheme", [this]() { onSchemeClicked(); }, true);
    if (controller_.draculaAbilityAvailable()) {
        addButton("Dracula", [this]() { onDraculaAbilityClicked(); }, true);
    }
    addButton("Discard", [this]() { onDiscardClicked(); }, false);
    addButton("Undo", [this]() { onUndoClicked(); }, false);
    addButton("Save", [this]() { onSaveClicked(); }, false);
    addButton("Load", [this]() { onLoadClicked(); }, false);
    addButton("Help", [this]() { onHelpClicked(); }, false);
    addButton("Menu", [this]() { onMainMenuClicked(); }, false);
    addButton("End Turn", [this]() { onEndTurnClicked(); }, false);
}

void GameScreen::rebuildChipButtons(const std::vector<std::string>& labels,
                                    std::function<void(int)> onSelect, bool vertical) {
    chipButtons_.clear();
    chipListVertical_ = vertical;
    sf::Font& font = app_.resources().getFont("assets/fonts/title_font.ttf");
    const float h = vertical ? 32.f : 34.f;

    if (vertical) {
        const float w = 480.f, gap = 6.f;
        const float x = boardBounds_.position.x + (boardBounds_.size.x - w) / 2.f;
        const float y = boardBounds_.position.y + 16.f;

        for (std::size_t i = 0; i < labels.size(); ++i) {
            float cy = y + static_cast<float>(i) * (h + gap);
            chipButtons_.emplace_back(font, labels[i], sf::Vector2f(x, cy), sf::Vector2f(w, h));
            int index = static_cast<int>(i);
            chipButtons_.back().onClick = [onSelect, index]() { onSelect(index); };
            chipButtons_.back().accent = sf::Color(155, 145, 130);
        }
    } else {
        float x = kBoardX + 100.f, y = kControlRowY, w = 118.f, gap = 7.f;
        int maxPerRow = 5;

        for (std::size_t i = 0; i < labels.size(); ++i) {
            float cx = x + static_cast<float>(i % maxPerRow) * (w + gap);
            float cy = y + static_cast<float>(i / maxPerRow) * (h + gap);
            chipButtons_.emplace_back(font, labels[i], sf::Vector2f(cx, cy), sf::Vector2f(w, h));
            int index = static_cast<int>(i);
            chipButtons_.back().onClick = [onSelect, index]() { onSelect(index); };
            chipButtons_.back().accent = sf::Color(155, 145, 130);
        }
    }
}

void GameScreen::enterMode(Mode mode) {
    mode_ = mode;
    clearInteractiveWidgets();
    actionButtons_.clear();
    backToMenuButton_.reset();
    infoOkButton_.reset();

    try {
    switch (mode_) {
        case Mode::Idle:
            rebuildActionButtons();
            setStatus("Choose an action.");
            break;

        case Mode::ManeuverSelectFighter: {
            std::vector<std::string> ids = controller_.movableCurrentFighterIds();
            selectableFighterIds_ = ids;
            std::vector<std::string> labels;
            int totalRemaining = 0;
            for (const auto& id : ids) totalRemaining += std::max(0, controller_.remainingMovementForFighter(id));
            if (maneuverBegun_) labels.push_back("Finish movement (" + std::to_string(totalRemaining) + " moves left)");
            for (auto& id : ids) labels.push_back(fighterLabel(id));
            if (labels.empty()) {
                try {
                    controller_.finishManeuver();
                    setStatus("Maneuver finished: no fighter had a legal move.");
                } catch (const std::exception& e) {
                    setError(e.what());
                }
                maneuverBegun_ = false;
                enterMode(Mode::Idle);
                return;
            }
            rebuildChipButtons(labels, [this, ids](int i) {
                if (i == 0) {
                    onFinishManeuverClicked();
                    return;
                }
                onManeuverFighterChosen(i - 1, ids);
            }, true);
            setStatus("Choose a fighter to move, or finish movement.");
            break;
        }

        case Mode::ManeuverSelectBoost: {
            auto boosts = controller_.legalBoostCardIndexes();
            const auto& hand = controller_.currentPlayer().hand();
            std::vector<std::string> labels;
            for (int idx : boosts) labels.push_back(cardListLabel(hand.at(static_cast<std::size_t>(idx))));
            rebuildChipButtons(labels, [this, boosts](int i) { onBoostCardChosen(boosts[static_cast<std::size_t>(i)]); }, true);

            sf::Font& font = app_.resources().getFont("assets/fonts/title_font.ttf");
            const float w = 480.f, h = 32.f, gap = 6.f;
            const float x = boardBounds_.position.x + (boardBounds_.size.x - w) / 2.f;
            const float skipY = boardBounds_.position.y + 16.f + static_cast<float>(labels.size()) * (h + gap) + 6.f;
            chipButtons_.emplace_back(font, "Skip Boost", sf::Vector2f(x, skipY), sf::Vector2f(w, h));
            chipButtons_.back().onClick = [this]() { onBoostCardChosen(-1); };
            setStatus("Choose a Boost card from the list, or Skip Boost.");
            break;
        }

        case Mode::ManeuverSelectDestination: {
            highlightedSpaces_ = controller_.reachableDestinationsFor(selectedFighterId_);
            sf::Font& font = app_.resources().getFont("assets/fonts/title_font.ttf");
            int remaining = std::max(0, controller_.remainingMovementForFighter(selectedFighterId_));
            chipButtons_.emplace_back(font, "Finish this fighter (" + std::to_string(remaining) + ")",
                                      sf::Vector2f(kBoardX + 60.f, kControlRowY),
                                      sf::Vector2f(210.f, kControlRowH));
            chipButtons_.back().onClick = [this]() {
                controller_.finishCurrentFighter(selectedFighterId_);
                auto next = controller_.movableCurrentFighterIds();
                if (next.empty()) onFinishManeuverClicked();
                else enterMode(Mode::ManeuverSelectFighter);
            };
            chipButtons_.emplace_back(font, "Finish Maneuver", sf::Vector2f(kBoardX + 282.f, kControlRowY),
                                      sf::Vector2f(150.f, kControlRowH));
            chipButtons_.back().onClick = [this]() { onFinishManeuverClicked(); };
            setStatus("Click a glowing destination, finish this fighter, or finish the maneuver.");
            break;
        }

        case Mode::AttackSelectAttacker: {
            auto ids = controller_.legalAttackers();
            std::vector<std::string> labels;
            for (auto& id : ids) labels.push_back(fighterLabel(id));
            if (labels.empty()) {
                setError("No legal attacker has both a target and an attack card.");
                enterMode(Mode::Idle);
                return;
            }
            rebuildChipButtons(labels, [this, ids](int i) { onAttackerChosen(i, ids); });
            setStatus("Select the attacking fighter.");
            break;
        }

        case Mode::AttackSelectTarget: {
            auto ids = controller_.legalTargetsFor(selectedFighterId_);
            std::vector<std::string> labels;
            for (auto& id : ids) labels.push_back(fighterLabel(id));
            if (labels.empty()) {
                setError("No target in range.");
                enterMode(Mode::Idle);
                return;
            }
            rebuildChipButtons(labels, [this, ids](int i) { onTargetChosen(i, ids); });
            setStatus("Select the target fighter.");
            break;
        }

        case Mode::AttackSelectCard: {
            auto indexes = controller_.legalAttackCardsFor(selectedFighterId_);
            if (indexes.empty()) {
                setError("No legal attack card in hand.");
                enterMode(Mode::Idle);
                return;
            }
            const auto& hand = controller_.currentPlayer().hand();
            std::vector<std::string> labels;
            for (int idx : indexes) labels.push_back(cardListLabel(hand.at(static_cast<std::size_t>(idx))));
            rebuildChipButtons(labels, [this, indexes](int i) { onAttackCardChosen(indexes[static_cast<std::size_t>(i)]); }, true);
            setStatus("Choose an Attack card from the list.");
            break;
        }

        case Mode::AttackSelectDefenseCard: {
            auto indexes = controller_.legalDefenseCardsFor(selectedTargetId_);
            const Player* defenderOwner = controller_.ownerOfFighter(selectedTargetId_);
            if (!defenderOwner) {
                setError("Could not find defender owner.");
                enterMode(Mode::Idle);
                return;
            }
            const auto& hand = defenderOwner->hand();
            std::vector<std::string> labels;
            for (int idx : indexes) labels.push_back(cardListLabel(hand.at(static_cast<std::size_t>(idx))));
            rebuildChipButtons(labels, [this, indexes](int i) { onDefenseCardChosen(indexes[static_cast<std::size_t>(i)]); }, true);

            sf::Font& font = app_.resources().getFont("assets/fonts/title_font.ttf");
            const float w = 480.f, h = 32.f, gap = 6.f;
            const float x = boardBounds_.position.x + (boardBounds_.size.x - w) / 2.f;
            const float skipY = boardBounds_.position.y + 16.f + static_cast<float>(labels.size()) * (h + gap) + 6.f;
            chipButtons_.emplace_back(font, "No Defense", sf::Vector2f(x, skipY), sf::Vector2f(w, h));
            chipButtons_.back().onClick = [this]() { onDefenseCardChosen(-1); };
            setStatus(defenderOwner->name() + ": choose a Defense card, or No Defense.");
            break;
        }

        case Mode::AttackBeastBoost: {
            const auto& hand = controller_.currentPlayer().hand();
            std::vector<int> indexes;
            std::vector<std::string> labels;
            labels.push_back("Done choosing Beastform discards (+" +
                             std::to_string(selectedBeastFormBoostIndexes_.size()) + " attack)");
            for (int i = 0; i < static_cast<int>(hand.size()); ++i) {
                if (i == selectedAttackCardIndex_) continue;
                if (std::find(selectedBeastFormBoostIndexes_.begin(), selectedBeastFormBoostIndexes_.end(), i) !=
                    selectedBeastFormBoostIndexes_.end()) continue;
                indexes.push_back(i);
                labels.push_back(cardListLabel(hand.at(static_cast<std::size_t>(i))));
            }
            rebuildChipButtons(labels, [this, indexes](int i) {
                if (i == 0) {
                    onBeastBoostDone();
                } else {
                    onBeastBoostChosen(indexes.at(static_cast<std::size_t>(i - 1)));
                }
            }, true);
            setStatus("BEASTFORM: discard any number of other cards for +1 attack each.");
            break;
        }

        case Mode::AttackElementaryPrediction: {
            std::vector<std::string> labels;
            for (int value = 0; value <= 6; ++value) labels.push_back("Predict " + std::to_string(value));
            rebuildChipButtons(labels, [this](int i) { onElementaryPredictionChosen(i); });
            setStatus("ELEMENTARY: predict the attack value.");
            break;
        }

        case Mode::SchemeSelectCard: {
            auto indexes = controller_.legalSchemeCards();
            if (indexes.empty()) {
                setError("No legal scheme card in hand.");
                enterMode(Mode::Idle);
                return;
            }
            const auto& hand = controller_.currentPlayer().hand();
            std::vector<std::string> labels;
            for (int idx : indexes) labels.push_back(cardListLabel(hand.at(static_cast<std::size_t>(idx))));
            rebuildChipButtons(labels, [this, indexes](int i) { onSchemeCardChosen(indexes[static_cast<std::size_t>(i)]); }, true);
            setStatus("Choose a Scheme card from the list.");
            break;
        }

        case Mode::SchemeSelectTarget: {
            auto ids = controller_.targetChoicesForScheme(pendingSchemeHandIndex_);
            std::vector<std::string> labels;
            for (auto& id : ids) labels.push_back(fighterLabel(id));
            rebuildChipButtons(labels, [this, ids](int i) { onSchemeTargetChosen(i, ids); });
            setStatus("Select the scheme target.");
            break;
        }

        case Mode::SchemeSelectDestination: {
            highlightedSpaces_ = controller_.destinationChoicesForScheme(pendingSchemeHandIndex_, pendingSchemeChoice_);
            setStatus("Click the scheme's destination space on the board.");
            break;
        }

        case Mode::SchemeSelectNamedValue: {
            auto values = controller_.namedValueChoicesForScheme(pendingSchemeHandIndex_);
            std::vector<std::string> labels;
            for (int v : values) labels.push_back(std::to_string(v));
            rebuildChipButtons(labels, [this, values](int i) { onSchemeNamedValueChosen(i, values); });
            setStatus("Choose a value for this scheme.");
            break;
        }

        case Mode::SchemeSelectOpponentCard: {
            auto indexes = controller_.opponentHandChoicesForScheme(pendingSchemeHandIndex_);
            std::vector<std::string> labels;
            for (std::size_t i = 0; i < indexes.size(); ++i) labels.push_back("Card #" + std::to_string(i + 1));
            rebuildChipButtons(labels, [this, indexes](int i) { onSchemeOpponentCardChosen(i, indexes); });
            setStatus("Choose one of the opponent's hand cards.");
            break;
        }

        case Mode::SchemeStepLightlyTarget: {
            std::vector<std::string> ids;
            const Fighter& invisible = controller_.currentPlayer().heroFighter();
            for (const auto& fighter : controller_.opponentPlayer().fighters()) {
                if (!fighter->defeated() &&
                    controller_.board().areAdjacentForCombat(invisible.spaceId(), fighter->spaceId())) {
                    ids.push_back(fighter->id());
                }
            }
            if (ids.empty()) {
                try {
                    Card played = controller_.currentPlayer().removeCardFromHand(pendingSchemeHandIndex_);
                    controller_.currentPlayer().addToDiscard(std::move(played));
                    controller_.decrementActions();
                    controller_.endTurnIfNeeded();
                    setError("No adjacent enemy fighters.");
                } catch (const std::exception& e) {
                    setError(e.what());
                }
                pendingSchemeHandIndex_ = -1;
                enterMode(Mode::Idle);
                return;
            }
            std::vector<std::string> labels;
            for (auto& id : ids) labels.push_back(fighterLabel(id));
            rebuildChipButtons(labels, [this, ids](int i) { onSchemeStepLightlyTargetChosen(i, ids); });
            setStatus("STEP LIGHTLY: choose an enemy fighter to damage.");
            break;
        }

        case Mode::RaveningTarget: {
            auto ids = controller_.getRaveningTargets();
            std::vector<std::string> labels;
            labels.push_back("Done moving fighters");
            for (auto& id : ids) labels.push_back(fighterLabel(id));
            rebuildChipButtons(labels, [this, ids](int i) {
                if (i == 0) onRaveningFinish();
                else onRaveningTargetChosen(i - 1, ids);
            }, true);
            setStatus("RAVENING SEDUCTION: choose a fighter to move up to 2 spaces, or finish.");
            break;
        }

        case Mode::RaveningDestination: {
            highlightedSpaces_ = controller_.getRaveningDestinations(selectedRaveningFighterId_);
            setStatus("RAVENING SEDUCTION: click a destination for " + fighterLabel(selectedRaveningFighterId_) + ".");
            break;
        }

        case Mode::RaveningContinue: {
            sf::Font& font = app_.resources().getFont("assets/fonts/title_font.ttf");
            chipButtons_.emplace_back(font, "Move another fighter", sf::Vector2f(kBoardX + 100.f, kControlRowY),
                                      sf::Vector2f(190.f, kControlRowH));
            chipButtons_.back().onClick = [this]() { onRaveningContinue(); };
            chipButtons_.emplace_back(font, "Finish Ravening", sf::Vector2f(kBoardX + 305.f, kControlRowY),
                                      sf::Vector2f(170.f, kControlRowH));
            chipButtons_.back().onClick = [this]() { onRaveningFinish(); };
            setStatus("RAVENING SEDUCTION: move another fighter or finish.");
            break;
        }

        case Mode::ConfoundYesNo: {
            sf::Font& font = app_.resources().getFont("assets/fonts/title_font.ttf");
            const auto& pending = controller_.pendingConfoundChoice();
            const std::string& askedName = controller_.players()[static_cast<std::size_t>(pending.playerIndex)].name();
            chipButtons_.emplace_back(font, "Yes, discard", sf::Vector2f(kBoardX + 100.f, kControlRowY),
                                      sf::Vector2f(150.f, kControlRowH));
            chipButtons_.back().onClick = [this]() { onConfoundYes(); };
            chipButtons_.emplace_back(font, "No, move fog", sf::Vector2f(kBoardX + 260.f, kControlRowY),
                                      sf::Vector2f(150.f, kControlRowH));
            chipButtons_.back().onClick = [this]() { onConfoundNo(); };
            setStatus("CONFOUND: " + askedName + " must choose -- discard a card, or move a fog token.");
            break;
        }

        case Mode::ConfoundDiscardCard: {
            const auto& pending = controller_.pendingConfoundChoice();
            const auto& hand = controller_.players()[static_cast<std::size_t>(pending.playerIndex)].hand();
            std::vector<std::string> labels;
            for (const auto& card : hand) labels.push_back(cardListLabel(card));
            rebuildChipButtons(labels, [this](int i) { onConfoundCardChosen(i); }, true);
            setStatus("CONFOUND: choose one card from your hand to discard.");
            break;
        }

        case Mode::ConfoundFogSelect: {
            std::vector<int> options;
            std::vector<std::string> labels;
            for (int i = 0; i < 3; ++i) {
                if (!controller_.getValidConfoundDestinations(i).empty()) {
                    options.push_back(i);
                    labels.push_back("Fog Token " + std::to_string(i + 1));
                }
            }
            if (labels.empty()) {
                setError("No movable fog token is available.");
                enterMode(Mode::Idle);
                return;
            }
            rebuildChipButtons(labels, [this, options](int i) { onConfoundFogTokenChosen(options[static_cast<std::size_t>(i)]); });
            setStatus("CONFOUND: choose which fog token to move.");
            break;
        }

        case Mode::ConfoundFogDestination: {
            highlightedSpaces_ = controller_.getValidConfoundDestinations(pendingConfoundFogIndex_);
            setStatus("CONFOUND: click a destination for the fog token.");
            break;
        }

        case Mode::CodedNotesSelectCards: {
            const auto& pending = controller_.pendingCodedNotes();
            const auto& hand = controller_.players()[static_cast<std::size_t>(pending.playerIndex)].hand();
            std::vector<std::string> labels;
            for (std::size_t i = 0; i < hand.size(); ++i) {
                bool selected = std::find(codedNotesSelection_.begin(), codedNotesSelection_.end(),
                                          static_cast<int>(i)) != codedNotesSelection_.end();
                labels.push_back((selected ? "[X] " : "[ ] ") + cardListLabel(hand[i]));
            }
            rebuildChipButtons(labels, [this](int i) { onCodedNotesToggleCard(i); }, true);

            sf::Font& font = app_.resources().getFont("assets/fonts/title_font.ttf");
            const float w = 480.f, h = 32.f, gap = 6.f;
            const float x = boardBounds_.position.x + (boardBounds_.size.x - w) / 2.f;
            const float confirmY = boardBounds_.position.y + 16.f + static_cast<float>(labels.size()) * (h + gap) + 6.f;
            chipButtons_.emplace_back(font, "Confirm (" + std::to_string(codedNotesSelection_.size()) + "/2)",
                                      sf::Vector2f(x, confirmY), sf::Vector2f(w, h));
            if (codedNotesSelection_.size() == 2) {
                chipButtons_.back().onClick = [this]() { onCodedNotesConfirm(); };
            }
            setStatus("CODED NOTES: pick exactly 2 cards from your hand to put on top of your deck.");
            break;
        }

        case Mode::LurkingChoice: {
            sf::Font& font = app_.resources().getFont("assets/fonts/title_font.ttf");
            chipButtons_.emplace_back(font, "Teleport to a Fog space", sf::Vector2f(kBoardX + 60.f, kControlRowY),
                                      sf::Vector2f(220.f, kControlRowH));
            chipButtons_.back().onClick = [this]() { onLurkingChoiceChosen(0); };
            chipButtons_.emplace_back(font, "Move a Fog token", sf::Vector2f(kBoardX + 290.f, kControlRowY),
                                      sf::Vector2f(220.f, kControlRowH));
            chipButtons_.back().onClick = [this]() { onLurkingChoiceChosen(1); };
            
            chipButtons_.emplace_back(font, "Skip", sf::Vector2f(kBoardX + 530.f, kControlRowY), sf::Vector2f(100.f, kControlRowH));
            chipButtons_.back().onClick = [this]() { onLurkingSkip(); };
            chipButtons_.back().accent = sf::Color(130, 125, 120);

            setStatus("LURKING: teleport Invisible Man to a Fog space, or move a Fog token.");
            break;
        }

        case Mode::LurkingFogToken: {
            auto tokens = controller_.getLurkingFogTokens();
            std::vector<std::string> labels;
            for (int token : tokens) labels.push_back("Fog Token " + std::to_string(token + 1));
            if (labels.empty()) {
                setError("No fog token is available.");
                enterMode(Mode::Idle);
                return;
            }
            rebuildChipButtons(labels, [this, tokens](int i) { onLurkingFogTokenChosen(tokens[static_cast<std::size_t>(i)]); });
            setStatus("LURKING: choose which fog token to move.");
            break;
        }

        case Mode::LurkingDestination: {
            const auto& pending = controller_.pendingLurking();
            if (pending.choice == 1) {
                highlightedSpaces_ = controller_.getLurkingDestinations(pending.selectedFogIndex);
                setStatus("LURKING: click a destination for the fog token.");
            } else {
                highlightedSpaces_ = controller_.getLurkingFogPositions();
                setStatus("LURKING: click a Fog space to teleport Invisible Man there.");
            }
            break;
        }

        case Mode::SlipAwayFogToken: {
            auto tokens = controller_.getSlipAwayFogTokens();
            std::vector<std::string> labels;
            for (int token : tokens) labels.push_back("Fog Token " + std::to_string(token + 1));
            if (labels.empty()) {
                setError("No fog token is available.");
                enterMode(Mode::Idle);
                return;
            }
            sf::Font& font = app_.resources().getFont("assets/fonts/title_font.ttf");
            rebuildChipButtons(labels, [this, tokens](int i) { onSlipAwayFogTokenChosen(tokens[static_cast<std::size_t>(i)]); });
            
            float xOffset = kBoardX + 100.f + labels.size() * 125.f;
            chipButtons_.emplace_back(font, "Skip", sf::Vector2f(xOffset, kControlRowY), sf::Vector2f(90.f, kControlRowH));
            chipButtons_.back().onClick = [this]() { onSlipAwaySkip(); };
            chipButtons_.back().accent = sf::Color(130, 125, 120);

            setStatus("SLIP AWAY: choose a fog token to teleport Invisible Man to.");
            break;
        }

        case Mode::SlipAwayDestination: {
            highlightedSpaces_ = controller_.getSlipAwayDestinations();
            setStatus("SLIP AWAY: click the Fog space to teleport to.");
            break;
        }

        case Mode::RollingFogFogToken: {
            auto tokens = controller_.getRollingFogTokens();
            std::vector<std::string> labels;
            for (int token : tokens) labels.push_back("Fog Token " + std::to_string(token + 1));
            if (labels.empty()) {
                setError("No fog token is available.");
                enterMode(Mode::Idle);
                return;
            }
            sf::Font& font = app_.resources().getFont("assets/fonts/title_font.ttf");
            rebuildChipButtons(labels, [this, tokens](int i) { onRollingFogFogTokenChosen(tokens[static_cast<std::size_t>(i)]); });
            
            float xOffset = kBoardX + 100.f + labels.size() * 125.f;
            chipButtons_.emplace_back(font, "Skip", sf::Vector2f(xOffset, kControlRowY), sf::Vector2f(90.f, kControlRowH));
            chipButtons_.back().onClick = [this]() { onRollingFogSkip(); };
            chipButtons_.back().accent = sf::Color(130, 125, 120);

            setStatus("ROLLING FOG: choose which fog token to move.");
            break;
        }

        case Mode::RollingFogDestination: {
            const auto& pending = controller_.pendingRollingFog();
            highlightedSpaces_ = controller_.getRollingFogDestinations(pending.selectedFogIndex);
            setStatus("ROLLING FOG: click a destination for the fog token.");
            break;
        }

        case Mode::FogTokenSelect: {
            auto options = controller_.pendingFogChoices();
            std::vector<std::string> labels;
            for (int token : options) labels.push_back("Fog Token " + std::to_string(token + 1));

            if (labels.empty()) {
                setError("No movable fog token is available.");
                enterMode(Mode::Idle);
                return;
            }

            rebuildChipButtons(labels, [this, options](int i) { onFogTokenChosen(options[static_cast<std::size_t>(i)]); });

            const auto& pending = controller_.pendingFogChoice();
            const std::string& chooserName = controller_.players()[static_cast<std::size_t>(pending.chooserPlayerIndex)].name();

            setStatus(chooserName + ": Choose which Fog token to move.");
            break;
        }

        case Mode::FogTokenDestination: {
            highlightedSpaces_ = controller_.getReachableFogDestinations(pendingFogTokenIndex_);

            const auto& pending = controller_.pendingFogChoice();
            const std::string& chooserName = controller_.players()[static_cast<std::size_t>(pending.chooserPlayerIndex)].name();

            setStatus(chooserName + ": Click a glowing destination for the selected Fog token.");
            break;
        }

        case Mode::VanishedPlacement: {
            highlightedSpaces_ = controller_.getValidPlacementSpacesForVanished();
            setStatus("VANISH: click an empty space on the board to reappear.");
            break;
        }

        case Mode::InfoPopup: {
            sf::Font& font = app_.resources().getFont("assets/fonts/title_font.ttf");
            infoOkButton_.emplace(font, "OK",
                sf::Vector2f(static_cast<float>(app_.window().getSize().x) / 2.f - 60.f, 260.f),
                sf::Vector2f(120.f, 40.f));
            infoOkButton_->onClick = [this]() { onInfoPopupAcknowledged(); };
            break;
        }

        case Mode::DraculaAbilityTarget: {
            std::vector<std::string> ids;
            const Fighter& dracula = controller_.currentPlayer().heroFighter();
            for (const auto& player : controller_.players()) {
                for (const auto& fighter : player.fighters()) {
                    if (!fighter->defeated() && fighter->id() != dracula.id() &&
                        controller_.board().areAdjacentForCombat(dracula.spaceId(), fighter->spaceId())) {
                        ids.push_back(fighter->id());
                    }
                }
            }
            if (ids.empty()) {
                setError("No valid target adjacent to Dracula.");
                enterMode(Mode::Idle);
                return;
            }
            std::vector<std::string> labels;
            for (auto& id : ids) labels.push_back(fighterLabel(id));
            rebuildChipButtons(labels, [this, ids](int i) { onDraculaAbilityTargetChosen(i, ids); });
            setStatus("Dracula's ability: choose an adjacent fighter to damage.");
            break;
        }

        case Mode::DiscardSelectCard: {
            const auto& hand = controller_.currentPlayer().hand();
            std::vector<std::string> labels;
            for (const auto& card : hand) labels.push_back(cardListLabel(card));
            rebuildChipButtons(labels, [this](int i) { onDiscardCardChosen(i); }, true);
            setStatus("Choose a card to discard.");
            break;
        }

        case Mode::OptionalMovementDestination: {
            highlightedSpaces_ = controller_.pendingOptionalMovementDestinations();
            sf::Font& font = app_.resources().getFont("assets/fonts/title_font.ttf");
            chipButtons_.emplace_back(font, "Skip Movement", sf::Vector2f(kBoardX + 100.f, kControlRowY),
                                      sf::Vector2f(150.f, kControlRowH));
            chipButtons_.back().onClick = [this]() { onSkipOptionalMovementClicked(); };
            setStatus("Optional movement: click a highlighted space, or skip.");
            break;
        }

        case Mode::LoadGame: {
            auto slots = controller_.getSaveSlots();
            std::vector<std::string> labels;
            std::vector<int> slotNumbers;
            for (const auto& slot : slots) {
                slotNumbers.push_back(slot.first);
                labels.push_back(slot.second);
            }
            if (labels.empty()) {
                setError("No save files found.");
                enterMode(Mode::Idle);
                return;
            }
            rebuildChipButtons(labels, [this, slotNumbers](int i) {
                onLoadSlotChosen(slotNumbers.at(static_cast<std::size_t>(i)));
            }, true);
            setStatus("Choose a save slot to load.");
            break;
        }

        case Mode::GameOver: {
            sf::Font& font = app_.resources().getFont("assets/fonts/title_font.ttf");
            backToMenuButton_.emplace(font, "Back to Menu",
                sf::Vector2f(static_cast<float>(app_.window().getSize().x) / 2.f - 100.f, 420.f),
                sf::Vector2f(200.f, 50.f));
            backToMenuButton_->onClick = [this]() { app_.setScreen(std::make_unique<MainMenuScreen>(app_)); };
            break;
        }
    }
    } catch (const std::exception& e) {

        mode_ = Mode::Idle;
        clearInteractiveWidgets();
        actionButtons_.clear();
        rebuildActionButtons();
        setError(std::string("UI error: ") + e.what());
    } catch (...) {
        mode_ = Mode::Idle;
        clearInteractiveWidgets();
        actionButtons_.clear();
        rebuildActionButtons();
        setError("UI error: unknown exception while switching mode.");
    }
}

void GameScreen::onManeuverClicked() {
    maneuverBegun_ = false;
    selectedFighterId_.clear();
    enterMode(Mode::ManeuverSelectBoost);
}
void GameScreen::onAttackClicked() {
    if (controller_.legalAttackers().empty()) {
        setError("No legal attacker has both a target and an attack card.");
        return;
    }
    enterMode(Mode::AttackSelectAttacker);
}

void GameScreen::onSchemeClicked() {
    if (controller_.legalSchemeCards().empty()) {
        setError("No legal scheme card is available.");
        return;
    }
    enterMode(Mode::SchemeSelectCard);
}
void GameScreen::onDiscardClicked() { enterMode(Mode::DiscardSelectCard); }
void GameScreen::onDraculaAbilityClicked() { enterMode(Mode::DraculaAbilityTarget); }

void GameScreen::onEndTurnClicked() {

    if (controller_.actionsRemaining() > 0) {
        controller_.addAction(-controller_.actionsRemaining());
    }
    controller_.endTurnIfNeeded();
    setStatus("Turn ended.");
    enterMode(Mode::Idle);
}

void GameScreen::onUndoClicked() {
    if (!controller_.canUndo()) {
        setError("Nothing to undo.");
        return;
    }
    try {
        controller_.undoLastAction();
        setStatus("Last action undone.");
    } catch (const std::exception& e) {
        setError(e.what());
    }
    enterMode(Mode::Idle);
}

void GameScreen::onSaveClicked() {
    try {
        controller_.saveGame();
        setStatus("Game saved.");
    } catch (const std::exception& e) {
        setError(e.what());
    }
}

void GameScreen::onBoostCardChosen(int handIndex) {
    pendingBoostIndex_ = handIndex;
    try {
        controller_.beginManeuver(pendingBoostIndex_);
    } catch (const std::exception& e) {
        setError(e.what());
        maneuverBegun_ = false;
        enterMode(Mode::Idle);
        return;
    }
    maneuverBegun_ = true;

    enterMode(Mode::ManeuverSelectFighter);
}

void GameScreen::onLoadClicked() {
    enterMode(Mode::LoadGame);
}

void GameScreen::onLoadSlotChosen(int slot) {
    try {
        controller_.loadGame(slot);
        maneuverBegun_ = false;
        selectedFighterId_.clear();
        selectedTargetId_.clear();
        selectedRaveningFighterId_.clear();
        selectedBeastFormBoostIndexes_.clear();
        setStatus("Saved game loaded.");
    } catch (const std::exception& e) {
        setError(e.what());
    }
    enterMode(Mode::Idle);
}

void GameScreen::onHelpClicked() {
    infoPopupTitle_ = "HELP";
    infoPopupMessage_ =
        "Actions: Maneuver, Attack, Scheme.\n"
        "Maneuver draws 1 card, then movement may be boosted.\n"
        "Attack needs a legal attacker, target, and attack card.\n"
        "Schemes resolve their card effect immediately.\n"
        "Shortcuts: S Save, L Load, Z Undo.";
    enterMode(Mode::InfoPopup);
}

void GameScreen::onMainMenuClicked() {
    app_.setScreen(std::make_unique<MainMenuScreen>(app_));
}

void GameScreen::onManeuverFighterChosen(int index, const std::vector<std::string>& ids) {
    if (index < 0 || static_cast<std::size_t>(index) >= ids.size()) return;
    selectedFighterId_ = ids[static_cast<std::size_t>(index)];
    if (!maneuverBegun_) {

        enterMode(Mode::ManeuverSelectBoost);
    } else {

        enterMode(Mode::ManeuverSelectDestination);
    }
}

void GameScreen::onDestinationSpaceClicked(int spaceId) {
    try {
        controller_.moveCurrentFighter(selectedFighterId_, spaceId);
        setStatus(fighterLabel(selectedFighterId_) + " moved.");
    } catch (const std::exception& e) {
        setError(e.what());
        return;
    }

    if (controller_.remainingMovementForFighter(selectedFighterId_) > 0) {
        highlightedSpaces_ = controller_.reachableDestinationsFor(selectedFighterId_);
        const Fighter* fighter = controller_.findFighterById(selectedFighterId_);
        bool hasOther = false;
        if (fighter) {
            for (int destination : highlightedSpaces_) {
                if (destination != fighter->spaceId() &&
                    controller_.getMovementCost(selectedFighterId_, destination) > 0) {
                    hasOther = true;
                    break;
                }
            }
        }
        if (hasOther) {
            enterMode(Mode::ManeuverSelectDestination);
            return;
        }
    }

    controller_.finishCurrentFighter(selectedFighterId_);
    const auto next = controller_.movableCurrentFighterIds();
    if (next.empty()) onFinishManeuverClicked();
    else enterMode(Mode::ManeuverSelectFighter);
}

void GameScreen::onFinishManeuverClicked() {
    try {
        controller_.finishManeuver();
    } catch (const std::exception& e) {
        setError(e.what());
    }
    maneuverBegun_ = false;
    enterMode(Mode::Idle);
}

void GameScreen::onAttackerChosen(int index, const std::vector<std::string>& ids) {
    selectedFighterId_ = ids[static_cast<std::size_t>(index)];
    enterMode(Mode::AttackSelectTarget);
}

void GameScreen::onTargetChosen(int index, const std::vector<std::string>& ids) {
    selectedTargetId_ = ids[static_cast<std::size_t>(index)];
    enterMode(Mode::AttackSelectCard);
}

void GameScreen::onAttackCardChosen(int handIndex) {
    selectedAttackCardIndex_ = handIndex;
    selectedBeastFormBoostIndexes_.clear();
    const Card& attackCard = controller_.currentPlayer().hand().at(static_cast<std::size_t>(handIndex));
    if (attackCard.getTitle() == "BEASTFORM") {
        enterMode(Mode::AttackBeastBoost);
        return;
    }
    enterMode(Mode::AttackSelectDefenseCard);
}

void GameScreen::onBeastBoostChosen(int handIndex) {
    if (std::find(selectedBeastFormBoostIndexes_.begin(), selectedBeastFormBoostIndexes_.end(), handIndex) ==
        selectedBeastFormBoostIndexes_.end()) {
        selectedBeastFormBoostIndexes_.push_back(handIndex);
    }
    enterMode(Mode::AttackBeastBoost);
}

void GameScreen::onBeastBoostDone() {
    enterMode(Mode::AttackSelectDefenseCard);
}

void GameScreen::onDefenseCardChosen(int handIndex) {
    if (handIndex != -1) {
        const Player* defenderOwner = controller_.ownerOfFighter(selectedTargetId_);
        if (defenderOwner && defenderOwner->hand().at(static_cast<std::size_t>(handIndex)).getTitle() == "ELEMENTARY") {
            selectedDefenseCardIndex_ = handIndex;
            enterMode(Mode::AttackElementaryPrediction);
            return;
        }
    }
    try {
        controller_.resolveAttack(selectedFighterId_, selectedTargetId_, selectedAttackCardIndex_, handIndex,
                                  selectedBeastFormBoostIndexes_, -1);
        setStatus("Attack resolved.");
    } catch (const std::exception& e) {
        setError(e.what());
    }
    selectedBeastFormBoostIndexes_.clear();
    selectedDefenseCardIndex_ = -1;
    enterMode(Mode::Idle);
}

void GameScreen::onElementaryPredictionChosen(int value) {
    try {
        controller_.resolveAttack(selectedFighterId_, selectedTargetId_, selectedAttackCardIndex_,
                                  selectedDefenseCardIndex_, selectedBeastFormBoostIndexes_, value);
        setStatus("Attack resolved.");
    } catch (const std::exception& e) {
        setError(e.what());
    }
    selectedBeastFormBoostIndexes_.clear();
    selectedDefenseCardIndex_ = -1;
    enterMode(Mode::Idle);
}

void GameScreen::onDraculaAbilityTargetChosen(int index, const std::vector<std::string>& ids) {
    try {
        controller_.useDraculaStartAbility(ids[static_cast<std::size_t>(index)]);
        setStatus("Dracula's ability used.");
    } catch (const std::exception& e) {
        setError(e.what());
    }
    enterMode(Mode::Idle);
}

void GameScreen::onSchemeCardChosen(int handIndex) {
    pendingSchemeHandIndex_ = handIndex;
    pendingSchemeChoice_ = SchemeChoice{};

    const Card& card = controller_.currentPlayer().hand().at(static_cast<std::size_t>(handIndex));

    if (card.getTitle() == "CONFIRM SUSPICION") {
        enterMode(Mode::SchemeSelectNamedValue);
        return;
    }

    if (card.getTitle() == "RAVENING SEDUCTION") {
        try {
            controller_.playScheme(pendingSchemeHandIndex_, pendingSchemeChoice_);
            setStatus("Ravening Seduction: choose fighters to move.");
        } catch (const std::exception& e) {
            setError(e.what());
            pendingSchemeHandIndex_ = -1;
            enterMode(Mode::Idle);
            return;
        }
        pendingSchemeHandIndex_ = -1;
        enterMode(Mode::RaveningTarget);
        return;
    }

    if (card.getTitle() == "VANISH") {
        try {
            controller_.handleVanish(pendingSchemeHandIndex_);
            setStatus("Vanish: choose an empty space to reappear.");
        } catch (const std::exception& e) {
            setError(e.what());
            enterMode(Mode::Idle);
            return;
        }
        pendingSchemeHandIndex_ = -1;
        enterMode(Mode::Idle);
        return;
    }
    if (card.getTitle() == "STEP LIGHTLY") {
        enterMode(Mode::SchemeStepLightlyTarget);
        return;
    }
    if (card.getTitle() == "ROLLING FOG") {
        try {
            controller_.beginRollingFog(pendingSchemeHandIndex_);
            setStatus("Rolling Fog: choose a fog token to move.");
        } catch (const std::exception& e) {
            setError(e.what());
            enterMode(Mode::Idle);
            return;
        }
        pendingSchemeHandIndex_ = -1;
        enterMode(Mode::Idle);
        return;
    }

    pendingSchemeKind_ = controller_.requiredChoiceForScheme(pendingSchemeHandIndex_);

    switch (pendingSchemeKind_) {
        case SchemeChoiceKind::None:
            finalizeSchemeIfReady();
            break;
        case SchemeChoiceKind::Destination:
            enterMode(Mode::SchemeSelectDestination);
            break;
        case SchemeChoiceKind::TargetFighter:
            enterMode(Mode::SchemeSelectTarget);
            break;
        case SchemeChoiceKind::TargetAndDestination:
            enterMode(Mode::SchemeSelectTarget);
            break;
        case SchemeChoiceKind::NamedValue:
            enterMode(Mode::SchemeSelectNamedValue);
            break;
        case SchemeChoiceKind::OpponentHandCard:
            enterMode(Mode::SchemeSelectOpponentCard);
            break;
    }
}

void GameScreen::onSchemeTargetChosen(int index, const std::vector<std::string>& ids) {
    pendingSchemeChoice_.targetFighterId = ids[static_cast<std::size_t>(index)];
    if (pendingSchemeKind_ == SchemeChoiceKind::TargetAndDestination) {
        enterMode(Mode::SchemeSelectDestination);
    } else {
        finalizeSchemeIfReady();
    }
}

void GameScreen::onSchemeStepLightlyTargetChosen(int index, const std::vector<std::string>& ids) {
    const std::string targetId = ids[static_cast<std::size_t>(index)];
    try {
        controller_.handleStepLightly(pendingSchemeHandIndex_, targetId);
        setStatus("Step Lightly resolved.");
    } catch (const std::exception& e) {
        setError(e.what());
    }
    pendingSchemeHandIndex_ = -1;
    enterMode(Mode::Idle);
}

void GameScreen::onSchemeNamedValueChosen(int index, const std::vector<int>& values) {
    int value = values[static_cast<std::size_t>(index)];
    pendingSchemeChoice_.namedValue = value;

    const Card& card = controller_.currentPlayer().hand().at(static_cast<std::size_t>(pendingSchemeHandIndex_));
    if (card.getTitle() == "CONFIRM SUSPICION") {

        auto matching = controller_.getMatchingCardIndicesForConfirmSuspicion(value);
        if (matching.empty()) {
            controller_.playConfirmSuspicion(pendingSchemeHandIndex_, value);
            pendingSchemeHandIndex_ = -1;
            enterMode(Mode::Idle);
            return;
        }
        std::vector<std::string> labels;
        for (std::size_t i = 0; i < matching.size(); ++i) labels.push_back("Card #" + std::to_string(i + 1));
        rebuildChipButtons(labels, [this, matching](int i) {
            controller_.applyConfirmSuspicion(matching[static_cast<std::size_t>(i)]);
            Player& current = controller_.currentPlayer();
            Card played = current.removeCardFromHand(pendingSchemeHandIndex_);
            current.addToDiscard(std::move(played));
            controller_.decrementActions();
            controller_.endTurnIfNeeded();
            pendingSchemeHandIndex_ = -1;
            setStatus("Confirm Suspicion resolved.");
            enterMode(Mode::Idle);
        });
        setStatus("A matching card was found -- choose which one to burn.");
        return;
    }

    finalizeSchemeIfReady();
}

void GameScreen::onSchemeOpponentCardChosen(int index, const std::vector<int>& indexes) {
    pendingSchemeChoice_.opponentHandIndex = indexes[static_cast<std::size_t>(index)];
    finalizeSchemeIfReady();
}

void GameScreen::finalizeSchemeIfReady() {
    try {
        controller_.playScheme(pendingSchemeHandIndex_, pendingSchemeChoice_);
        setStatus("Scheme resolved.");
    } catch (const std::exception& e) {
        setError(e.what());
    }
    pendingSchemeHandIndex_ = -1;
    pendingSchemeKind_ = SchemeChoiceKind::None;
    enterMode(Mode::Idle);
}

void GameScreen::onRaveningTargetChosen(int index, const std::vector<std::string>& ids) {
    if (index < 0 || static_cast<std::size_t>(index) >= ids.size()) return;
    selectedRaveningFighterId_ = ids[static_cast<std::size_t>(index)];
    enterMode(Mode::RaveningDestination);
}

void GameScreen::onRaveningDestinationClicked(int spaceId) {
    try {
        controller_.applyRaveningMove(selectedRaveningFighterId_, spaceId);
        setStatus("Ravening move selected.");
    } catch (const std::exception& e) {
        setError(e.what());
        return;
    }
    enterMode(Mode::RaveningContinue);
}

void GameScreen::onRaveningContinue() {
    if (controller_.getRaveningTargets().empty()) {
        onRaveningFinish();
    } else {
        enterMode(Mode::RaveningTarget);
    }
}

void GameScreen::onRaveningFinish() {
    try {
        controller_.finishRaveningScheme();
        setStatus("Ravening Seduction resolved.");
    } catch (const std::exception& e) {
        setError(e.what());
        return;
    }
    selectedRaveningFighterId_.clear();
    enterMode(Mode::Idle);
}

void GameScreen::onConfoundYes() {
    try {
        controller_.resolveConfoundChoice(true);
        setStatus("CONFOUND: choose a card to discard.");
    } catch (const std::exception& e) {
        setError(e.what());
        return;
    }
    enterMode(Mode::Idle);
}

void GameScreen::onConfoundNo() {
    try {
        controller_.resolveConfoundChoice(false);
        setStatus("CONFOUND: discard declined -- move a fog token instead.");
    } catch (const std::exception& e) {
        setError(e.what());
        return;
    }
    enterMode(Mode::Idle);
}

void GameScreen::onConfoundCardChosen(int handIndex) {
    try {
        controller_.resolveConfoundDiscard(handIndex);
        setStatus("Opponent discarded a card.");
    } catch (const std::exception& e) {
        setError(e.what());
        return;
    }
    enterMode(Mode::Idle);
}

void GameScreen::onConfoundFogTokenChosen(int fogIndex) {
    pendingConfoundFogIndex_ = fogIndex;
    enterMode(Mode::ConfoundFogDestination);
}

void GameScreen::onConfoundFogDestinationClicked(int spaceId) {
    try {
        controller_.resolveConfoundFogMove(pendingConfoundFogIndex_, spaceId);
        pendingConfoundFogIndex_ = -1;
        setStatus("CONFOUND resolved: fog token moved.");
    } catch (const std::exception& e) {
        setError(e.what());
        return;
    }
    enterMode(Mode::Idle);
}

void GameScreen::onCodedNotesToggleCard(int handIndex) {
    auto it = std::find(codedNotesSelection_.begin(), codedNotesSelection_.end(), handIndex);
    if (it != codedNotesSelection_.end()) {
        codedNotesSelection_.erase(it);
    } else if (codedNotesSelection_.size() < 2) {
        codedNotesSelection_.push_back(handIndex);
    }
    enterMode(Mode::CodedNotesSelectCards);
}

void GameScreen::onCodedNotesConfirm() {
    try {
        controller_.finishCodedNotes(codedNotesSelection_);
        setStatus("CODED NOTES resolved.");
    } catch (const std::exception& e) {
        setError(e.what());
    }
    codedNotesSelection_.clear();
    enterMode(Mode::Idle);
}

void GameScreen::onLurkingChoiceChosen(int choice) {
    try {
        controller_.resolveLurkingChoice(choice);
    } catch (const std::exception& e) {
        setError(e.what());
        return;
    }
    enterMode(Mode::Idle);
}

void GameScreen::onLurkingFogTokenChosen(int fogIndex) {
    try {
        controller_.resolveLurkingFogToken(fogIndex);
    } catch (const std::exception& e) {
        setError(e.what());
        return;
    }
    enterMode(Mode::Idle);
}

void GameScreen::onLurkingDestinationClicked(int spaceId) {
    try {
        controller_.resolveLurkingDestination(spaceId);
        setStatus("LURKING resolved.");
    } catch (const std::exception& e) {
        setError(e.what());
        return;
    }
    enterMode(Mode::Idle);
}

void GameScreen::onLurkingSkip() {
    controller_.cancelLurking();
    enterMode(Mode::Idle);
}

void GameScreen::onSlipAwayFogTokenChosen(int fogIndex) {
    try {
        controller_.resolveSlipAwayFogToken(fogIndex);
    } catch (const std::exception& e) {
        setError(e.what());
        return;
    }
    enterMode(Mode::Idle);
}

void GameScreen::onSlipAwayDestinationClicked(int spaceId) {
    try {
        controller_.resolveSlipAwayDestination(spaceId);
        setStatus("SLIP AWAY resolved.");
    } catch (const std::exception& e) {
        setError(e.what());
        return;
    }
    enterMode(Mode::Idle);
}

void GameScreen::onSlipAwaySkip() {
    controller_.cancelSlipAway();
    enterMode(Mode::Idle);
}

void GameScreen::onRollingFogFogTokenChosen(int fogIndex) {
    try {
        controller_.resolveRollingFogFogToken(fogIndex);
    } catch (const std::exception& e) {
        setError(e.what());
        return;
    }
    enterMode(Mode::Idle);
}

void GameScreen::onRollingFogDestinationClicked(int spaceId) {
    try {
        controller_.resolveRollingFogDestination(spaceId);
        setStatus("ROLLING FOG resolved.");
    } catch (const std::exception& e) {
        setError(e.what());
        return;
    }
    enterMode(Mode::Idle);
}

void GameScreen::onRollingFogSkip() {
    controller_.cancelRollingFog();
    enterMode(Mode::Idle);
}

void GameScreen::onFogTokenChosen(int tokenIndex) {
    pendingFogTokenIndex_ = tokenIndex;
    enterMode(Mode::FogTokenDestination);
}

void GameScreen::onFogTokenDestinationClicked(int spaceId) {
    try {
        controller_.moveFogToken(pendingFogTokenIndex_, spaceId);
        pendingFogTokenIndex_ = -1;
        setStatus("Fog token moved.");
    } catch (const std::exception& e) {
        setError(e.what());
        return;
    }
    enterMode(Mode::Idle);
}

void GameScreen::onVanishedSpaceClicked(int spaceId) {
    try {
        controller_.placeVanishedInvisibleMan(spaceId);
        setStatus("Invisible Man reappeared.");
    } catch (const std::exception& e) {
        setError(e.what());
        return;
    }
    enterMode(Mode::Idle);
}

void GameScreen::onDiscardCardChosen(int handIndex) {
    try {
        controller_.discardCurrentPlayerCard(handIndex);
        setStatus("Card discarded.");
    } catch (const std::exception& e) {
        setError(e.what());
    }
    enterMode(Mode::Idle);
}

void GameScreen::onOptionalMovementDestinationClicked(int spaceId) {
    try {
        controller_.resolvePendingOptionalMovement(spaceId);
    } catch (const std::exception& e) {
        setError(e.what());
    }
    enterMode(Mode::Idle);
}

void GameScreen::onSkipOptionalMovementClicked() {
    controller_.resolvePendingOptionalMovement();
    enterMode(Mode::Idle);
}

void GameScreen::onInfoPopupAcknowledged() {
    controller_.clearStudyMethodsHandInfo();
    controller_.clearConfirmSuspicionHandInfo();
    infoPopupMessage_.clear();
    enterMode(Mode::Idle);
}



std::optional<std::string> GameScreen::fighterIdAtPoint(sf::Vector2f point) const {
    const float hitRadius = 34.f;
    float bestDistanceSq = hitRadius * hitRadius;
    std::optional<std::string> result;


    for (const auto& player : controller_.players()) {
        for (const auto& fighterPtr : player.fighters()) {
            const Fighter& fighter = *fighterPtr;
            if (fighter.defeated() || fighter.spaceId() <= 0) continue;
            if (mode_ == Mode::ManeuverSelectFighter && !isSelectableFighter(fighter.id())) continue;

            auto it = spacePositions_.find(fighter.spaceId());
            if (it == spacePositions_.end()) continue;
            const float dx = point.x - it->second.x;
            const float dy = point.y - it->second.y;
            const float d2 = dx * dx + dy * dy;
            if (d2 <= bestDistanceSq) {
                bestDistanceSq = d2;
                result = fighter.id();
            }
        }
    }

    return result;
}

bool GameScreen::isSelectableFighter(const std::string& fighterId) const {
    return std::find(selectableFighterIds_.begin(), selectableFighterIds_.end(), fighterId)
           != selectableFighterIds_.end();
}

bool GameScreen::handleBoardClick(sf::Vector2f point) {
    if (mode_ == Mode::ManeuverSelectFighter) {
        auto id = fighterIdAtPoint(point);
        if (id && isSelectableFighter(*id)) {

            const auto it = std::find(selectableFighterIds_.begin(),
                                      selectableFighterIds_.end(), *id);
            if (it != selectableFighterIds_.end()) {
                const int index = static_cast<int>(
                    std::distance(selectableFighterIds_.begin(), it));
                onManeuverFighterChosen(index, selectableFighterIds_);
                return true;
            }
        }
    }

    if (!highlightedSpaces_.empty()) {
        for (int spaceId : highlightedSpaces_) {
            auto it = spacePositions_.find(spaceId);
            if (it == spacePositions_.end()) continue;
            const float dx = point.x - it->second.x;
            const float dy = point.y - it->second.y;
            if (dx * dx + dy * dy <= 34.f * 34.f) {
                switch (mode_) {
                    case Mode::ManeuverSelectDestination: onDestinationSpaceClicked(spaceId); return true;
                    case Mode::SchemeSelectDestination:
                        pendingSchemeChoice_.destinationSpace = spaceId;
                        finalizeSchemeIfReady();
                        return true;
                    case Mode::OptionalMovementDestination: onOptionalMovementDestinationClicked(spaceId); return true;
                    case Mode::VanishedPlacement: onVanishedSpaceClicked(spaceId); return true;
                    case Mode::FogTokenDestination: onFogTokenDestinationClicked(spaceId); return true;
                    case Mode::ConfoundFogDestination: onConfoundFogDestinationClicked(spaceId); return true;
                    case Mode::LurkingDestination: onLurkingDestinationClicked(spaceId); return true;
                    case Mode::SlipAwayDestination: onSlipAwayDestinationClicked(spaceId); return true;
                    case Mode::RollingFogDestination: onRollingFogDestinationClicked(spaceId); return true;
                    case Mode::RaveningDestination: onRaveningDestinationClicked(spaceId); return true;
                    default: break;
                }
            }
        }
    }
    return false;
}

void GameScreen::setStatus(const std::string& message) {
    statusText_.setString(message);
    errorText_.setString("");
}

void GameScreen::setError(const std::string& message) {
    errorText_.setString("Error: " + message);
}

std::string GameScreen::cardStatsLine(const Card& card) const {
    std::ostringstream oss;
    if (card.canAttack()) oss << "ATK:" << card.getAttack() << " ";
    if (card.canDefend()) oss << "DEF:" << card.getDefense() << " ";
    if (card.getBoost() != 0) oss << "BST:" << card.getBoost();
    return oss.str();
}

std::string GameScreen::cardListLabel(const Card& card) const {
    const std::string stats = cardStatsLine(card);
    if (stats.empty()) return card.getTitle();
    return card.getTitle() + "   [" + stats + "]";
}

std::string GameScreen::fighterLabel(const std::string& fighterId) const {
    const Fighter* fighter = controller_.findFighterById(fighterId);
    if (!fighter) return fighterId;
    std::ostringstream oss;
    oss << fighter->displayName() << "\n" << fighter->health() << "/" << fighter->maxHealth() << " HP";
    return oss.str();
}

} // namespace unmatched::gfx
