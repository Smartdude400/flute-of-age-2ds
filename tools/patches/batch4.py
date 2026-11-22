"""Scene transitions: update logic (timers, isDone) untouched; drawing rewritten natively.
Fade -> native full-screen fill; circle/wipe/seal/tile -> presentation stubs (STUBS.md P-series)."""
import os
import re
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
from cfuncs import stub_funcs, find_func  # noqa: E402

D = os.path.join(os.path.dirname(__file__), "..", "..", "source", "game", "code")


def rd(n):
    return open(os.path.join(D, n), encoding="utf-8").read()


def wr(n, s):
    open(os.path.join(D, n), "w", encoding="utf-8", newline="\n").write(s)


def strip_data(s):
    # static render data (display lists, vertices, textures) used only by the old draw code
    s = re.sub(r"\n(?:static )?(?:Gfx|Vtx|u64|u8) \w+\[[^\]]*\] = \{.*?\n\};\n", "\n", s, flags=re.S)
    return s


def add_stub_include(s):
    if '#include "engine/stub.h"' not in s:
        s = re.sub(r'(#include "[^"]+"\n)', r'\1#include "engine/stub.h"\n#include "engine/gfx2d.h"\n', s, count=1)
    return s


# fade: native full-screen fill
s = add_stub_include(strip_data(rd("z_fbdemo_fade.c")))
a, b = find_func(s, "TransitionFade_Draw")
s = s[:a] + r'''void TransitionFade_Draw(void* thisx) {
    // native: full-screen fill with the transition colour (original: filled rectangle on the RDP)
    TransitionFade* this = (TransitionFade*)thisx;
    Color_RGBA8_u32* color = &this->color;

    if (color->a != 0) {
        Gfx2D_FillScreen(color->r, color->g, color->b, color->a);
    }
}

''' + s[b:]
wr("z_fbdemo_fade.c", s)

for name, fn in (("z_fbdemo_circle.c", "TransitionCircle_Draw"), ("z_fbdemo_wipe1.c", "TransitionWipe_Draw"),
                 ("z_fbdemo_dawnseal.c", "TransitionDawnseal_Draw")):
    s = add_stub_include(strip_data(rd(name)))
    s = s.replace(fn + "(void* thisx, Gfx** gfxP)", fn + "(void* thisx)")
    s = stub_funcs(s, [fn])
    wr(name, s)

s = add_stub_include(strip_data(rd("z_fbdemo.c")))
s = stub_funcs(s, ["TransitionTile_InitGraphics", "TransitionTile_Destroy", "TransitionTile_Init",
                   "TransitionTile_SetVtx", "TransitionTile_Draw"])
s = s.replace("TransitionTile_Draw(TransitionTile* this, Gfx** gfxP)", "TransitionTile_Draw(TransitionTile* this)")
wr("z_fbdemo.c", s)
print("batch4 done")
