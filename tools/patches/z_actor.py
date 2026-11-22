"""Hand adaptation of source/game/code/z_actor.c (run once after import)."""
import re
import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
from cfuncs import stub_funcs, find_func  # noqa: E402

p = os.path.join(os.path.dirname(__file__), "..", "..", "source", "game", "code", "z_actor.c")
s = open(p, encoding="utf-8").read()

s = s.replace('#include "engine/gfx3d.h"\n', '#include "engine/gfx3d.h"\n#include "engine/stub.h"\n#include "exact_math.h"\n', 1)
# display-list references
s = s.replace("PlayState* play, Gfx* dlist, Color_RGBA8* color)", "PlayState* play, DListRef dlist, Color_RGBA8* color)")
s = re.sub(r"\n\s*gDPSetCombineLERP\([^;]*\);", "", s)
s = re.sub(r"\bLight\*", "LightN*", s)
s = s.replace("Gfx gActorSetupOpaDL[] = {\n    gsSPEndDisplayList(),\n};\n", "// gActorSetupOpaDL: empty render-state list, not needed natively\n")
s = s.replace("Mtx D_8015BBA8;\n", "")
# object dependency: bind the actor's converted object to segment 6
s = s.replace("    gSegments[6] = OS_K0_TO_PHYSICAL(play->objectCtx.slots[actor->objectSlot].segment);",
              "    Asset_SetSegment(6, play->objectCtx.slots[actor->objectSlot].dsObj);")
s = s.replace("    temp = gSegments[6];\n    Actor_Init(actor, play);\n    gSegments[6] = temp;",
              "    temp = Asset_GetSegment(6);\n    Actor_Init(actor, play);\n    Asset_SetSegment(6, temp);")
# Actor_Spawn: temp variable type
s = re.sub(r"(\n\s*)u32 temp;(\s*\n)", r"\1struct DsObj* temp;\2", s, count=1)
# DoorLockInfo / BodyBreak DL fields
s = s.replace("    /* 0x14 */ Gfx* chainDL;\n    /* 0x18 */ Gfx* lockDL;", "    /* 0x14 */ DListRef chainDL;\n    /* 0x18 */ DListRef lockDL;")
s = s.replace("u32 count, Gfx** dList,", "u32 count, DListRef* dList,")
# debug no-op strings and fault clients
s = re.sub(r"\n\s*gDPNoOpString\([^;]*\);", "", s)
s = re.sub(r"\n\s*FaultClient faultClient;", "", s)
s = re.sub(r"\n\s*Fault_AddClient\([^;]*\);", "", s)
s = re.sub(r"\n\s*Fault_RemoveClient\([^;]*\);", "", s)
# render-mode-only dynamic display lists (segment 0x0C): the preset set just before covers it
s = s.replace("    Gfx3D_SetSegment(0x0C, (const void*)(func_80034B28(play->state.gfxCtx)));\n", "")
s = s.replace("    Gfx3D_SetSegment(0x0C, (const void*)(func_80034B54(play->state.gfxCtx)));\n", "")
# presentation-only functions (draw-only; no game state written) -> logged stubs
s = stub_funcs(s, ["TitleCard_Draw", "func_8002E830", "func_8002EABC", "func_8002EB44", "func_8002EBCC",
                   "func_8002ED80", "Actor_FaultPrint", "Actor_DrawLensOverlay", "func_80034B28", "func_80034B54"])
for name, ret in (("func_8002E830", "Gfx*"), ("func_8002EABC", "Hilite*"), ("func_8002EB44", "Hilite*"),
                  ("func_80034B28", "Gfx*"), ("func_80034B54", "Gfx*")):
    a, _ = find_func(s, name)
    line_end = s.index("{", a)
    head = s[a:line_end]
    head = head.replace(ret, "void*", 1).replace("Gfx* gfx", "void* gfx").replace("Hilite** hilite", "void** hilite")
    s = s[:a] + head + s[line_end:]
# title card texture: loaded from the converted title file (not yet converted -> stub P-TITLECARD)
s = s.replace(
    "        DMA_REQUEST_SYNC(texture, loadedScene->titleFile.vromStart, size, \"../z_actor.c\", 2765);",
    "        PRESENTATION_STUB(\"TitleCard_InitPlaceName texture load\"); // title texture not converted yet")
open(p, "w", encoding="utf-8", newline="\n").write(s)
print("patched", p)

# ---- pass 2: lens-of-truth actors are drawn without the z-buffer lens mask (presentation only)
s = open(p, encoding="utf-8").read()
a, b = find_func(s, "Actor_DrawLensActors")
s = s[:a] + r'''void Actor_DrawLensActors(PlayState* play, s32 numInvisibleActors, Actor** invisibleActors) {
    // native: the original masked these draws with a z-buffer stencil of the lens circle. The DS
    // draws them unmasked (presentation-only; which actors are drawn is decided by the caller).
    Actor** invisibleActor = &invisibleActors[0];
    s32 i;

    PRESENTATION_STUB("Actor_DrawLensActors lens mask");
    for (i = 0; i < numInvisibleActors; i++) {
        Actor_Draw(play, *(invisibleActor++));
    }
}

''' + s[b:]
open(p, "w", encoding="utf-8", newline="\n").write(s)
print("patched pass 2")

# ---- pass 3: actor "overlay" loading on a statically linked build
s = open(p, encoding="utf-8").read()
s = s.replace(r'''            Overlay_Load(overlayEntry->file.vromStart, overlayEntry->file.vromEnd, overlayEntry->vramStart,
                         overlayEntry->vramEnd, overlayEntry->loadedRamAddr);''',
              r'''            // native: code is statically linked; the reservation above keeps the original heap usage''')
s = s.replace(r'''        profile = (void*)(uintptr_t)((overlayEntry->profile != NULL)
                                         ? (void*)((uintptr_t)overlayEntry->profile -
                                                   (intptr_t)((uintptr_t)overlayEntry->vramStart -
                                                              (uintptr_t)overlayEntry->loadedRamAddr))
                                         : NULL);''',
              r'''        profile = overlayEntry->profile; // native: statically linked, no relocation''')
s = s.replace(r'''    objectSlot = Object_GetSlot(&play->objectCtx, profile->objectId);''',
              r'''    if (profile == NULL) {
        ActorNative_LogUnported(actorId);
        Actor_FreeOverlay(overlayEntry);
        return NULL;
    }

    objectSlot = Object_GetSlot(&play->objectCtx, profile->objectId);''', 1)
open(p, "w", encoding="utf-8", newline="\n").write(s)
print("patched pass 3")
