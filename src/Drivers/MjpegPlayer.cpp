#include "Drivers/Mjpeg.h"
#include "Drivers/LCD.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <algorithm>
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
constexpr int    kNominalFps    = 15;
constexpr int    kTailFrames    = 4;   // last N frames looped as idle micro-loop

std::ifstream g_file;
std::vector<uint8_t>  g_jpegBuf;
std::vector<uint16_t> g_screen565;
bool           g_playing       = false;
bool           g_loop          = false;
bool           g_finishedFlag  = false;
bool           g_tailLooping   = false;  // looping tail after story clip ends
std::streampos g_tailLoopPos   = 0;      // file offset of tail loop start
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

bool decodeJpegToScreen565(const uint8_t* data, size_t len) {
    int w = 0;
    int h = 0;
    int comp = 0;
    unsigned char* rgb =
        stbi_load_from_memory(data, static_cast<int>(len), &w, &h, &comp, 3);
    if (!rgb || w <= 0 || h <= 0) {
        if (rgb) stbi_image_free(rgb);
        return false;
    }

    const size_t outCount =
        static_cast<size_t>(kScreenWidth) * static_cast<size_t>(kScreenHeight);
    g_screen565.assign(outCount, 0);

    if (w == kScreenWidth && h == kScreenHeight) {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                const size_t si = static_cast<size_t>(y * w + x) * 3;
                g_screen565[static_cast<size_t>(y * kScreenWidth + x)] =
                    rgb888To565(rgb[si], rgb[si + 1], rgb[si + 2]);
            }
        }
    } else {
        for (int dy = 0; dy < kScreenHeight; dy++) {
            const int sy = std::min(h - 1, dy * h / kScreenHeight);
            for (int dx = 0; dx < kScreenWidth; dx++) {
                const int sx = std::min(w - 1, dx * w / kScreenWidth);
                const size_t si = static_cast<size_t>(sy * w + sx) * 3;
                g_screen565[static_cast<size_t>(dy * kScreenWidth + dx)] =
                    rgb888To565(rgb[si], rgb[si + 1], rgb[si + 2]);
            }
        }
        std::fprintf(stderr,
                     "Mjpeg: frame %dx%d redimensionnee en %dx%d\n",
                     w,
                     h,
                     kScreenWidth,
                     kScreenHeight);
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

// Scans the open file and returns the byte offset where the tail loop begins
// (i.e. the start of the last kTailFrames frames).  Leaves the file rewound.
std::streampos findTailLoopPos(std::ifstream& f) {
    std::vector<std::streampos> offsets;
    offsets.reserve(256);

    f.clear();
    f.seekg(0);
    while (f.good()) {
        const std::streampos pos = f.tellg();
        const size_t n = readOneJpeg(f, g_jpegBuf.data(), g_jpegBuf.size());
        if (n == 0) break;
        offsets.push_back(pos);
    }

    f.clear();
    f.seekg(0);

    if (offsets.empty()) return std::streampos(0);
    const int startIdx = std::max(0, static_cast<int>(offsets.size()) - kTailFrames);
    return offsets[static_cast<size_t>(startIdx)];
}

}  // namespace

bool isPlaying() { return g_playing; }

bool isActive() { return g_playing || g_tailLooping; }

bool loopEnabled() { return g_loop; }

void setLoop(bool enabled) { g_loop = enabled; }

bool consumeFinished() {
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

    // Pre-scan to find the tail-loop start offset (used for non-looping clips).
    g_tailLoopPos = loop ? std::streampos(0) : findTailLoopPos(g_file);

    g_loop         = loop;
    g_tailLooping  = false;
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
    g_playing      = false;
    g_tailLooping  = false;
    g_tailLoopPos  = 0;
    g_lastFrameMs  = 0;
    g_openPath.clear();
}

void service(uint32_t nowMs) {
    if (!g_playing && !g_tailLooping) return;
    if (!g_file.is_open()) return;

    const uint32_t interval = frameIntervalMs();
    if (g_lastFrameMs != 0 && (nowMs - g_lastFrameMs) < interval) return;

    size_t n = readOneJpeg(g_file, g_jpegBuf.data(), g_jpegBuf.size());

    if (n == 0) {
        if (g_loop) {
            // Full-file loop (anim6 sleeping loop).
            g_file.clear();
            g_file.seekg(0);
            n = readOneJpeg(g_file, g_jpegBuf.data(), g_jpegBuf.size());
        } else if (g_playing) {
            // First EOF on a one-shot clip: notify game logic, start tail loop.
            g_finishedFlag = true;
            g_playing      = false;
            g_tailLooping  = true;
            g_file.clear();
            g_file.seekg(g_tailLoopPos);
            n = readOneJpeg(g_file, g_jpegBuf.data(), g_jpegBuf.size());
        } else if (g_tailLooping) {
            // EOF during tail loop: wrap back to tail start.
            g_file.clear();
            g_file.seekg(g_tailLoopPos);
            n = readOneJpeg(g_file, g_jpegBuf.data(), g_jpegBuf.size());
        }

        if (n == 0) {
            std::fprintf(stderr, "Mjpeg: fin flux %s\n", g_openPath.c_str());
            g_tailLooping = false;
            closeFile();
            return;
        }
    }

    if (!decodeJpegToScreen565(g_jpegBuf.data(), n)) {
        std::fprintf(stderr, "Mjpeg: decode JPEG echoue, frame saute\n");
        g_lastFrameMs = nowMs;
        return;
    }

    pushScreen565(g_screen565.data());
    g_lastFrameMs = nowMs;
}

}  // namespace Mjpeg
}  // namespace Drivers
