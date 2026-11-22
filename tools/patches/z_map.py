"""Hand adaptation of z_map_exp.c (minimap) and z_map_mark.c (dungeon map marks).

  - Minimap texture DMA into interfaceCtx->mapSegment -> RomBind_Set(dst, vrom, size): the converted
    minimap texture backing that buffer is recorded for the native HUD (the buffer keeps its original
    game-arena allocation).
  - Minimap_Draw / Minimap_DrawCompassIcons / MapMark_DrawForDungeon: logic kept (minimap toggle on L,
    its sound effect, treasure-flag tests); display-list statements removed (tools/strip_gbi.py).
  - ovl_map_mark_data: statically linked (z_map_mark_data.c); MapMark_Init still reserves the original
    overlay size on the game arena (misc_overlay_sizes.h) and points the table at the linked data.
usage: python tools/patches/z_map.py"""
import os
import re
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
from cfuncs import find_func  # noqa: E402
import strip_gbi  # noqa: E402

CODE = os.path.join(os.path.dirname(__file__), "..", "..", "source", "game", "code")


def strip_funcs(s, names):
    total = 0
    for fn in names:
        span = find_func(s, fn)
        if span is None:
            print("  (not found)", fn)
            continue
        a, b = span
        body, n = strip_gbi.strip(s[a:b])
        total += n
        s = s[:a] + body + s[b:]
    return s, total


def add_inc(s, inc):
    if inc not in s:
        s = re.sub(r'(#include "[^"]+"\n)', r"\1" + inc + "\n", s, count=1)
    return s


# ---- z_map_exp.c
p = os.path.join(CODE, "z_map_exp.c")
s = open(p, encoding="utf-8").read()
s = add_inc(s, '#include "engine/rom_bind.h"')
s = re.sub(r"DMA_REQUEST_SYNC\(([^,]+),\s*(.*?),\s*([^,]+),\s*\"\.\./z_map_exp\.c\", \d+\);",
           lambda m: f"RomBind_Set({m.group(1).strip()}, {m.group(2).strip()}, {m.group(3).strip()});", s, flags=re.S)
s, n = strip_funcs(s, ["Minimap_DrawCompassIcons", "Minimap_Draw"])
open(p, "w", encoding="utf-8", newline="\n").write(s)
print("z_map_exp: removed", n, "display-list statements")

# ---- z_map_mark.c
p = os.path.join(CODE, "z_map_mark.c")
s = open(p, encoding="utf-8").read()
s = add_inc(s, '#include "misc_overlay_sizes.h"')
# texture format constants of the icon table (original image format/size codes, data only)
for k, v in {"G_IM_FMT_RGBA": "0 /* RGBA */", "G_IM_FMT_IA": "3 /* IA */", "G_IM_SIZ_16b": "2 /* 16b */",
             "G_IM_SIZ_8b": "1 /* 8b */"}.items():
    s = s.replace(k, v)
s = s.replace("NULL, ROM_FILE(ovl_map_mark_data), _ovl_map_mark_dataSegmentStart, _ovl_map_mark_dataSegmentEnd, gMapMarkDataTable,",
              "NULL, { 0, 0 }, (void*)0, (void*)OVL_SIZE_MAP_MARK_DATA, gMapMarkDataTable, // native: overlay statically linked")
a, b = find_func(s, "MapMark_Init")
body = s[a:b]
body = re.sub(r"    Overlay_Load\([^;]*\);\n", "    // native: ovl_map_mark_data is statically linked; the original overlay size stays reserved above\n", body)
body = re.sub(r"    sLoadedMarkDataTable =\n        \(void\*\)\(uintptr_t\)\(\(overlay->vramTable != NULL\).*?: NULL\);\n",
              "", body, flags=re.S)
s = s[:a] + body + s[b:]
s, n = strip_funcs(s, ["MapMark_DrawForDungeon"])
open(p, "w", encoding="utf-8", newline="\n").write(s)
print("z_map_mark: removed", n, "display-list statements")
