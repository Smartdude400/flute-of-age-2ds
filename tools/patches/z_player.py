"""Hand adaptation of code/z_player.c (the player actor, ovl_player_actor) after import_source.py.

  - display-list pointers (Gfx*, Gfx**) are asset handles (DListRef) in the native code;
  - get-item object load (async ROM DMA + message queue into giObjectSegment) -> the converted object is
    placed in the same giObjectSegment buffer (or the linear heap when larger) and reported loaded through
    giObjectLoadPending, polled by Player_DrawGetItem (z_player_lib.c): same one-update model as rooms /
    objects (assumptions A2/A3);
  - damage flicker fog (Gfx_SetFog2 / Play_SetFog into the opaque list) and the cull-mode clears of both
    lists -> renderer calls with their original buffers.
usage: python tools/patches/z_player.py"""
import os
import re

P = os.path.join(os.path.dirname(__file__), "..", "..", "source", "game", "code", "z_player.c")
s = open(P, encoding="utf-8").read()


def rep(old, new, count=1):
    global s
    if old not in s:
        raise SystemExit("z_player patch: not found: " + old[:70])
    s = s.replace(old, new, count)


s = re.sub(r"\bGfx\*\*", "DListRef*", s)
s = re.sub(r"\bGfx\* (\w+)", r"DListRef \1", s)
s = re.sub(r"\bstatic Gfx\* ", "static DListRef ", s)

rep("""        this->giObjectLoading = true;
        osCreateMesgQueue(&this->giObjectLoadQueue, &this->giObjectLoadMsg, 1);
""", """        this->giObjectLoading = true;
""")
s = re.sub(r"DMA_REQUEST_ASYNC\(&this->giObjectDmaRequest, this->giObjectSegment, gObjectTable\[objectId\]\.vromStart, size, 0,\s*"
           r"&this->giObjectLoadQueue, NULL, \"\.\./z_player\.c\", 9099\);",
           "PlayerNative_LoadGetItemObject(this, gObjectTable[objectId].vromStart, size);", s)

rep("""            POLY_OPA_DISP = Gfx_SetFog2(POLY_OPA_DISP, 255, 0, 0, 0, 0,
                                        4000 - (s32)(Math_CosS(this->damageFlickerAnimCounter * 256) * 2000.0f));""",
    """            GFX_TO(OPA, Gfx3D_SetFog(255, 0, 0, 0, 0,
                                     4000 - (s32)(Math_CosS(this->damageFlickerAnimCounter * 256) * 2000.0f)));""")
rep("""        gSPClearGeometryMode(POLY_OPA_DISP++, G_CULL_BOTH);
        gSPClearGeometryMode(POLY_XLU_DISP++, G_CULL_BOTH);""",
    """        GFX_TO(OPA, Gfx3D_GeometryMode(GEOM_CULL_FRONT | GEOM_CULL_BACK, 0));
        GFX_TO(XLU, Gfx3D_GeometryMode(GEOM_CULL_FRONT | GEOM_CULL_BACK, 0));""")
rep("""            POLY_OPA_DISP = Play_SetFog(play, POLY_OPA_DISP);""", """            GFX_TO(OPA, Play_SetFog(play));""")
# bunny hood ears: two matrices in segment 0x0B (the mask list addresses them by offset)
rep("""        Mtx* bunnyEarMtx = GRAPH_ALLOC(play->state.gfxCtx, 2 * sizeof(Mtx));""",
    """        MtxF* bunnyEarMtx = Gfx3D_FrameAlloc(2 * sizeof(MtxF)); // native: read by the renderer at replay""")
rep("""            GFX_TO(OPA, Gfx3D_SetSegment(0x0B, (const void*)(bunnyEarMtx)));""",
    """            GFX_TO(OPA, Gfx3D_SetSegmentMtxArray(0x0B, bunnyEarMtx, 2));""")
s = re.sub(r"MATRIX_TO_MTX\((bunnyEarMtx(?:\+\+)?), \"\.\./z_player\.c\", \d+\);", r"Matrix_Get(\1);", s)
if '#include "engine/player_native.h"' not in s:
    s = s.replace('#include "engine/gfx3d.h"\n', '#include "engine/gfx3d.h"\n#include "engine/player_native.h"\n', 1)
open(P, "w", encoding="utf-8", newline="\n").write(s)
print("patched z_player.c")
