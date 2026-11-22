"""Hand adaptation of z_scene_table.c (scene draw configs).

The draw configs are NOT pure presentation: several step game state (Math_StepToS on
roomCtx.drawParams, static wave phases), so every function is kept and only its display-list
construction is translated:
  - GRAPH_ALLOC'd per-frame segment lists      -> Gfx3D_SegDLBegin(seg) ... Gfx3D_SegDLEnd()
  - Gfx_TexScroll / Gfx_TwoTexScroll[Env|Prim]  -> Gfx3D_SetTexScroll[2] (+ recorded colour)
  - MATRIX_FINALIZE into a segment              -> Gfx3D_SetSegmentMtx(seg, Matrix_GetCurrent())
  - sDefaultDisplayList (static Gfx[])          -> Scene_DefaultSegments() (same commands, native)
usage: python tools/patches/z_scene_table.py"""
import os
import re
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
from rewrite_calls import find_calls, split_args  # noqa: E402
from cfuncs import find_func  # noqa: E402

P = os.path.join(os.path.dirname(__file__), "..", "..", "source", "game", "code", "z_scene_table.c")


def tile(x):
    return "0" if x.strip() == "G_TX_RENDERTILE" else x


def seg_inner(seg, inner):
    """Native statement(s) for binding `inner` (an original segment value expression) to `seg`."""
    m = re.match(r"(\w+)\((.*)\)$", inner.strip(), re.S)
    if m:
        fn, args = m.group(1), [tile(a) for a in split_args(m.group(2))]
        if fn == "Gfx_TexScroll":  # (gfxCtx, x, y, w, h): tile 0
            return f"Gfx3D_SetTexScroll({seg}, 0, {', '.join(args[1:])})"
        if fn == "Gfx_TwoTexScroll":
            return f"Gfx3D_SetTexScroll2({seg}, {', '.join(args[1:])})"
        if fn in ("Gfx_TwoTexScrollEnvColor", "Gfx_TwoTexScrollPrimColor"):
            col = "Env" if fn.endswith("EnvColor") else "Prim"
            return (f"Gfx3D_SegDLBegin({seg}); Gfx3D_SetTexScroll2({seg}, {', '.join(args[1:11])}); "
                    f"Gfx3D_Set{col}Color({', '.join(args[11:15])}); Gfx3D_SegDLEnd()")
        if fn == "MATRIX_FINALIZE":
            return f"Gfx3D_SetSegmentMtx({seg}, Matrix_GetCurrent())"
    if inner.strip() == "displayListHead":
        return f"Gfx3D_SegDLBegin({seg})"
    return f"Gfx3D_SetSegment({seg}, (const void*)({inner}))"


def rewrite_segments(s):
    out, last = [], 0
    calls = [(a, b, x, "gSPSegment") for a, b, x in find_calls(s, "gSPSegment")]
    calls += [(a, b, x, "Gfx3D_SetSegment") for a, b, x in find_calls(s, "Gfx3D_SetSegment")]
    calls.sort()
    n = 0
    for start, end, args, kind in calls:
        if start < last:
            continue
        a = split_args(args)
        if kind == "gSPSegment":
            if len(a) != 3:
                continue
            seg, inner = a[1], a[2]
        else:
            if len(a) != 2:
                continue
            seg = a[0]
            m = re.match(r"\(const void\*\)\((.*)\)$", a[1], re.S)
            inner = m.group(1) if m else a[1]
        rep = seg_inner(seg, inner)
        if rep == f"Gfx3D_SetSegment({seg}, (const void*)({inner}))" and kind == "Gfx3D_SetSegment":
            continue
        out.append(s[last:start])
        out.append(rep)
        last = end
        n += 1
    out.append(s[last:])
    return "".join(out), n


s = open(P, encoding="utf-8").read()
if '#include "engine/gfx3d.h"' not in s:
    s = re.sub(r'(#include "[^"]+"\n)', r'\1#include "engine/gfx3d.h"\n', s, count=1)

# sDefaultDisplayList -> native function with the same effect
s = re.sub(r"Gfx sDefaultDisplayList\[\] = \{.*?\n\};\n",
           "// native: was the static list sDefaultDisplayList (segments 08-0D -> gEmptyDL, prim/env 128).\n"
           "static void Scene_DefaultSegments(void) {\n"
           "    for (int seg = 0x08; seg <= 0x0D; seg++) {\n"
           "        Gfx3D_SegDLBegin(seg); // empty list == gEmptyDL\n"
           "        Gfx3D_SegDLEnd();\n"
           "    }\n"
           "    Gfx3D_SetPrimColor(128, 128, 128, 128);\n"
           "    Gfx3D_SetEnvColor(128, 128, 128, 128);\n"
           "}\n", s, flags=re.S)
s = s.replace("Gfx3D_DrawDL((u32)(uintptr_t)(sDefaultDisplayList));", "Scene_DefaultSegments();")

# Gfx_TwoTexScrollPrimColor: helper that built a runtime list; callers are translated, drop it
a, b = find_func(s, "Gfx_TwoTexScrollPrimColor")
if a is not None:
    s = s[:a] + "// native: Gfx_TwoTexScrollPrimColor (runtime tile-scroll list builder) translated at call sites\n" + s[b:]

s, n = rewrite_segments(s)

# per-frame list allocation / termination
s = re.sub(r"\n[ \t]*Gfx\* displayListHead = GRAPH_ALLOC\([^;]*\);", "", s)
s = re.sub(r"\n[ \t]*Gfx\* displayListHead;", "", s)
s = re.sub(r"\n[ \t]*displayListHead = GRAPH_ALLOC\([^;]*\);", "", s)
s = re.sub(r"gSPEndDisplayList\(displayListHead(?:\+\+)?\);", "Gfx3D_SegDLEnd();", s)
s = s.replace("G_TX_RENDERTILE", "0")

open(P, "w", encoding="utf-8", newline="\n").write(s)
print("z_scene_table: rewrote", n, "segment binds")
