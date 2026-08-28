#include "unmatched/graphics/FighterSelectScreen.hpp"
#include "unmatched/graphics/AgeSetupScreen.hpp"
#include "unmatched/graphics/StartSelectScreen.hpp"
#include <iostream>

namespace unmatched::gfx {

FighterSelectScreen::FighterSelectScreen(Application& app, int playerOneAge, int playerTwoAge)
    : FighterSelectScreen(app, playerOneAge, playerTwoAge, playerOneAge <= playerTwoAge ? 0 : 1) {
}

FighterSelectScreen::FighterSelectScreen(Application& app, int playerOneAge, int playerTwoAge, int firstPlayerIndex)
    : Screen(app), background_(app_.resources().getTexture("assets/images/menu_background.jpg"))
    , title_(app_.resources().getFont("assets/fonts/title_font.ttf")), subtitle_(app_.resources().getFont("assets/fonts/title_font.ttf"))
    , statusText_(app_.resources().getFont("assets/fonts/title_font.ttf")), playerOneAge_(playerOneAge), playerTwoAge_(playerTwoAge)
    , firstPlayerIndex_(firstPlayerIndex)
    , selectionPhase_(0), selectedFighter1_(-1), selectedFighter2_(-1), logosLoaded_(false) {

    fighterNames_.reserve(3); fighterLogos_.reserve(3);
    logoTextures_.resize(3); cardBackgrounds_.resize(3);
    fitBackgroundToWindow();

    title_.setString("PLAYER 1, CHOOSE YOUR LEGEND");
    title_.setCharacterSize(36); title_.setFillColor(sf::Color(220, 200, 150)); title_.setStyle(sf::Text::Bold);
    centerTitle();

    subtitle_.setString("SELECT ONE FIGHTER TO STEP INTO THE FOG");
    subtitle_.setCharacterSize(22); subtitle_.setFillColor(sf::Color(180, 170, 140));
    centerSubtitle();

    statusText_.setCharacterSize(18); statusText_.setFillColor(sf::Color(210, 190, 100));
    statusText_.setPosition(sf::Vector2f(20.f, 740.f));

    const char* logoPaths[] = { "assets/images/draculaa.png", "assets/images/sherlockTran.png", "assets/images/tranInv.png" };
    const char* fighterNames[] = { "DRACULA", "SHERLOCK HOLMES", "INVISIBLE MAN" };

    sf::Vector2f windowSize(static_cast<sf::Vector2f>(app_.window().getSize()));
    float cardWidth = 280.f, cardHeight = 380.f, startY = 200.f, gap = 40.f;
    float startX = (windowSize.x - (cardWidth * 3 + gap * 2)) / 2.f;

    for (int i = 0; i < 3; ++i) {
        cardBackgrounds_[i].setSize(sf::Vector2f(cardWidth, cardHeight));
        cardBackgrounds_[i].setPosition(sf::Vector2f(startX + i * (cardWidth + gap), startY));
        cardBackgrounds_[i].setFillColor(sf::Color(40, 35, 45, 220));
        cardBackgrounds_[i].setOutlineThickness(2.f);
        cardBackgrounds_[i].setOutlineColor(sf::Color(120, 100, 70));

        bool loaded = logoTextures_[i].loadFromFile(logoPaths[i]);
        fighterLogos_.emplace_back(logoTextures_[i]);
        if (loaded) {
            sf::Vector2u ts = logoTextures_[i].getSize();
            float scale = std::min((cardWidth - 40.f) / ts.x, (cardHeight * 0.5f) / ts.y);
            fighterLogos_[i].setScale(sf::Vector2f(scale, scale));
            sf::FloatRect bounds = fighterLogos_[i].getLocalBounds();
            fighterLogos_[i].setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f));
            fighterLogos_[i].setPosition(sf::Vector2f(startX + i * (cardWidth + gap) + cardWidth / 2.f, startY + cardHeight * 0.35f));
            logosLoaded_ = true;
        }

        fighterNames_.emplace_back(app_.resources().getFont("assets/fonts/title_font.ttf"));
        fighterNames_[i].setString(fighterNames[i]);
        fighterNames_[i].setCharacterSize(20); fighterNames_[i].setFillColor(sf::Color(220, 200, 150));
        fighterNames_[i].setStyle(sf::Text::Bold);
        sf::FloatRect nb = fighterNames_[i].getLocalBounds();
        fighterNames_[i].setOrigin(sf::Vector2f(nb.position.x + nb.size.x / 2.f, nb.position.y + nb.size.y / 2.f));
        fighterNames_[i].setPosition(sf::Vector2f(startX + i * (cardWidth + gap) + cardWidth / 2.f, startY + cardHeight * 0.75f));

