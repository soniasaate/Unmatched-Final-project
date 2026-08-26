#pragma once
#include "Application.hpp"
#include "Button.hpp"
#include "Screen.hpp"
#include <SFML/Graphics.hpp>
#include <memory>

namespace unmatched::gfx {
class StartSelectScreen : public Screen {
public:
    StartSelectScreen(Application& app, int playerOneAge, int playerTwoAge, int fighter1Index, int fighter2Index);
    void handleEvent(const sf::Event& event) override;
    void update(float deltaSeconds) override;
    void render(sf::RenderWindow& window) override;
private:
    void fitBackgroundToWindow();
    void centerTitle();
    void centerSubtitle();
    void onSlotSelected(int slot);
    void startGame();
    void onBack();
    sf::Sprite background_;
    sf::Text title_;
    sf::Text subtitle_;
    sf::Text statusText_;
    std::vector<Button> buttons_;
    std::unique_ptr<Button> backButton_;
    int playerOneAge_, playerTwoAge_, fighter1Index_, fighter2Index_, selectedSlot_;
    std::string statusMessage_;
};
}