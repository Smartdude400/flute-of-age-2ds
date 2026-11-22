"""Static stats over the exported OBJ/PNG assets: triangles/verts per room and texture bytes per scene."""
import os, sys, glob, struct
from PIL import Image

ROOT = sys.argv[1]
N64_BPP = {"00": 16, "02": 16, "03": 32, "10": 16, "20": 4, "21": 8, "30": 4, "31": 8, "32": 16,
           "40": 4, "41": 8, "42": 16, "43": 32, "60": 4, "61": 8, "62": 16, "70": 4, "71": 8, "72": 16}

def obj_counts(path):
    v = f = 0
    with open(path) as fh:
        for line in fh:
            if line.startswith("v "): v += 1
            elif line.startswith("f "): f += len(line.split()) - 3  # fan triangulation count
    return v, f

def tex_bytes(png):
    parts = os.path.basename(png)[:-4].split("_")
    fmt = parts[2] if len(parts) > 2 else "02"
    w, h = map(int, parts[3].split("x")) if len(parts) > 3 and "x" in parts[3] else Image.open(png).size
    return w * h * N64_BPP.get(fmt, 16) // 8, fmt

rows = []
for scene in sorted(glob.glob(os.path.join(ROOT, "scenes", "*"))):
    rooms = sorted(glob.glob(os.path.join(scene, "*.obj")))
    tris = [obj_counts(r)[1] for r in rooms]
    verts = [obj_counts(r)[0] for r in rooms]
    tb = sum(tex_bytes(p)[0] for p in glob.glob(os.path.join(scene, "*.png")))
    rows.append((os.path.basename(scene), len(rooms), max(tris or [0]), sum(tris), max(verts or [0]), tb))
print("scene,rooms,max_room_tris,total_tris,max_room_verts,tex_bytes_n64")
for r in rows: print(",".join(map(str, r)))
fmts = {}
for p in glob.glob(os.path.join(ROOT, "**", "*.png"), recursive=True):
    b, fm = tex_bytes(p); fmts[fm] = fmts.get(fm, 0) + 1
print("# texture format histogram (N64 fmt/siz code -> count):", dict(sorted(fmts.items())), file=sys.stderr)
