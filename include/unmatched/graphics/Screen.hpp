#pragma once
#include <SFML/Graphics.hpp>
namespace unmatched::gfx {
class Application;
class Screen {
public:
    explicit Screen(Application& app) : app_(app) {}
    virtual ~Screen() = default;
    virtual void handleEvent(const sf::Event& event) = 0;
    virtual void update(float deltaSeconds) = 0;
    virtual void render(sf::RenderWindow& window) = 0;
protected:
    Application& app_;
};
} // namespace unmatched::gfx