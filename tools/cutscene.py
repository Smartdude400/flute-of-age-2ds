"""Cutscene scripts -> native (little-endian) layout, offline.

A script is a stream of 32-bit words on the original, but most commands pack 8- and 16-bit fields into
those words (`CsCmd*` unions in game/include/cutscene.h, read by z_demo.c Cutscene_ProcessScript through
the struct fields). Swapping whole words would swap the halves of those fields, so every field is swapped
at its own width, following the command layouts. Stream and sizes are unchanged: the native script is the
same byte length with the same offsets, so the game's pointer walks over it work as written.

  python tools/cutscene.py spot04 gSylvanForestSylvaGreetingCs     (dump one script, for checking)
"""
import struct

# field widths: B = 1 byte (no swap), H = 2, I = 4 (s32 / f32)
L_MISC = "HHHH" + "I" * 10             # CsCmdMisc, CsCmdFadeOutSeq        (0x30)
L_BYTEID = "BBHHH" + "I" * 10          # CsCmdLightSetting, Start/StopSeq  (0x30)
L_RUMBLE = "HHHBBBBH"                  # CsCmdRumble                       (0xC)
L_TIME = "HHHBBI"                      # CsCmdTime                         (0xC)
L_HHHH = "HHHH"                        # CsCmdDestination, CsCmdTransition, CsCmdCam (0x8)
L_TEXT = "HHHHHH"                      # CsCmdText                         (0xC)
L_CUE = "HHHHHH" + "III" + "III" + "III"  # CsCmdActorCue: id, frames, rot, start, end, normal (0x30)
L_CAMPOINT = "BBHI" + "HHHH"           # CutsceneCameraPoint               (0x10)
L_UNKNOWN = "I" * 12                   # unimplemented commands: 0x30 entries, never read

CMD_END = 0xFFFFFFFF
CMD_CAM_EYE_SPLINE, CMD_CAM_AT_SPLINE, CMD_MISC, CMD_LIGHT_SETTING = 1, 2, 3, 4
CMD_CAM_EYE_SPLINE_REL, CMD_CAM_AT_SPLINE_REL, CMD_CAM_EYE, CMD_CAM_AT = 5, 6, 7, 8
CMD_RUMBLE, CMD_PLAYER_CUE, CMD_TEXT, CMD_TRANSITION = 9, 0x0A, 0x13, 0x2D
CMD_START_SEQ, CMD_STOP_SEQ, CMD_FADE_OUT_SEQ, CMD_TIME, CMD_DESTINATION = 0x56, 0x57, 0x7C, 0x8C, 0x3E8
# commands the runtime skips as `count` 0x30-byte entries (CS_CMD_UNIMPLEMENTED_*)
UNIMPLEMENTED = {0x0B, 0x0D, 0x15, 0x16, 0x1B, 0x1C, 0x20, 0x21, 0x3B, 0x3D, 0x47, 0x49, 0x6D, 0x70, 0x71}
COUNTED = {CMD_MISC: L_MISC, CMD_LIGHT_SETTING: L_BYTEID, CMD_START_SEQ: L_BYTEID, CMD_STOP_SEQ: L_BYTEID,
           CMD_FADE_OUT_SEQ: L_MISC, CMD_RUMBLE: L_RUMBLE, CMD_TIME: L_TIME, CMD_TEXT: L_TEXT,
           CMD_PLAYER_CUE: L_CUE}


def size_of(layout):
    return sum({"B": 1, "H": 2, "I": 4}[c] for c in layout)


class Converter:
    def __init__(self, data, off):
        self.src = data
        self.pos = off
        self.out = bytearray()
        self.counts = {}

    def fields(self, layout):
        for c in layout:
            n = {"B": 1, "H": 2, "I": 4}[c]
            chunk = self.src[self.pos:self.pos + n]
            if len(chunk) < n:
                raise ValueError(f"script runs past the file end at {self.pos:X}")
            self.out += chunk[::-1]
            self.pos += n

    def word(self):
        v = struct.unpack_from(">I", self.src, self.pos)[0]
        self.fields("I")
        return v

    def convert(self):
        total = self.word()
        self.word()  # frame count
        for _ in range(total):
            cmd = self.word()
            self.counts[cmd] = self.counts.get(cmd, 0) + 1
            if cmd == CMD_END:
                break
            if cmd in (CMD_CAM_EYE_SPLINE, CMD_CAM_AT_SPLINE, CMD_CAM_EYE_SPLINE_REL, CMD_CAM_AT_SPLINE_REL):
                self.fields(L_HHHH)
                while True:  # points up to and including the one with continueFlag == CS_CAM_STOP (-1)
                    stop = self.src[self.pos] == 0xFF
                    self.fields(L_CAMPOINT)
                    if stop:
                        break
            elif cmd in (CMD_CAM_EYE, CMD_CAM_AT):
                self.fields(L_HHHH)
                self.fields(L_CAMPOINT)
            elif cmd in (CMD_DESTINATION, CMD_TRANSITION):
                self.word()  # entry count (always 1; skipped by the runtime)
                self.fields(L_HHHH)
            else:
                n = self.word()
                if n > 0x400:
                    raise ValueError(f"command {cmd:X} with {n} entries at {self.pos:X}: not a script")
                layout = COUNTED.get(cmd) or (L_UNKNOWN if cmd in UNIMPLEMENTED else L_CUE)
                # every other command id is an actor cue list (CS_CMD_ACTOR_CUE_*), 0x30-byte entries
                for _ in range(n):
                    self.fields(layout)
        return bytes(self.out)


def convert(data, off):
    """-> (native bytes, {command id: count}) for the script at file offset `off`."""
    c = Converter(data, off)
    out = c.convert()
    return out, c.counts


assert size_of(L_MISC) == 0x30 and size_of(L_BYTEID) == 0x30 and size_of(L_CUE) == 0x30
assert size_of(L_RUMBLE) == 0xC and size_of(L_TIME) == 0xC and size_of(L_TEXT) == 0xC
assert size_of(L_HHHH) == 0x8 and size_of(L_CAMPOINT) == 0x10 and size_of(L_UNKNOWN) == 0x30

if __name__ == "__main__":
    import sys
    from romfs import RomFS
    import xmlspec
    fs = RomFS("ntsc-1.0")
    scene, sym = sys.argv[1], sys.argv[2]
    spec = xmlspec.load(scene)
    for fname, fspec in spec.items():
        for tag, s, off, attrs in fspec["symbols"]:
            if s == sym:
                out, counts = convert(fs.get(fname), off)
                print(f"{sym} in {fname} @{off:X}: {len(out)} bytes, commands {{{', '.join(f'{k:X}: {v}' for k, v in sorted(counts.items()))}}}")
