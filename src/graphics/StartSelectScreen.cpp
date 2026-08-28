#include "unmatched/graphics/StartSelectScreen.hpp"
#include "unmatched/graphics/FighterSelectScreen.hpp"
#include "unmatched/graphics/GameScreen.hpp"

namespace unmatched::gfx {

StartSelectScreen::StartSelectScreen(Application& app, int playerOneAge, int playerTwoAge, int fighter1Index, int fighter2Index)
    : StartSelectScreen(app, playerOneAge, playerTwoAge, fighter1Index, fighter2Index, playerOneAge <= playerTwoAge ? 0 : 1) {
}

StartSelectScreen::StartSelectScreen(Application& app, int playerOneAge, int playerTwoAge, int fighter1Index, int fighter2Index, int firstPlayerIndex)
    : Screen(app), background_(app_.resources().getTexture("assets/images/menu_background.jpg"))
    , title_(app_.resources().getFont("assets/fonts/title_font.ttf")), subtitle_(app_.resources().getFont("assets/fonts/title_font.ttf"))
    , statusText_(app_.resources().getFont("assets/fonts/title_font.ttf"))
    , playerOneAge_(playerOneAge), playerTwoAge_(playerTwoAge)
    , fighter1Index_(fighter1Index), fighter2Index_(fighter2Index), selectedSlot_(-1), firstPlayerIndex_(firstPlayerIndex) {

    fitBackgroundToWindow();
    title_.setString("CHOOSE START POSITION");
    title_.setCharacterSize(48); title_.setFillColor(sf::Color(200, 30, 30)); title_.setStyle(sf::Text::Bold);
    centerTitle();

    std::string youngerName = firstPlayerIndex_ == 0 ? "Player 1" : "Player 2";
    std::string fighterNames[] = {"Dracula", "Sherlock Holmes", "Invisible Man"};
    std::string youngerFighter = fighterNames[firstPlayerIndex_ == 0 ? fighter1Index_ : fighter2Index_];
    subtitle_.setString(youngerName + " (" + youngerFighter + ") chooses start space");
    subtitle_.setCharacterSize(28); subtitle_.setFillColor(sf::Color(190, 180, 160));
    centerSubtitle();

    statusText_.setCharacterSize(20); statusText_.setFillColor(sf::Color(210, 190, 100)); statusText_.setPosition(sf::Vector2f(20.f, 720.f));

    sf::Font& buttonFont = app_.resources().getFont("assets/fonts/title_font.ttf");
    sf::Vector2f ws(static_cast<sf::Vector2f>(app_.window().getSize()));
    float startX = (ws.x - (200.f * 2 + 60.f)) / 2.f;

    buttons_.emplace_back(buttonFont, "Start Space 1", sf::Vector2f(startX, 300.f), sf::Vector2f(200.f, 120.f));
    buttons_.back().onClick = [this]() { onSlotSelected(1); };

    buttons_.emplace_back(buttonFont, "Start Space 2", sf::Vector2f(startX + 260.f, 300.f), sf::Vector2f(200.f, 120.f));
    buttons_.back().onClick = [this]() { onSlotSelected(2); };

    backButton_ = std::make_unique<Button>(buttonFont, "Back", sf::Vector2f(50.f, 650.f), sf::Vector2f(120.f, 50.f));
    backButton_->onClick = [this]() { onBack(); };
}

void StartSelectScreen::handleEvent(const sf::Event& event) {
    for (auto& b : buttons_) b.handleEvent(event, app_.window());
    if (backButton_) backButton_->handleEvent(event, app_.window());
}
void StartSelectScreen::update(float) {}
void StartSelectScreen::render(sf::RenderWindow& window) {
    window.draw(background_); window.draw(title_); window.draw(subtitle_);
    for (auto& b : buttons_) b.render(window);
    if (backButton_) backButton_->render(window);
    if (!statusMessage_.empty()) { statusText_.setString(statusMessage_); window.draw(statusText_); }
}
void StartSelectScreen::fitBackgroundToWindow() {
    sf::Vector2u ts = background_.getTexture().getSize();
    sf::Vector2u ws = app_.window().getSize();
    background_.setScale(sf::Vector2f(static_cast<float>(ws.x) / ts.x, static_cast<float>(ws.y) / ts.y));
}
void StartSelectScreen::centerTitle() {
    sf::FloatRect bounds = title_.getLocalBounds();
    title_.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f));
    title_.setPosition(sf::Vector2f(app_.window().getSize().x / 2.f, 100.f));
}
void StartSelectScreen::centerSubtitle() {
    sf::FloatRect bounds = subtitle_.getLocalBounds();
    subtitle_.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f));
    subtitle_.setPosition(sf::Vector2f(app_.window().getSize().x / 2.f, 180.f));
}
void StartSelectScreen::onSlotSelected(int slot) {
    selectedSlot_ = slot;
    statusMessage_ = "Starting game...";
    startGame();
}
void StartSelectScreen::startGame() {
    app_.setScreen(std::make_unique<GameScreen>(app_, playerOneAge_, playerTwoAge_, fighter1Index_, fighter2Index_, selectedSlot_, firstPlayerIndex_));
}
void StartSelectScreen::onBack() {
    app_.setScreen(std::make_unique<FighterSelectScreen>(app_, playerOneAge_, playerTwoAge_, firstPlayerIndex_));
}
}
