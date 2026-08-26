#pragma once

#include "Application.hpp"
#include "Button.hpp"
#include "Screen.hpp"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <memory>

namespace unmatched::gfx {

class MainMenuScreen : public Screen {
public:
    explicit MainMenuScreen(Application& app);
    ~MainMenuScreen() override = default;

    void handleEvent(const sf::Event& event) override;
    void update(float deltaSeconds) override;
    void render(sf::RenderWindow& window) override;

private:
    void fitBackgroundToWindow();
    void centerTitle();
    void centerSubtitle();
    void onStartGame();
    void onLoadGame();
    void onExit();
    void refreshLoadMenu();
    void onSlotChosen(int slot);
    void onLoadBack();

    sf::Sprite background_;
    sf::Text title_;
    sf::Text subtitle_;
    sf::Text statusText_;
    std::vector<Button> buttons_;
    std::string statusMessage_;

    bool showingLoadMenu_ = false;
    std::vector<Button> loadSlotButtons_;
    std::unique_ptr<Button> loadBackButton_;
    sf::Text loadMenuTitle_;
};

} // namespace unmatched::gfx
