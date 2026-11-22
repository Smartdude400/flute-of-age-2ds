// adapted from flute-of-age/src/overlays/gamestates/ovl_opening/z_opening.c (TitleSetup gamestate)
#include "title_setup_state.h"
#include "play_state.h"
#include "regs.h"
#include "save.h"
#include "sram.h"
#include "sys_matrix.h"
#include "engine/log.h"

void TitleSetup_SetupTitleScreen(TitleSetupState* this) {
    gSaveContext.gameMode = GAMEMODE_TITLE_SCREEN;
    this->state.running = false;
    gSaveContext.save.arenAge = AREN_AGE_ADULT;
    Sram_InitDebugSave();
    gSaveContext.save.cutsceneIndex = CS_INDEX_3;
    // assigning scene layer here is redundant, as Play_Init sets it right away
    gSaveContext.sceneLayer = GET_CUTSCENE_LAYER(CS_INDEX_3);
    log_printf("[state] TitleSetup: -> Play (title cutscene)\n");
    SET_NEXT_GAMESTATE(&this->state, Play_Init, PlayState);
}

void TitleSetup_Main(GameState* thisx) {
    TitleSetupState* this = (TitleSetupState*)thisx;

    TitleSetup_SetupTitleScreen(this);
}

void TitleSetup_Destroy(GameState* thisx) {
    (void)thisx;
}

void TitleSetup_Init(GameState* thisx) {
    TitleSetupState* this = (TitleSetupState*)thisx;

    log_printf("[state] TitleSetup: init\n");
    R_UPDATE_RATE = 1;
    Matrix_Init(&this->state);
    View_Init(&this->view, this->state.gfxCtx);
    this->state.main = TitleSetup_Main;
    this->state.destroy = TitleSetup_Destroy;
}
