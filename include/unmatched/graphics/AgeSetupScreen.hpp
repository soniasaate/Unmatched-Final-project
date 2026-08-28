#pragma once
#include "Application.hpp"
#include "Button.hpp"
#include "Screen.hpp"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

namespace unmatched::gfx {
class AgeSetupScreen : public Screen {
public:
    explicit AgeSetupScreen(Application& app);
    void handleEvent(const sf::Event& event) override;
    void update(float deltaSeconds) override;
    void render(sf::RenderWindow& window) override;
private:
    void fitBackgroundToWindow();
    void centerTitle();
    void updateAgeText();
    void onOK();
    void onBack();
    sf::Sprite background_;
    sf::Text title_;
    sf::Text ageText_;
    sf::Text statusText_;
    std::vector<Button> buttons_;
    int playerIndex_;
    std::string ageInput_;
    std::string errorMessage_;
    int playerOneAge_;
    int playerTwoAge_;
    int firstPlayerIndex_;
};
}
