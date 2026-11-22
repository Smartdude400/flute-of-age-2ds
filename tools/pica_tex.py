"""PICA200 texture encoding for the 3DS port (offline).

Texels are stored in 8x8 tiles, tiles left-to-right then top-to-bottom, texels inside a tile in
Morton (Z) order with x in the low bit. Row 0 is the top of the image. The GPU samples t = 1 at the
first stored row, so draw code uses t_gpu = 1 - t_image (verified on the font, P1). Formats use citro3d's GPU_TEXCOLOR values.
.ptex = header (magic "PTEX", u16 w, u16 h, u8 format, u8 flags, u16 pad, u32 dataSize) + data."""
import struct

GPU_RGBA8, GPU_RGB8, GPU_RGBA5551, GPU_RGB565, GPU_RGBA4, GPU_LA8, GPU_HILO8, GPU_L8, GPU_A8, GPU_LA4, GPU_L4, \
    GPU_A4, GPU_ETC1, GPU_ETC1A4 = range(14)


def morton(x, y):
    m = 0
    for i in range(3):
        m |= ((x >> i) & 1) << (2 * i)
        m |= ((y >> i) & 1) << (2 * i + 1)
    return m


def texel_bytes(fmt, px):
    r, g, b, a = px
    if fmt == GPU_RGBA8:
        return struct.pack("<I", (r << 24) | (g << 16) | (b << 8) | a)
    if fmt == GPU_RGBA5551:
        return struct.pack("<H", ((r >> 3) << 11) | ((g >> 3) << 6) | ((b >> 3) << 1) | (1 if a >= 128 else 0))
    if fmt == GPU_RGB565:
        return struct.pack("<H", ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))
    if fmt == GPU_RGBA4:
        return struct.pack("<H", ((r >> 4) << 12) | ((g >> 4) << 8) | ((b >> 4) << 4) | (a >> 4))
    if fmt == GPU_LA8:
        return struct.pack("<H", (r << 8) | a)
    if fmt == GPU_L8:
        return bytes([r])
    if fmt == GPU_A8:
        return bytes([a])
    if fmt == GPU_LA4:
        return bytes([((r >> 4) << 4) | (a >> 4)])
    raise ValueError(fmt)


def encode(pixels, w, h, fmt):
    """pixels: list of (r,g,b,a) rows top to bottom, w*h entries. w, h: powers of two >= 8."""
    assert w >= 8 and h >= 8 and w & (w - 1) == 0 and h & (h - 1) == 0, (w, h)
    if fmt in (GPU_L4, GPU_A4):
        raise ValueError("4-bit formats: use encode4")
    out = bytearray()
    for ty in range(0, h, 8):
        for tx in range(0, w, 8):
            tile = [None] * 64
            for y in range(8):
                for x in range(8):
                    tile[morton(x, y)] = texel_bytes(fmt, pixels[(ty + y) * w + tx + x])
            out += b"".join(tile)
    return bytes(out)


def ptex(pixels, w, h, fmt, flags=0):
    data = encode(pixels, w, h, fmt)
    return b"PTEX" + struct.pack("<HHBBHI", w, h, fmt, flags, 0, len(data)) + data
