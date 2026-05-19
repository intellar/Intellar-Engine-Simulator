# Assets simulateur

Placez ici les strips RGB565 compatibles **Intellar-Engine** (ex. `expression-chat.bin`).

- Format habituel : hauteur **240 px**, largeur **N × 240** (souvent 1200 px = 5 poses).
- RGB565 little-endian, même convention que `Drivers::setAnimation` sur la carte.

Au premier lancement, la démo crée `demo_strip.bin` si aucun fichier n’est présent.

### Yeux / expressions animés (comme sur la carte)

Copiez un strip depuis Intellar-Engine :

```powershell
copy ..\Intellar-Engine\assets\expression-chat-5-240x240.bin data\
```

Puis lancez `engine_sim_demo.exe` : les colonnes du strip défilent automatiquement (`showCatFace`, ~30 FPS).
