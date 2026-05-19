#!/usr/bin/env python3
"""Génère data/demo_strip.bin — strip 5×240 RGB565 pour tests simulateur."""

from pathlib import Path

W_FACE = 240
N_FACES = 5
H = 240
COLORS = [
    (255, 80, 120),
    (80, 200, 255),
    (120, 255, 140),
    (255, 200, 80),
    (180, 120, 255),
]
WHITE = ((0xFF, 0xFF, 0xFF),)


def rgb565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def main() -> None:
    out = Path(__file__).resolve().parents[1] / "data" / "demo_strip.bin"
    out.parent.mkdir(parents=True, exist_ok=True)
    strip_w = W_FACE * N_FACES
    buf = bytearray(strip_w * H * 2)
    for y in range(H):
        for face, (r, g, b) in enumerate(COLORS):
            c = rgb565(r, g, b)
            for x in range(W_FACE):
                border = x < 4 or x >= W_FACE - 4 or y < 4 or y >= H - 4
                pix = rgb565(255, 255, 255) if border else c
                off = (y * strip_w + face * W_FACE + x) * 2
                buf[off] = pix & 0xFF
                buf[off + 1] = (pix >> 8) & 0xFF
    out.write_bytes(buf)
    print(f"Wrote {out} ({len(buf)} bytes)")


if __name__ == "__main__":
    main()
