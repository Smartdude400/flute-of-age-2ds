"""Hand adaptation of z_kanfont.c (font glyph / message text loading).
  - message text -> MessageNative_LoadText (bytes decoded by the translated game code);
  - glyph bitmaps -> RomBind_Set: the native text renderer draws characters from the offline-converted
    font (tools/font_conv.py) by character code, so no glyph bytes are copied at run time.
usage: python tools/patches/z_kanfont.py"""
import os
import re
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
from rewrite_calls import find_calls, split_args  # noqa: E402

P = os.path.join(os.path.dirname(__file__), "..", "..", "source", "game", "code", "z_kanfont.c")
s = open(P, encoding="utf-8").read()
for inc in ('#include "engine/rom_bind.h"', '#include "engine/message_native.h"'):
    if inc not in s:
        s = re.sub(r'(#include "[^"]+"\n)', r"\1" + inc + "\n", s, count=1)
out, last = [], 0
for start, end, args in list(find_calls(s, "DMA_REQUEST_SYNC")):
    a = split_args(args)
    dst, vrom, size = a[0], " ".join(a[1].split()), a[2]
    if re.search(r"msgBuf", dst):
        m = re.match(r"\(uintptr_t\)(\w+SegmentRomStart)\s*\+\s*(.*)$", vrom, re.S)
        rep = f"MessageNative_LoadText({dst}, (uintptr_t){m.group(1)}, {m.group(2)}, {size})"
    else:
        rep = f"RomBind_Set({dst}, {vrom}, {size})"
    out.append(s[last:start])
    out.append(rep)
    last = end
out.append(s[last:])
open(P, "w", encoding="utf-8", newline="\n").write("".join(out))
print("patched z_kanfont.c")
