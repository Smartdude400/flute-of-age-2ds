"""Write romfs/manifest.txt: '<crc32 hex> <size> <path>' for every file (verified at boot)."""
import os, sys, zlib

root = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "..", "romfs")
lines = []
for dp, _, files in os.walk(root):
    for f in sorted(files):
        if f == "manifest.txt":
            continue
        p = os.path.join(dp, f)
        rel = os.path.relpath(p, root).replace("\\", "/")
        data = open(p, "rb").read()
        lines.append(f"{zlib.crc32(data):08x} {len(data)} /{rel}")
lines.sort(key=lambda l: l.split()[2])
with open(os.path.join(root, "manifest.txt"), "w", newline="\n") as fh:
    fh.write("\n".join(lines) + "\n")
print(f"manifest: {len(lines)} files")
