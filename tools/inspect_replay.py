"""Look at an asset-inspector capture again (headless): writes the capture line as the inspector's start
position (sdmc:/3ds/foa/inspect_goto.txt in the test emulator), runs foa_3ds_inspect.3dsx and saves the top
screen. usage:
  python tools/inspect_replay.py <captures.txt> <capture number> [--out evidence/replay_N.png] [--frames 120]
  python tools/inspect_replay.py --line "<capture line>" [--out ...]
Scene captures (mode=fly/play) need more frames: the scene loads first (default 200)."""
import argparse
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import run_test  # noqa: E402

ROOT = os.path.normpath(os.path.join(HERE, ".."))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("captures", nargs="?")
    ap.add_argument("number", nargs="?", type=int)
    ap.add_argument("--line")
    ap.add_argument("--out")
    ap.add_argument("--frames", type=int)
    a = ap.parse_args()
    line = a.line
    if line is None:
        for l in open(a.captures, encoding="utf-8", errors="replace"):
            if l.startswith(f"capture={a.number} "):
                line = l.strip()
        if line is None:
            raise SystemExit(f"capture {a.number} not found in {a.captures}")
    number = a.number if a.number is not None else 0
    scene = " mode=fly" in line or " mode=play" in line
    frames = a.frames or (200 if scene else 60)
    sd = os.path.join(os.path.dirname(run_test.SDLOG), "3ds", "foa")
    os.makedirs(sd, exist_ok=True)
    goto = os.path.join(sd, "inspect_goto.txt")
    with open(goto, "w", newline="\n") as f:
        f.write(line.replace(" mode=play", " mode=fly") + "\n")  # play captures: the game camera, flown to
    out = a.out or os.path.join(ROOT, "evidence", f"replay_{number:03d}.png")
    try:
        r = subprocess.run([sys.executable, os.path.join(HERE, "run_test.py"),
                            os.path.join(ROOT, "foa_3ds_inspect.3dsx"), "--frames", str(frames), "--timeout", "300",
                            "--shot", out], capture_output=True, text=True)
    finally:
        os.remove(goto)
    ok = "OK" in r.stdout
    print(("saved " + out) if ok else r.stdout[-2000:])


if __name__ == "__main__":
    main()
