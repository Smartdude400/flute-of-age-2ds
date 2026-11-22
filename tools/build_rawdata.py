"""Copy ROM data files that the translated game code reads as plain BYTE records into romfs/vrom/<vrom>.bin.
Only files whose records are made of u8 fields only belong here (no endianness, no pointers), so the
bytes are the native layout as-is:
  elf_message_field, elf_message_ydan: QuestHintCmd scripts (u8 byte0..byte3, quest_hint_commands.h),
  loaded by Play_LoadFile for the companion's hints."""
import os
from romfs import RomFS

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "romfs", "vrom")
FILES = ["elf_message_field", "elf_message_ydan"]

fs = RomFS("ntsc-1.0")
os.makedirs(OUT, exist_ok=True)
for name in FILES:
    vrom = fs.entries[name][0]
    open(os.path.join(OUT, f"{vrom:08X}.bin"), "wb").write(fs.get(name))
print("rawdata:", len(FILES), "files")
