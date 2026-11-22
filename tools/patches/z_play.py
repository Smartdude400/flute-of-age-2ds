"""Hand adaptation of z_play.c (the Play game state).

Play_Draw interleaves game logic (Camera_Update/Camera_Finish, Environment_UpdateSkybox and
Environment_UpdateLightningStrike [RNG], Actor_DrawAll [actor draw functions], the pause-background
and transition-tile state machines) with frame setup. Every logic statement is kept in its original
order; only the platform parts are translated:
  - gSegments[n] = OS_K0_TO_PHYSICAL(x)   -> Asset_SetSegment(n, DsObj)  (segment table = native assets)
  - Gfx_SetupFrame / Play_SetFog / list open+close -> Gfx3D_SetupFrame / Play_SetFog(this) / native draws
  - billboard Mtx for segment 1           -> Gfx3D_SetSegmentMtx(1, &billboardMtxF)
  - framebuffer copies for the pause background / transition tile -> dropped (state machine kept)
  - Play_LoadFile: same game-arena allocation; the converted file is placed in it (DsObj handle).
  - Rand_Seed(osGetTime()): deterministic native time (Runner_OsTime).
usage: python tools/patches/z_play.py"""
import os
import re
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
from cfuncs import find_func  # noqa: E402

P = os.path.join(os.path.dirname(__file__), "..", "..", "source", "game", "code", "z_play.c")
s = open(P, encoding="utf-8").read()


def rep(old, new, count=1):
    global s
    if old not in s:
        raise SystemExit("z_play patch: not found: " + old[:80])
    s = s.replace(old, new, count)


for inc in ('#include "line_numbers.h"', '#include "engine/asset.h"', '#include "engine/gfx.h"',
            '#include "engine/runner.h"', '#include "engine/object_native.h"'):
    if inc not in s:
        rep('#include "engine/gfx3d.h"\n', '#include "engine/gfx3d.h"\n' + inc + "\n")

# segment table
SEG = {
    "gSegments[4] = OS_K0_TO_PHYSICAL(this->objectCtx.slots[this->objectCtx.mainKeepSlot].segment);":
        "Asset_SetSegment(4, this->objectCtx.slots[this->objectCtx.mainKeepSlot].dsObj);",
    "gSegments[5] = OS_K0_TO_PHYSICAL(this->objectCtx.slots[this->objectCtx.subKeepSlot].segment);":
        "Asset_SetSegment(5, this->objectCtx.slots[this->objectCtx.subKeepSlot].dsObj);",
    "gSegments[2] = OS_K0_TO_PHYSICAL(this->sceneSegment);": "Asset_SetSegment(2, (DsObj*)this->sceneSegment);",
}
for k, v in SEG.items():
    s = s.replace(k, v)
s = re.sub(r"\n[ \t]*Gfx3D_SetSegment\(0x0[0245], \(const void\*\)\([^;]*\)\);", "", s)

# fog
a, b = find_func(s, "Play_SetFog")
s = s[:a] + ("void Play_SetFog(PlayState* this) {\n"
             "    Gfx3D_SetFog(this->lightCtx.fogColor[0], this->lightCtx.fogColor[1], this->lightCtx.fogColor[2], 0,\n"
             "                 this->lightCtx.fogNear, 1000);\n}\n\n") + s[b:]

rep("Rand_Seed((u32)osGetTime());", "Rand_Seed((u32)Runner_OsTime());")
rep("                        gTransitionTile.zBuffer = (u16*)gZBuffer;\n",
    "                        // native: the tile transition captures the frame natively (no z-buffer reuse)\n")

# ---- Play_Draw
a, b = find_func(s, "Play_Draw")
d = s[a:b]


def drep(old, new):
    global d
    if old not in d:
        raise SystemExit("Play_Draw patch: not found: " + old[:80])
    d = d.replace(old, new, 1)


drep("    GraphicsContext* gfxCtx = this->state.gfxCtx;\n", "    GraphicsContext* gfxCtx = this->state.gfxCtx;\n    (void)gfxCtx;\n")
drep("    Gfx_SetupFrame(gfxCtx, 0, 0, 0);", "    Gfx3D_SetupFrame(0, 0, 0);")
drep("        POLY_OPA_DISP = Play_SetFog(this, POLY_OPA_DISP);\n        POLY_XLU_DISP = Play_SetFog(this, POLY_XLU_DISP);\n",
     "        Play_SetFog(this);\n")
drep("        Matrix_MtxToMtxF(&this->view.viewing, &this->billboardMtxF);\n"
     "        Matrix_MtxToMtxF(&this->view.projection, &this->viewProjectionMtxF);\n",
     "        // native: view->viewing / view->projection are already the s15.16-quantized float matrices\n"
     "        this->billboardMtxF = this->view.viewing;\n"
     "        this->viewProjectionMtxF = this->view.projection;\n")
