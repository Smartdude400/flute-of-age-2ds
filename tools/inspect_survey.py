"""Survey the asset inspector: capture a run of objects (first part: the first skeleton, else the first list) in
one emulator run and lay the captures out as labelled contact sheets.

  python tools/inspect_survey.py [--start 0] [--count 100] [--per 45] [--out evidence/survey]

Needs the inspector build (make INSPECT=1). Objects are taken in catalog order (romfs/inspect/catalog.txt);
each is captured `per` retraces after it was selected. Output: <out>/sheet_<first>.png (4x3 per sheet) and
<out>/survey_<first>.txt (the capture lines).
"""
import argparse
import os
import shutil
import subprocess
import sys

from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")
sys.path.insert(0, HERE)
import run_test  # noqa: E402


def objects():
    out = []
    for line in open(os.path.join(ROOT, "romfs", "inspect", "catalog.txt"), encoding="utf-8"):
        if line.startswith("O "):
            _, oid, name = line.split()[:3]
            out.append((int(oid, 16), name))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--start", type=int, default=0)
    ap.add_argument("--count", type=int, default=100)
    ap.add_argument("--per", type=int, default=45, help="retraces per object (60 = 1 s)")
    ap.add_argument("--out", default=os.path.join(ROOT, "evidence", "survey"))
    a = ap.parse_args()
    objs = objects()[a.start:a.start + a.count]
    sd = os.path.join(os.path.dirname(run_test.SDLOG), "3ds", "foa")
    os.makedirs(sd, exist_ok=True)
    for f in os.listdir(sd):
        if f.startswith("inspect_") and (f.endswith(".bmp") or f == "inspect_captures.txt"):
            os.remove(os.path.join(sd, f))
    with open(os.path.join(sd, "inspect_goto.txt"), "w", newline="\n") as f:
        f.write(f"capture=0 mode=objects object=0x{objs[0][0]:04X} part=0\n")
    # per object: capture (SELECT), then the next object (A); the first capture after the boot settles
    t0, lines = 300, ["# inspector survey (tools/inspect_survey.py)"]
    for i in range(len(objs)):
        t = t0 + i * a.per
        lines += [f"{t} SELECT", f"{t + 6} -", f"{t + 9} A", f"{t + 15} -"]
    script = os.path.join(sd, "survey_input.txt")
    open(script, "w", newline="\n").write("\n".join(lines) + "\n")
    frames = (t0 + len(objs) * a.per) // 3 + 20
    try:
        r = subprocess.run([sys.executable, os.path.join(HERE, "run_test.py"),
                            os.path.join(ROOT, "foa_3ds_inspect.3dsx"), "--frames", str(frames),
                            "--timeout", "590", "--script", script], capture_output=True, text=True)
    finally:
        os.remove(os.path.join(sd, "inspect_goto.txt"))
    if "OK" not in r.stdout:
        print(r.stdout[-1500:])
    caps = {}
    cap_file = os.path.join(sd, "inspect_captures.txt")
    if os.path.exists(cap_file):
        for l in open(cap_file, encoding="utf-8", errors="replace"):
            if l.startswith("capture="):
                caps[int(l.split()[0][8:])] = l.strip()
    os.makedirs(a.out, exist_ok=True)
    shutil.copy(cap_file, os.path.join(a.out, f"survey_{a.start:03d}.txt")) if os.path.exists(cap_file) else None
    n = len(caps)
    cols, rows, W, H = 4, 3, 320, 240
    for s in range(0, n, cols * rows):
        sheet = Image.new("RGB", (cols * W, rows * (H + 14)), (20, 20, 20))
        d = ImageDraw.Draw(sheet)
        for k in range(cols * rows):
            i = s + k
            bmp = os.path.join(sd, f"inspect_{i:03d}.bmp")
            if i not in caps or not os.path.exists(bmp):
                continue
            im = Image.open(bmp).convert("RGB")
            im = im.crop(((im.width - W) // 2, 0, (im.width - W) // 2 + W, H)) if im.width > W else im
            x, y = (k % cols) * W, (k // cols) * (H + 14)
            sheet.paste(im, (x, y + 14))
            f = dict(kv.split("=", 1) for kv in caps[i].split() if "=" in kv)
            d.text((x + 3, y + 1), f"{a.start + i} {f.get('name', '?')} {f.get('symbol', '')}", fill=(255, 230, 120))
        sheet.save(os.path.join(a.out, f"sheet_{a.start + s:03d}.png"))
    print(f"{n} captures -> {a.out}")


if __name__ == "__main__":
    main()
