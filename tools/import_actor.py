"""Import actor overlays into the native build (development tool: needs the decompilation checkout).

  python tools/import_actor.py En_Elf En_Ko ...

For each actor: its sources (src/overlays/actors/ovl_<Name>/*.c) go through import_source.py (provenance
banner, exact math, buffer-aware draw rewrite) into source/game/actors/, its headers through
import_header.py; then the common display-list types are translated (Gfx* / Gfx** data and parameters
are asset handles: DListRef). The actor's profile symbol links it into the native actor table
(z_actor_dlftbls.c, weak profiles). Remaining platform code is fixed by hand / per-actor patches."""
import glob
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "..", "..", "flute-of-age")
GAME = os.path.join(HERE, "..", "source", "game")


def translate(path):
    s = open(path, encoding="utf-8").read()
    s = re.sub(r"\bGfx\*\*", "DListRef*", s)
    s = re.sub(r"\b(static |const )*Gfx\* (\w+)(\[|\s*=|;|,|\))", lambda m: (m.group(1) or "") + "DListRef " + m.group(2) + m.group(3), s)
    s = re.sub(r"\(Gfx\*\)", "(DListRef)", s)
    # limb callbacks of the Gfx** variant: the native skeleton code has one callback type (no list cursor)
    s = re.sub(r",\s*DListRef\* gfx\)", ")", s)
    # segment tables: gSegments[] is the CPU side only, gSPSegment the renderer's (resolved per limb)
    s = re.sub(r"gSegments\[(\w+)\] = OS_K0_TO_PHYSICAL\(play->objectCtx\.slots\[([^\]]+)\]\.segment\);",
               r"Asset_SetCpuSegment(\1, play->objectCtx.slots[\2].dsObj);", s)
    s = re.sub(r"gSPSegment\(\(\*gfx\)\+\+, (0x0?6|6), play->objectCtx\.slots\[([^\]]+)\]\.segment\);",
               r"Gfx3D_SetSegmentObj(6, play->objectCtx.slots[\2].dsObj);", s)
    s = re.sub(r"gSPSegment\(\(\*gfx\)\+\+, (\w+), ([^;]+)\);", r"Gfx3D_SetSegment(\1, (const void*)(\2));", s)
    # multiply by the billboard matrix (segment 1) -> the same product on the CPU, loaded as the model matrix
    s = re.sub(r"gSPMatrix\(POLY_(OPA|XLU)_DISP\+\+, &D_01000000, G_MTX_NOPUSH \| G_MTX_MUL \| G_MTX_MODELVIEW\);",
               r"Matrix_Push(); /* (native) multiply by the billboard matrix (segment 1) */ "
               r"Matrix_Mult(&play->billboardMtxF, MTXMODE_APPLY); GFX_TO(\1, Gfx3D_LoadMtxF(Matrix_GetCurrent())); "
               r"Matrix_Pop();", s)
    # fog commands appended to a display buffer -> the native call into that buffer
    s = re.sub(r"POLY_(OPA|XLU)_DISP = Gfx_SetFog2\(POLY_\1_DISP, ([^;]+)\);", r"GFX_TO(\1, Gfx3D_SetFog(\2));", s)
    s = re.sub(r"POLY_(OPA|XLU)_DISP = Play_SetFog\((\w+), POLY_\1_DISP\);", r"GFX_TO(\1, Play_SetFog(\2));", s)
    open(path, "w", encoding="utf-8", newline="\n").write(s)


def own_headers(path, ovl_rel):
    """`#include "z_x.h"` of a header that lives in the actor's own overlay directory (resolved from there by
    the original build) -> its full imported path; shared headers (z_lib.h, ...) stay as they are."""
    s = open(path, encoding="utf-8").read()

    def fix(m):
        if os.path.exists(os.path.join(SRC, ovl_rel, m.group(1))):
            return f'#include "{ovl_rel}/{m.group(1)}"'
        return m.group(0)

    s = re.sub(r'#include "(z_\w+\.h)"', fix, s)
    with open(path, "w", encoding="utf-8", newline="") as fh:
        fh.write(s)


def embedded_data(path):
    """Vertex arrays / display lists the overlay keeps in its own data (`static Gfx sX[] = { #include
    "assets/overlays/ovl_N/sX.inc.c" };`) -> handles from the generated header (tools/build_overlays.py)."""
    s = open(path, encoding="utf-8").read()
    pat = re.compile(r'static (?:Vtx|Gfx) \w+\[\w*\] = \{\n#include "assets/overlays/(ovl_\w+)/\w+\.inc\.c"\n\};\n')
    first = pat.search(s)
    if first is None:
        return
    ovl = first.group(1)
    hdr = f'#include "assets/overlays/{ovl}/{ovl}.h" // embedded data as handles (tools/build_overlays.py)\n'
    s = s[:first.start()] + hdr + pat.sub("", s[first.start():])
    with open(path, "w", encoding="utf-8", newline="") as fh:
        fh.write(s)


def main():
    for name in sys.argv[1:]:
        ovl = os.path.join(SRC, "src", "overlays", "actors", "ovl_" + name)
        if not os.path.isdir(ovl):
            print(f"{name}: no overlay directory")
            continue
        for c in sorted(glob.glob(os.path.join(ovl, "*.c"))):
            rel = os.path.relpath(c, SRC).replace("\\", "/")
            dst = os.path.join(GAME, "actors", os.path.basename(c))
            if os.path.exists(dst):
                print(f"{name}: {os.path.basename(c)} already imported")
                continue
            subprocess.run([sys.executable, os.path.join(HERE, "import_source.py"), rel, "actors"], check=True)
            translate(dst)
            own_headers(dst, "src/overlays/actors/ovl_" + name)
            embedded_data(dst)
        for h in sorted(glob.glob(os.path.join(ovl, "*.h"))):
            rel = os.path.relpath(h, SRC).replace("\\", "/")
            if not os.path.exists(os.path.join(GAME, "include", rel)):
                subprocess.run([sys.executable, os.path.join(HERE, "import_header.py"), rel], check=True)
                translate(os.path.join(GAME, "include", rel))


main()