d = re.sub(r"        this->billboardMtx = Matrix_MtxFToMtx\(.*?\);\n\n        Gfx3D_SetSegment\(0x01, \(const void\*\)\(this->billboardMtx\)\);\n",
           "        Gfx3D_SetSegmentMtx(0x01, &this->billboardMtxF);\n", d, flags=re.S)
# cover elements: the list open/close goes, the draws stay
d = re.sub(r"            Gfx\* gfxP;\n            Gfx\* sp1CC = POLY_OPA_DISP;\n\n            gfxP = Gfx_Open\(sp1CC\);\n            Gfx3D_DrawDL\(\(u32\)\(uintptr_t\)\(gfxP\)\);\n",
           "", d)
drep("                View_ApplyTo(&view, VIEW_ALL, &gfxP);\n                this->transitionCtx.draw(&this->transitionCtx.instanceData, &gfxP);",
     "                View_Apply(&view, VIEW_ALL);\n                this->transitionCtx.draw(&this->transitionCtx.instanceData);")
drep("TransitionFade_Draw(&this->transitionFadeFlash, &gfxP);", "TransitionFade_Draw(&this->transitionFadeFlash);")
drep("VisMono_Draw(&sPlayVisMono, &gfxP);", "VisMono_Draw(&sPlayVisMono, NULL);")
d = re.sub(r"\n            gSPEndDisplayList\(gfxP\+\+\);\n            Gfx_Close\(sp1CC, gfxP\);\n            POLY_OPA_DISP = gfxP;\n", "\n", d)
drep("            Gfx* sp88 = POLY_OPA_DISP;\n\n            TransitionTile_Draw(&gTransitionTile, &sp88);\n            POLY_OPA_DISP = sp88;\n",
     "            TransitionTile_Draw(&gTransitionTile);\n")
drep("        PreRender_SetValues(&this->pauseBgPreRender, SCREEN_WIDTH, SCREEN_HEIGHT, gfxCtx->curFrameBuffer, gZBuffer);\n",
     "        // native: the pause background is captured natively; PreRender keeps its state only\n")
drep("            Sched_FlushTaskQueue();\n", "")
drep("            Gfx* gfxP = POLY_OPA_DISP;\n\n            PreRender_RestoreFramebuffer(&this->pauseBgPreRender, &gfxP);\n            POLY_OPA_DISP = gfxP;\n",
     "            PreRender_DrawSaved(&this->pauseBgPreRender);\n")
drep("            Gfx* gfxP = OVERLAY_DISP;\n\n", "")
drep("            this->pauseBgPreRender.fbuf = gfxCtx->curFrameBuffer;\n            this->pauseBgPreRender.fbufSave = (u16*)gZBuffer;\n"
     "            PreRender_SaveFramebuffer(&this->pauseBgPreRender, &gfxP);\n",
     "            PreRender_SaveFrame(&this->pauseBgPreRender);\n")
drep("                this->pauseBgPreRender.cvgSave = (u8*)gfxCtx->curFrameBuffer;\n                PreRender_DrawCoverage(&this->pauseBgPreRender, &gfxP);\n\n", "")
drep("            OVERLAY_DISP = gfxP;\n", "")
drep("            R_GRAPH_TASKSET00_FLAGS |= 1;\n", "            R_GRAPH_TASKSET00_FLAGS |= 1; // (graph task flag; no effect natively)\n")
s = s[:a] + d + s[b:]

# ---- Play_LoadFile
a, b = find_func(s, "Play_LoadFile")
s = s[:a] + ("void* Play_LoadFile(PlayState* this, RomFile* file) {\n"
             "    u32 size;\n    void* allocp;\n    u32 used = 0;\n\n"
             "    size = file->vromEnd - file->vromStart;\n"
             "    allocp = GAME_STATE_ALLOC(&this->state, size, \"../z_play.c\", 4692);\n"
             "    // native: the converted file (keyed by the original vrom) is placed in the same allocation;\n"
             "    // the returned value is its asset handle (bound to segment 2 for scenes).\n"
             "    return Vrom_LoadInto(file->vromStart, allocp, size, &used);\n}\n\n") + s[b:]

# scene header: commands are read through the scene's segment
rep("    Scene_ExecuteCommands(this, this->sceneSegment);",
    "    Scene_ExecuteCommands(this, Asset_Resolve((void*)(uintptr_t)0x82000000u)); // scene header at 02000000")

open(P, "w", encoding="utf-8", newline="\n").write(s)
print("patched z_play.c")
