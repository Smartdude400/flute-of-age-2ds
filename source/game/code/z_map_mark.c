// adapted from flute-of-age/src/code/z_map_mark.c
#include "engine/rdp_gbi.h"
#include "engine/rdp_disps.h"
#include "libu64/debug.h"
#include "misc_overlay_sizes.h"
#include "engine/gfx3d.h"
#include "attributes.h"
#include "libu64/overlay.h"
#include "map.h"
#include "printf.h"
#if PLATFORM_N64
#endif
#include "regs.h"
#include "romfile.h"
#include "segment_symbols.h"
#include "translation.h"
#include "map_mark.h"
#include "play_state.h"
#include "save.h"

#include "assets/textures/parameter_static/parameter_static.h"

typedef struct MapMarkInfo {
    /* 0x00 */ void* texture;
    /* 0x04 */ u32 imageFormat;
    /* 0x08 */ u32 imageSize;
    /* 0x0C */ u32 textureWidth;
    /* 0x10 */ u32 textureHeight;
    /* 0x14 */ u32 rectWidth;
    /* 0x18 */ u32 rectHeight;
    /* 0x1C */ u32 dsdx;
    /* 0x20 */ u32 dtdy;
} MapMarkInfo; // size = 0x24

typedef struct MapMarkDataOverlay {
    /* 0x00 */ void* loadedRamAddr; // original name: "allocp"
    /* 0x04 */ RomFile file;
    /* 0x0C */ void* vramStart;
    /* 0x10 */ void* vramEnd;
    /* 0x14 */ void* vramTable;
} MapMarkDataOverlay; // size = 0x18

#define GDP_LOADTEXTUREBLOCK_RUNTIME_QUALIFIERS
// native: runtime texture-load macros not needed (map marks are drawn natively)

MapMarkInfo sMapMarkInfoTable[] = {
    { gMapChestIconTex, 0 /* RGBA */, 2 /* 16b */, 8, 8, 32, 32, 1 << 10, 1 << 10 }, // Chest Icon
    { gMapBossIconTex, 3 /* IA */, 1 /* 8b */, 8, 8, 32, 32, 1 << 10, 1 << 10 },     // Boss Skull Icon
};

static MapMarkDataOverlay sMapMarkDataOvl = {
    NULL, { 0, 0 }, (void*)0, (void*)OVL_SIZE_MAP_MARK_DATA, gMapMarkDataTable, // native: overlay statically linked
};

static MapMarkData** sLoadedMarkDataTable;

void MapMark_Init(PlayState* play) {
    MapMarkDataOverlay* overlay = &sMapMarkDataOvl;
    u32 overlaySize = (uintptr_t)overlay->vramEnd - (uintptr_t)overlay->vramStart;

    overlay->loadedRamAddr = GAME_STATE_ALLOC(&play->state, overlaySize, "../z_map_mark.c", 235);
    LOG_UTILS_CHECK_NULL_POINTER("dlftbl->allocp", overlay->loadedRamAddr, "../z_map_mark.c", 236);

    // native: ovl_map_mark_data is statically linked; the original overlay size stays reserved above

    sLoadedMarkDataTable = gMapMarkDataTable;

    // native: 64DD hook removed (no disk drive)
}

void MapMark_ClearPointers(UNUSED PlayState* play) {
    // native: 64DD hook removed (no disk drive)

    sMapMarkDataOvl.loadedRamAddr = NULL;
    sLoadedMarkDataTable = NULL;
}

void MapMark_DrawForDungeon(PlayState* play) {
    // (restored: original body, native RDP commands via engine/rdp_gbi.h)
    InterfaceContext* interfaceCtx;
    MapMarkIconData* mapMarkIconData;
    MapMarkPoint* markPoint;
    MapMarkInfo* markInfo;
    u16 dungeon = gSaveContext.mapIndex;
    s32 i;
    s32 rectLeft;
    s32 rectTop;

    interfaceCtx = &play->interfaceCtx;

    if ((gMapData != NULL) && (play->interfaceCtx.mapRoomNum >= gMapData->dgnMinimapCount[dungeon])) {
        PRINTF(VT_COL(RED, WHITE) T("部屋番号がオーバーしてるで,ヤバイで %d/%d  \nMapMarkDraw の処理を中断します\n",
                                    "Room number exceeded, yikes %d/%d  \nMapMarkDraw processing interrupted\n"),
               VT_RST, play->interfaceCtx.mapRoomNum, gMapData->dgnMinimapCount[dungeon]);
        return;
    }

    mapMarkIconData = &sLoadedMarkDataTable[dungeon][interfaceCtx->mapRoomNum][0];

    OPEN_DISPS(play->state.gfxCtx, "../z_map_mark.c", 303);

    while (true) {
        if (mapMarkIconData->markType == MAP_MARK_NONE) {
            break;
        }

        gDPPipeSync(OVERLAY_DISP++);
        gDPSetTextureLUT(OVERLAY_DISP++, G_TT_NONE);
        gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, interfaceCtx->minimapAlpha);
        gDPSetEnvColor(OVERLAY_DISP++, 0, 0, 0, interfaceCtx->minimapAlpha);

        markPoint = &mapMarkIconData->points[0];
        for (i = 0; i < mapMarkIconData->count; i++) {
            if ((mapMarkIconData->markType != MAP_MARK_CHEST) || !Flags_GetTreasure(play, markPoint->chestFlag)) {
                markInfo = &sMapMarkInfoTable[mapMarkIconData->markType];

                gDPPipeSync(OVERLAY_DISP++);
                gDPLoadTextureBlock_Runtime(OVERLAY_DISP++, markInfo->texture, markInfo->imageFormat,
                                            markInfo->imageSize, markInfo->textureWidth, markInfo->textureHeight, 0,
                                            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK,
                                            G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

                rectLeft = ((DEBUG_FEATURES ? GREG(94) : 0) + markPoint->x + 204) << 2;
                rectTop = ((DEBUG_FEATURES ? GREG(95) : 0) + markPoint->y + 140) << 2;
                gSPTextureRectangle(OVERLAY_DISP++, rectLeft, rectTop, markInfo->rectWidth + rectLeft,
                                    rectTop + markInfo->rectHeight, G_TX_RENDERTILE, 0, 0, markInfo->dsdx,
                                    markInfo->dtdy);
            }
            markPoint++;
        }
        mapMarkIconData++;
    }

    CLOSE_DISPS(play->state.gfxCtx, "../z_map_mark.c", 339);
}

void MapMark_Draw(PlayState* play) {
    switch (play->sceneId) {
        case SCENE_ROOT_TREE:
        case SCENE_CRAGJAWS_CAVERN:
        case SCENE_TIDE_MAW:
        case SCENE_FOREST_TEMPLE:
        case SCENE_FIRE_TEMPLE:
        case SCENE_WATER_TEMPLE:
        case SCENE_SPIRIT_TEMPLE:
        case SCENE_SHADOW_TEMPLE:
        case SCENE_BOTTOM_OF_THE_WELL:
        case SCENE_ICE_CAVERN:
        case SCENE_ROOT_TREE_BOSS:
        case SCENE_CRAGJAWS_CAVERN_BOSS:
        case SCENE_TIDE_MAW_BOSS:
        case SCENE_FOREST_TEMPLE_BOSS:
        case SCENE_FIRE_TEMPLE_BOSS:
            MapMark_DrawForDungeon(play);
            break;
    }
}
