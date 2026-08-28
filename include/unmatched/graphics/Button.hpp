#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <string>

namespace unmatched::gfx {
class Button {
public:
    Button(sf::Font& font, const std::string& label, sf::Vector2f position, sf::Vector2f size)
        : text_(font), size_(size), enabled_(true), transparent_(false) {
        shape_.setPosition(position);
        shape_.setSize(size);
        shape_.setFillColor(normalColor_);
        shape_.setOutlineThickness(2.f);
        shape_.setOutlineColor(sf::Color(120, 100, 70));
        text_.setString(label);
        text_.setCharacterSize(26);
        text_.setFillColor(sf::Color(230, 220, 200));
        centerText();
    }
    void setEnabled(bool enabled) { 
        enabled_ = enabled; 
        shape_.setFillColor(enabled ? normalColor_ : disabledColor_);
    }
    bool isEnabled() const { return enabled_; }
    void setTransparent(bool transparent) {
        transparent_ = transparent;
        if (transparent) {
            shape_.setFillColor(sf::Color(0, 0, 0, 0));
            shape_.setOutlineThickness(0.f);
        } else {
            shape_.setFillColor(normalColor_);
            shape_.setOutlineThickness(2.f);
        }
    }
    void handleEvent(const sf::Event& event, const sf::RenderWindow& /*window*/) {
        if (!enabled_) return;
        if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
            sf::Vector2f mousePos(static_cast<float>(moved->position.x), static_cast<float>(moved->position.y));
            hovered_ = shape_.getGlobalBounds().contains(mousePos);
            if (!transparent_) shape_.setFillColor(hovered_ ? hoverColor_ : normalColor_);
        } else if (const auto* released = event.getIf<sf::Event::MouseButtonReleased>()) {
            if (released->button == sf::Mouse::Button::Left) {
                sf::Vector2f mousePos(static_cast<float>(released->position.x), static_cast<float>(released->position.y));
                if (shape_.getGlobalBounds().contains(mousePos) && onClick) onClick();
            }
        }
    }
    void render(sf::RenderWindow& window) {
        window.draw(shape_);
        if (!text_.getString().isEmpty()) window.draw(text_);
    }
    std::function<void()> onClick;

private:
    void centerText() {
        unsigned int size = text_.getCharacterSize();
        while (size > 12 && text_.getLocalBounds().size.x > size_.x - 18.f) {
            text_.setCharacterSize(--size);
        }
        sf::FloatRect bounds = text_.getLocalBounds();
        text_.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f));
        text_.setPosition(sf::Vector2f(shape_.getPosition().x + size_.x / 2.f, shape_.getPosition().y + size_.y / 2.f));
    }
    sf::RectangleShape shape_;
    sf::Text text_;
    sf::Vector2f size_;
    bool hovered_ = false;
    bool enabled_ = true;
    bool transparent_ = false;
    sf::Color normalColor_{40, 35, 30, 200};
    sf::Color hoverColor_{70, 60, 45, 220};
    sf::Color disabledColor_{20, 20, 20, 150};
};
} // namespace unmatched::gfx
