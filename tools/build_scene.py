"""Build DS-native scene + room files from one original scene (BUILD TIME ONLY).

  python tools/build_scene.py spot04 [more scenes...]

Outputs (keyed by the original ROM file address so the game's scene/room tables, which hold those
addresses as identifiers, resolve to the converted files):
  romfs/vrom/<vromStart>.pobj   one per scene file and per room file
      scene: extras 'SCEN' (header @ segment offset 0, all reachable data in ARM layout), 'COLL'
      room : meshes for every room-shape DL, extras 'ROOM' (room header + data)
  build/scene_report.txt

Struct layouts follow source/game/include (scene.h, room.h, bgcheck.h, light.h, path.h,
environment.h). Every struct is re-emitted little-endian field by field. Packed-byte command
payloads keep their byte order, 32-bit payloads are swapped, pointers are relocated, and display
lists become asset handles (0x80000000 | segmented address) compiled into meshes.
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from romfs import RomFS
import xmlspec
import dl2pica

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "romfs", "vrom")
TOKEN = 0x80000000

# command ids (scene.h)
(C_PLAYER_ENTRY, C_ACTOR_LIST, C_UNUSED2, C_COL, C_ROOM_LIST, C_WIND, C_SPAWN, C_SPECIAL, C_BEHAVIOR,
 C_UNDEF9, C_MESH, C_OBJECTS, C_LIGHTS, C_PATHS, C_TRANSI, C_LIGHT_SETTINGS, C_TIME, C_SKYBOX, C_SKYDIS,
 C_EXITS, C_END, C_SOUND, C_ECHO, C_CUTSCENE, C_ALT, C_MISC) = range(26)
BYTE_PAYLOAD = {C_WIND, C_TIME, C_SKYBOX, C_SKYDIS, C_SOUND, C_ECHO}
U32_PAYLOAD = {C_SPECIAL, C_BEHAVIOR, C_MISC, C_END, C_UNDEF9}


class Writer:
    """Relocatable payload builder: emit structs, remember where each original object went."""
    def __init__(self, seg, data):
        self.seg, self.src = seg, data
        self.out = bytearray()
        self.relocs = []
        self.placed = {}      # (kind, srcOff) -> out offset
        self.pending = []     # (out_field_off, kind, srcOff, count)

    def be(self, fmt, off):
        return struct.unpack_from(">" + fmt, self.src, off)

    def align(self, n=4):
        while len(self.out) % n:
            self.out.append(0)

    def ptr_field(self, kind, addr, count=None):
        """Write a pointer field that will point at the converted (kind, addr) object."""
        if addr == 0:
            self.out += b"\0\0\0\0"
            return
        if (addr >> 24) != self.seg:
            # foreign segment (e.g. gameplay_keep data): leave as an asset handle
            self.out += struct.pack("<I", TOKEN | addr)
            return
        self.relocs.append(len(self.out))
        self.pending.append((len(self.out), kind, addr & 0xFFFFFF, count))
        self.out += b"\0\0\0\0"

    def place(self, kind, off, count, emit):
        key = (kind, off)
        if key in self.placed:
            return self.placed[key]
        self.align(4)
        start = len(self.out)
        self.placed[key] = start
        emit(off, count)
        return start

    def resolve(self, emitters):
        while self.pending:
            field, kind, off, count = self.pending.pop(0)
            start = self.place(kind, off, count, lambda o, c, k=kind: emitters[k](o, c))
            struct.pack_into("<I", self.out, field, start)

    def blob(self):
        return struct.pack(f"<I{len(self.relocs)}I", len(self.relocs), *self.relocs) + bytes(self.out)


def s16s(w, off, n):
    vals = w.be(f"{n}h", off)
    w.out += struct.pack(f"<{n}h", *vals)


class SceneConverter:
    def __init__(self, fs, scene_name, warn):
        self.fs, self.name, self.warn = fs, scene_name, warn
        self.scene = fs.get(scene_name + "_scene")

    # ------------------------------------------------------------------ header walking
    def emit_header(self, w, off, segfile, comp, is_room):
        """SceneCmd list until END (code 0x14)."""
        pos = off
        while True:
            code, data1 = w.src[pos], w.src[pos + 1]
            w.out += bytes([code, data1, w.src[pos + 2], w.src[pos + 3]])
            p = w.be("I", pos + 4)[0]
            if code in BYTE_PAYLOAD:
                w.out += w.src[pos + 4:pos + 8]
            elif code in U32_PAYLOAD:
                w.out += struct.pack("<I", p)
            elif code == C_PLAYER_ENTRY or code == C_ACTOR_LIST:
                w.ptr_field("actors", p, data1)
            elif code == C_COL:
                w.ptr_field("colheader", p)
            elif code == C_ROOM_LIST:
                w.ptr_field("roomlist", p, data1)
            elif code == C_SPAWN:
                w.ptr_field("spawns", p, None)
            elif code == C_MESH:
                w.ptr_field("mesh", p)
            elif code == C_OBJECTS:
                w.ptr_field("s16list", p, data1)
            elif code == C_LIGHTS:
                w.ptr_field("lights", p, data1)
            elif code == C_PATHS:
                w.ptr_field("paths", p, None)
            elif code == C_TRANSI:
                w.ptr_field("transi", p, data1)
            elif code == C_LIGHT_SETTINGS:
                w.ptr_field("envlights", p, data1)
            elif code == C_EXITS:
                w.ptr_field("exits", p, None)
            elif code == C_ALT:
                w.ptr_field("althdr", p, None)
            elif code == C_CUTSCENE:
                self.warn(f"{self.name}: cutscene data at {p:08X} not converted yet (handle kept)")
                w.out += struct.pack("<I", TOKEN | p)
            else:
                w.out += struct.pack("<I", p)
            pos += 8
            if code == C_END:
                break

    def convert(self, seg, data, segfile, comp, known_starts, is_room):
        w = Writer(seg, data)
        starts = sorted(known_starts)

        def gap(off, unit, cap):
            nxt = next((s for s in starts if s > off), len(data))
            return max(1, min(cap, (nxt - off) // unit))

        def e_actors(o, n):
            for i in range(n):
                s16s(w, o + i * 16, 8)

        def e_roomlist(o, n):
            for i in range(n):
                w.out += struct.pack("<II", *w.be("II", o + i * 8))

        def e_spawns(o, n):
            n = gap(o, 2, 64)
            w.out += data[o:o + 2 * n]

        def e_s16list(o, n):
            n = n if n is not None else gap(o, 2, 256)
            s16s(w, o, n)

        def e_exits(o, n):
            s16s(w, o, gap(o, 2, 64))

        def e_lights(o, n):
            for i in range(n):
                b = o + i * 14
                typ = data[b]
                w.out += bytes([typ, data[b + 1]])
                if typ == 1:  # directional: bytes
                    w.out += data[b + 2:b + 14]
                else:         # point: s16 x,y,z, u8 color[3], u8 drawGlow, s16 radius
                    s16s(w, b + 2, 3)
                    w.out += data[b + 8:b + 12]
                    s16s(w, b + 12, 1)

        def e_envlights(o, n):
            for i in range(n):
                b = o + i * 0x16
                w.out += data[b:b + 0x12]
                s16s(w, b + 0x12, 2)

        def e_transi(o, n):
            for i in range(n):
                b = o + i * 16
                w.out += data[b:b + 4]
                s16s(w, b + 4, 6)

        def e_paths(o, n):
            n = gap(o, 8, 64)
            for i in range(n):
                cnt = data[o + i * 8]
                w.out += bytes([cnt, 0, 0, 0])
                w.ptr_field("vec3s", w.be("I", o + i * 8 + 4)[0], cnt)

        def e_vec3s(o, n):
            s16s(w, o, 3 * n)

        def e_althdr(o, n):
            n = gap(o, 4, 16)
            for i in range(n):
                w.ptr_field("header", w.be("I", o + i * 4)[0])

        def e_header(o, n):
            self.emit_header(w, o, segfile, comp, is_room)

        def e_mesh(o, n):
            typ = data[o]
            if typ in (0, 2):   # NORMAL / CULLABLE
                cnt = data[o + 1]
                w.out += bytes([typ, cnt, 0, 0])
                ent, ent_end = w.be("II", o + 4)
                kind = "dlentries" if typ == 0 else "cullentries"
                w.ptr_field(kind, ent, cnt)
                # entriesEnd = entries + count (pointer into the same converted array)
                w.relocs.append(len(w.out))
                w.pending.append((len(w.out), kind + "_end", ent & 0xFFFFFF, cnt))
                w.out += b"\0\0\0\0"
            else:
                self.warn(f"{self.name}: room shape type {typ} (prerendered image) not converted")
                w.out += bytes([typ, data[o + 1], 0, 0]) + b"\0" * 8

        def dl_token(addr):
            if addr == 0:
                return 0
            if (addr >> 24) == seg and comp is not None:
                if addr not in self.meshes:
                    try:
                        self.meshes[addr] = comp.compile(addr)
                    except (KeyError, RuntimeError, IndexError, ValueError, struct.error) as e:
                        self.warn(f"DL {addr:08X}: {e}")
            return TOKEN | addr

        def e_dlentries(o, n):
            for i in range(n):
                opa, xlu = w.be("II", o + i * 8)
                w.out += struct.pack("<II", dl_token(opa), dl_token(xlu))

        def e_cullentries(o, n):
            for i in range(n):
                b = o + i * 16
                s16s(w, b, 4)
                opa, xlu = w.be("II", b + 8)
                w.out += struct.pack("<II", dl_token(opa), dl_token(xlu))

        # ---- collision
        def e_colheader(o, n):
            s16s(w, o, 6)
            nv = w.be("H", o + 0xC)[0]
            w.out += struct.pack("<H2x", nv)
            w.ptr_field("colverts", w.be("I", o + 0x10)[0], nv)
            npoly = w.be("H", o + 0x14)[0]
            w.out += struct.pack("<H2x", npoly)
            polys_addr = w.be("I", o + 0x18)[0]
            w.ptr_field("colpolys", polys_addr, npoly)
            # surface type count = max(poly.type) + 1
            po = polys_addr & 0xFFFFFF
            nsurf = 1 + max((w.be("H", po + i * 16)[0] for i in range(npoly)), default=-1)
            surf_addr = w.be("I", o + 0x1C)[0]
            w.ptr_field("surfaces", surf_addr, nsurf)
            so = surf_addr & 0xFFFFFF
            ncam = 0
            for i in range(nsurf):
                ncam = max(ncam, (w.be("I", so + i * 8)[0] & 0xFF) + 1)
            nwb = w.be("H", o + 0x24)[0]
            wb_addr = w.be("I", o + 0x28)[0]
            for i in range(nwb):
                ncam = max(ncam, (w.be("I", (wb_addr & 0xFFFFFF) + i * 0x10 + 0xC)[0] & 0xFF) + 1)
            cam_addr = w.be("I", o + 0x20)[0]
            if cam_addr:
                ncam = max(ncam, gap(cam_addr & 0xFFFFFF, 8, 64))
            w.ptr_field("bgcams", cam_addr, ncam)
            w.out += struct.pack("<H2x", nwb)
            w.ptr_field("waterboxes", wb_addr, nwb)

        def e_colverts(o, n):
            s16s(w, o, 3 * n)

        def e_colpolys(o, n):
            for i in range(n):
                vals = w.be("HHHHhhhh", o + i * 16)
                w.out += struct.pack("<HHHHhhhh", *vals)

        def e_surfaces(o, n):
            for i in range(n):
                w.out += struct.pack("<II", *w.be("II", o + i * 8))

        def e_bgcams(o, n):
            for i in range(n):
                setting, count, fd = w.be("HhI", o + i * 8)
                w.out += struct.pack("<Hh", setting, count)
                w.ptr_field("bgcamdata", fd, count if count > 0 else 0)

        def e_bgcamdata(o, n):
            # crawlspace point lists (count Vec3s) or a BgCamFuncData (9 x s16)
            s16s(w, o, 3 * n if n else 9)

        def e_waterboxes(o, n):
            for i in range(n):
                b = o + i * 0x10
                s16s(w, b, 5)
                w.out += b"\0\0" + struct.pack("<I", w.be("I", b + 0xC)[0])

        emitters = dict(actors=e_actors, roomlist=e_roomlist, spawns=e_spawns, s16list=e_s16list,
                        exits=e_exits, lights=e_lights, envlights=e_envlights, transi=e_transi,
                        paths=e_paths, vec3s=e_vec3s, althdr=e_althdr, header=e_header, mesh=e_mesh,
                        dlentries=e_dlentries, cullentries=e_cullentries, colheader=e_colheader,
                        colverts=e_colverts, colpolys=e_colpolys, surfaces=e_surfaces, bgcams=e_bgcams,
                        bgcamdata=e_bgcamdata, waterboxes=e_waterboxes)
        # "<kind>_end" pointers resolve to the end of the converted array of <kind>
        for k, unit in (("dlentries", 8), ("cullentries", 16)):
            def end_emitter(o, n, k=k, unit=unit):
                raise AssertionError("end pointers are resolved after placement")
            emitters[k + "_end"] = end_emitter

        # header first, at offset 0 of the payload
        w.place("header", 0, None, lambda o, c: self.emit_header(w, o, segfile, comp, is_room))
        # resolve; handle *_end specially
        while w.pending:
            field, kind, off, count = w.pending.pop(0)
            if kind.endswith("_end"):
                base_kind = kind[:-4]
                unit = 8 if base_kind == "dlentries" else 16
                if (base_kind, off) not in w.placed:
                    w.pending.append((field, kind, off, count))
                    # make sure the base gets placed
                    if not any(p[1] == base_kind and p[2] == off for p in w.pending):
                        w.place(base_kind, off, count, lambda o, c, k=base_kind: emitters[k](o, c))
                    continue
                struct.pack_into("<I", w.out, field, w.placed[(base_kind, off)] + unit * count)
                continue
            start = w.place(kind, off, count, lambda o, c, k=kind: emitters[k](o, c))
            struct.pack_into("<I", w.out, field, start)
        return w

    # ------------------------------------------------------------------ known block starts
    @staticmethod
    def collect_starts(data, seg):
        """All 4-aligned words that look like pointers into this segment (for gap sizing)."""
        starts = set()
        for i in range(0, len(data) - 3, 4):
            v = struct.unpack_from(">I", data, i)[0]
            if (v >> 24) == seg and (v & 0xFFFFFF) < len(data):
                starts.add(v & 0xFFFFFF)
        return starts

    def build(self, report):
        fs = self.fs
        os.makedirs(OUT, exist_ok=True)
        scene_vrom = fs.entries[self.name + "_scene"][0]
        # rooms from the scene's room list command
        room_names = [n for n in fs.names if n.startswith(self.name + "_room_")]
        segs_common = {2: self.scene, 4: fs.get("gameplay_keep")}
        try:
            segs_common[5] = fs.get("gameplay_field_keep")
        except KeyError:
            pass

        # scene file: header + collision etc. (no meshes)
        self.meshes = {}
        w = self.convert(2, self.scene, self.name + "_scene", None, self.collect_starts(self.scene, 2), False)
        scene_blob = w.blob()
        # SCEN table with one entry (segment offset 0 = header). The same .pobj also carries the
        # scene file's XML-declared textures/display lists (used by the scene draw configs).
        scen_table = struct.pack("<III", 1, 0x02000000, 12) + scene_blob
        import build_object
        build_object.build(fs, self.name + "_scene", report, xml=self.name, more_extras=[(b"SCEN", scen_table)])

        for rn in sorted(room_names, key=lambda n: int(n.rsplit("_", 1)[1])):
            room = fs.get(rn)
            segs = dict(segs_common)
            segs[3] = room
            comp = dl2pica.DLCompiler(segs, warn=self.warn)
            self.meshes = {}
            w = self.convert(3, room, rn, comp, self.collect_starts(room, 3), True)
            room_table = struct.pack("<III", 1, 0x03000000, 12) + w.blob()
            mesh_list = sorted(self.meshes.items())
            obj = dl2pica.build_pobj(comp.textures, mesh_list, [(b"ROOM", room_table)])
            vrom = fs.entries[rn][0]
            open(os.path.join(OUT, f"{vrom:08X}.pobj"), "wb").write(obj)
            ntris = sum(m["ntris"] for _, m in mesh_list if m)
            nverts = sum(m["nverts"] for _, m in mesh_list if m)
            report.append(f"{rn} -> vrom/{vrom:08X}.pobj  {len(mesh_list)} meshes {ntris} tris {nverts} verts "
                          f"{len(comp.textures)} tex {len(obj)} bytes unsupported={comp.unsupported}")


if __name__ == "__main__":
    fs = RomFS("ntsc-1.0")
    report, warns = [], []
    for sc in sys.argv[1:]:
        SceneConverter(fs, sc, warns.append).build(report)
    print("\n".join(report))
    print(f"{len(warns)} warnings")
    for w_ in warns[:20]:
        print("  warn:", w_)
