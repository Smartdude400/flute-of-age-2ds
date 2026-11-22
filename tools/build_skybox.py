"""Convert every skybox file (vr_*_static + its vr_*_pal_static) into a textures-only .pobj:
romfs/vrom/<vrom of the static file>.pobj, textures in face order.

Layouts (src/code/z_vr_box.c): 128-type skies hold four 128x64 side faces then a 128x128 top face (and a
128x128 bottom face in the cutscene-map sky), CI8 with a 128-colour palette whose indices sit in either
half of the combined lookup table (so index & 0x7F into the file's own palette); 256-type skies hold
2..4 faces of 256x256 CI8, face k using the k-th 256-colour palette. Palettes are RGBA16 -> RGBA5551 (exact)."""
import os
import struct

from romfs import RomFS
import n64tex
import pica_tex
import dl2pica

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "romfs", "vrom")


def faces(data, pal):
    size = len(data)
    out = []
    if size in (49152, 65536):
        tlut = n64tex.decode_tlut(pal, len(pal) // 2)
        layout = [(128, 64, 128 * 64 * k) for k in range(4)] + [(128, 128, 128 * 64 * 4)]
        if size == 65536:
            layout.append((128, 128, 128 * 64 * 4 + 128 * 128))
        for w, h, off in layout:
            px = [tlut[b & 0x7F] for b in data[off:off + w * h]]
            out.append((w, h, px))
    else:
        for k in range(size // 65536):
            tlut = n64tex.decode_tlut(pal[k * 512:(k + 1) * 512], 256)
            off = k * 65536
            out.append((256, 256, [tlut[b] for b in data[off:off + 65536]]))
    return out


def main():
    fs = RomFS("ntsc-1.0")
    os.makedirs(OUT, exist_ok=True)
    n = 0
    for name in sorted(fs.entries):
        if not (name.startswith("vr_") and name.endswith("_static")) or name.endswith("_pal_static"):
            continue
        pal_name = name[:-len("_static")] + "_pal_static"
        texs = [pica_tex.ptex(px, w, h, pica_tex.GPU_RGBA5551) for w, h, px in faces(fs.get(name), fs.get(pal_name))]
        obj = dl2pica.build_pobj(texs, [])
        vrom = fs.entries[name][0]
        open(os.path.join(OUT, f"{vrom:08X}.pobj"), "wb").write(obj)
        n += 1
    print(f"skybox: {n} files converted")


main()
