#pragma once

#include <cstdint>

namespace Drivers {
namespace Mjpeg {

bool isPlaying();
/** Lecture en cours ou micro-boucle de fin de clip (idle visuel). */
bool isActive();
bool loopEnabled();
void setLoop(bool enabled);

/** Vrai une fois après la fin naturelle d'un clip non bouclé ; remis à false par consumeFinished(). */
bool consumeFinished();

/** Chemin absolu ou relatif au répertoire data (setDataDirectory). */
bool playFile(const char* path, bool loop = false);
/** Ex. "anim1.mjpeg" — résolu dans data/. */
bool playByName(const char* basename, bool loop = false);

void stop();

/** Cadence ~15 FPS par défaut ; appeler chaque frame depuis la boucle principale. */
void service(uint32_t nowMs);

}  // namespace Mjpeg
}  // namespace Drivers
