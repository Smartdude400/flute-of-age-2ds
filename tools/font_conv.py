"""Convert the debug-print font (sGfxPrintFontData in the ROM's `code` file, the table defined in
src/libu64/gfxprint.c) into a PICA200 texture (.ptex).

Original layout (read from GfxPrint_Setup / GfxPrint_PrintCharImpl): a 16x256 CI4 sheet; each
4-bit texel packs one pixel of 4 different glyphs, one per bit-plane. The bit-plane is chosen
by the TLUT bank = (c & 3). Glyph c's 8x8 cell sits at x = (c & 4) ? 8 : 0, y = (c >> 3) * 8.
Output: 128x128 RGBA8 texture (glyph c at cell (c % 16, c // 16)), transparent background,
white glyph pixels (coloured by the vertex colour at draw time).
"""
import os, re, sys
sys.path.insert(0, os.path.dirname(__file__))
import pica_tex

HERE = os.path.dirname(os.path.abspath(__file__))
from romfs import RomFS  # noqa: E402

FONT_OFFSET = 0xF3B78  # sGfxPrintFontData in `code` (NTSC 1.0), 16x256 4-bit sheet = 2048 bytes
data = list(RomFS("ntsc-1.0").get("code")[FONT_OFFSET:FONT_OFFSET + 16 * 256 // 2])
assert len(data) == 16 * 256 // 2, len(data)

def sheet_texel(x, y):
    b = data[(y * 16 + x) // 2]
    return (b >> 4) if (x & 1) == 0 else (b & 0xF)

W = H = 128
idx = [0] * (W * H)
for c in range(256):
    sx = 8 if (c & 4) else 0
    sy = (c >> 3) * 8
    plane = c & 3
    dx, dy = (c % 16) * 8, (c // 16) * 8
    for y in range(8):
        for x in range(8):
            if (sheet_texel(sx + x, sy + y) >> plane) & 1:
                idx[(dy + y) * W + dx + x] = 1
px = [(255, 255, 255, 255) if i else (0, 0, 0, 0) for i in idx]
out = pica_tex.ptex(px, W, H, pica_tex.GPU_RGBA8)
dst = os.path.join(HERE, "..", "romfs", "font")
os.makedirs(dst, exist_ok=True)
open(os.path.join(dst, "gfxprint.ptex"), "wb").write(out)
print("font: wrote", len(out), "bytes")
