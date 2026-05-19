cmake_minimum_required(VERSION 3.16)
project(IntellarEngineSimulator VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SDL2 (téléchargé au premier configure — pas d'install système requise)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-2.28.5
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(SDL2)

add_library(engine_sim_lcd STATIC src/Drivers/LCD.cpp)
target_include_directories(engine_sim_lcd PUBLIC include)
target_compile_definitions(engine_sim_lcd PUBLIC SDL_MAIN_HANDLED)
target_link_libraries(engine_sim_lcd PUBLIC SDL2::SDL2)

add_executable(engine_sim_demo sim/main.cpp)
target_include_directories(engine_sim_demo PRIVATE include)
target_compile_definitions(engine_sim_demo PRIVATE SDL_MAIN_HANDLED)
target_link_libraries(engine_sim_demo PRIVATE engine_sim_lcd SDL2::SDL2)

# Copier le répertoire data/ à côté de l'exécutable (optionnel)
add_custom_command(
    TARGET engine_sim_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:engine_sim_demo>/data
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> $<TARGET_FILE_DIR:engine_sim_demo>
    COMMENT "Deploy SDL2.dll and data/ beside demo executable"
)
