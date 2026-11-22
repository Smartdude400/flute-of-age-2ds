"""Hand adaptations of small engine files (run once after import)."""
import os
import re
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
from cfuncs import stub_funcs  # noqa: E402

D = os.path.join(os.path.dirname(__file__), "..", "..", "source", "game", "code")


def edit(name, fn):
    p = os.path.join(D, name)
    s = open(p, encoding="utf-8").read()
    t = fn(s)
    open(p, "w", encoding="utf-8", newline="\n").write(t)
    print("patched", name)


def inc(s, header):
    return re.sub(r'(#include "[^"]+"\n)', r'\1#include "' + header + '"\n', s, count=1)


def sram(s):
    s = inc(s, "engine/sram_native.h")
    s = s.replace("SsSram_ReadWrite(addr, dramAddr, size, OS_READ)", "SsSram_ReadWrite(addr, dramAddr, size, SRAM_DIR_READ)")
    s = s.replace("SsSram_ReadWrite(addr, dramAddr, size, OS_WRITE)", "SsSram_ReadWrite(addr, dramAddr, size, SRAM_DIR_WRITE)")
    # SRAM is addressed by offset natively (the original added the cartridge SRAM base address)
    s = s.replace("OS_K1_TO_PHYSICAL(0xA8000000) + ", "").replace("OS_K1_TO_PHYSICAL(0xA8000000)", "0")
    s = re.sub(r"#else\n\nvoid Sram_ReadWriteIQue.*?\n#endif\n", "#endif\n", s, flags=re.S)
    return s


def construct(s):
    s = inc(s, "segment_symbols.h")
    s = s.replace(r'''    DMA_REQUEST_SYNC(interfaceCtx->parameterSegment, (uintptr_t)_parameter_staticSegmentRomStart, parameterSize,
                     "../z_construct.c", 162);''',
                  r'''    // native: the HUD draws converted textures (assets_agg/parameter_static.h); the arena allocation
    // above is kept for identical game-arena usage, its contents are unused.''')
    s = re.sub(r"\n\s*DMA_REQUEST_SYNC\([^;]*\);", "\n    /* native: HUD/text textures come from converted files; allocation kept */", s)
    return s


def item00(s):
    return s.replace("POLY_OPA_DISP = Play_SetFog(play, POLY_OPA_DISP);", "Play_SetFog(play);")


def akeep(s):
    return s.replace("static Gfx* sDLists[] = {", "static DListRef sDLists[] = {")


def elfmsg(s):
    return re.sub(r"\n\s*LOG_STRING_T\([^;]*\);", "", s)


def fcurve(s):
    return s.replace("    Gfx* dList;", "    DListRef dList;")


def skin(s):
    s = inc(s, "engine/stub.h")
    s = s.replace("Vtx* vtxBuf, Vec3f* pos", "void* vtxBuf, Vec3f* pos").replace("    Vtx* vtxBuf;", "    void* vtxBuf;")
    s = s.replace("s32 limbIndex, Gfx* dlistOverride,", "s32 limbIndex, DListRef dlistOverride,")
    return stub_funcs(s, ["Skin_UpdateVertices", "Skin_DrawLimb", "Skin_DrawImpl"])


def skin_awb(s):
    s = inc(s, "engine/stub.h")
    s = stub_funcs(s, ["Skin_InitAnimatedLimb"])
    # vertex buffers: keep the original 16-byte vertex size for identical actor-heap usage
    return s.replace("totalVtxCount * sizeof(Vtx)", "totalVtxCount * 16 /* original vertex size */")


edit("z_sram.c", sram)
edit("z_construct.c", construct)
edit("z_en_item00.c", item00)
edit("z_en_a_keep.c", akeep)
edit("z_elf_message.c", elfmsg)
edit("z_fcurve_data_skelanime.c", fcurve)
edit("z_skin.c", skin)
edit("z_skin_awb.c", skin_awb)
