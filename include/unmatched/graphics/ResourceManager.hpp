#pragma once
#include <SFML/Graphics.hpp>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

namespace unmatched::gfx {
class ResourceManager {
public:
    sf::Texture& getTexture(const std::string& path) {
        auto it = textures_.find(path);
        if (it != textures_.end()) return *it->second;
        auto texture = std::make_unique<sf::Texture>();
        if (!texture->loadFromFile(path)) throw std::runtime_error("Failed to load texture: " + path);
        sf::Texture& ref = *texture;
        textures_[path] = std::move(texture);
        return ref;
    }
    sf::Font& getFont(const std::string& path) {
        auto it = fonts_.find(path);
        if (it != fonts_.end()) return *it->second;
        auto font = std::make_unique<sf::Font>();
        if (!font->openFromFile(path)) throw std::runtime_error("Failed to load font: " + path);
        sf::Font& ref = *font;
        fonts_[path] = std::move(font);
        return ref;
    }
private:
    std::map<std::string, std::unique_ptr<sf::Texture>> textures_;
    std::map<std::string, std::unique_ptr<sf::Font>> fonts_;
};
} // namespace unmatched::gfx