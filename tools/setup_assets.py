"""One-step asset setup: everything the build needs that comes from YOUR copy of the game.

  python tools/setup_assets.py            (ROM at <repo>/baserom.z64, or FOA_BASEROM=<path>)

1. checks the ROM (NTSC 1.0 US, big-endian .z64, MD5 from tools/romdata/ntsc-1.0/config.yml);
2. generates the ROM-derived headers (file address table, overlay sizes, message tables, kanji table);
3. converts all game data into romfs/ (objects, scenes, render presets, skies, text, font) - native 2DS
   formats, written only on your machine and never committed.
Takes a few minutes; output: romfs/ (~50 MB) and the generated headers listed in .gitignore."""
import hashlib
import os
import re
import runpy
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import romfs  # noqa: E402

STEPS = [
    ("file address table", "gen_segment_symbols.py", []),
    ("overlay memory sizes", "build_actor_table.py", []),
    ("message tables + text", "build_text.py", []),
    ("kanji lookup table", "gen_kanji_tex.py", []),
    ("debug font", "font_conv.py", []),
    ("render-state presets", "build_setupdl.py", []),
    ("skyboxes", "build_skybox.py", []),
    ("display lists inside actor overlays", "build_overlays.py", []),
    ("plain data files", "build_rawdata.py", []),
    ("objects", "build_all.py", ["objects"]),
    ("scenes and rooms", "build_all.py", ["scenes"]),
    ("asset headers used by game code", "gen_asset_headers.py", []),
    ("RomFS manifest", "manifest.py", []),
]


def check_rom():
    path = romfs.find_rom("ntsc-1.0")
    cfg = open(os.path.join(HERE, "romdata", "ntsc-1.0", "config.yml"), encoding="utf-8").read()
    want = re.search(r"md5:\s*([0-9a-f]{32})", cfg).group(1)
    got = hashlib.md5(open(path, "rb").read()).hexdigest()
    if got != want:
        sys.exit(f"ROM check failed: {path}\n  MD5 {got}\n  expected {want} (NTSC 1.0 US, big-endian .z64).\n"
                 "  A .n64 (little-endian) or .v64 (byte-swapped) dump must be converted to .z64 first.")
    print(f"ROM ok: {path}")


def main():
    check_rom()
    t0 = time.time()
    for i, (what, script, args) in enumerate(STEPS, 1):
        print(f"[{i}/{len(STEPS)}] {what} ...", flush=True)
        sys.argv = [script] + args
        cwd = os.getcwd()
        os.chdir(HERE)
        try:
            runpy.run_path(os.path.join(HERE, script), run_name="__main__")
        finally:
            os.chdir(cwd)
    print(f"done in {time.time() - t0:.0f}s - now build with `make` (see README).")


if __name__ == "__main__":
    main()
