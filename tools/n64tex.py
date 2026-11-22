"""Decode original texel formats (build time only) into RGBA8 lists.

Format codes follow the G_IM_FMT / G_IM_SIZ pairs used by the exported texture names:
fmt 0=RGBA 2=CI 3=IA 4=I ; siz 0=4b 1=8b 2=16b 3=32b. Data is stored big-endian and
row-linear in the ROM.
"""
import struct

FMT_RGBA, FMT_YUV, FMT_CI, FMT_IA, FMT_I = 0, 1, 2, 3, 4
SIZ_4, SIZ_8, SIZ_16, SIZ_32 = 0, 1, 2, 3
BITS = {SIZ_4: 4, SIZ_8: 8, SIZ_16: 16, SIZ_32: 32}


def tex_bytes(fmt, siz, w, h):
    return w * h * BITS[siz] // 8


def rgba5551(v):
    r = (v >> 11) & 31; g = (v >> 6) & 31; b = (v >> 1) & 31; a = v & 1
    return ((r << 3) | (r >> 2), (g << 3) | (g >> 2), (b << 3) | (b >> 2), 255 if a else 0)


def decode_tlut(data, count, ia=False):
    out = []
    for i in range(count):
        v = struct.unpack_from(">H", data, i * 2)[0]
        if ia:
            out.append((v >> 8, v >> 8, v >> 8, v & 0xFF))
        else:
            out.append(rgba5551(v))
    return out


def decode(data, fmt, siz, w, h, tlut=None):
    """Returns (pixels, indices). pixels: list of RGBA tuples. indices: list or None for CI."""
    n = w * h
    px, idx = [], None
    if fmt == FMT_RGBA and siz == SIZ_16:
        for i in range(n):
            px.append(rgba5551(struct.unpack_from(">H", data, i * 2)[0]))
    elif fmt == FMT_RGBA and siz == SIZ_32:
        for i in range(n):
            px.append(tuple(data[i * 4:i * 4 + 4]))
    elif fmt == FMT_CI:
        idx = []
        for i in range(n):
            if siz == SIZ_4:
                b = data[i >> 1]
                idx.append((b >> 4) if (i & 1) == 0 else (b & 0xF))
            else:
                idx.append(data[i])
        px = [tlut[k] if tlut and k < len(tlut) else (255, 0, 255, 255) for k in idx]
    elif fmt == FMT_I:
        for i in range(n):
            if siz == SIZ_4:
                b = data[i >> 1]; v = (b >> 4) if (i & 1) == 0 else (b & 0xF); v = v * 17
            else:
                v = data[i]
            px.append((v, v, v, v))
    elif fmt == FMT_IA:
        for i in range(n):
            if siz == SIZ_4:
                b = data[i >> 1]; nib = (b >> 4) if (i & 1) == 0 else (b & 0xF)
                iv = ((nib >> 1) * 255) // 7; a = 255 if nib & 1 else 0
            elif siz == SIZ_8:
                b = data[i]; iv = (b >> 4) * 17; a = (b & 0xF) * 17
            else:
                iv = data[i * 2]; a = data[i * 2 + 1]
            px.append((iv, iv, iv, a))
    else:
        raise ValueError(f"unsupported texture fmt={fmt} siz={siz}")
    return px, idx
