// adapted from flute-of-age/src/code/title_setup.c (Setup gamestate)
#include "console_logo_state.h"
#include "setup_state.h"
#include "save.h"
#include "engine/log.h"
#ifdef PORT_BOOT_FILESELECT
#include "file_select_state.h"
#endif

#ifdef PORT_BOOT_ENTRANCE
#include "play_state.h"
#include "sram.h"
#include "sequence.h"
#include "interface.h"
#include "message.h"
// Test boot (build flag BOOT=<entrance>): what file select does when a NEW file is created and started
// (Sram_InitSave + FileSelect_LoadGame), but entering the given entrance without the intro cutscene.
// File select itself (ovl_file_choose) is not ported yet (STUBS S12).
static void Setup_BootIntoPlay(SetupState* this) {
    Sram_InitNewSave();
    {
        // the name file select's name entry would store: the original debug save's name (z_sram.c)
        static const u8 kName[8] = { FILENAME_UPPERCASE('A'), FILENAME_UPPERCASE('R'), FILENAME_UPPERCASE('E'),
                                     FILENAME_UPPERCASE('N'), FILENAME_SPACE,          FILENAME_SPACE,
                                     FILENAME_SPACE,          FILENAME_SPACE };
        for (int i = 0; i < 8; i++) {
            gSaveContext.save.info.playerData.playerName[i] = kName[i];
        }
    }
    gSaveContext.save.entranceIndex = PORT_BOOT_ENTRANCE;
    gSaveContext.save.arenAge = AREN_AGE_CHILD;
    gSaveContext.save.dayTime = CLOCK_TIME(10, 0);
    gSaveContext.save.cutsceneIndex = CS_INDEX_NONE;
    gSaveContext.fileNum = 0;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    gSaveContext.respawn[RESPAWN_MODE_DOWN].entranceIndex = ENTR_LOAD_OPENING;
    gSaveContext.respawnFlag = 0;
    gSaveContext.seqId = (u8)NA_BGM_DISABLED;
    gSaveContext.natureAmbienceId = 0xFF;
    gSaveContext.showTitleCard = true;
    gSaveContext.dogParams = 0;
    gSaveContext.timerState = TIMER_STATE_OFF;
    gSaveContext.subTimerState = SUBTIMER_STATE_OFF;
    gSaveContext.eventInf[0] = gSaveContext.eventInf[1] = gSaveContext.eventInf[2] = gSaveContext.eventInf[3] = 0;
    gSaveContext.prevHudVisibilityMode = HUD_VISIBILITY_ALL;
    gSaveContext.nerisLoveTimer = 0;
    gSaveContext.healthAccumulator = 0;
    gSaveContext.magicState = MAGIC_STATE_IDLE;
    gSaveContext.prevMagicState = MAGIC_STATE_IDLE;
    gSaveContext.forcedSeqId = NA_BGM_GENERAL_SFX;
    gSaveContext.skyboxTime = CLOCK_TIME(0, 0);
    gSaveContext.nextTransitionType = TRANS_NEXT_TYPE_DEFAULT;
    gSaveContext.nextCutsceneIndex = NEXT_CS_INDEX_NONE;
    gSaveContext.cutsceneTrigger = 0;
    gSaveContext.chamberCutsceneNum = CHAMBER_CS_FOREST;
    gSaveContext.nextDayTime = NEXT_TIME_NONE;
    gSaveContext.retainWeatherMode = false;
    gSaveContext.buttonStatus[0] = gSaveContext.buttonStatus[1] = gSaveContext.buttonStatus[2] =
        gSaveContext.buttonStatus[3] = gSaveContext.buttonStatus[4] = BTN_ENABLED;
    gSaveContext.forceRisingButtonAlphas = gSaveContext.nextHudVisibilityMode = gSaveContext.hudVisibilityMode =
        gSaveContext.hudVisibilityModeTimer = gSaveContext.magicCapacity = 0;
    gSaveContext.magicFillTarget = gSaveContext.save.info.playerData.magic;
    gSaveContext.save.info.playerData.magicLevel = gSaveContext.save.info.playerData.magic = 0;
    log_printf("[state] Setup: test boot into entrance 0x%X\n", PORT_BOOT_ENTRANCE);
    SET_NEXT_GAMESTATE(&this->state, Play_Init, PlayState);
}
#endif

void Setup_InitImpl(SetupState* this) {
    log_printf("[state] Setup: common data init\n");
    SaveContext_Init();
    this->state.running = false;
#ifdef PORT_BOOT_ENTRANCE
    Setup_BootIntoPlay(this);
    return;
#endif
#ifdef PORT_BOOT_FILESELECT
    // Test boot (build flag FILESELECT=1): straight into file select, as after pressing Start on the title
    // screen (whose logo actor is not ported yet)
    log_printf("[state] Setup: test boot into file select\n");
    SET_NEXT_GAMESTATE(&this->state, FileSelect_Init, FileSelectState);
    return;
#endif
    SET_NEXT_GAMESTATE(&this->state, ConsoleLogo_Init, ConsoleLogoState);
}

void Setup_Destroy(GameState* thisx) {
    (void)thisx;
}

void Setup_Init(GameState* thisx) {
    SetupState* this = (SetupState*)thisx;

    this->state.destroy = Setup_Destroy;
    Setup_InitImpl(this);
}
