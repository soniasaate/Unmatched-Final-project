cmake_minimum_required(VERSION 3.20)
project(Unmatched VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


add_compile_definitions(_UNICODE UNICODE)


include(FetchContent)

FetchContent_Declare(
    ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
    GIT_TAG v5.0.0
)
FetchContent_MakeAvailable(ftxui)


FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.0
)
FetchContent_MakeAvailable(SFML)

set(GAME_LOGIC_SOURCES
    src/Board.cpp
    src/Card.cpp
    src/Factories.cpp
    src/Fighter.cpp
    src/FighterDefinition.cpp
    src/GameController.cpp
    src/Player.cpp
    src/Space.cpp
    src/Dracula.cpp
    src/Sherlock.cpp
    src/InvisibleMan.cpp
    src/Serialization.cpp
)


set(GRAPHICAL_SOURCES
    src/graphics/MainMenuScreen.cpp
    src/graphics/AgeSetupScreen.cpp
    src/graphics/FighterSelectScreen.cpp
    src/graphics/StartSelectScreen.cpp
    src/graphics/GameScreen.cpp
)


set(COMMON_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/unmatched
    ${CMAKE_CURRENT_SOURCE_DIR}/include/unmatched/graphics
)


add_executable(unmatched_tui
    ${GAME_LOGIC_SOURCES}
    src/main.cpp
    src/TuiApp.cpp
)

target_include_directories(unmatched_tui PRIVATE
    ${COMMON_INCLUDE_DIRS}
    ${json_SOURCE_DIR}/include
)

target_link_libraries(unmatched_tui PRIVATE
    ftxui::screen
    ftxui::dom
    ftxui::component
)

if(MSVC)
    target_compile_options(unmatched_tui PRIVATE
        /W4
        /permissive-
    )
else()
    target_compile_options(unmatched_tui PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()


add_executable(unmatched_graphical
    ${GAME_LOGIC_SOURCES}
    ${GRAPHICAL_SOURCES}
    src/main_graphical.cpp
)

target_include_directories(unmatched_graphical PRIVATE
    ${COMMON_INCLUDE_DIRS}
    ${SFML_INCLUDE_DIR}
    ${json_SOURCE_DIR}/include
)

target_link_libraries(unmatched_graphical PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
)

if(MSVC)
    target_compile_options(unmatched_graphical PRIVATE
        /W4
        /permissive-
    )
else()
    target_compile_options(unmatched_graphical PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()


add_custom_command(TARGET unmatched_graphical POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:unmatched_graphical>/assets
    COMMENT "Copying assets to output directory"
)


install(TARGETS
    unmatched_tui
    unmatched_graphical
    RUNTIME DESTINATION bin
)