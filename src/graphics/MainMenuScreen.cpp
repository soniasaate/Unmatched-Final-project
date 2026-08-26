#include "unmatched/graphics/MainMenuScreen.hpp"
#include "unmatched/graphics/AgeSetupScreen.hpp"
#include "unmatched/graphics/GameScreen.hpp"
#include "unmatched/GameController.hpp"
#include <iostream>

namespace unmatched::gfx {

MainMenuScreen::MainMenuScreen(Application& app)
    : Screen(app)
    , background_(app_.resources().getTexture("assets/images/menu_background.jpg"))
    , title_(app_.resources().getFont("assets/fonts/title_font.ttf"))
    , subtitle_(app_.resources().getFont("assets/fonts/title_font.ttf"))
    , statusText_(app_.resources().getFont("assets/fonts/title_font.ttf"))
    , loadMenuTitle_(app_.resources().getFont("assets/fonts/title_font.ttf")) {

    fitBackgroundToWindow();

    title_.setString("UNMATCHED");
    title_.setCharacterSize(64);
    title_.setFillColor(sf::Color(200, 30, 30));
    title_.setStyle(sf::Text::Bold);
    centerTitle();

    subtitle_.setString("Baskerville Manor");
    subtitle_.setCharacterSize(28);
    subtitle_.setFillColor(sf::Color(190, 180, 160));
    centerSubtitle();

    sf::Font& buttonFont = app_.resources().getFont("assets/fonts/title_font.ttf");
    sf::Vector2f windowSize(static_cast<sf::Vector2f>(app_.window().getSize()));
    float buttonWidth = 260.f;
    float buttonHeight = 60.f;
    float startY = windowSize.y * 0.55f;
    float gap = 20.f;

    buttons_.emplace_back(buttonFont, "Start Game",
        sf::Vector2f((windowSize.x - buttonWidth) / 2.f, startY),
        sf::Vector2f(buttonWidth, buttonHeight));
    buttons_.back().onClick = [this]() { onStartGame(); };

    buttons_.emplace_back(buttonFont, "Load Game",
        sf::Vector2f((windowSize.x - buttonWidth) / 2.f, startY + (buttonHeight + gap)),
        sf::Vector2f(buttonWidth, buttonHeight));
    buttons_.back().onClick = [this]() { onLoadGame(); };

    buttons_.emplace_back(buttonFont, "Exit",
        sf::Vector2f((windowSize.x - buttonWidth) / 2.f, startY + 2 * (buttonHeight + gap)),
        sf::Vector2f(buttonWidth, buttonHeight));
    buttons_.back().onClick = [this]() { onExit(); };

    statusText_.setCharacterSize(20);
    statusText_.setFillColor(sf::Color(210, 190, 100));
    statusText_.setPosition(sf::Vector2f(20.f, windowSize.y - 40.f));

    loadMenuTitle_.setString("CHOOSE A SAVED GAME");
    loadMenuTitle_.setCharacterSize(30);
    loadMenuTitle_.setFillColor(sf::Color(220, 200, 150));
    loadMenuTitle_.setStyle(sf::Text::Bold);
}

void MainMenuScreen::handleEvent(const sf::Event& event) {
    if (showingLoadMenu_) {
        for (auto& button : loadSlotButtons_) {
            button.handleEvent(event, app_.window());
        }
        if (loadBackButton_) {
            loadBackButton_->handleEvent(event, app_.window());
        }
        return;
    }
    for (auto& button : buttons_) {
        button.handleEvent(event, app_.window());
    }
}

void MainMenuScreen::update(float /*deltaSeconds*/) {}

void MainMenuScreen::render(sf::RenderWindow& window) {
    window.draw(background_);
    window.draw(title_);

    if (showingLoadMenu_) {
        window.draw(loadMenuTitle_);
        for (auto& button : loadSlotButtons_) {
            button.render(window);
        }
        if (loadBackButton_) {
            loadBackButton_->render(window);
        }
        if (!statusMessage_.empty()) {
            statusText_.setString(statusMessage_);
            window.draw(statusText_);
        }
        return;
    }

    window.draw(subtitle_);
    for (auto& button : buttons_) {
        button.render(window);
    }
    if (!statusMessage_.empty()) {
        statusText_.setString(statusMessage_);
        window.draw(statusText_);
    }
}

void MainMenuScreen::fitBackgroundToWindow() {
    sf::Vector2u textureSize = background_.getTexture().getSize();
    sf::Vector2u windowSize = app_.window().getSize();
    background_.setScale(sf::Vector2f(
        static_cast<float>(windowSize.x) / static_cast<float>(textureSize.x),
        static_cast<float>(windowSize.y) / static_cast<float>(textureSize.y)));
}

void MainMenuScreen::centerTitle() {
    sf::FloatRect bounds = title_.getLocalBounds();
    title_.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x / 2.f,
                                   bounds.position.y + bounds.size.y / 2.f));
    title_.setPosition(sf::Vector2f(static_cast<float>(app_.window().getSize().x) / 2.f, 140.f));

    sf::FloatRect loadBounds = loadMenuTitle_.getLocalBounds();
    loadMenuTitle_.setOrigin(sf::Vector2f(loadBounds.position.x + loadBounds.size.x / 2.f,
                                           loadBounds.position.y + loadBounds.size.y / 2.f));
    loadMenuTitle_.setPosition(sf::Vector2f(static_cast<float>(app_.window().getSize().x) / 2.f, 220.f));
}

