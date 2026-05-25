#pragma once

#include <cstdint>

struct SDL_Renderer;

/**
 * API d'affichage compatible Intellar-Engine (ILI9341 paysage 320×240).
 * Implémentation PC : SDL2 — voir src/Drivers/LCD.cpp
 */
namespace Drivers {

#define TFT_BLACK    0x0000
#define TFT_NAVY     0x000F
#define TFT_MAROON   0x7800
#define TFT_DARKGREY 0x7BEF
#define TFT_BLUE     0x001F
#define TFT_GREEN    0x07E0
#define TFT_CYAN     0x07FF
#define TFT_RED      0xF800
#define TFT_MAGENTA  0xF81F
#define TFT_YELLOW   0xFFE0
#define TFT_WHITE    0xFFFF
#define TFT_ORANGE   0xFDA0
#define TFT_PINK     0xFC9F

enum class DisplayIndex { LEFT, RIGHT };

constexpr int kScreenWidth  = 320;
constexpr int kScreenHeight = 240;
constexpr int kSpriteSize   = 240;
constexpr int kSpriteOffsetX = (kScreenWidth - kSpriteSize) / 2;  // 40

void initLCD(uint8_t cs, uint8_t dc, uint8_t rst, uint8_t led);
void clearLCD();
void drawTouchMarker(int x, int y);
void displayTouchCoords(int x, int y);
void setAnimation(const char* filename, DisplayIndex display = DisplayIndex::LEFT);
void updateLCD();
void showCatFace(int leftIndex, int rightIndex);
void pushVideo565(DisplayIndex which, const uint16_t* rgb565);
/** Framebuffer plein écran 320×240 RGB565 (MJPEG, etc.). */
void pushScreen565(const uint16_t* rgb565);
void loadRobotEyeRes(const char* filename);
void showRobotEyes(float normX, float normY, const uint16_t* grid = nullptr);
bool isRobotEyeResourceReady();
bool haveCatStripAtlas();
/** Nombre de colonnes 240 px dans le strip chargé (0 si aucun atlas). */
int stripColumnCount(DisplayIndex display = DisplayIndex::LEFT);
void reportTimings();

bool tftTouchSubsystemReady();
void drawTftTouchFeedback();
bool getIli9341TouchScreenPos(int16_t* outX, int16_t* outY);

/** Traite clavier / souris SDL ; retourne false si l'utilisateur ferme la fenêtre. */
bool pumpEvents();

/** Dessin par-dessus la fenêtre SDL après le framebuffer (simulateur, optionnel). */
using SimOverlayDrawFn = void (*)(::SDL_Renderer* renderer, int windowScale, void* userdata);
void setSimOverlayDraw(SimOverlayDrawFn fn, void* userdata = nullptr);

/** Répertoire des assets (défaut : ./data). */
void setDataDirectory(const char* path);
const char* getDataDirectory();

}  // namespace Drivers
