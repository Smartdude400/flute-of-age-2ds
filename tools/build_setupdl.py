"""Compile the game's render-state presets (sSetupDL[SETUPDL_MAX][6] in src/code/z_rcp.c, the lists
behind Gfx_SetupDL_*) into native op programs: romfs/setupdl.pobj, mesh key = preset index.
The table is located in the ROM `code` file by its shape (71 entries of 6 commands, each starting with
a pipe sync and ending with an end-of-list), then compiled with tools/dl2pica.py like any list."""
import os
import struct

from romfs import RomFS
import dl2pica

HERE = os.path.dirname(os.path.abspath(__file__))
SETUPDL_MAX = 71
CODE_SEG = 0x0F  # pseudo segment for the code file (build time only)


def find_table(code):
    for off in range(0, len(code) - 48 * SETUPDL_MAX, 8):
        if all(struct.unpack_from(">I", code, off + 48 * i)[0] == 0xE7000000 and
               struct.unpack_from(">I", code, off + 48 * i + 40)[0] == 0xDF000000 for i in range(SETUPDL_MAX)):
            return off
    raise SystemExit("sSetupDL table not found")


def main():
    fs = RomFS("ntsc-1.0")
    code = fs.get("code")
    base = find_table(code)
    warns = []
    comp = dl2pica.DLCompiler({CODE_SEG: code}, warn=warns.append)
    meshes = [(i, comp.compile((CODE_SEG << 24) | (base + 48 * i))) for i in range(SETUPDL_MAX)]
    obj = dl2pica.build_pobj(comp.textures, meshes)
    out = os.path.join(HERE, "..", "romfs", "setupdl.pobj")
    open(out, "wb").write(obj)
    print(f"setupdl: table at code+0x{base:X}, {SETUPDL_MAX} presets, {len(obj)} bytes, {len(warns)} warnings,"
          f" unsupported={comp.unsupported}")


main()
