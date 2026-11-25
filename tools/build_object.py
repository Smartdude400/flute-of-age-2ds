"""Build a 2DS-native object (.pobj) + its symbol header from one original object file.

  python tools/build_object.py object_aren_child [more objects...]

Outputs:
  romfs/vrom/<vrom>.pobj                   textures, meshes (one per DList symbol), extras:
      'SKEL' skeletons   'ANIM' animations   'PANM' player animation headers (gameplay_keep)
  source/game/assets/<name>.h          symbol -> asset handle (original segmented address)
  romfs/anim/aren_animetion.bin           (with gameplay_keep) player frame data, little-endian s16

All data is converted to little-endian, native layouts here. Nothing original is read at runtime.
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from romfs import RomFS
import xmlspec
import dl2pica
import pica_tex

HERE = os.path.dirname(os.path.abspath(__file__))
OUT_VROM = os.path.join(HERE, "..", "romfs", "vrom")
OUT_HDR = os.path.join(HERE, "..", "source", "game", "assets_agg")
OUT_ANIM = os.path.join(HERE, "..", "romfs", "anim")

KEEP_SEGMENTS = {4: "gameplay_keep"}

# Lists the game only ever draws right after another list that loads their texture / material state
# (found at the call sites; on the original the second list samples what the first left in texture memory).
# They are compiled with that state as their starting state: {file: {list: prelude list}}.
PRELUDES = {
    "gameplay_keep": {
        "gGlowCircleDL": "gGlowCircleTextureLoadDL",       # z_lights.c Lights_DrawGlow
        "gSylvanDustMoteModelDL": "gSylvanDustMoteMaterialDL",  # z_object_kankyo.c ObjectKankyo_DrawFairies
    },
    "object_gs": {
        "gEchoStoneDL": "gEchoStoneMaterialDL",            # z_en_gs.c EnGs_Draw
        "gEchoStoneSquishedDL": "gEchoStoneMaterialDL",
    },
}
TOKEN = 0x80000000  # asset handles = TOKEN | original segmented address (see asset_token.h)


class Blob:
    """Payload with relocations: offsets (within payload) holding payload-relative offsets that
    the native loader turns into absolute pointers. Layout: u32 nRelocs, u32 relocs[n], payload."""
    def __init__(self):
        self.data = bytearray()
        self.relocs = []

    def ptr(self, target_off):
        self.relocs.append(len(self.data))
        self.data += struct.pack("<I", target_off)

    def align(self, n=4):
        while len(self.data) % n:
            self.data.append(0)

    def bytes(self):
        return struct.pack(f"<I{len(self.relocs)}I", len(self.relocs), *self.relocs) + bytes(self.data)


def be16(b, o):
    return struct.unpack_from(">h", b, o)[0]


def beu16(b, o):
    return struct.unpack_from(">H", b, o)[0]


def be32(b, o):
    return struct.unpack_from(">I", b, o)[0]


_GAME_ENUMS = None


def game_enums():
    """Enum names the game headers define themselves (e.g. PlayerLimb in player.h): not generated again."""
    global _GAME_ENUMS
    if _GAME_ENUMS is None:
        import re
        _GAME_ENUMS = set()
        inc = os.path.join(HERE, "..", "source", "game", "include")
        for dp, _, files in os.walk(inc):
            if os.path.join("include", "assets") in dp:
                continue
            for f in files:
                if f.endswith(".h"):
                    _GAME_ENUMS |= set(re.findall(r"typedef enum (\w+)", open(os.path.join(dp, f), encoding="utf-8",
                                                                             errors="replace").read()))
    return _GAME_ENUMS


def limb_enums(data, seg, symbols, warns):
    """Limb index enums of skeletons that name them in the XML (EnumName / LimbNone / LimbMax): NONE = 0,
    then the limbs in the order of the skeleton's limb table (read from the file), then MAX."""
    limb_enum = {off: a["EnumName"] for tag, sym, off, a in symbols if tag == "Limb" and "EnumName" in a}
    out = []
    for tag, sym, off, a in symbols:
        if tag != "Skeleton" or "EnumName" not in a or "LimbNone" not in a or a["EnumName"] in game_enums():
            continue
        table, count = be32(data, off), data[off + 4]
        names = []
        for i in range(count):
            la = be32(data, (table & 0xFFFFFF) + 4 * i)
            if la >> 24 != seg or (la & 0xFFFFFF) not in limb_enum:
                warns.append(f"{sym}: limb {i} has no enum name")
                names = None
                break
            names.append(limb_enum[la & 0xFFFFFF])
        if names is None:
            continue
        out.append(f"typedef enum {a['EnumName']} {{")
        out.append(f"    /* 0x00 */ {a['LimbNone']},")
        out += [f"    /* 0x{i + 1:02X} */ {n}," for i, n in enumerate(names)]
        out.append(f"    /* 0x{count + 1:02X} */ {a['LimbMax']}")
        out.append(f"}} {a['EnumName']};")
    return out


