"""Hand adaptation of z_kankyo.c (environment: time of day, weather, skybox state, lens flare).

  - Skybox texture DMA (async ROM->RAM + message queue): replaced by SkyboxNative_Request /
    SkyboxNative_Poll. The skyboxDmaState machine is kept unchanged; completion is reported on the
    next Environment_UpdateSkybox call (assumption A4, same policy as rooms A2 / objects A3).
  - z-buffer readback (sun occlusion for the lens flare): the DS depth buffer is not readable;
    Environment_GetPixelDepth returns the far-plane value (lens flare treated as unoccluded).
    Presentation-only (sSunScreenDepth feeds only lens flare drawing).
  - Graph callback: the native GraphicsContext keeps the callback hook (engine/gfx.h).
  - Draw functions: GAME LOGIC (Rand_* calls, lightning/rain/sandstorm state) kept line for line,
    display-list statements removed (tools/strip_gbi.py); full-screen fills -> Gfx3D_FillScreenPrim.
usage: python tools/patches/z_kankyo.py"""
import os
import re
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
from cfuncs import find_func  # noqa: E402
import strip_gbi  # noqa: E402

P = os.path.join(os.path.dirname(__file__), "..", "..", "source", "game", "code", "z_kankyo.c")
s = open(P, encoding="utf-8").read()

for inc in ('#include "engine/gfx.h"', '#include "engine/gfx3d.h"', '#include "engine/skybox_native.h"'):
    if inc not in s:
        s = re.sub(r'(#include "[^"]+"\n)', r"\1" + inc + "\n", s, count=1)

# z-buffer far value (was GPACK_ZDZ(G_MAXFBZ, 0))
s = s.replace("GPACK_ZDZ(G_MAXFBZ, 0)", "ENV_ZBUF_FAR")
if "#define ENV_ZBUF_FAR" not in s:
    s = s.replace("u16 sSunScreenDepth", "#define ENV_ZBUF_FAR 0xFFFC // far plane, z-buffer packed format\nu16 sSunScreenDepth", 1)
s = s.replace("    s32 pixelDepth = gZBuffer[y][x];",
              "    // native: the DS depth buffer is not readable; report the far plane (nothing occludes).\n"
              "    s32 pixelDepth = ENV_ZBUF_FAR;\n    (void)x;\n    (void)y;")

# skybox DMA
s = re.sub(r"\n[ \t]*osCreateMesgQueue\(&envCtx->loadQueue, &envCtx->loadMsg, 1\);", "", s)
s = re.sub(r"DMA_REQUEST_ASYNC\(&envCtx->dmaRequest,\s*([^;]*?),\s*(\S+?vromStart),\s*size,\s*0,\s*&envCtx->loadQueue,\s*NULL,\s*\"[^\"]*\",\s*\d+\);",
           r"SkyboxNative_Request(envCtx, \1, \2, size);", s, flags=re.S)
s = s.replace("osRecvMesg(&envCtx->loadQueue, NULL, OS_MESG_NOBLOCK) == 0", "SkyboxNative_Poll(envCtx)")
a, b = find_func(s, "Environment_UpdateSkybox")
body = s[a:b]
if "SkyboxNative_Tick" not in body:
    body = body.replace("    u8 skyboxBlend = 0;\n", "    u8 skyboxBlend = 0;\n\n    SkyboxNative_Tick(envCtx);\n", 1)
s = s[:a] + body + s[b:]

# full-screen fills
s = re.sub(r"gDPFillRectangle\((?:POLY_OPA_DISP|POLY_XLU_DISP)\+\+, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1\);",
           "Gfx3D_FillScreenPrim();", s)

DRAW = ["Environment_DrawSunAndMoon", "Environment_DrawSunLensFlare", "Environment_DrawLensFlare",
        "Environment_DrawRain", "Environment_DrawSkyboxFilters", "Environment_DrawLightningFlash",
        "Environment_DrawLightning", "Environment_DrawCustomLensFlare", "Environment_FillScreen",
        "Environment_DrawSandstorm"]
total = 0
for fn in DRAW:
    span = find_func(s, fn)
    if span is None:
        continue
    a, b = span
    body, n = strip_gbi.strip(s[a:b])
    total += n
    s = s[:a] + body + s[b:]

open(P, "w", encoding="utf-8", newline="\n").write(s)
print("z_kankyo: removed", total, "display-list statements")
