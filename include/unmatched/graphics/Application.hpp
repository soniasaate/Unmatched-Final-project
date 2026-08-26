#pragma once
#include "ResourceManager.hpp"
#include "Screen.hpp"
#include <SFML/Graphics.hpp>
#include <memory>
#include <optional>

namespace unmatched::gfx {
class Application {
public:
    Application()
        : window_(sf::VideoMode({1280u, 800u}), "Unmatched: Baskerville Manor", sf::Style::Titlebar | sf::Style::Close) {
        window_.setFramerateLimit(60);
    }
    ResourceManager& resources() { return resources_; }
    sf::RenderWindow& window() { return window_; }
    void setScreen(std::unique_ptr<Screen> screen) { nextScreen_ = std::move(screen); }
    void requestExit() { window_.close(); }

    void run() {
        sf::Clock clock;
        while (window_.isOpen()) {
            if (nextScreen_) currentScreen_ = std::move(nextScreen_);
            while (const std::optional<sf::Event> event = window_.pollEvent()) {
                if (event->is<sf::Event::Closed>()) window_.close();
                if (currentScreen_) currentScreen_->handleEvent(*event);
            }
            float dt = clock.restart().asSeconds();
            if (currentScreen_) currentScreen_->update(dt);
            window_.clear(sf::Color::Black);
            if (currentScreen_) currentScreen_->render(window_);
            window_.display();
        }
    }
private:
    sf::RenderWindow window_;
    ResourceManager resources_;
    std::unique_ptr<Screen> currentScreen_;
    std::unique_ptr<Screen> nextScreen_; 
};
} // namespace unmatched::gfx