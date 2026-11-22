"""Mechanical call-site rewriting of common draw idioms into the native draw API (engine/gfx3d.h).

This is NOT a shim: the original display-list macros are not provided anywhere. Each matched call
site is replaced by an explicit call to a native function. Anything that does not match stays as is
and fails to compile, so the remaining sites are rewritten by hand.

Calls into one of the original display-list buffers keep that buffer: GFX_TO(OPA|XLU|OVL, call).
Calls through a local list cursor (gfx++) keep the buffer their caller selected. Render-state preset
helpers (Gfx_SetupDL_NNOpa(gfxCtx) ...) stay as they are: they are native functions (code/z_rcp.c);
the list-pointer forms become Gfx3D_SetupDLTo(buffer, N).

usage: python tools/rewrite_draw.py file.c [...]   (also used by import_source.py)
"""
import re
import sys

DISP = r"(POLY_OPA_DISP|POLY_XLU_DISP|OVERLAY_DISP|POLY_KAL_DISP|WORK_DISP|gfx|\w+)\+\+"
ARG = r"((?:[^,()]|\([^()]*(?:\([^()]*\))*[^()]*\))+?)"
BUF = {"POLY_OPA_DISP": "OPA", "POLY_XLU_DISP": "XLU", "OVERLAY_DISP": "OVL", "POLY_KAL_DISP": "OVL",
       "WORK_DISP": "OPA"}


def to(buf_name, call):
    b = BUF.get(buf_name)
    return f"GFX_TO({b}, {call})" if b else call


def _dl(m):
    return to(m.group(1), f"Gfx3D_DrawDL((u32)(uintptr_t)({m.group(2)}))") + ";"


def _mtx(m):
    return to(m.group(1), "Gfx3D_LoadMtxF(Matrix_GetCurrent())") + ";"


def _prim(m):
    return to(m.group(1), f"Gfx3D_SetPrimColor({m.group(4)}, {m.group(5)}, {m.group(6)}, {m.group(7)})") + ";"


def _env(m):
    return to(m.group(1), f"Gfx3D_SetEnvColor({m.group(2)}, {m.group(3)}, {m.group(4)}, {m.group(5)})") + ";"


def _seg(m):
    return to(m.group(1), f"Gfx3D_SetSegment({m.group(2)}, (const void*)({m.group(3)}))") + ";"


def _preset_assign(m):
    """POLY_X_DISP = Gfx_SetupDL_NN(POLY_X_DISP) / Gfx_SetupDL(POLY_X_DISP, N): preset N into that buffer."""
    num = re.search(r"Gfx_SetupDL_(\d+)", m.group(2))
    if num:
        n = int(num.group(1))
    else:
        arg = re.search(r",\s*(?:SETUPDL_)?(\d+)\s*\)", m.group(0))
        n = int(arg.group(1)) if arg else 25
    return f"Gfx3D_SetupDLTo(GFX_BUF_{BUF[m.group(1)]}, {n});"


def _skel(m):
    return to(m.group(1), f"{m.group(2)}({m.group(3)})") + ";"


RULES = [
    # display buffer open/close
    (r"\n[ \t]*(?:OPEN|CLOSE)_DISPS\([^;]*\);", ""),
    # model matrix
    (r"MATRIX_FINALIZE_AND_LOAD\(" + DISP + r",[^;]*\);", _mtx),
    (r"gSPMatrix\(" + DISP + r",\s*MATRIX_FINALIZE\([^;]*\),\s*G_MTX_NOPUSH \| G_MTX_LOAD \| G_MTX_MODELVIEW\);", _mtx),
    (r"gSPMatrix\(" + DISP + r",\s*MATRIX_FINALIZE\([^;]*\),\s*G_MTX_MODELVIEW \| G_MTX_LOAD\);", _mtx),
    # display lists
    (r"gSPDisplayList\(" + DISP + r",\s*" + ARG + r"\);", _dl),
    # colours
    (r"gDPSetPrimColor\(" + DISP + r",\s*" + ARG + r",\s*" + ARG + r",\s*" + ARG + r",\s*" + ARG + r",\s*" + ARG +
     r",\s*" + ARG + r"\);", _prim),
    (r"gDPSetEnvColor\(" + DISP + r",\s*" + ARG + r",\s*" + ARG + r",\s*" + ARG + r",\s*" + ARG + r"\);", _env),
    # segments
    (r"gSPSegment\(" + DISP + r",\s*" + ARG + r",\s*" + ARG + r"\);", _seg),
    # render-state presets written through a list pointer
    (r"(POLY_OPA_DISP|POLY_XLU_DISP|OVERLAY_DISP)\s*=\s*(Gfx_SetupDL\w*)\(\s*\1\s*(?:,[^;]*)?\);", _preset_assign),
    # skeleton draws into an explicit display buffer
    (r"(POLY_OPA_DISP|POLY_XLU_DISP|OVERLAY_DISP)\s*=\s*(SkelAnime_Draw\w*)\(([^;]*?),\s*\1\s*\);", _skel),
    # pipeline syncs are meaningless natively
    (r"\n[ \t]*gDP(?:PipeSync|LoadSync|TileSync|FullSync)\(" + DISP + r"\);", ""),
]


def rewrite(text):
    from rewrite_calls import rewrite_tex_scroll
    counts = {}
    text, n = rewrite_tex_scroll(text)
    if n:
        counts["texscroll"] = n
    for pat, rep in RULES:
        text, n = re.subn(pat, rep, text)
        if n:
            counts[pat[:30]] = counts.get(pat[:30], 0) + n
    if counts and '#include "engine/gfx3d.h"' not in text:
        text = re.sub(r'(#include "[^"]+"\n)', r'\1#include "engine/gfx3d.h"\n', text, count=1)
    return text, counts


if __name__ == "__main__":
    for p in sys.argv[1:]:
        s = open(p, encoding="utf-8").read()
        t, c = rewrite(s)
        if t != s:
            open(p, "w", encoding="utf-8", newline="\n").write(t)
        print(p, sum(c.values()), "rewrites")
