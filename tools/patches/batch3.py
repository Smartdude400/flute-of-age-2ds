"""Render-buffer sizes, scroll constants and render-only static data (see batch2.py)."""
import glob
import os
import re

G = os.path.join(os.path.dirname(__file__), "..", "..", "source", "game") + "/"


def rd(p):
    return open(p, encoding="utf-8").read()


def wr(p, s):
    open(p, "w", encoding="utf-8", newline="\n").write(s)


p = G + "include/skybox.h"
s = rd(p)
s = s.replace("    // native: display-list/vertex buffers removed (sky drawn by engine/sky.c)\n",
              "    // native: render-only buffers (opaque). Still allocated with the original sizes so the game\n"
              "    // arena usage is unchanged; the sky is drawn natively.\n"
              "    void* dListBuf;\n    void* gfx;\n    void* roomVtx;\n")
wr(p, s)

p = G + "code/z_vr_box.c"
s = rd(p)
s = s.replace("sizeof(Gfx)", "8 /* original display-list word size */")
s = s.replace("sizeof(Vtx)", "16 /* original vertex size */")
s = re.sub(r"\bVtx\* roomVtx", "void* roomVtx", s)
wr(p, s)

p = G + "code/z_eff_blure.c"
s = rd(p)
s = re.sub(r"\bVtx\* vtx\b", "void* vtx", s)
s = re.sub(r"\nVtx_t D_8011578C\[\] = \{.*?\n\};\n", "\n", s, flags=re.S)
wr(p, s)

p = G + "code/z_eff_shield_particle.c"
s = rd(p)
s = re.sub(r"\nstatic Vtx sVertices\[5\] = \{.*?\n\};\n", "\n", s, flags=re.S)
wr(p, s)

p = G + "code/z_draw.c"
s = rd(p)
s = s.replace("/* 0x04 */ Gfx* dlists[8];", "/* 0x04 */ DListRef dlists[8];")
wr(p, s)

for p in glob.glob(G + "code/*.c"):
    s = rd(p)
    t = re.sub(r"(Gfx3D_SetTexScroll2?\(\w+, )G_TX_RENDERTILE", r"\g<1>0", s)
    if t != s:
        wr(p, t)
print("batch3 done")
