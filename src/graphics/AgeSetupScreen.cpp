#include "unmatched/graphics/AgeSetupScreen.hpp"
#include "unmatched/graphics/FighterSelectScreen.hpp"
#include "unmatched/graphics/MainMenuScreen.hpp"
#include <cctype>

namespace unmatched::gfx {

AgeSetupScreen::AgeSetupScreen(Application& app)
    : Screen(app), background_(app_.resources().getTexture("assets/images/menu_background.jpg"))
    , title_(app_.resources().getFont("assets/fonts/title_font.ttf")), ageText_(app_.resources().getFont("assets/fonts/title_font.ttf"))
    , statusText_(app_.resources().getFont("assets/fonts/title_font.ttf"))
    , playerIndex_(0), ageInput_(""), errorMessage_(""), playerOneAge_(0), playerTwoAge_(0) {

    fitBackgroundToWindow();
    title_.setString("ENTER AGES");
    title_.setCharacterSize(48);
    title_.setFillColor(sf::Color(200, 30, 30));
    title_.setStyle(sf::Text::Bold);
    centerTitle();

    ageText_.setCharacterSize(36);
    ageText_.setFillColor(sf::Color(230, 220, 200));
    updateAgeText();

    statusText_.setCharacterSize(24);
    statusText_.setFillColor(sf::Color(210, 190, 100));
    statusText_.setPosition(sf::Vector2f(20.f, 650.f));

    sf::Font& buttonFont = app_.resources().getFont("assets/fonts/title_font.ttf");
    sf::Vector2f windowSize(static_cast<sf::Vector2f>(app_.window().getSize()));
    float buttonWidth = 150.f;
    float buttonHeight = 50.f;
    float startY = windowSize.y * 0.7f;
    float gap = 30.f;

    buttons_.emplace_back(buttonFont, "OK", sf::Vector2f((windowSize.x - buttonWidth * 2 - gap) / 2.f, startY), sf::Vector2f(buttonWidth, buttonHeight));
    buttons_.back().onClick = [this]() { onOK(); };

    buttons_.emplace_back(buttonFont, "Back", sf::Vector2f((windowSize.x + gap) / 2.f, startY), sf::Vector2f(buttonWidth, buttonHeight));
    buttons_.back().onClick = [this]() { onBack(); };
}

void AgeSetupScreen::handleEvent(const sf::Event& event) {
    for (auto& button : buttons_) button.handleEvent(event, app_.window());
    if (const auto* text = event.getIf<sf::Event::TextEntered>()) {
        if (text->unicode < 128) {
            char ch = static_cast<char>(text->unicode);
            if (ch >= '0' && ch <= '9' && ageInput_.length() < 3) {
                ageInput_ += ch;
                updateAgeText();
                errorMessage_ = "";
            } else if (ch == '\b' && !ageInput_.empty()) {
                ageInput_.pop_back();
                updateAgeText();
                errorMessage_ = "";
            }
        }
    }
}

void AgeSetupScreen::update(float /*deltaSeconds*/) {}
void AgeSetupScreen::render(sf::RenderWindow& window) {
    window.draw(background_);
    window.draw(title_);
    window.draw(ageText_);
    for (auto& button : buttons_) button.render(window);
    if (!errorMessage_.empty()) {
        statusText_.setString(errorMessage_);
        statusText_.setFillColor(sf::Color::Red);
    } else {
        statusText_.setString("Enter age for Player " + std::to_string(playerIndex_ + 1) + " (1-999)");
        statusText_.setFillColor(sf::Color(210, 190, 100));
    }
    window.draw(statusText_);
}
void AgeSetupScreen::fitBackgroundToWindow() {
    sf::Vector2u textureSize = background_.getTexture().getSize();
    sf::Vector2u windowSize = app_.window().getSize();
    background_.setScale(sf::Vector2f(static_cast<float>(windowSize.x) / static_cast<float>(textureSize.x), static_cast<float>(windowSize.y) / static_cast<float>(textureSize.y)));
}
void AgeSetupScreen::centerTitle() {
    sf::FloatRect bounds = title_.getLocalBounds();
    title_.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f));
    title_.setPosition(sf::Vector2f(static_cast<float>(app_.window().getSize().x) / 2.f, 100.f));
}
void AgeSetupScreen::updateAgeText() {
    ageText_.setString("Player " + std::to_string(playerIndex_ + 1) + " Age: " + (ageInput_.empty() ? "_" : ageInput_));
    sf::FloatRect bounds = ageText_.getLocalBounds();
    ageText_.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f));
    ageText_.setPosition(sf::Vector2f(static_cast<float>(app_.window().getSize().x) / 2.f, 350.f));
}
void AgeSetupScreen::onOK() {
    if (ageInput_.empty()) { errorMessage_ = "Please enter an age."; return; }
    int age = std::stoi(ageInput_);
    if (age <= 0) { errorMessage_ = "Age must be positive."; return; }
    if (playerIndex_ == 0) {
        playerOneAge_ = age; playerIndex_ = 1; ageInput_ = "";
        updateAgeText(); errorMessage_ = "";
    } else {
        playerTwoAge_ = age;
        app_.setScreen(std::make_unique<FighterSelectScreen>(app_, playerOneAge_, playerTwoAge_));
    }
}
void AgeSetupScreen::onBack() { app_.setScreen(std::make_unique<MainMenuScreen>(app_)); }
}