        fighters_.emplace_back(app_.resources().getFont("assets/fonts/title_font.ttf"), "", sf::Vector2f(startX + i * (cardWidth + gap), startY), sf::Vector2f(cardWidth, cardHeight));
        fighters_.back().onClick = [this, i]() { onFighterSelected(i); };
        fighters_.back().setTransparent(true);
    }
    sf::Font& buttonFont = app_.resources().getFont("assets/fonts/title_font.ttf");
    buttons_.emplace_back(buttonFont, "BACK", sf::Vector2f(50.f, 660.f), sf::Vector2f(120.f, 45.f));
    buttons_.back().onClick = [this]() { onBack(); };
    buttons_.emplace_back(buttonFont, "CONFIRM", sf::Vector2f(windowSize.x - 190.f, 660.f), sf::Vector2f(150.f, 45.f));
    buttons_.back().onClick = [this]() { onConfirm(); };
    buttons_.back().setEnabled(false);

    updateStatus();
}

void FighterSelectScreen::handleEvent(const sf::Event& event) {
    for (auto& f : fighters_) f.handleEvent(event, app_.window());
    for (auto& b : buttons_) b.handleEvent(event, app_.window());
}
void FighterSelectScreen::update(float) {}
void FighterSelectScreen::render(sf::RenderWindow& window) {
    window.draw(background_); window.draw(title_); window.draw(subtitle_);
    for (int i = 0; i < 3; ++i) {
        window.draw(cardBackgrounds_[i]);
        if (logosLoaded_) window.draw(fighterLogos_[i]);
        window.draw(fighterNames_[i]);
        fighters_[i].render(window);
    }
    for (auto& b : buttons_) b.render(window);
    if (!statusMessage_.empty()) { statusText_.setString(statusMessage_); window.draw(statusText_); }
}
void FighterSelectScreen::fitBackgroundToWindow() {
    sf::Vector2u ts = background_.getTexture().getSize();
    sf::Vector2u ws = app_.window().getSize();
    background_.setScale(sf::Vector2f(static_cast<float>(ws.x) / ts.x, static_cast<float>(ws.y) / ts.y));
}
void FighterSelectScreen::centerTitle() {
    sf::FloatRect bounds = title_.getLocalBounds();
    title_.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f));
    title_.setPosition(sf::Vector2f(app_.window().getSize().x / 2.f, 60.f));
}
void FighterSelectScreen::centerSubtitle() {
    sf::FloatRect bounds = subtitle_.getLocalBounds();
    subtitle_.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f));
    subtitle_.setPosition(sf::Vector2f(app_.window().getSize().x / 2.f, 105.f));
}
void FighterSelectScreen::updateStatus() {
    if (selectionPhase_ == 0) {
        int firstPlayer = firstPlayerIndex_ + 1;
        title_.setString("PLAYER " + std::to_string(firstPlayer) + ", CHOOSE YOUR LEGEND");
        statusMessage_ = "PLAYER " + std::to_string(firstPlayer) + " SELECTS FIRST";
    } else if (selectionPhase_ == 1) {
        int secondPlayer = (1 - firstPlayerIndex_) + 1;
        title_.setString("PLAYER " + std::to_string(secondPlayer) + ", CHOOSE YOUR LEGEND");
        statusMessage_ = "PLAYER " + std::to_string(secondPlayer) + " SELECTS THEIR FIGHTER";
    } else {
        title_.setString("FIGHTERS SELECTED");
        statusMessage_ = "FIGHTERS SELECTED! PRESS CONFIRM TO CONTINUE";
    }
    centerTitle();
}
void FighterSelectScreen::onFighterSelected(int index) {
    if (selectionPhase_ == 0) {
        selectedFighter1_ = index; selectionPhase_ = 1;
        cardBackgrounds_[index].setOutlineColor(sf::Color(200, 180, 50)); cardBackgrounds_[index].setOutlineThickness(3.f);
        fighters_[index].setEnabled(false); buttons_.back().setEnabled(false); updateStatus();
    } else if (selectionPhase_ == 1 && index != selectedFighter1_) {
        selectedFighter2_ = index; selectionPhase_ = 2;
        cardBackgrounds_[index].setOutlineColor(sf::Color(200, 180, 50)); cardBackgrounds_[index].setOutlineThickness(3.f);
        fighters_[index].setEnabled(false); buttons_.back().setEnabled(true); updateStatus();
    }
}
void FighterSelectScreen::onConfirm() {
    if (selectionPhase_ == 2) {
        bool playerOneFirst = firstPlayerIndex_ == 0;
        int playerOneFighter = playerOneFirst ? selectedFighter1_ : selectedFighter2_;
        int playerTwoFighter = playerOneFirst ? selectedFighter2_ : selectedFighter1_;
        app_.setScreen(std::make_unique<StartSelectScreen>(app_, playerOneAge_, playerTwoAge_, playerOneFighter, playerTwoFighter, firstPlayerIndex_));
    }
}
void FighterSelectScreen::onBack() {
    if (selectionPhase_ > 0) {
        for (int i = 0; i < 3; ++i) {
            fighters_[i].setEnabled(true);
            cardBackgrounds_[i].setOutlineColor(sf::Color(120, 100, 70)); cardBackgrounds_[i].setOutlineThickness(2.f);
        }
        selectionPhase_ = 0; selectedFighter1_ = -1; selectedFighter2_ = -1;
        buttons_.back().setEnabled(false); updateStatus();
    } else app_.setScreen(std::make_unique<AgeSetupScreen>(app_));
}
}
