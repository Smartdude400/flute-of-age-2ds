"""Offline ROM file-system reader (build-time only).

Reads the human-supplied ROM in place, parses the DMA table (16-byte entries:
vromStart, vromEnd, romStart, romEnd; romEnd == 0 means uncompressed, written from
include/dma.h), names entries by order from baseroms/<ver>/segments.csv, and
decompresses Yaz0 files (algorithm reimplemented from src/boot/yaz0.c).

The ROM is the user's own copy: <repo>/baserom.z64, or the path in the FOA_BASEROM environment variable.
Its layout data (DMA table position, file names in table order) is in tools/romdata/<version>/.
Nothing produced here is the ROM itself: the converters consume it and emit native 2DS formats.
"""
import csv, os, struct, re

HERE = os.path.dirname(os.path.abspath(__file__))
ROMDATA = os.path.join(HERE, "romdata")


def find_rom(version):
    """The user's ROM: FOA_BASEROM, <repo>/baserom.z64, or a decomp checkout next to the repo (development)."""
    for p in (os.environ.get("FOA_BASEROM"), os.path.join(HERE, "..", "baserom.z64"),
              os.path.join(HERE, "..", "..", "flute-of-age", "baseroms", version, "baserom.z64")):
        if p and os.path.exists(p):
            return p
    raise SystemExit("ROM not found: put your NTSC 1.0 (US) ROM at baserom.z64 in the repository root "
                     "(big-endian .z64), or set FOA_BASEROM to its path.")
CACHE = os.path.join(os.path.dirname(__file__), "..", ".cache", "romcache")


def yaz0_decompress(src: bytes) -> bytes:
    assert src[:4] == b"Yaz0", "not Yaz0"
    dec_size = struct.unpack_from(">I", src, 4)[0]
    dst = bytearray(dec_size)
    s, d = 16, 0
    while d < dec_size:
        header = src[s]; s += 1
        for bit in range(8):
            if d >= dec_size:
                break
            if header & (0x80 >> bit):
                dst[d] = src[s]; d += 1; s += 1
            else:
                b0, b1 = src[s], src[s + 1]; s += 2
                off = ((b0 & 0xF) << 8 | b1) + 1
                n = b0 >> 4
                if n == 0:
                    n = src[s] + 0x12; s += 1
                else:
                    n += 2
                back = d - off
                if off >= n:
                    dst[d:d + n] = dst[back:back + n]
                    d += n
                else:  # overlapping copy, byte by byte
                    for _ in range(n):
                        dst[d] = dst[back]; d += 1; back += 1
    return bytes(dst)


class RomFS:
    def __init__(self, version="ntsc-1.0"):
        self.version = version
        base = os.path.join(ROMDATA, version)
        with open(find_rom(version), "rb") as fh:
            self.rom = fh.read()
        cfg = open(os.path.join(base, "config.yml")).read()
        self.dmadata_start = int(re.search(r"dmadata_start:\s*(0x[0-9A-Fa-f]+)", cfg).group(1), 16)
        with open(os.path.join(base, "segments.csv")) as fh:
            rows = list(csv.reader(fh))[1:]
        self.names = [r[0] for r in rows]
        self.entries = {}
        off = self.dmadata_start
        for idx, name in enumerate(self.names):
            vs, ve, rs, re_ = struct.unpack_from(">IIII", self.rom, off + idx * 16)
            if vs == 0 and ve == 0 and idx > 0:
                break
            self.entries[name] = (vs, ve, rs, re_)
        os.makedirs(os.path.join(CACHE, version), exist_ok=True)

    def get(self, name: str) -> bytes:
        cpath = os.path.join(CACHE, self.version, name + ".bin")
        if os.path.exists(cpath):
            return open(cpath, "rb").read()
        vs, ve, rs, re_ = self.entries[name]
        if re_ == 0:
            data = self.rom[rs:rs + (ve - vs)]
        else:
            data = yaz0_decompress(self.rom[rs:re_])
            assert len(data) == ve - vs, (name, len(data), ve - vs)
        with open(cpath, "wb") as fh:
            fh.write(data)
        return data


if __name__ == "__main__":
    import sys, zlib
    fs = RomFS(sys.argv[1] if len(sys.argv) > 1 else "ntsc-1.0")
    print("dma entries:", len(fs.entries))
    for n in sys.argv[2:] or ["gameplay_keep", "spot04_scene", "spot04_room_0", "object_aren_child"]:
        d = fs.get(n)
        print(f"{n}: {len(d)} bytes crc32={zlib.crc32(d):08x}")
