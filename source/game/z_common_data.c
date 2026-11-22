// adapted from flute-of-age/src/code/z_common_data.c
#include "alignment.h"
#include "region.h"
#include "sequence.h"
#include "versions.h"
#include "z_locale.h"
#include "environment.h"
#include "save.h"
#include "transition.h"

ALIGNED(16) SaveContext gSaveContext;
u32 D_8015FA88;
u32 D_8015FA8C;

void SaveContext_Init(void) {
    memset(&gSaveContext, 0, sizeof(gSaveContext));
    D_8015FA88 = 0;
    D_8015FA8C = 0;
    gSaveContext.seqId = (u8)NA_BGM_DISABLED;
    gSaveContext.natureAmbienceId = NATURE_ID_DISABLED;
    gSaveContext.forcedSeqId = NA_BGM_GENERAL_SFX;
    gSaveContext.nextCutsceneIndex = NEXT_CS_INDEX_NONE;
    gSaveContext.cutsceneTrigger = 0;
    gSaveContext.chamberCutsceneNum = CHAMBER_CS_FOREST;
    gSaveContext.nextDayTime = NEXT_TIME_NONE;
    gSaveContext.skyboxTime = 0;
    gSaveContext.dogIsLost = true;
    gSaveContext.nextTransitionType = TRANS_NEXT_TYPE_DEFAULT;
    gSaveContext.prevHudVisibilityMode = HUD_VISIBILITY_ALL;
#if OOT_NTSC && OOT_VERSION < GC_US || PLATFORM_IQUE
    if (gCurrentRegion == REGION_JP) {
        gSaveContext.language = LANGUAGE_JPN;
    }
    if (gCurrentRegion == REGION_US) {
        gSaveContext.language = LANGUAGE_ENG;
    }
#elif OOT_VERSION == GC_US || OOT_VERSION == GC_US_MQ
    gSaveContext.language = LANGUAGE_ENG;
#elif OOT_VERSION == GC_JP_CE
    gSaveContext.language = LANGUAGE_JPN;
#endif
}
