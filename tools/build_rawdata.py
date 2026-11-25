"""Copy ROM data files that the translated game code reads as plain BYTE records into romfs/vrom/<vrom>.bin.
Only files whose records are made of u8 fields only belong here (no endianness, no pointers), so the
bytes are the native layout as-is:
  elf_message_field, elf_message_ydan: QuestHintCmd scripts (u8 byte0..byte3, quest_hint_commands.h),
  loaded by Play_LoadFile for the companion's hints.
  parameter_static, do_action_static (HUD), title_static, object_mag (file select), nes_font_static, kanji, message_static, message_texture_static, icon_item_static, icon_item_24_static:
  texture data the game copies into its own buffers (font glyphs, text box frames and backgrounds,
  item icons in text boxes); kept in the original texel order, converted at draw time by the renderer
  (engine/gfx3d.c dyn_texture). engine/rom_bind.c copies them where the original DMA'd them.
romfs/rawfiles.bin indexes them: u32 count, then (u32 vromStart, u32 size) per file."""
import os
import struct
from romfs import RomFS

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "romfs", "vrom")
FILES = ["elf_message_field", "elf_message_ydan", "parameter_static", "do_action_static", "title_static", "object_mag",
         "nes_font_static", "kanji", "message_static", "message_texture_static", "icon_item_static",
         "icon_item_24_static"]

fs = RomFS("ntsc-1.0")
os.makedirs(OUT, exist_ok=True)
index = []
for name in FILES:
    vrom = fs.entries[name][0]
    data = fs.get(name)
    open(os.path.join(OUT, f"{vrom:08X}.bin"), "wb").write(data)
    index.append((vrom, len(data)))
with open(os.path.join(HERE, "..", "romfs", "rawfiles.bin"), "wb") as fh:
    fh.write(struct.pack("<I", len(index)) + b"".join(struct.pack("<II", v, n) for v, n in sorted(index)))
print("rawdata:", len(FILES), "files")
