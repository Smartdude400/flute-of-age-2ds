"""Hand adaptation of z_message.c (text boxes, ocarina prompts).

Message_DrawMain and the text drawing functions contain the textbox STATE MACHINE (msgMode steps,
text reveal position, choice handling, sound effects), so all of their statements are kept; only
the display-list plumbing is translated:
  - the `Gfx** p` / `Gfx** gfxP` cursor parameters and `gfx` cursor copies are removed;
  - display-list statements are removed (tools/strip_gbi.py); the native HUD draws the textbox;
  - ROM loads: message text -> MessageNative_LoadText (raw message bytes from the converted file,
    decoded by the translated game code exactly as the original did); textbox/icon textures ->
    RomBind_Set on the original game buffer (recorded for the native renderer).
usage: python tools/patches/z_message.py"""
import os
import re
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
from cfuncs import find_func  # noqa: E402
from rewrite_calls import find_calls, split_args  # noqa: E402
import strip_gbi  # noqa: E402

P = os.path.join(os.path.dirname(__file__), "..", "..", "source", "game", "code", "z_message.c")
s = open(P, encoding="utf-8").read()

for inc in ('#include "engine/rom_bind.h"', '#include "engine/message_native.h"'):
    if inc not in s:
        s = re.sub(r'(#include "[^"]+"\n)', r"\1" + inc + "\n", s, count=1)

# ---- ROM loads
out, last = [], 0
for start, end, args in list(find_calls(s, "DMA_REQUEST_SYNC")):
    a = split_args(args)
    if len(a) != 5:
        continue
    dst, vrom, size = a[0], a[1], a[2]
    if dst == "MSG_BUF":
        m = re.match(r"\(uintptr_t\)(\w+SegmentRomStart)\s*\+\s*(.*)$", vrom, re.S)
        assert m, vrom
        rep = f"MessageNative_LoadText(MSG_BUF, (uintptr_t){m.group(1)}, {m.group(2).strip()}, {size})"
    else:
        rep = f"RomBind_Set({dst}, {' '.join(vrom.split())}, {size})"
    out.append(s[last:start])
    out.append(rep)
    last = end
out.append(s[last:])
s = "".join(out)

# ---- display-list cursor parameters
s = re.sub(r",\s*Gfx\*\* (?:p|gfxP)\)", ")", s)
s = re.sub(r"\(PlayState\* play, Gfx\*\* p, ", "(PlayState* play, ", s)
s = s.replace(", &gfx)", ")").replace(", &gfx, ", ", ")
s = s.replace("Message_DrawTextWide(play, gfx)", "Message_DrawTextWide(play)").replace(
    "Message_DrawText(play, gfx)", "Message_DrawText(play)")
s = re.sub(r"DRAW_TEXT\(play, gfx, ", "DRAW_TEXT(play, ", s)
s = re.sub(r"#define DRAW_TEXT\(play, gfx, isCredits\)", "#define DRAW_TEXT(play, isCredits)", s)
s = re.sub(r"DRAW_TEXT\(play, &gfx, ", "DRAW_TEXT(play, ", s)

DRAW = ["Message_DrawTextChar", "Message_DrawTextboxIcon", "Message_DrawItemIcon", "Message_DrawTextWide",
        "Message_DrawText", "Message_DrawTextBox", "Message_DrawMain"]
total = 0
for fn in DRAW:
    span = find_func(s, fn)
    if span is None:
        print("  (not found)", fn)
        continue
    a, b = span
    body = s[a:b]
    body = re.sub(r"\n[ \t]*Gfx\* gfx = \*(?:p|gfxP);", "", body)
    body = re.sub(r"\n[ \t]*Gfx\* gfx;", "", body)
    body = re.sub(r"\*(?:p|gfxP) = gfx;", "MSG_GFX_SYNC();", body)
    body, n = strip_gbi.strip(body, extra_names=["MSG_GFX_SYNC"])
    total += n
    s = s[:a] + body + s[b:]

# ---- Message_Draw
a, b = find_func(s, "Message_Draw")
body = s[a:b]
body = body.replace("    Gfx* plusOne;\n    Gfx* polyOpaP;\n", "")
body = re.sub(r"    plusOne = Gfx_Open\(polyOpaP = POLY_OPA_DISP\);\n    Gfx3D_DrawDL\(\(u32\)\(uintptr_t\)\(plusOne\)\);\n"
              r"    Message_DrawMain\(play, &plusOne\);\n    gSPEndDisplayList\(plusOne\+\+\);\n"
              r"    Gfx_Close\(polyOpaP, plusOne\);\n    POLY_OPA_DISP = plusOne;\n",
              "    Message_DrawMain(play);\n", body)
s = s[:a] + body + s[b:]

open(P, "w", encoding="utf-8", newline="\n").write(s)
print("z_message: removed", total, "display-list statements")
