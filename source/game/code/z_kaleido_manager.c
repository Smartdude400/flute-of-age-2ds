// adapted from flute-of-age/src/code/z_kaleido_manager.c
// The pause menu and the player actor were overlays sharing one "kaleido area" in the game arena.
// On DS their code is statically linked:
//  - the kaleido area is still allocated with the ORIGINAL size (largest of the two overlays'
//    text+data+rodata+bss, from the ROM), keeping game-arena usage identical;
//  - KaleidoManager_GetRamAddr returns its argument (functions are already at their final address).
#include "array_count.h"
#include "kaleido_manager.h"
#include "printf.h"
#include "play_state.h"

#define NATIVE_OVL_BASE 0x1C000000u
#define KALEIDO_SCOPE_OVL_SIZE 0x1B2F0 // ROM ovl_kaleido_scope relocation trailer
#define PLAYER_ACTOR_OVL_SIZE 0x231F0  // ROM ovl_player_actor relocation trailer

KaleidoMgrOverlay gKaleidoMgrOverlayTable[] = {
    { NULL, ROM_FILE_UNSET, (void*)NATIVE_OVL_BASE, (void*)(NATIVE_OVL_BASE + KALEIDO_SCOPE_OVL_SIZE), 0,
      "kaleido_scope" },
    { NULL, ROM_FILE_UNSET, (void*)NATIVE_OVL_BASE, (void*)(NATIVE_OVL_BASE + PLAYER_ACTOR_OVL_SIZE), 0,
      "player_actor" },
};

void* sKaleidoAreaPtr = NULL;
KaleidoMgrOverlay* gKaleidoMgrCurOvl = NULL;
u8 gBossMarkState = 0;

void KaleidoManager_LoadOvl(KaleidoMgrOverlay* ovl) {
    ovl->loadedRamAddr = sKaleidoAreaPtr;
    ovl->offset = 0; // native: no relocation
    gKaleidoMgrCurOvl = ovl;
}

void KaleidoManager_ClearOvl(KaleidoMgrOverlay* ovl) {
    if (ovl->loadedRamAddr != NULL) {
        ovl->offset = 0;
        memset(ovl->loadedRamAddr, 0, (uintptr_t)ovl->vramEnd - (uintptr_t)ovl->vramStart);
        ovl->loadedRamAddr = NULL;
        gKaleidoMgrCurOvl = NULL;
    }
}

void KaleidoManager_Init(PlayState* play) {
    s32 largestSize = 0;
    s32 size;
    u32 i;

    for (i = 0; i < ARRAY_COUNT(gKaleidoMgrOverlayTable); i++) {
        size = (uintptr_t)gKaleidoMgrOverlayTable[i].vramEnd - (uintptr_t)gKaleidoMgrOverlayTable[i].vramStart;
        if (size > largestSize) {
            largestSize = size;
        }
    }

    sKaleidoAreaPtr = GAME_STATE_ALLOC(&play->state, largestSize, "../z_kaleido_manager.c", 150);
    gKaleidoMgrCurOvl = NULL;
}

void KaleidoManager_Destroy(void) {
    if (gKaleidoMgrCurOvl != NULL) {
        KaleidoManager_ClearOvl(gKaleidoMgrCurOvl);
        gKaleidoMgrCurOvl = NULL;
    }

    sKaleidoAreaPtr = NULL;
}

void* KaleidoManager_GetRamAddr(void* vram) {
    return vram; // native: statically linked
}
