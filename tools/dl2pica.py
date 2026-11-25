"""Offline display-list compiler: original F3DEX2 display lists -> 2DS-native mesh programs.

BUILD TIME ONLY. A display list is walked once with the full RSP/RDP state model (vertex cache, matrix
stack, TMEM loads, tiles, TLUT, combiner, other modes, geometry mode, colours) and compiled into:

  * a vertex array (16-byte native vertices, transformed later by the PICA200 vertex shader),
  * a u16 index array (triangles in their original order),
  * an op program: state changes, texture bindings and draws IN THE ORIGINAL COMMAND ORDER, with every
    field already decoded into the renderer's native enums (see source/engine/pmesh.h). Calls to
    display lists in dynamic segments become OP_CALLSEG (resolved by the renderer at draw time),
  * converted textures (tools/pica_tex.py layouts), one per distinct (image, format, tile, TLUT).

Nothing original is interpreted at run time. Encodings: include/ultra64/gbi.h (F3DEX_GBI_2).
"""
import copy
import struct

import n64tex
import pica_tex

# ---- F3DEX2 opcodes
G_SPNOOP, G_VTX, G_MODIFYVTX, G_CULLDL, G_BRANCH_Z, G_TRI1, G_TRI2, G_QUAD = 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
G_TEXTURE, G_POPMTX, G_GEOMETRYMODE, G_MTX, G_MOVEWORD, G_MOVEMEM = 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC
G_DL, G_ENDDL, G_RDPHALF_1, G_SETOTHERMODE_L, G_SETOTHERMODE_H = 0xDE, 0xDF, 0xE1, 0xE2, 0xE3
G_TEXRECT, G_TEXRECTFLIP, G_RDPLOADSYNC, G_RDPPIPESYNC, G_RDPTILESYNC, G_RDPFULLSYNC = 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9
G_SETSCISSOR, G_SETPRIMDEPTH, G_RDPSETOTHERMODE = 0xED, 0xEE, 0xEF
G_LOADTLUT, G_SETTILESIZE, G_LOADBLOCK, G_LOADTILE, G_SETTILE = 0xF0, 0xF2, 0xF3, 0xF4, 0xF5
G_FILLRECT, G_SETFILLCOLOR, G_SETFOGCOLOR, G_SETBLENDCOLOR, G_SETPRIMCOLOR, G_SETENVCOLOR = 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB
G_SETCOMBINE, G_SETTIMG, G_SETZIMG, G_SETCIMG = 0xFC, 0xFD, 0xFE, 0xFF
G_MW_NUMLIGHT, G_MW_SEGMENT, G_MW_FOG, G_MW_LIGHTCOL = 0x02, 0x06, 0x08, 0x0A
G_MV_LIGHT = 0x0A

DYNAMIC_DL_SEGMENTS = (0x01, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D)  # set at draw time by game code
DYNAMIC_TEX_SEGMENTS = (0x08, 0x09, 0x0A, 0x0B, 0x0C)
# vertex buffers the game fills at runtime and binds to a segment (skinned limbs: segment 8)
DYNAMIC_VTX_SEGMENTS = (0x08,)
MTX_SEGMENT = 0x0D

# ---- native op codes (source/engine/pmesh.h)
OP_END, OP_DRAW, OP_GEOM, OP_MODE, OP_COMBINE, OP_PRIM, OP_ENV, OP_TEX, OP_TEXSCALE, OP_CALLSEG, OP_LIGHTS, \
    OP_FOGCOLOR, OP_BLENDCOLOR = range(13)

# ---- native geometry flags (pmesh.h GEOM_*), same meaning as the original geometry mode bits
GEOM_ZBUFFER, GEOM_SHADE, GEOM_CULL_FRONT, GEOM_CULL_BACK, GEOM_FOG, GEOM_LIGHTING, GEOM_TEXGEN, \
    GEOM_TEXGEN_LINEAR, GEOM_SMOOTH = (1 << i for i in range(9))
GEOM_MAP = {0x1: GEOM_ZBUFFER, 0x4: GEOM_SHADE, 0x200: GEOM_CULL_FRONT, 0x400: GEOM_CULL_BACK, 0x10000: GEOM_FOG,
            0x20000: GEOM_LIGHTING, 0x40000: GEOM_TEXGEN, 0x80000: GEOM_TEXGEN_LINEAR, 0x200000: GEOM_SMOOTH}


