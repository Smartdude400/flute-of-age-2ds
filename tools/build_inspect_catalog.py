"""Catalog for the asset inspector build (make INSPECT=1, source/test/inspector.c): for every object of the
object table, its skeletons (with their type) and display lists, by the symbol names of the asset XMLs.
Written to romfs/inspect/catalog.txt (derived from the XMLs only; no ROM data). Lines:
  O <objectId hex> <file name> <segment>
  S <segmented address hex> <normal|flex|lod|flexlod|skin|curve> <symbol>
  D <segmented address hex> <symbol>
  A <segmented address hex> <symbol>        (animations: the inspector plays the first one on the skeletons)
usage: python tools/build_inspect_catalog.py"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import xmlspec  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
INC = os.path.join(HERE, "..", "source", "game", "include", "tables")
OUT = os.path.join(HERE, "..", "romfs", "inspect")


def skel_type(attrs):
    limb = attrs.get("LimbType", "Standard")
    if limb == "Skin":
        return "skin"
    if limb == "Curve" or attrs.get("Type") == "Curve":
        return "curve"
    if limb == "LOD":
        return "flexlod" if attrs.get("Type") == "Flex" else "lod"
    return "flex" if attrs.get("Type") == "Flex" else "normal"


def main():
    table = open(os.path.join(INC, "object_table.h"), encoding="utf-8").read()
    entries = re.findall(r"^/\* 0x([0-9A-Fa-f]+) \*/ DEFINE_OBJECT(?:_UNSET|_EMPTY)?\((\w+)", table, re.M)
    lines, nobj, nskel, ndl = [], 0, 0, 0
    for idx_hex, name in entries:
        if not name.startswith(("object_", "gameplay_")):
            continue
        try:
            spec = xmlspec.load(name)
        except FileNotFoundError:
            continue
        if name not in spec:
            continue
        f = spec[name]
        seg = f["segment"]
        lines.append(f"O {int(idx_hex, 16):X} {name} {seg}")
        nobj += 1
        for tag, sym, off, attrs in f["symbols"]:
            if tag == "Skeleton":
                lines.append(f"S {(seg << 24) | off:08X} {skel_type(attrs)} {sym}")
                nskel += 1
        for tag, sym, off, attrs in f["symbols"]:
            if tag == "Animation":
                lines.append(f"A {(seg << 24) | off:08X} {sym}")
        for tag, sym, off, attrs in f["symbols"]:
            if tag == "DList":
                lines.append(f"D {(seg << 24) | off:08X} {sym}")
                ndl += 1
    os.makedirs(OUT, exist_ok=True)
    with open(os.path.join(OUT, "catalog.txt"), "w", newline="\n") as out:
        out.write("\n".join(lines) + "\n")
    print(f"inspect catalog: {nobj} objects, {nskel} skeletons, {ndl} display lists")


if __name__ == "__main__":
    main()
