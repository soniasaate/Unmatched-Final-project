cmake_minimum_required(VERSION 3.20)
project(UnmatchedTui VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
    GIT_TAG v5.0.0
)
FetchContent_MakeAvailable(ftxui)

add_executable(unmatched_tui
    src/Board.cpp
    src/Card.cpp
    src/Factories.cpp
    src/Fighter.cpp
    src/FighterDefinition.cpp
    src/GameController.cpp
    src/main.cpp
    src/Player.cpp
    src/Space.cpp
    src/TuiApp.cpp
)

target_include_directories(unmatched_tui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(unmatched_tui PRIVATE ftxui::screen ftxui::dom ftxui::component)

if(MSVC)
    target_compile_options(unmatched_tui PRIVATE /W4 /permissive-)
else()
    target_compile_options(unmatched_tui PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS unmatched_tui RUNTIME DESTINATION bin)