def geom_native(bits):
    out = 0
    for k, v in GEOM_MAP.items():
        if bits & k:
            out |= v
    return out


# ---- native render-mode word (pmesh.h MODE_*): fields decoded from the two other-mode words
#   0-1 cycle type  2-3 texture filter (0 point, 2 bilerp, 3 average)  4-5 alpha compare (0 none, 1 thresh, 3 dither)
#   6 z compare  7 z update  8-9 z mode (0 opaque, 1 interpenetrating, 2 translucent, 3 decal)
#   10 coverage x alpha  11 alpha coverage select  12 force blend  13 image read  14-15 TLUT type
#   16-23 blender cycle 1 (P:2 A:2 M:2 B:2)  24-31 blender cycle 2
def mode_fields(omh, oml):
    v = 0
    v |= (omh >> 20) & 3
    v |= ((omh >> 12) & 3) << 2
    v |= (oml & 3) << 4
    v |= ((oml >> 4) & 1) << 6
    v |= ((oml >> 5) & 1) << 7
    v |= ((oml >> 10) & 3) << 8
    v |= ((oml >> 12) & 1) << 10
    v |= ((oml >> 13) & 1) << 11
    v |= ((oml >> 14) & 1) << 12
    v |= ((oml >> 6) & 1) << 13
    v |= ((omh >> 14) & 3) << 14
    c1 = (((oml >> 30) & 3) << 6) | (((oml >> 26) & 3) << 4) | (((oml >> 22) & 3) << 2) | ((oml >> 18) & 3)
    c2 = (((oml >> 28) & 3) << 6) | (((oml >> 24) & 3) << 4) | (((oml >> 20) & 3) << 2) | ((oml >> 16) & 3)
    v |= c1 << 16
    v |= c2 << 24
    return v


def mode_mask(mask_h, mask_l):
    """Which native mode bits a partial other-mode update touches."""
    return mode_fields(mask_h, mask_l)


# ---- native combiner operands (pmesh.h CC_*)
CC_COMBINED, CC_TEXEL0, CC_TEXEL1, CC_PRIM, CC_SHADE, CC_ENV, CC_ONE, CC_ZERO, CC_NOISE, CC_CENTER, CC_K4, CC_SCALE, \
    CC_COMBINED_A, CC_TEXEL0_A, CC_TEXEL1_A, CC_PRIM_A, CC_SHADE_A, CC_ENV_A, CC_LOD_FRAC, CC_PRIM_LOD_FRAC, CC_K5 = range(21)
_CA = [CC_COMBINED, CC_TEXEL0, CC_TEXEL1, CC_PRIM, CC_SHADE, CC_ENV, CC_ONE, CC_NOISE] + [CC_ZERO] * 8
_CB = [CC_COMBINED, CC_TEXEL0, CC_TEXEL1, CC_PRIM, CC_SHADE, CC_ENV, CC_CENTER, CC_K4] + [CC_ZERO] * 8
_CC = [CC_COMBINED, CC_TEXEL0, CC_TEXEL1, CC_PRIM, CC_SHADE, CC_ENV, CC_SCALE, CC_COMBINED_A, CC_TEXEL0_A,
       CC_TEXEL1_A, CC_PRIM_A, CC_SHADE_A, CC_ENV_A, CC_LOD_FRAC, CC_PRIM_LOD_FRAC, CC_K5] + [CC_ZERO] * 16
_CD = [CC_COMBINED, CC_TEXEL0, CC_TEXEL1, CC_PRIM, CC_SHADE, CC_ENV, CC_ONE, CC_ZERO]
_AABD = [CC_COMBINED, CC_TEXEL0, CC_TEXEL1, CC_PRIM, CC_SHADE, CC_ENV, CC_ONE, CC_ZERO]
_AC = [CC_LOD_FRAC, CC_TEXEL0, CC_TEXEL1, CC_PRIM, CC_SHADE, CC_ENV, CC_PRIM_LOD_FRAC, CC_ZERO]


