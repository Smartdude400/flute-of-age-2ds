"""Hand adaptation of z_parameter.c (HUD / interface).

Interface_Draw contains GAMEPLAY LOGIC (timers, timed respawns, button item reverts) interleaved with
drawing. It is kept line for line; only display-list command statements are removed (strip_gbi), and
the HUD is redrawn by the native HUD module (engine/hud.c) from the same game state.
Pure draw/texture-load helpers are presentation stubs."""
import os
import re
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
from cfuncs import stub_funcs, find_func  # noqa: E402

GBI_CALL = re.compile(r"\b(?:g(?:s)?(?:SP|DP)\w+|gSP\w+|gDP\w+|Gfx_Texture\w+|Interface_Draw(?:ItemIconTexture|AmmoCount|"
                      r"ActionButton|ActionLabel|ItemButtons)|Magic_DrawMeter|Health_DrawMeter|Map_Draw\w*|"
                      r"Minimap_Draw\w*|Interface_InitVertices|View_ApplyOrthoToOverlay|func_8008A994)\(")


def strip_gbi(body):
    """Remove statements that are display-list commands (calls ending at ';' at paren depth 0),
    and `OVERLAY_DISP = ...;` style assignments. Everything else is kept verbatim."""
    out, i, n = [], 0, len(body)
    removed = 0
    while i < n:
        m = GBI_CALL.search(body, i)
        m2 = re.compile(r"\b(?:OVERLAY_DISP|POLY_OPA_DISP|POLY_XLU_DISP)\s*=").search(body, i)
        cand = [x for x in (m, m2) if x]
        if not cand:
            out.append(body[i:])
            break
        m = min(cand, key=lambda x: x.start())
        # statement must start at line start (after indentation)
        line_start = body.rfind("\n", 0, m.start()) + 1
        if body[line_start:m.start()].strip():
            out.append(body[i:m.end()])
            i = m.end()
            continue
        j, depth = m.end() - 1, 0
        while j < n:
            c = body[j]
            if c == "(":
                depth += 1
            elif c == ")":
                depth -= 1
            elif c == ";" and depth == 0:
                break
            j += 1
        out.append(body[i:line_start])
        i = j + 1
        if i < n and body[i] == "\n":
            i += 1
        removed += 1
    return "".join(out), removed


p = os.path.join(os.path.dirname(__file__), "..", "..", "source", "game", "code", "z_parameter.c")
s = open(p, encoding="utf-8").read()
if '#include "engine/stub.h"' not in s:
    s = re.sub(r'(#include "[^"]+"\n)', r'\1#include "engine/stub.h"\n#include "engine/hud.h"\n', s, count=1)

s = stub_funcs(s, ["Gfx_TextureIA8", "Gfx_TextureI8", "Interface_LoadItemIcon1", "Interface_LoadItemIcon2",
                   "Interface_LoadActionLabel", "Interface_LoadActionLabelB", "Magic_DrawMeter",
                   "Interface_DrawActionLabel", "Interface_DrawItemButtons", "Interface_DrawItemIconTexture",
                   "Interface_DrawAmmoCount", "Interface_DrawActionButton", "Interface_InitVertices"])
s = re.sub(r"^Gfx\* (Gfx_Texture\w+\()", r"void* \1", s, flags=re.M)
s = re.sub(r"(void\* Gfx_Texture\w+\()Gfx\* gfx", r"\1void* gfx", s)

a, b = find_func(s, "Interface_Draw")
body, n = strip_gbi(s[a:b])
body = body.replace("void Interface_Draw(PlayState* play) {",
                    "void Interface_Draw(PlayState* play) {\n    // native: HUD graphics are drawn by Hud_Draw (engine/hud.c); this function keeps the original\n"
                    "    // logic (timers, respawns, button items) with only display-list statements removed.\n"
                    "    Hud_Draw(play);", 1)
s = s[:a] + body + s[b:]
open(p, "w", encoding="utf-8", newline="\n").write(s)
print("patched z_parameter.c; removed", n, "draw statements from Interface_Draw")

# ---- pass 2
s = open(p, encoding="utf-8").read()
s = re.sub(r"\nstatic Gfx \w+\[\] = \{.*?\n\};\n", "\n", s, flags=re.S)
s = re.sub(r"(void\* Gfx_Texture\w+\()Gfx\* displayListHead", r"\1void* displayListHead", s)
# equip-animation cursor vertices (render-only buffer writes)
s = re.sub(r"\n[ \t]*pauseCtx->cursorVtx\[\d+\]\.v\.ob\[\d\] = [^;]*;", "", s)
open(p, "w", encoding="utf-8", newline="\n").write(s)
print("z_parameter pass 2")
