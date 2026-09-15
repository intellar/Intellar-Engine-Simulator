#include "Drivers/Mjpeg.h"
#include "Drivers/LCD.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace Drivers {
namespace Mjpeg {

namespace {

constexpr size_t kJpegMaxBytes  = 512 * 1024;
constexpr int    kNominalFps    = 15;   // aligné sur les clips (ffmpeg -r 15)
constexpr int    kCanvas        = 240;  // stage 240x240 (identique carte ILI9341)

std::ifstream g_file;
std::vector<uint8_t>  g_jpegBuf;
std::vector<uint16_t> g_canvas565;      // stage 240x240 RGB565
bool           g_playing       = false;
bool           g_loop          = false;
bool           g_finishedFlag  = false;
uint32_t       g_lastFrameMs   = 0;
std::string    g_openPath;

std::string resolveMediaPath(const char* filename) {
    if (!filename || !filename[0]) return {};
    fs::path p(filename);
    if (p.is_absolute() && fs::exists(p)) return p.string();
    const fs::path rel = fs::path(Drivers::getDataDirectory()) / p.filename();
    if (fs::exists(rel)) return rel.string();
    if (fs::exists(p)) return p.string();
    return rel.string();
}

size_t readOneJpeg(std::ifstream& f, uint8_t* dst, size_t cap) {
    int c = 0;
    while (f.good()) {
        c = f.get();
        if (c < 0) return 0;
        if (c != 0xFF) continue;
        c = f.get();
        if (c < 0) return 0;
        if (c == 0xD8) {
            if (cap < 2) return 0;
            dst[0]     = 0xFF;
            dst[1]     = 0xD8;
            size_t len = 2;
            int    prev = 0xD8;
            while (len < cap && f.good()) {
                c = f.get();
                if (c < 0) return 0;
                dst[len++] = static_cast<uint8_t>(c);
                if (prev == 0xFF && c == 0xD9) return len;
                prev = c;
            }
            return 0;
        }
    }
    return 0;
}

uint16_t rgb888To565(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

/**
 * Décode un JPEG et le réduit dans le canvas 240x240 : proportions préservées,
 * bandes noires si le cadre n'est pas carré — soit exactement le comportement
 * de la carte (scaleContain240 de Drivers::Mjpeg dans l'Engine).
 */
bool decodeJpegToCanvas565(const uint8_t* data, size_t len) {
    int w = 0;
    int h = 0;
    int comp = 0;
    unsigned char* rgb =
        stbi_load_from_memory(data, static_cast<int>(len), &w, &h, &comp, 3);
    if (!rgb || w <= 0 || h <= 0) {
        if (rgb) stbi_image_free(rgb);
        return false;
    }

    const size_t canvasElts = static_cast<size_t>(kCanvas) * static_cast<size_t>(kCanvas);
    if (g_canvas565.size() < canvasElts) {
        g_canvas565.resize(canvasElts);
    }
    std::fill(g_canvas565.begin(), g_canvas565.end(), static_cast<uint16_t>(0));

    const float sc = std::min(static_cast<float>(kCanvas - 1) / static_cast<float>(w),
                              static_cast<float>(kCanvas - 1) / static_cast<float>(h));
    const int nw   = std::max(1, static_cast<int>(std::lround(static_cast<double>(w) * sc)));
    const int nh   = std::max(1, static_cast<int>(std::lround(static_cast<double>(h) * sc)));
    const int offx = (kCanvas - nw) / 2;
    const int offy = (kCanvas - nh) / 2;

    for (int dy = 0; dy < nh; dy++) {
        const int sy =
            std::min(h - 1, static_cast<int>((static_cast<int64_t>(dy) * h + nh / 2) / nh));
        for (int dx = 0; dx < nw; dx++) {
            const int sx =
                std::min(w - 1, static_cast<int>((static_cast<int64_t>(dx) * w + nw / 2) / nw));
            const size_t si = static_cast<size_t>(sy * w + sx) * 3;
            g_canvas565[static_cast<size_t>(offy + dy) * kCanvas + static_cast<size_t>(offx + dx)] =
                rgb888To565(rgb[si], rgb[si + 1], rgb[si + 2]);
        }
    }

    stbi_image_free(rgb);
    return true;
}

void closeFile() {
    if (g_file.is_open()) {
        g_file.close();
    }
}

uint32_t frameIntervalMs() {
    return static_cast<uint32_t>((1000 + kNominalFps - 1) / kNominalFps);
}

}  // namespace

bool isPlaying() { return g_playing; }

bool loopEnabled() { return g_loop; }

void setLoop(bool enabled) { g_loop = enabled; }

bool takeFinished() {
    if (!g_finishedFlag) return false;
    g_finishedFlag = false;
    return true;
}

bool playFile(const char* path, bool loop) {
    const std::string resolved = resolveMediaPath(path);
    if (!fs::exists(resolved)) {
        std::fprintf(stderr, "Mjpeg::playFile introuvable: %s\n", resolved.c_str());
        return false;
    }

    stop();
    g_file.open(resolved, std::ios::binary);
    if (!g_file) {
        std::fprintf(stderr, "Mjpeg::playFile open fail: %s\n", resolved.c_str());
        return false;
    }

    if (g_jpegBuf.size() < kJpegMaxBytes) {
        g_jpegBuf.resize(kJpegMaxBytes);
    }

    g_loop         = loop;
    g_finishedFlag = false;
    g_openPath     = resolved;
    g_playing      = true;
    g_lastFrameMs  = 0;
    std::fprintf(stderr, "Mjpeg::play %s loop=%d\n", resolved.c_str(), loop ? 1 : 0);
    return true;
}

bool playByName(const char* basename, bool loop) {
    if (!basename || !basename[0]) return false;
    return playFile(basename, loop);
}

void stop() {
    closeFile();
    g_playing     = false;
    g_lastFrameMs = 0;
    g_openPath.clear();
}

void service(uint32_t nowMs) {
    if (!g_playing) return;
    if (!g_file.is_open()) return;

    const uint32_t interval = frameIntervalMs();
    if (g_lastFrameMs != 0 && (nowMs - g_lastFrameMs) < interval) return;

    size_t n = readOneJpeg(g_file, g_jpegBuf.data(), g_jpegBuf.size());

    if (n == 0) {
        if (g_loop) {
            // Reboucle plein fichier (ex. anim6 : boucle de sommeil).
            g_file.clear();
            g_file.seekg(0);
            n = readOneJpeg(g_file, g_jpegBuf.data(), g_jpegBuf.size());
        } else {
            // Fin naturelle d'un clip one-shot : signal net via takeFinished(),
            // l'image reste figée sur la dernière frame (comme sur la carte).
            std::fprintf(stderr, "Mjpeg: fin clip %s\n", g_openPath.c_str());
            g_finishedFlag = true;
            g_playing      = false;
            closeFile();
            return;
        }

        if (n == 0) {
            std::fprintf(stderr, "Mjpeg: fin flux %s\n", g_openPath.c_str());
            g_playing = false;
            closeFile();
            return;
        }
    }

    if (!decodeJpegToCanvas565(g_jpegBuf.data(), n)) {
        std::fprintf(stderr, "Mjpeg: decode JPEG echoue, frame saute\n");
        g_lastFrameMs = nowMs;
        return;
    }

    pushVideo565(DisplayIndex::LEFT, g_canvas565.data());
    g_lastFrameMs = nowMs;
}

}  // namespace Mjpeg
}  // namespace Drivers