def combine_native(w0, w1):
    """-> 16 bytes: cycle0 rgb a,b,c,d, alpha a,b,c,d, cycle1 rgb a,b,c,d, alpha a,b,c,d."""
    a0 = (w0 >> 20) & 0xF; c0 = (w0 >> 15) & 0x1F; Aa0 = (w0 >> 12) & 7; Ac0 = (w0 >> 9) & 7
    a1 = (w0 >> 5) & 0xF; c1 = w0 & 0x1F
    b0 = (w1 >> 28) & 0xF; b1 = (w1 >> 24) & 0xF; Aa1 = (w1 >> 21) & 7; Ac1 = (w1 >> 18) & 7
    d0 = (w1 >> 15) & 7; Ab0 = (w1 >> 12) & 7; Ad0 = (w1 >> 9) & 7; d1 = (w1 >> 6) & 7; Ab1 = (w1 >> 3) & 7; Ad1 = w1 & 7
    return bytes([_CA[a0], _CB[b0], _CC[c0], _CD[d0], _AABD[Aa0], _AABD[Ab0], _AC[Ac0], _AABD[Ad0],
                  _CA[a1], _CB[b1], _CC[c1], _CD[d1], _AABD[Aa1], _AABD[Ab1], _AC[Ac1], _AABD[Ad1]])


def combine_uses_texel(cb):
    return any(x in (CC_TEXEL0, CC_TEXEL1, CC_TEXEL0_A, CC_TEXEL1_A) for x in cb)


# ---- matrix sources (pmesh.h MSRC_*)
MSRC_BASE, MSRC_LIMB, MSRC_SEGMUL = 0, 1, 2


def pow2ceil(v):
    p = 8
    while p < v:
        p <<= 1
    return p


