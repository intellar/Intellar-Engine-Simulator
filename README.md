# Intellar-Engine-Simulator

Simulateur d’affichage PC pour [Intellar-Engine](https://github.com/Intellar-Robotics/Intellar-Engine) : fenêtre **320×240** (ILI9341 paysage) avec une API **`Drivers::LCD`** compatible.

Licence : **MIT** — le firmware Engine reste sous **CC BY-NC 4.0**.

## Objectif

Développer [Intellar-CyberAnima](https://github.com/Intellar-Robotics/Intellar-CyberAnima) et l’UI pixel art **sans flasher l’ESP32** à chaque changement.

| Dépôt | Rôle |
| :--- | :--- |
| Intellar-Engine | Firmware réel (ESP32) |
| **Intellar-Engine-Simulator** | Affichage + API LCD sur PC |
| Intellar-CyberAnima | Logique compagnon (propriétaire) |

## Run / Debug dans Cursor ou VS Code

1. Ouvrir le dossier **Intellar-Engine-Simulator** (ou le sélectionner dans le workspace multi-racine).
2. Installer l’extension **C/C++** (`ms-vscode.cpptools`) si demandée.
3. Menu **Run and Debug** (Ctrl+Shift+D) → **Simulator: Run (sans débogueur)** → F5.

La config compile via CMake puis lance l’exe avec `cwd` = racine du repo (`data/` trouvé).

> **Erreur `cppvsdbg is not supported` dans Cursor ?** Utilise **Simulator: Run (sans débogueur)** (type `node`, pas de débogueur C++). Ou : Terminal → **Run Task** → **Run: engine_sim_demo**.

> **Pourquoi pas PlatformIO Run ?** Ce dépôt se build avec **CMake**, pas `[env:sim]` (SDL2 non lié sous PIO).

## Build (recommandé — CMake + SDL2 automatique)

Prérequis : [CMake](https://cmake.org/) 3.16+ et un compilateur C++ (Visual Studio Build Tools, ou MinGW).

```powershell
cd Intellar-Engine-Simulator
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Lancer la démo (depuis la racine du repo, pour que `data/` soit trouvé) :

```powershell
.\build\engine_sim_demo.exe
# ou avec un dossier d'assets :
.\build\engine_sim_demo.exe path\to\data
```

Au premier run, un strip de test `data/demo_strip.bin` est créé si besoin.

### Contrôles démo

| Entrée | Action |
| :--- | :--- |
| Flèches | Changer la pose du strip |
| Clic souris | Marqueur tactile (debug, comme ILI9341) |
| Échap | Quitter |

## API (sous-ensemble Engine)

Headers : `include/Drivers/LCD.h`

| Fonction | Comportement simulateur |
| :--- | :--- |
| `initLCD(cs, dc, rst, led)` | Ouvre SDL 320×240 (×3 à l’écran) ; pins ignorés |
| `clearLCD()` | Fond noir |
| `setAnimation(path, display)` | Charge un `.bin` strip depuis `data/` |
| `showCatFace(left, right)` | Blit 240×240 centré (offset X=40) |
| `pushVideo565(which, buf)` | Affiche un framebuffer 240×240 |
| `getIli9341TouchScreenPos` | Position souris (miroir X+Y comme Engine) |
| `pumpEvents()` | Traite SDL ; `false` = fermeture |

Stubs (no-op) : `loadRobotEyeRes`, `showRobotEyes`, `updateLCD`, etc.

## CyberAnima (interactions sur PC)

Si **Intellar-CyberAnima** est en dépôt frère, CMake produit aussi `cyberanima_sim.exe` :

```powershell
cmake --build build --config Release
.\build\Release\cyberanima_sim.exe ..\Intellar-CyberAnima\data
```

Run and Debug → **CyberAnima: Run (simulateur)**. Voir le README CyberAnima pour les contrôles (clic = tap, touches 1–6 = états).

## PlatformIO (optionnel)

`pio run -e sim` nécessite **SDL2 installé** sur la machine (ex. MSYS2 : `pacman -S mingw-w64-ucrt-x86_64-SDL2` + flags dans `platformio.ini`). Préférez **CMake** si SDL n’est pas encore configuré.

## Workspace

```text
GitHub/
  Intellar-Engine/
  Intellar-Engine-Simulator/
  Intellar-CyberAnima/
```
