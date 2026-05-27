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

add_library(engine_sim_mjpeg STATIC src/Drivers/MjpegPlayer.cpp)
target_include_directories(engine_sim_mjpeg PUBLIC include)
target_include_directories(engine_sim_mjpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
target_link_libraries(engine_sim_mjpeg PUBLIC engine_sim_lcd)

add_executable(engine_sim_demo sim/main.cpp)
target_include_directories(engine_sim_demo PRIVATE include)
target_compile_definitions(engine_sim_demo PRIVATE SDL_MAIN_HANDLED)
target_link_libraries(engine_sim_demo PRIVATE engine_sim_lcd SDL2::SDL2)

# CyberAnima — logique compagnon + affichage simulateur (dev rapide sans ESP32)
set(CYBERANIMA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Intellar-CyberAnima")
if(EXISTS "${CYBERANIMA_DIR}/src/Anima/AnimaApp.cpp")
    add_executable(cyberanima_sim
        "${CYBERANIMA_DIR}/sim/main.cpp"
        "${CYBERANIMA_DIR}/sim/EngineHostSim.cpp"
        "${CYBERANIMA_DIR}/sim/AnimaTouchBar.cpp"
        "${CYBERANIMA_DIR}/sim/AnimaPlayerSim.cpp"
        "${CYBERANIMA_DIR}/src/Anima/AnimaApp.cpp"
        "${CYBERANIMA_DIR}/src/Anima/CompanionNeeds.cpp"
        "${CYBERANIMA_DIR}/src/Anima/CompanionEvolution.cpp"
        "${CYBERANIMA_DIR}/src/Anima/ScenarioDirector.cpp"
        "${CYBERANIMA_DIR}/src/Anima/SkelLoader.cpp"
        "${CYBERANIMA_DIR}/src/Anima/SkeletalPlayer.cpp"
    )
    target_include_directories(cyberanima_sim PRIVATE
        include
        "${CYBERANIMA_DIR}/src"
        "${CYBERANIMA_DIR}/sim"
        "${CYBERANIMA_DIR}/sim/platform"
    )
    target_compile_definitions(cyberanima_sim PRIVATE CYBERANIMA_SIM SDL_MAIN_HANDLED)
    target_link_libraries(cyberanima_sim PRIVATE engine_sim_lcd engine_sim_mjpeg SDL2::SDL2)
    add_custom_command(
        TARGET cyberanima_sim POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> $<TARGET_FILE_DIR:cyberanima_sim>
        COMMENT "Deploy SDL2.dll beside cyberanima_sim"
    )
endif()

# Copier le répertoire data/ à côté de l'exécutable (optionnel)
add_custom_command(
    TARGET engine_sim_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:engine_sim_demo>/data
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> $<TARGET_FILE_DIR:engine_sim_demo>
    COMMENT "Deploy SDL2.dll and data/ beside demo executable"
)
