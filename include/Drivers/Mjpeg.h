#pragma once

#include <cstdint>

namespace Drivers {
namespace Mjpeg {

bool isPlaying();
bool loopEnabled();
/** Active/désactive la boucle explicitement (préférable à un toggle). */
void setLoop(bool enabled);
/**
 * Vrai UNE seule fois après la fin naturelle d'un clip non bouclé.
 * Drapeau consommé par l'appel ; réarmé par playFile().
 * Sémantique alignée sur Drivers::Mjpeg de la carte (Intellar-Engine).
 */
bool takeFinished();

/** Chemin absolu ou relatif au répertoire data (setDataDirectory). */
bool playFile(const char* path, bool loop = false);
/** Ex. "anim1.mjpeg" — résolu dans data/. */
bool playByName(const char* basename, bool loop = false);

void stop();

/** Cadence ~15 FPS par défaut ; appeler chaque frame depuis la boucle principale. */
void service(uint32_t nowMs);

}  // namespace Mjpeg
}  // namespace Drivers
