"""Original asset files for the runtime display-list interpreter (engine/f3d.c): every object of the object
table and every scene / room file of the scene table, copied from your ROM unchanged (big-endian display lists,
vertices, textures) to romfs/vrom/<vrom>.raw, next to the converted .pobj of the same file. ROM-derived:
generated locally, never committed (romfs/ is ignored).
usage: python tools/build_rawassets.py"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from romfs import RomFS  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
INC = os.path.join(HERE, "..", "source", "game", "include", "tables")
OUT = os.path.join(HERE, "..", "romfs", "vrom")


def main():
    fs = RomFS("ntsc-1.0")
    names = re.findall(r"DEFINE_OBJECT\((\w+),", open(os.path.join(INC, "object_table.h"), encoding="utf-8").read())
    scenes = re.findall(r"DEFINE_SCENE\((\w+),", open(os.path.join(INC, "scene_table.h"), encoding="utf-8").read())
    for sc in scenes:
        base = sc[:-len("_scene")] if sc.endswith("_scene") else sc
        names.append(sc)
        names += sorted((n for n in fs.entries if n.startswith(base + "_room_")),
                        key=lambda n: int(n.rsplit("_", 1)[1]))
    os.makedirs(OUT, exist_ok=True)
    total = count = 0
    for name in names:
        if name not in fs.entries:
            continue
        data = fs.get(name)
        vrom = fs.entries[name][0]
        open(os.path.join(OUT, f"{vrom:08X}.raw"), "wb").write(data)
        total += len(data)
        count += 1
    print(f"raw assets: {count} files, {total / 1e6:.1f} MB")


if __name__ == "__main__":
    main()
