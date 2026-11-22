"""Copy original game headers into source/game/include for adaptation.

The platform umbrella header ("ultra64.h") is replaced with the DS-native "game_types.h",
which defines scalar types only (no platform APIs). Any header that then needs a platform type
fails to compile and is fixed by hand. That's intended: it surfaces every platform
dependency instead of shimming it. Each copied header gets a provenance banner, and its status
is tracked in NATIVENESS.md.

usage: python tools/import_header.py save.h inventory.h ...
"""
import os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC_INC = os.path.join(HERE, "..", "..", "flute-of-age", "include")
DST_INC = os.path.join(HERE, "..", "source", "game", "include")

REPO = os.path.join(HERE, "..", "..", "flute-of-age")
for name in sys.argv[1:]:
    src = os.path.join(SRC_INC, name)
    if name.startswith("src/"):
        src = os.path.join(REPO, name)
    dst = os.path.join(DST_INC, name)
    if os.path.exists(dst):
        print("exists, skipped:", name)
        continue
    text = open(src, encoding="utf-8").read()
    text = re.sub(r'#include "ultra64(\.h|/ultratypes\.h)"', '#include "game_types.h"', text)
    text = f"// adapted from flute-of-age/include/{name} (ultra64.h -> game_types.h)\n" + text
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    open(dst, "w", encoding="utf-8", newline="\n").write(text)
    incs = re.findall(r'#include "([^"]+)"', text)
    missing = [i for i in incs if not os.path.exists(os.path.join(DST_INC, i))]
    print(f"imported {name}; not yet imported deps: {' '.join(missing) or '-'}")
