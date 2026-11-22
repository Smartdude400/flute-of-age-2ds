"""Regenerate source/game/code/z_vr_box.c from the original (skybox setup and face geometry).

Kept as game logic: Skybox_Init, Skybox_Setup (skybox choice from the time of day, the ORIGINAL game-arena
allocations of the texture/palette buffers), Skybox_Calculate*/CalculateFace* (vertex grid, texture
coordinates, quad order). Translated:
  - DMA_REQUEST_SYNC(dst, vrom, size)      -> RomBind_Set(dst, vrom, size): the converted sky textures
    (tools/build_skybox.py) are found by the vrom bound to the buffer at draw time;
  - the per-face display lists built into dListBuf -> native quad lists in the same buffer
    (engine/skybox_native.h: SkyFace_Begin / _Vertex / _Quad / _End); tile loads are not needed because
    each face is one whole texture, sampled at the same absolute texture coordinates; the vertex arrays
    keep the original 16-byte layout, which is also the renderer's vertex layout.
usage: python tools/patches/z_vr_box.py"""
import os
import re
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
from rewrite_calls import find_calls, split_args  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "..", "..", "..", "flute-of-age", "src", "code", "z_vr_box.c")
DST = os.path.join(HERE, "..", "..", "source", "game", "code", "z_vr_box.c")

s = open(SRC, encoding="utf-8").read()
s = s.replace('#include "ultra64.h"\n#include <assert.h>\n#include <stddef.h>\n#include <stdint.h>\n',
              '#include "engine/rom_bind.h"\n#include "engine/skybox_native.h"\n#include "game_types.h"\n')
s = s.replace('#include "dma.h"\n', "")
s = "// adapted from flute-of-age/src/code/z_vr_box.c (patch script: tools/patches/z_vr_box.py)\n" + s


def rewrite_calls(text, name, fn):
    out, last = [], 0
    for start, end, args in list(find_calls(text, name)):
        out.append(text[last:start])
        out.append(fn(split_args(args)))
        last = end
    out.append(text[last:])
    return "".join(out)


s = rewrite_calls(s, "DMA_REQUEST_SYNC", lambda a: f"RomBind_Set({a[0]}, {' '.join(a[1].split())}, {a[2]})")
s = re.sub(r"\bVtx\* roomVtx", "SkyVtx* roomVtx", s)
s = re.sub(r"skyboxCtx->gfx = skyboxCtx->dListBuf\[([^\]]+)\];", r"list = SkyFace_Begin(skyboxCtx, \1, faceNum);", s)
s = re.sub(r"skyboxCtx->gfx = &skyboxCtx->dListBuf\[([^\]]+)\]\[0\];", r"list = SkyFace_Begin(skyboxCtx, \1, faceNum);", s)
s = rewrite_calls(s, "gSPVertex", lambda a: f"SkyFace_Vertex(list, {a[1].replace('&roomVtx[', '').rstrip(']')})")
s = rewrite_calls(s, "gSPCullDisplayList", lambda a: "(void)0 /* native: whole-face draw, no cull list */")
s = rewrite_calls(s, "gDPLoadTextureTile", lambda a: "(void)0 /* native: whole-face texture */")
s = rewrite_calls(s, "gDPLoadMultiTile", lambda a: "(void)0 /* native: whole-face texture */")
s = rewrite_calls(s, "gSP1Quadrangle", lambda a: f"SkyFace_Quad(list, {a[1]}, {a[2]}, {a[3]}, {a[4]}, {a[5]})")
s = rewrite_calls(s, "gSPEndDisplayList", lambda a: "SkyFace_End(list)")
# the face builders declare the list cursor
s = re.sub(r"(s32 Skybox_CalculateFace(?:256|128)\([^{]*\{\n)", r"\1    SkyFaceList* list;\n", s)
s = s.replace("#define TC(x) ((s16)((x)*32))", "#define TC(x) ((s16)((x)*32))\n")
# original sizes kept: 8-byte display-list words, 16-byte vertices (same size as SkyVtx)
s = s.replace("sizeof(Gfx)", "8 /* original display-list command size */").replace("sizeof(Vtx)", "sizeof(SkyVtx)")
open(DST, "w", encoding="utf-8", newline="\n").write(s)
left = [m for m in re.findall(r"\bg[sDS]P\w+|\bgfx\b", s)]
print("z_vr_box.c regenerated; remaining display-list tokens:", sorted(set(left)))