void MainMenuScreen::centerSubtitle() {
    sf::FloatRect bounds = subtitle_.getLocalBounds();
    subtitle_.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x / 2.f,
                                      bounds.position.y + bounds.size.y / 2.f));
    subtitle_.setPosition(sf::Vector2f(static_cast<float>(app_.window().getSize().x) / 2.f, 200.f));
}

void MainMenuScreen::onStartGame() {
    app_.setScreen(std::make_unique<AgeSetupScreen>(app_));
}

void MainMenuScreen::onLoadGame() {
    refreshLoadMenu();
}

void MainMenuScreen::refreshLoadMenu() {
    
    unmatched::GameController tmp;
    auto slots = tmp.getSaveSlots();

    loadSlotButtons_.clear();
    statusMessage_.clear();

    sf::Font& buttonFont = app_.resources().getFont("assets/fonts/title_font.ttf");
    sf::Vector2f windowSize(static_cast<sf::Vector2f>(app_.window().getSize()));
    float buttonWidth = 420.f;
    float buttonHeight = 60.f;
    float startY = 300.f;
    float gap = 20.f;

    if (slots.empty()) {
        statusMessage_ = "No saved games found yet.";
    } else {
        for (std::size_t i = 0; i < slots.size(); ++i) {
            int slotNumber = slots[i].first;
            const std::string& label = slots[i].second; 
            loadSlotButtons_.emplace_back(buttonFont, label,
                sf::Vector2f((windowSize.x - buttonWidth) / 2.f, startY + static_cast<float>(i) * (buttonHeight + gap)),
                sf::Vector2f(buttonWidth, buttonHeight));
            loadSlotButtons_.back().onClick = [this, slotNumber]() { onSlotChosen(slotNumber); };
        }
    }

    sf::Font& font = app_.resources().getFont("assets/fonts/title_font.ttf");
    loadBackButton_ = std::make_unique<Button>(
        font, "Back",
        sf::Vector2f(50.f, windowSize.y - 100.f),
        sf::Vector2f(120.f, 50.f));
    loadBackButton_->onClick = [this]() { onLoadBack(); };

    showingLoadMenu_ = true;
}

void MainMenuScreen::onSlotChosen(int slot) {
    app_.setScreen(std::make_unique<GameScreen>(app_, slot));
}

void MainMenuScreen::onLoadBack() {
    showingLoadMenu_ = false;
    loadSlotButtons_.clear();
    loadBackButton_.reset();
    statusMessage_.clear();
}

void MainMenuScreen::onExit() {
    app_.requestExit();
}

} // namespace unmatched::gfx
