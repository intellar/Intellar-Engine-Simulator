#pragma once

/** Sous-ensemble Arduino pour compiler l'API Drivers sur PC (pas de framework ESP32). */
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>

using byte = uint8_t;

inline unsigned long millis() {
    using clock = std::chrono::steady_clock;
    static const auto t0 = clock::now();
    return static_cast<unsigned long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - t0).count());
}

inline void delay(unsigned long ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

struct SerialStub {
    template <typename... Args>
    void printf(const char* fmt, Args... args) {
        std::fprintf(stdout, fmt, args...);
    }
    void println(const char* msg) { std::fprintf(stdout, "%s\n", msg); }
};

inline SerialStub Serial;
