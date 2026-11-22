"""Convert every game asset into romfs/ (offline): all objects of the object table, all scenes of the
scene table, the render-state presets, skyboxes, message text, the debug font and the manifest.
usage: python tools/build_all.py [objects|scenes|all]   (report: build_all_report.txt)"""
import os
import re
import sys
import time
import traceback

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from romfs import RomFS  # noqa: E402
import build_object  # noqa: E402
import build_scene  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
INC = os.path.join(HERE, "..", "source", "game", "include", "tables")


def main():
    what = sys.argv[1] if len(sys.argv) > 1 else "all"
    fs = RomFS("ntsc-1.0")
    report, failed = [], []
    t0 = time.time()
    if what in ("objects", "all"):
        objs = re.findall(r"DEFINE_OBJECT\((\w+),", open(os.path.join(INC, "object_table.h"), encoding="utf-8").read())
        for name in objs:
            try:
                build_object.build(fs, name, report)
            except SystemExit as e:
                failed.append(f"object {name}: {e}")
            except Exception as e:  # noqa: BLE001 - report and continue
                failed.append(f"object {name}: {type(e).__name__}: {e}")
        build_object.build_player_anim_data(fs)
    if what in ("scenes", "all"):
        scenes = re.findall(r"DEFINE_SCENE\((\w+),", open(os.path.join(INC, "scene_table.h"), encoding="utf-8").read())
        for name in scenes:
            base = name[:-len("_scene")] if name.endswith("_scene") else name
            warns = []
            try:
                build_scene.SceneConverter(fs, base, warns.append).build(report)
            except SystemExit as e:
                failed.append(f"scene {base}: {e}")
            except Exception as e:  # noqa: BLE001
                failed.append(f"scene {base}: {type(e).__name__}: {e} | {traceback.format_exc().splitlines()[-3]}")
    out = os.path.join(HERE, "..", "build_all_report.txt")
    with open(out, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("\n".join(report) + "\n\nFAILED:\n" + "\n".join(failed) + "\n")
    print(f"{what}: {len(report)} report lines, {len(failed)} failures, {time.time() - t0:.0f}s -> {out}")


main()
