#include "unmatched/graphics/Application.hpp"
#include "unmatched/graphics/MainMenuScreen.hpp"

#include <exception>
#include <iostream>

int main() {
    try {
        unmatched::gfx::Application app;
        app.setScreen(std::make_unique<unmatched::gfx::MainMenuScreen>(app));
        app.run();
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "Fatal error: " << exception.what() << '\n';
        return 1;
    }
}