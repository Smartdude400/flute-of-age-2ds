"""Presentation stubs for draw-only functions of effects/transitions/sky/debug text/HUD meters.
Each stubbed function was checked to only build display lists (no game-state writes).
Listed in STUBS.md (P-series). Logic functions in the same files are untouched."""
import os
import re
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
from cfuncs import stub_funcs, find_func  # noqa: E402

D = os.path.join(os.path.dirname(__file__), "..", "..", "source", "game", "code")


def edit(name, funcs, extra=None):
    p = os.path.join(D, name)
    s = open(p, encoding="utf-8").read()
    if '#include "engine/stub.h"' not in s:
        s = re.sub(r'(#include "[^"]+"\n)', r'\1#include "engine/stub.h"\n', s, count=1)
    if extra:
        s = extra(s)
    s = stub_funcs(s, funcs)
    # stubbed functions that returned display lists now return void*
    s = re.sub(r"^Gfx\* (\w+\()", r"void* \1", s, flags=re.M)
    open(p, "w", encoding="utf-8", newline="\n").write(s)
    print("patched", name, len(funcs))


def drop_static_gfx(s):
    # static display-list data used only by the stubbed draw code
    return re.sub(r"\n(static )?Gfx \w+\[\w*\] = \{.*?\n\};\n", "\n", s, flags=re.S)


edit("z_eff_blure.c", ["EffectBlure_DrawElemNoInterpolation", "EffectBlure_DrawElemHermiteInterpolation",
                       "EffectBlure_DrawSmooth", "EffectBlure_SetupSimple", "EffectBlure_SetupSimpleAlt",
                       "EffectBlure_DrawSimpleVertices", "EffectBlure_DrawSimple", "EffectBlure_Draw"])
edit("z_eff_spark.c", ["EffectSpark_Draw"])
edit("z_eff_shield_particle.c", ["EffectShieldParticle_Draw"], drop_static_gfx)
edit("z_eff_ss_dead.c", ["func_80026230", "func_80026400", "func_80026608", "func_80026690", "func_80026860",
                         "func_80026A6C"])
edit("z_moji.c", ["Moji_DrawChar", "Moji_DrawString"])
edit("z_lifemeter.c", ["Health_DrawMeter"])
edit("z_vr_box.c", ["Skybox_CalculateFace256", "Skybox_CalculateFace128", "Skybox_Calculate256",
                    "Skybox_Calculate128", "Skybox_Setup"])
