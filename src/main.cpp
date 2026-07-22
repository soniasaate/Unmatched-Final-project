#include "unmatched/TuiApp.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include <exception>
#include <iostream>

int main() {
    try {
        auto screen = ftxui::ScreenInteractive::Fullscreen();
        auto app = ftxui::Make<unmatched::TuiApp>(screen);
        screen.Loop(app);
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "Fatal error: " << exception.what() << '\n';
        return 1;
    }
}