class DLCompiler:
    def __init__(self, segments, warn=None):
        """segments: {seg_id: bytes} for statically known segments (object/scene/keep data)."""
        self.segments = dict(segments)
        self.textures = []  # PTEX blobs
        self.tex_index = {}
        self.warn = warn or (lambda m: None)
        self.unsupported = {}

    def read(self, addr, n):
        seg, off = addr >> 24, addr & 0xFFFFFF
        if seg not in self.segments:
            raise KeyError(f"segment {seg:02X} not available (addr {addr:08X})")
        d = self.segments[seg][off:off + n]
        if len(d) < n:
            raise IndexError(f"read past end at {addr:08X}+{n}")
        return d

    def note(self, what):
        self.unsupported[what] = self.unsupported.get(what, 0) + 1

    # ------------------------------------------------------------------ textures
    def convert_texture(self, src):
        """src: dict(addr, fmt, siz, w, h, stride_bytes, tlut_addr, tlut_ia, pal, mask_s, mask_t, clamp_s, clamp_t).
        Returns (index, W, H) of the converted PICA texture."""
        key = tuple(sorted(src.items()))
        if key in self.tex_index:
            return self.tex_index[key]
        fmt, siz, w, h = src["fmt"], src["siz"], src["w"], src["h"]
        bpp = n64tex.BITS[siz]
        raw = bytearray()
        row_bytes = w * bpp // 8
        # rows actually present: limited by the texels the load brought into TMEM and by the file end;
        # rows past them (stale TMEM on the original) repeat the loaded rows
        avail = h
        if src.get("texels"):
            avail = max(1, min(avail, src["texels"] * bpp // 8 // max(1, row_bytes)))
        rows = []
        for y in range(h):
            if y < avail:
                try:
                    rows.append(self.read(src["addr"] + y * src["stride"], row_bytes))
                    continue
                except IndexError:
                    avail = max(1, y)
                    self.note("texture-rows-past-data")
            rows.append(rows[y % avail])
        raw = b"".join(rows)
        tlut = None
        if fmt == n64tex.FMT_CI:
            ncol = 16 if siz == n64tex.SIZ_4 else 256
            pal_off = src["pal"] * 16 * 2 if siz == n64tex.SIZ_4 else 0
            tlut = n64tex.decode_tlut(self.read(src["tlut_addr"] + pal_off, ncol * 2), ncol, ia=src["tlut_ia"])
        px, _ = n64tex.decode(bytes(raw), fmt, siz, w, h, tlut)
        # PICA size: the wrap period (mask) if set, else the next power of two >= 8
        W = pow2ceil(1 << src["mask_s"]) if src["mask_s"] else pow2ceil(w)
        H = pow2ceil(1 << src["mask_t"]) if src["mask_t"] else pow2ceil(h)
        W = min(W, 1024)
        H = min(H, 1024)
        out = []
        for y in range(H):
            sy = y % h if y < h or not src["clamp_t"] else h - 1  # wrap or replicate the last row
            if src["mask_t"] and (1 << src["mask_t"]) < h:
                sy = y % (1 << src["mask_t"])
            for x in range(W):
                sx = x % w if x < w or not src["clamp_s"] else w - 1
                out.append(px[sy * w + sx])
        pfmt = pica_format(fmt, siz, src["tlut_ia"])
        blob = pica_tex.ptex(out, W, H, pfmt)
        self.tex_index[key] = (len(self.textures), W, H)
        self.textures.append(blob)
        return self.tex_index[key]

    def tile_texture(self, st, tile_no):
        """Texture state for one tile at draw time -> OP_TEX payload words (or None = no texture)."""
        tile = st["tiles"][tile_no]
        if tile is None:
            return None
        load = st["tmem"].get(tile["tmem"])
        if load is None:
            return None
        w = ((tile["lrs"] - tile["uls"]) >> 2) + 1 if tile["lrs"] > tile["uls"] else 0
        h = ((tile["lrt"] - tile["ult"]) >> 2) + 1 if tile["lrt"] > tile["ult"] else 0
        if w == 0 or tile_no in st["dynsize"]:  # size set by a runtime list (texture scroll): use the mask period
            w = (1 << tile["masks"]) if tile["masks"] else max(w, 1)
        if h == 0 or tile_no in st["dynsize"]:
            h = (1 << tile["maskt"]) if tile["maskt"] else max(h, 1)
        if tile["masks"]:
            w = min(w, 1 << tile["masks"]) if not (tile["cms"] & 2) else w
        if tile["maskt"]:
            h = min(h, 1 << tile["maskt"]) if not (tile["cmt"] & 2) else h
        # bit0 clamp s, bit1 mirror s, bit2 clamp t, bit3 mirror t, bit4 tile offsets come from the runtime tile
        # a tile without a wrap mask is clamped to its size by the RDP, whatever its clamp bit says
        flags = (1 if (tile["cms"] & 2) or not tile["masks"] else 0) | (2 if tile["cms"] & 1 and tile["masks"] else 0) | \
                (4 if (tile["cmt"] & 2) or not tile["maskt"] else 0) | (8 if tile["cmt"] & 1 and tile["maskt"] else 0) | \
                (16 if tile_no in st["dynsize"] else 0)
        seg = load["addr"] >> 24
        uls, ult = tile["uls"], tile["ult"]
        shifts = tile["shifts"] | (tile["shiftt"] << 4)
        if seg in DYNAMIC_TEX_SEGMENTS:
            # bound at draw time (Gfx3D_SetSegment); the renderer uses the bound texture's own size
            return [OP_TEX | (0 << 16), (2 << 16) | seg, flags | (tile["fmt"] << 8) | (tile["siz"] << 12),
                    (w & 0xFFFF) | (h << 16), (uls & 0xFFFF) | (ult << 16), shifts]
        if load["kind"] == "block":
            stride = tile["line"] * 8 if tile["line"] else w * n64tex.BITS[tile["siz"]] // 8
            addr = load["addr"]
        else:  # loadtile: rectangle of a wider image
            stride = load["width"] * n64tex.BITS[load["siz"]] // 8
            addr = load["addr"] + (load["ult"] >> 2) * stride + ((load["uls"] >> 2) * n64tex.BITS[load["siz"]] // 8)
        src = {"addr": addr, "fmt": tile["fmt"], "siz": tile["siz"], "w": w, "h": h, "stride": stride,
               "tlut_addr": st["tlut"] if tile["fmt"] == n64tex.FMT_CI else 0,
               "tlut_ia": ((st["omh"] >> 14) & 3) == 3, "pal": tile["palette"],
               "mask_s": tile["masks"], "mask_t": tile["maskt"],
               "clamp_s": bool(tile["cms"] & 2) or not tile["masks"], "clamp_t": bool(tile["cmt"] & 2) or not tile["maskt"],
               "texels": (load["texels"] * 8 // n64tex.BITS[tile["siz"]]) if load.get("texels") else 0}
        if tile["fmt"] == n64tex.FMT_CI and (src["tlut_addr"] == 0 or (src["tlut_addr"] >> 24) in DYNAMIC_TEX_SEGMENTS):
            self.note("ci-without-static-tlut")
            return None
        try:
            idx, W, H = self.convert_texture(src)
        except (KeyError, ValueError, IndexError, struct.error) as e:
            self.warn(f"texture {load['addr']:08X}: {e}")
            return None
        return [OP_TEX, (1 << 16) | idx, flags | (tile["fmt"] << 8) | (tile["siz"] << 12),
                (W & 0xFFFF) | (H << 16), (uls & 0xFFFF) | (ult << 16), shifts]

    # ------------------------------------------------------------------ main
    def compile(self, dl_addr, prelude=None):
        """prelude: address of a list the game always runs right before this one (e.g. a texture-load list):
        its texture memory / tile / other-mode state is the starting state here (its own ops run at runtime)."""
        st = {"vtx": [None] * 64, "tiles": [None] * 8, "img": None, "tmem": {}, "tlut": 0, "omh": 0, "oml": 0,
              "tex_tile": 0, "mstack": [("base", 0)], "rdphalf": 0, "dynsize": set()}
        if prelude is not None:
            self.compile(prelude)
            for k in ("tiles", "img", "tmem", "tlut", "omh", "oml", "tex_tile"):
                st[k] = copy.deepcopy(self.last_state[k])
        verts = []          # native vertex records (bytes)
        indices = []        # u16
        ops = []            # u32 words
        palette = []        # (type, id)
        batch = {"start": None}
        emitted_tex = [None, None]

        def pal_index(src):
            if src not in palette:
                if len(palette) >= 8:
                    raise RuntimeError("more than 8 matrix sources in one display list")
                palette.append(src)
            return palette.index(src)

        def flush():
            if batch["start"] is not None and len(indices) > batch["start"]:
                ops.extend([OP_DRAW, batch["start"], len(indices) - batch["start"]])
            batch["start"] = None

        def op(*words):
            flush()
            ops.extend(words)

        def cur_msrc():
            kind, val = st["mstack"][-1]
            if kind == "base":
                return (MSRC_BASE, 0)
            if kind == "limb":
                return (MSRC_LIMB, val)
            return (MSRC_SEGMUL, val)

        stack, pc, steps = [], dl_addr, 0
        while pc is not None:
            steps += 1
            if steps > 200000:
                raise RuntimeError("display list too long / loop")
            w0, w1 = struct.unpack(">II", self.read(pc, 8))
            opc = w0 >> 24
            pc += 8
            if opc == G_VTX and (w1 >> 24) in DYNAMIC_VTX_SEGMENTS and (w1 >> 24) not in self.segments:
                # runtime vertices: placeholders holding the index into the game's buffer (x); the renderer
                # builds the real vertices from that buffer at replay (pmesh.h PMeshHdr.vtxSeg)
                n = (w0 >> 12) & 0xFF
                v0 = ((w0 >> 1) & 0x7F) - n
                if st.get("vtxseg") not in (None, w1 >> 24) or (verts and st.get("vtxseg") is None):
                    raise RuntimeError("static and runtime vertices in one list")
                st["vtxseg"] = w1 >> 24
                src = pal_index(cur_msrc())
                base = (w1 & 0xFFFFFF) // 16
                for i in range(n):
                    st["vtx"][v0 + i] = len(verts)
                    verts.append(struct.pack("<HhhBBhhBBBB", base + i, 0, 0, src, 0, 0, 0, 0, 0, 0, 0))
            elif opc == G_VTX:
                if st.get("vtxseg") is not None:
                    raise RuntimeError("static and runtime vertices in one list")
                n = (w0 >> 12) & 0xFF
                v0 = ((w0 >> 1) & 0x7F) - n
                data = self.read(w1, n * 16)
                src = pal_index(cur_msrc())
                for i in range(n):
                    x, y, z, _flag, s, t, a, b, c, d = struct.unpack_from(">hhhHhhBBBB", data, i * 16)
                    st["vtx"][v0 + i] = len(verts)
                    verts.append(struct.pack("<hhhBBhhBBBB", x, y, z, src, 0, s, t, a, b, c, d))
            elif opc in (G_TRI1, G_TRI2, G_QUAD):
                tris = [((w0 >> 16) & 0xFF, (w0 >> 8) & 0xFF, w0 & 0xFF)]
                if opc != G_TRI1:
                    tris.append(((w1 >> 16) & 0xFF, (w1 >> 8) & 0xFF, w1 & 0xFF))
                # texture bindings as seen by this draw (TEXEL0 = render tile, TEXEL1 = render tile + 1)
                for unit in (0, 1):
                    tex = self.tile_texture(st, (st["tex_tile"] + unit) & 7)
                    words = tex if tex is not None else [OP_TEX, 0, 0, 0, 0, 0]
                    words = [words[0] | (unit << 8)] + words[1:]
                    if words != emitted_tex[unit]:
                        op(*words)
                        emitted_tex[unit] = words
                for tri in tris:
                    vi = [st["vtx"][i // 2] for i in tri]
                    if any(v is None for v in vi):
                        self.warn(f"tri references unloaded vertex at {pc - 8:08X}")
                        continue
                    if batch["start"] is None:
                        batch["start"] = len(indices)
                    indices.extend(vi)
            elif opc == G_DL:
                target = w1
                if (target >> 24) in DYNAMIC_DL_SEGMENTS:
                    op(OP_CALLSEG | ((target >> 24) << 8), target & 0xFFFFFF)
                    emitted_tex = [None, None]  # the called list may change textures/tiles
                    st["dynsize"] = set(range(8))
                elif ((w0 >> 16) & 0xFF) == 0:
                    stack.append(pc)
                    pc = target
                else:
                    pc = target
            elif opc == G_ENDDL:
                pc = stack.pop() if stack else None
            elif opc == G_BRANCH_Z:
                self.note("branch_z(taken: nearest LOD)")
                pc = st["rdphalf"]
            elif opc == G_RDPHALF_1:
                st["rdphalf"] = w1
            elif opc == G_GEOMETRYMODE:
                clear = geom_native(~w0 & 0xFFFFFF)
                op(OP_GEOM, clear, geom_native(w1))
            elif opc in (G_SETOTHERMODE_L, G_SETOTHERMODE_H):
                sft = 32 - ((w0 >> 8) & 0xFF) - ((w0 & 0xFF) + 1)
                ln = (w0 & 0xFF) + 1
                mask = ((1 << ln) - 1) << sft
                if opc == G_SETOTHERMODE_L:
                    st["oml"] = (st["oml"] & ~mask) | (w1 & mask)
                    m = mode_mask(0, mask)
                else:
                    st["omh"] = (st["omh"] & ~mask) | (w1 & mask)
                    m = mode_mask(mask, 0)
                op(OP_MODE, m, mode_fields(st["omh"], st["oml"]) & m)
            elif opc == G_RDPSETOTHERMODE:
                st["omh"], st["oml"] = w0 & 0xFFFFFF, w1
                op(OP_MODE, 0xFFFFFFFF, mode_fields(st["omh"], st["oml"]))
            elif opc == G_SETCOMBINE:
                cb = combine_native(w0 & 0xFFFFFF, w1)
                op(OP_COMBINE, *struct.unpack("<4I", cb))
            elif opc == G_TEXTURE:
                st["tex_tile"] = (w0 >> 8) & 7
                on = ((w0 >> 1) & 0x7F) != 0
                op(OP_TEXSCALE | (st["tex_tile"] << 8) | ((1 if on else 0) << 16), w1)
            elif opc == G_SETTIMG:
                st["img"] = {"addr": w1, "fmt": (w0 >> 21) & 7, "siz": (w0 >> 19) & 3, "width": (w0 & 0xFFF) + 1}
            elif opc == G_SETTILE:
                t = (w1 >> 24) & 7
                old = st["tiles"][t] or {}
                st["tiles"][t] = {
                    "fmt": (w0 >> 21) & 7, "siz": (w0 >> 19) & 3, "line": (w0 >> 9) & 0x1FF, "tmem": w0 & 0x1FF,
                    "palette": (w1 >> 20) & 0xF, "cmt": (w1 >> 18) & 3, "maskt": (w1 >> 14) & 0xF,
                    "shiftt": (w1 >> 10) & 0xF, "cms": (w1 >> 8) & 3, "masks": (w1 >> 4) & 0xF, "shifts": w1 & 0xF,
                    "uls": old.get("uls", 0), "ult": old.get("ult", 0), "lrs": old.get("lrs", 0), "lrt": old.get("lrt", 0)}
            elif opc == G_SETTILESIZE:
                t = (w1 >> 24) & 7
                if st["tiles"][t] is None:
                    st["tiles"][t] = {"fmt": 0, "siz": 2, "line": 0, "tmem": 0, "palette": 0, "cmt": 0, "maskt": 0,
                                      "shiftt": 0, "cms": 0, "masks": 0, "shifts": 0}
                st["tiles"][t].update(uls=(w0 >> 12) & 0xFFF, ult=w0 & 0xFFF, lrs=(w1 >> 12) & 0xFFF, lrt=w1 & 0xFFF)
                st["dynsize"].discard(t)
            elif opc in (G_LOADBLOCK, G_LOADTILE):
                t = (w1 >> 24) & 7
                tile = st["tiles"][t]
                img = st["img"]
                if tile is not None and img is not None:
                    st["tmem"][tile["tmem"]] = {"kind": "block" if opc == G_LOADBLOCK else "tile", "addr": img["addr"],
                                                "width": img["width"], "siz": img["siz"],
                                                "uls": (w0 >> 12) & 0xFFF, "ult": w0 & 0xFFF,
                                                # LOADBLOCK: lrs = texel count - 1 (in the load tile's size)
                                                "texels": (((w1 >> 12) & 0xFFF) + 1) * n64tex.BITS[tile["siz"]] // 8
                                                if opc == G_LOADBLOCK else 0}
            elif opc == G_LOADTLUT:
                if st["img"] is not None:
                    st["tlut"] = st["img"]["addr"]
            elif opc == G_SETPRIMCOLOR:
                op(OP_PRIM | ((w0 & 0xFFFF) << 8), w1)
            elif opc == G_SETENVCOLOR:
                op(OP_ENV, w1)
            elif opc == G_SETFOGCOLOR:
                op(OP_FOGCOLOR, w1)
            elif opc == G_SETBLENDCOLOR:
                op(OP_BLENDCOLOR, w1)
            elif opc == G_MTX:
                p = (w0 & 0xFF) ^ 1
                push, load, proj = p & 1, p & 2, p & 4
                seg = w1 >> 24
                if proj:
                    self.note("G_MTX(projection)")
                    continue
                if seg == MTX_SEGMENT:
                    ent = ("limb", (w1 & 0xFFFFFF) // 0x40)
                elif seg == 0x01 or seg in DYNAMIC_DL_SEGMENTS:
                    # id = segment | matrix index << 4 (segments holding arrays of matrices, e.g. bunny ears)
                    ent = ("segmul", seg | ((((w1 & 0xFFFFFF) // 0x40) & 0xF) << 4))
                    if load:
                        self.note(f"G_MTX(load seg {seg:02X})")
                else:
                    self.note("G_MTX(static data)")
                    ent = st["mstack"][-1]
                if push:
                    st["mstack"].append(ent)
                else:
                    st["mstack"][-1] = ent
            elif opc == G_POPMTX:
                n = max(1, w1 // 64)
                for _ in range(n):
                    if len(st["mstack"]) > 1:
                        st["mstack"].pop()
            elif opc == G_MOVEMEM:
                index = w0 & 0xFF
                if index == G_MV_LIGHT:
                    self.note("movemem-light")  # lights are supplied by game code (Lights_Draw)
                else:
                    self.note(f"movemem-{index:02X}")
            elif opc == G_MOVEWORD:
                index = (w0 >> 16) & 0xFF
                if index == G_MW_SEGMENT:
                    self.note("moveword-segment")
                elif index != G_MW_NUMLIGHT:
                    self.note(f"moveword-{index:02X}")
            elif opc in (G_TEXRECT, G_TEXRECTFLIP):
                self.note("texrect-in-3D-list")
                pc += 16
            elif opc in (G_SPNOOP, G_CULLDL, G_MODIFYVTX, G_RDPLOADSYNC, G_RDPPIPESYNC, G_RDPTILESYNC,
                         G_RDPFULLSYNC, G_SETSCISSOR, G_SETPRIMDEPTH, G_FILLRECT, G_SETFILLCOLOR, G_SETZIMG,
                         G_SETCIMG, 0xF1):
                if opc == G_MODIFYVTX:
                    self.note("modifyvtx")
            else:
                self.note(f"op{opc:02X}")
        flush()
        ops.append(OP_END)
        self.last_state = st
        return {"verts": b"".join(verts), "nverts": len(verts), "indices": indices, "ops": ops, "palette": palette,
                "vtxseg": st.get("vtxseg") or 0,
                "ntris": len(indices) // 3}


def pica_format(fmt, siz, tlut_ia):
    if fmt == n64tex.FMT_RGBA:
        return pica_tex.GPU_RGBA8 if siz == n64tex.SIZ_32 else pica_tex.GPU_RGBA5551
    if fmt == n64tex.FMT_CI:
        return pica_tex.GPU_LA8 if tlut_ia else pica_tex.GPU_RGBA5551
    if fmt == n64tex.FMT_IA:
        return pica_tex.GPU_LA8 if siz == n64tex.SIZ_16 else pica_tex.GPU_LA4
    if fmt == n64tex.FMT_I:
        return pica_tex.GPU_LA4 if siz == n64tex.SIZ_4 else pica_tex.GPU_LA8
    raise ValueError(f"format {fmt}/{siz}")


# ---------------- .pobj container -------------------------------------------------------------
"""
.pobj (little-endian):
  0x00 'POBJ'  u16 version=1  u16 numTex  u16 numMesh  u16 numExtra  u32 texTableOff  u32 meshTableOff
       u32 extraTableOff
  texTable[numTex]   : u32 off, u32 size          (each a PTEX blob: tools/pica_tex.py)
  meshTable[numMesh] : u32 srcAddr (original segmented address of the DL), u32 off
  mesh @off (pmesh.h PMeshHdr): u32 numVerts, u32 vertsOff, u32 numIdx, u32 idxOff, u32 numOps, u32 opsOff,
       u8 numPalette, u8 pad[3], u8 palette[8][2] (type, id)      offsets relative to file start
  extraTable[numExtra]: u32 tag, u32 off, u32 size  (skeletons, animations, scene data ...)
Textures are stored last: only [0, first texture) is resident; textures are loaded on first use.
"""


def build_pobj(textures, meshes, extras=()):
    """meshes: list of (srcAddr, compiled mesh dict or None). extras: list of (tag4, bytes)."""
    meshes = [(a, m) for a, m in meshes if m is not None]
    hdr_size = 4 + 2 * 4 + 4 * 3
    tex_tab = hdr_size
    mesh_tab = tex_tab + 8 * len(textures)
    extra_tab = mesh_tab + 8 * len(meshes)
    body_start = extra_tab + 12 * len(extras)
    body = bytearray()

    def add(blob, align=8):
        while (body_start + len(body)) % align:
            body.append(0)
        off = body_start + len(body)
        body.extend(blob)
        return off

    mesh_entries = []
    for src, m in meshes:
        voff = add(m["verts"], 16)
        ioff = add(struct.pack(f"<{len(m['indices'])}H", *m["indices"]), 8)
        ooff = add(struct.pack(f"<{len(m['ops'])}I", *[w & 0xFFFFFFFF for w in m["ops"]]), 4)
        pal = bytearray(16)
        for i, (t, v) in enumerate(m["palette"]):
            pal[2 * i], pal[2 * i + 1] = t, v
        hdr = struct.pack("<6IBB2x", m["nverts"], voff, len(m["indices"]), ioff, len(m["ops"]), ooff,
                          len(m["palette"]), m.get("vtxseg", 0)) + bytes(pal)
        mesh_entries.append((src, add(hdr, 4)))
    extra_entries = [(tag, add(blob, 4), len(blob)) for tag, blob in extras]
    tex_entries = [(add(t, 16), len(t)) for t in textures]
    out = bytearray(b"POBJ" + struct.pack("<HHHHIII", 1, len(textures), len(meshes), len(extras), tex_tab,
                                          mesh_tab, extra_tab))
    for off, size in tex_entries:
        out += struct.pack("<II", off, size)
    for src, off in sorted(mesh_entries):
        out += struct.pack("<II", src, off)
    for tag, off, size in extra_entries:
        out += struct.pack("<4sII", tag, off, size)
    assert len(out) == body_start
    out += body
    return bytes(out)
