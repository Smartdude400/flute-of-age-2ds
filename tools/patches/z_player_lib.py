"""Hand adaptation of z_player_lib.c."""
import os
import re
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
from cfuncs import stub_funcs  # noqa: E402

G = os.path.join(os.path.dirname(__file__), "..", "..", "source", "game")
p = os.path.join(G, "code", "z_player_lib.c")
s = open(p, encoding="utf-8").read()
if '#include "engine/stub.h"' not in s:
    s = re.sub(r'(#include "[^"]+"\n)', r'\1#include "engine/stub.h"\n', s, count=1)

# display-list references
s = re.sub(r"\bGfx\s*\*\s*\*", "DListRef*", s)
s = re.sub(r"\bGfx\s*\*", "DListRef ", s)

# get-item model: the converted GI object is bound to segment 6 (original: raw object buffer)
s = s.replace("    gSegments[6] = OS_K0_TO_PHYSICAL(this->giObjectSegment);\n", "")
s = s.replace("    Gfx3D_SetSegment(0x06, (const void*)(this->giObjectSegment));\n    Gfx3D_SetSegment(0x06, (const void*)(this->giObjectSegment));\n",
              "    Asset_SetSegment(6, this->giDsObj);\n")
s = s.replace("    if (!this->giObjectLoading || osRecvMesg(&this->giObjectLoadQueue, NULL, OS_MESG_NOBLOCK) == 0) {",
              "    // native: the converted GI object was loaded synchronously when requested (draw-only flag)\n"
              "    if (!this->giObjectLoading || this->giObjectLoadPending) {\n        this->giObjectLoadPending = 0;")

# pause-menu player rendering (equipment screen): presentation stubs until the pause menu is ported
s = stub_funcs(s, ["Player_InitPauseDrawData", "Player_DrawPauseImpl", "Player_DrawPause"])
open(p, "w", encoding="utf-8", newline="\n").write(s)

# player.h: native GI object handle
h = os.path.join(G, "include", "player.h")
t = open(h, encoding="utf-8").read()
if "giDsObj" not in t:
    t = t.replace("    u8 giObjectLoadPending;\n", "    u8 giObjectLoadPending;\n    struct DsObj* giDsObj; // native: converted get-item object\n")
open(h, "w", encoding="utf-8", newline="\n").write(t)
print("patched z_player_lib.c")
