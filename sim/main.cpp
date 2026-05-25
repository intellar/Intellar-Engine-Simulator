/**
 * Démo Intellar-Engine-Simulator — yeux / expressions animés (ILI9341 320×240).
 *
 * Même principe que le firmware Engine : setAnimation(strip.bin) puis showCatFace(col)
 * à ~30 FPS pour faire défiler les poses du strip.
 *
 * Usage :
 *   engine_sim_demo.exe
 *   engine_sim_demo.exe data
 *   engine_sim_demo.exe data ../Intellar-Engine/assets/expression-chat-5-240x240.bin
 *
 * Contrôles :
 *   Espace     pause / reprise animation auto
 *   Flèches    pose manuelle (si pause)
 *   Clic       marqueur tactile
 *   Échap      quitter
 */
#include "Drivers/LCD.h"

#include <SDL.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace {

constexpr int    kTargetFps    = 30;
constexpr uint32_t kFrameMs    = 1000 / kTargetFps;
constexpr int      kFaceW      = 240;
constexpr int      kDemoFaces  = 5;
constexpr int      kDemoStripW = kFaceW * kDemoFaces;

uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

bool writeDemoStrip(const char* path) {
    std::vector<uint16_t> buf(static_cast<size_t>(kDemoStripW * 240));
    static const uint16_t colors[] = {
        rgb565(255, 80, 120),
        rgb565(80, 200, 255),
        rgb565(120, 255, 140),
        rgb565(255, 200, 80),
        rgb565(180, 120, 255),
    };
    for (int y = 0; y < 240; y++) {
        for (int face = 0; face < kDemoFaces; face++) {
            const uint16_t c = colors[face % 5];
            for (int x = 0; x < kFaceW; x++) {
                const bool border = (x < 4 || x >= kFaceW - 4 || y < 4 || y >= 239);
                buf[static_cast<size_t>(y * kDemoStripW + face * kFaceW + x)] =
                    border ? static_cast<uint16_t>(TFT_WHITE) : c;
            }
        }
    }
    FILE* f = std::fopen(path, "wb");
    if (!f) return false;
    const size_t n = buf.size() * sizeof(uint16_t);
    const bool ok  = std::fwrite(buf.data(), 1, n, f) == n;
    std::fclose(f);
    return ok;
}

bool fileExists(const std::string& p) {
    return !p.empty() && std::filesystem::exists(p);
}

void printHelp() {
    std::printf(
        "\n"
        "Yeux / expressions TFT (comme Engine ili9341) :\n"
        "  1. Copiez un strip Engine dans data/, ex. :\n"
        "     copy ..\\Intellar-Engine\\assets\\expression-chat-5-240x240.bin data\\\n"
        "  2. Ou passez le chemin en 2e argument.\n"
        "  3. L'animation auto fait defiler les colonnes du strip (showCatFace).\n"
        "\n"
        "Autres modes Engine (pas encore dans le simulateur) :\n"
        "  - Yeux OLED proceduraux (eye_animation) -> ecran SSD1306 separe\n"
        "  - RobotEye (image_giant.bin) -> showRobotEyes a implementer\n"
        "  - MJPEG -> cyberanima_sim touches 7-9 (anim1..3.mjpeg)\n"
        "\n");
}

}  // namespace

int main(int argc, char** argv) {
    const char* dataDir   = (argc > 1) ? argv[1] : "data";
    const char* stripArg = (argc > 2) ? argv[2] : nullptr;

    Drivers::setDataDirectory(dataDir);
    std::filesystem::create_directories(dataDir);

    std::string stripPath;
    if (stripArg && stripArg[0]) {
        stripPath = stripArg;
    } else {
        const std::string candidates[] = {
            std::string(dataDir) + "/expression-chat-5-240x240.bin",
            std::string(dataDir) + "/expression-monkey-5-240x240.bin",
            std::string(dataDir) + "/demo_strip.bin",
            "../Intellar-Engine/assets/expression-chat-5-240x240.bin",
            "../Intellar-Engine/assets/expression-monkey-5-240x240.bin",
        };
        for (const auto& c : candidates) {
            if (fileExists(c)) {
                stripPath = c;
                break;
            }
        }
    }

    Drivers::initLCD(47, 13, 14, 46);
    Drivers::clearLCD();

    if (stripPath.empty()) {
        stripPath = std::string(dataDir) + "/demo_strip.bin";
        if (!fileExists(stripPath)) writeDemoStrip(stripPath.c_str());
    }

    Drivers::setAnimation(stripPath.c_str(), Drivers::DisplayIndex::LEFT);

    if (!Drivers::haveCatStripAtlas()) {
        std::fprintf(stderr, "ERREUR: impossible de charger %s\n", stripPath.c_str());
        printHelp();
        return 1;
    }

    int numCols = Drivers::stripColumnCount();
    if (numCols < 1) numCols = kDemoFaces;

    std::printf("Strip: %s (%d pose(s))\n", stripPath.c_str(), numCols);
    printHelp();
    std::printf("Animation auto %d FPS — Espace: pause, fleches: pose manuelle\n", kTargetFps);

    int      faceIndex  = 0;
    bool     running    = true;
    bool     autoPlay   = true;
    uint32_t lastFrame  = SDL_GetTicks();
    uint32_t lastStep   = lastFrame;
    constexpr uint32_t kHoldMs = 800;  // temps par expression

    while (running) {
        const uint32_t now = SDL_GetTicks();

        if (autoPlay && (now - lastStep >= kHoldMs)) {
            lastStep    = now;
            faceIndex   = (faceIndex + 1) % numCols;
        }

        if (now - lastFrame >= kFrameMs) {
            lastFrame = now;
            Drivers::showCatFace(faceIndex, faceIndex);
        }

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE: running = false; break;
                    case SDLK_SPACE:
                        autoPlay = !autoPlay;
                        std::printf(autoPlay ? "Auto ON\n" : "Auto OFF (fleches)\n");
                        break;
                    case SDLK_RIGHT:
                    case SDLK_DOWN:
                        autoPlay  = false;
                        faceIndex = (faceIndex + 1) % numCols;
                        break;
                    case SDLK_LEFT:
                    case SDLK_UP:
                        autoPlay  = false;
                        faceIndex = (faceIndex + numCols - 1) % numCols;
                        break;
                    default: break;
                }
            }
        }
        if (!Drivers::pumpEvents()) running = false;

        SDL_Delay(1);
    }

    return 0;
}
