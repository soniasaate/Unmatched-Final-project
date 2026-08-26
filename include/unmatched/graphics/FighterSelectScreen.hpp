#pragma once
#include "Application.hpp"
#include "Button.hpp"
#include "Screen.hpp"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

namespace unmatched::gfx {
class FighterSelectScreen : public Screen {
public:
    FighterSelectScreen(Application& app, int playerOneAge, int playerTwoAge);
    void handleEvent(const sf::Event& event) override;
    void update(float deltaSeconds) override;
    void render(sf::RenderWindow& window) override;
private:
    void fitBackgroundToWindow();
    void centerTitle();
    void centerSubtitle();
    void updateStatus();
    void onFighterSelected(int index);
    void onConfirm();
    void onBack();

    sf::Sprite background_;
    sf::Text title_;
    sf::Text subtitle_;
    sf::Text statusText_;
    std::vector<sf::Text> fighterNames_;
    std::vector<sf::Sprite> fighterLogos_;
    std::vector<sf::Texture> logoTextures_;
    std::vector<sf::RectangleShape> cardBackgrounds_;
    std::vector<Button> fighters_;
    std::vector<Button> buttons_;
    int playerOneAge_;
    int playerTwoAge_;
    int selectionPhase_;
    int selectedFighter1_;
    int selectedFighter2_;
    std::string statusMessage_;
    bool logosLoaded_;
};
}