def build(fs, name, report, xml=None, more_extras=()):
    """xml: XML file name if it differs from the ROM file name (scenes: 'spot04' for 'spot04_scene').
    more_extras: additional (tag, blob) extras to store in the same .dsobj (scene command data)."""
    spec = xmlspec.load(xml or name)
    if name not in spec:
        raise SystemExit(f"{name}: no <File> in xml")
    fspec = spec[name]
    seg = fspec["segment"]
    data = fs.get(name)
    segments = {seg: data}
    for kseg, kname in KEEP_SEGMENTS.items():
        if kseg != seg:
            segments[kseg] = fs.get(kname)
    warns = []
    comp = dl2pica.DLCompiler(segments, warn=warns.append)

    def off_of(addr):
        assert addr >> 24 == seg, f"{addr:08X} not in segment {seg}"
        return addr & 0xFFFFFF

    meshes = {}
    bad_dl = 0
    sym_off = {sym: off for tag, sym, off, attrs in fspec["symbols"] if tag == "DList"}
    preludes = {sym_off[a]: (seg << 24) | sym_off[b] for a, b in PRELUDES.get(name, {}).items()
                if a in sym_off and b in sym_off}
    for tag, sym, off, attrs in fspec["symbols"]:
        if tag == "DList" and "Length" in attrs:
            # self-check of the (possibly computed) offset: the last command must be G_ENDDL
            end = off + int(attrs["Length"], 0) * 8 - 8
            if end + 8 > len(data) or data[end] != 0xDF:
                bad_dl += 1
                warns.append(f"DL {sym} @{off:X}: no ENDDL at declared end (offset suspect)")
        if tag == "DList":
            addr = (seg << 24) | off
            try:
                meshes[addr] = comp.compile(addr, prelude=preludes.get(off))
            except (KeyError, RuntimeError, IndexError, ValueError, struct.error) as e:
                warns.append(f"DL {sym} {addr:08X}: {e}")

    # ---- skeletons: payload = FlexSkeletonHeader-compatible header, limb pointer table, limbs
    skel_blobs = []
    for tag, sym, off, attrs in fspec["symbols"]:
        if tag != "Skeleton":
            continue
        limbs_ptr = be32(data, off)
        limb_count = data[off + 4]
        dl_count = data[off + 8] if attrs.get("Type") == "Flex" else 0
        lod = attrs.get("LimbType") == "LOD"
        b = Blob()
        table_off = 12
        limbs_off = table_off + 4 * limb_count
        b.ptr(table_off)                                   # void** segment
        b.data += struct.pack("<B3xB3x", limb_count, dl_count)  # u8 limbCount @4, u8 dListCount @8
        for i in range(limb_count):
            b.ptr(limbs_off + 16 * i)
        for i in range(limb_count):
            limb_addr = be32(data, off_of(limbs_ptr) + 4 * i)
            lo = off_of(limb_addr)
            jx, jy, jz = be16(data, lo), be16(data, lo + 2), be16(data, lo + 4)
            child, sibling = data[lo + 6], data[lo + 7]
            dl0 = be32(data, lo + 8)
            dl1 = be32(data, lo + 12) if lod else 0
            for dl in (dl0, dl1):
                if dl and dl not in meshes and (dl >> 24) == seg:
                    meshes[dl] = comp.compile(dl)
            b.data += struct.pack("<hhhBBII", jx, jy, jz, child, sibling, (dl0 | TOKEN) if dl0 else 0,
                                  (dl1 | TOKEN) if dl1 else 0)
        skel_blobs.append(((seg << 24) | off, b.bytes()))

    # ---- animations (normal)
    anim_blobs = []
    for tag, sym, off, attrs in fspec["symbols"]:
        if tag != "Animation":
            continue
        frame_count = be16(data, off)
        fd_ptr, ji_ptr = be32(data, off + 4), be32(data, off + 8)
        static_max = beu16(data, off + 12)
        fd_off, ji_off = off_of(fd_ptr), off_of(ji_ptr)
        # Layout in object files: frameData[], jointIndices[], header. Counts from the gaps.
        n_joints = (off - ji_off) // 6
        n_fd = (ji_off - fd_off) // 2
        if not (0 < n_joints < 128 and 0 < n_fd < 0x10000):
            warns.append(f"anim {sym}: unexpected layout ({n_joints} joints, {n_fd} values)")
            continue
        joints = [beu16(data, ji_off + 2 * i) for i in range(n_joints * 3)]
        fdata = [be16(data, fd_off + 2 * i) for i in range(n_fd)]
        if max(joints) >= n_fd + frame_count:
            warns.append(f"anim {sym}: joint index out of range")
        # payload = AnimationHeader-compatible: s16 frameCount, s16* frameData, JointIndex* jointIndices,
        # u16 staticIndexMax; then the two arrays
        b = Blob()
        fd_pos = 16
        ji_pos = fd_pos + 2 * n_fd
        ji_pos += (-ji_pos) % 4
        b.data += struct.pack("<h2x", frame_count)
        b.ptr(fd_pos)
        b.ptr(ji_pos)
        b.data += struct.pack("<H2x", static_max)
        b.data += struct.pack(f"<{len(fdata)}h", *fdata)
        b.align()
        b.data += struct.pack(f"<{len(joints)}H", *joints)
        blob = b.bytes()
        anim_blobs.append(((seg << 24) | off, blob))

    # ---- standalone textures (selected at runtime through texture segments, e.g. eyes/mouth)
    FMT = {"rgba16": (0, 2), "rgba32": (0, 3), "ci4": (2, 0), "ci8": (2, 1), "i4": (4, 0), "i8": (4, 1),
           "ia4": (3, 0), "ia8": (3, 1), "ia16": (3, 2)}
    import n64tex
    txmp = []
    for tag, sym, off, attrs in fspec["symbols"]:
        if tag != "Texture" or attrs.get("Format") not in FMT:
            continue
        fmt, siz = FMT[attrs["Format"]]
        w, h = int(attrs["Width"]), int(attrs["Height"])
        try:
            raw = data[off:off + n64tex.tex_bytes(fmt, siz, w, h)]
            tlut = None
            if fmt == n64tex.FMT_CI:
                if "TlutOffset" not in attrs:
                    continue
                ncol = 16 if siz == n64tex.SIZ_4 else 256
                toff = int(attrs["TlutOffset"], 16)
                tlut = n64tex.decode_tlut(data[toff:toff + ncol * 2], min(ncol, (len(data) - toff) // 2))
            px, idx = n64tex.decode(raw, fmt, siz, w, h, tlut)
            W, H = dl2pica.pow2ceil(w), dl2pica.pow2ceil(h)
            padded = [px[min(y, h - 1) * w + min(x, w - 1)] for y in range(H) for x in range(W)]
            txmp.append(((seg << 24) | off, len(comp.textures)))
            comp.textures.append(pica_tex.ptex(padded, W, H, dl2pica.pica_format(fmt, siz, False), flags=0))
        except (ValueError, IndexError, struct.error) as e:
            warns.append(f"texture {sym}: {e}")

    # ---- player animation headers (frameCount + offset into aren_animetion)
    panm = []
    for tag, sym, off, attrs in fspec["symbols"]:
        if tag == "PlayerAnimation":
            panm.append(((seg << 24) | off, be16(data, off), be32(data, off + 4) & 0xFFFFFF))

    def table(entries):
        out = bytearray(struct.pack("<I", len(entries)))
        body = bytearray()
        base = 4 + 8 * len(entries)
        for addr, blob in sorted(entries):
            while (base + len(body)) % 4:
                body.append(0)
            out += struct.pack("<II", addr, base + len(body))
            body += blob
        return bytes(out + body)

    # ---- collision headers (DynaPoly actors: CollisionHeader_GetVirtual), converted like scene collision
    coll_blobs = []
    for tag, sym, off, attrs in fspec["symbols"]:
        if tag != "Collision":
            continue
        import build_scene
        conv = build_scene.SceneConverter.__new__(build_scene.SceneConverter)
        conv.name, conv.warn, conv.cutscenes = name, warns.append, set()
        try:
            w = conv.convert(seg, data, name, None, build_scene.SceneConverter.collect_starts(data, seg), False,
                             root=("colheader", off))
            coll_blobs.append(((seg << 24) | off, w.blob()))
        except (KeyError, IndexError, ValueError, struct.error) as e:
            warns.append(f"collision {sym}: {e}")

    extras = list(more_extras)
    if coll_blobs and not any(t == b"COLL" for t, _ in extras):
        extras.append((b"COLL", table(coll_blobs)))
    if txmp:
        extras.append((b"TXMP", struct.pack("<I", len(txmp)) + b"".join(struct.pack("<II", a, i) for a, i in sorted(txmp))))
    if skel_blobs:
        extras.append((b"SKEL", table(skel_blobs)))
    if anim_blobs:
        extras.append((b"ANIM", table(anim_blobs)))
    if panm:
        # payload = ArenAnimationHeader-compatible: s16 frameCount, u32 offset into aren_animetion
        extras.append((b"PANM", table([(a, struct.pack("<I", 0) + struct.pack("<h2xI", fc, o)) for a, fc, o in panm])))

    mesh_list = sorted(meshes.items())
    obj = dl2pica.build_pobj(comp.textures, mesh_list, extras)
    # Keyed by the original ROM file address: the game's object table holds those addresses.
    vrom = fs.entries[name][0]
    os.makedirs(OUT_VROM, exist_ok=True)
    open(os.path.join(OUT_VROM, f"{vrom:08X}.pobj"), "wb").write(obj)

    # ---- symbol header
    os.makedirs(OUT_HDR, exist_ok=True)
    lines = [f"// GENERATED by tools/build_object.py from assets/xml for {name}. Asset handles: each symbol",
             "// evaluates to its original segmented address, resolved by the native engine at runtime.",
             f"#ifndef ASSET_{name.upper()}_H", f"#define ASSET_{name.upper()}_H", '#include "asset_token.h"', ""]
    for tag, sym, off, attrs in fspec["symbols"]:
        if tag in ("Scene", "Room"):
            continue  # file-level symbols are not handles
        lines.append(f"#define {sym} ASSET_TOKEN(0x{TOKEN | (seg << 24) | off:08X}) // {tag}")
        if tag == "Texture" and "Width" in attrs and "Height" in attrs:
            # the original build's asset headers also gave each texture's size (<sym>_WIDTH / _HEIGHT)
            lines.append(f"#define {sym}_WIDTH {int(attrs['Width'], 0)}")
            lines.append(f"#define {sym}_HEIGHT {int(attrs['Height'], 0)}")
    lines += limb_enums(data, seg, fspec["symbols"], warns)
    lines.append(f"#define {name.upper()}_VROM 0x{vrom:08X} // converted file: /vrom/{vrom:08X}.pobj")
    lines.append("#endif\n")
    open(os.path.join(OUT_HDR, name + ".h"), "w", newline="\n").write("\n".join(lines))

    ntris = sum(m["ntris"] for _, m in mesh_list if m)
    report.append(f"{name}: {len(mesh_list)} meshes, {ntris} tris, {len(comp.textures)} textures, "
                  f"{len(skel_blobs)} skeletons, {len(anim_blobs)} anims, {len(panm)} player anims, "
                  f"{len(obj)} bytes, {bad_dl} suspect DL offsets, {len(warns)} warnings, unsupported={comp.unsupported}")
    for w in warns[:8]:
        report.append("   warn: " + w)
    return name


def build_player_anim_data(fs):
    raw = fs.get("aren_animetion")
    n = len(raw) // 2
    vals = struct.unpack(f">{n}h", raw[:n * 2])
    os.makedirs(OUT_ANIM, exist_ok=True)
    open(os.path.join(OUT_ANIM, "aren_animetion.bin"), "wb").write(struct.pack(f"<{n}h", *vals))
    return len(raw)


if __name__ == "__main__":
    fs = RomFS("ntsc-1.0")
    report = []
    for nm in sys.argv[1:]:
        build(fs, nm, report)
        if nm == "gameplay_keep":
            report.append(f"aren_animetion: {build_player_anim_data(fs)} bytes")
    print("\n".join(report))
