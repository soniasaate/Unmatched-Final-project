cmake_minimum_required(VERSION 3.20)
project(UnmatchedTui VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(unmatched_tui
    # src رو اضافه میکنیم
)

target_include_directories(unmatched_tui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)