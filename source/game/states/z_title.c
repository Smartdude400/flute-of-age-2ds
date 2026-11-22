// adapted from flute-of-age/src/overlays/gamestates/ovl_title/z_title.c (ConsoleLogo gamestate)
#include "gfx.h"
#include "libu64/gfxprint.h"
// Logic (ConsoleLogo_Calc, exit condition, save-context writes, update rate) is kept verbatim for the
// N64 path. Drawing is rewritten for the DS renderer (engine/gfx2d). Removed: N64DD/CIC
// hardware branches, debug build-info printer.
#include "console_logo_state.h"
#include "title_setup_state.h"
#include "regs.h"
#include "save.h"
#include "sequence.h"
#include "sys_matrix.h"
#include "engine/gfx2d.h"
#include "engine/log.h"

void ConsoleLogo_Calc(ConsoleLogoState* this) {
    if ((this->coverAlpha == 0) && (this->visibleDuration != 0)) {
        this->unk_1D4--;
        this->visibleDuration--;
        if (this->unk_1D4 == 0) {
            this->unk_1D4 = 400;
        }
    } else {
        this->coverAlpha += this->addAlpha;
        if (this->coverAlpha <= 0) {
            this->coverAlpha = 0;
            this->addAlpha = 3;
        } else if (this->coverAlpha >= 255) {
            this->coverAlpha = 255;
            this->exit = true;
        }
    }
    this->uls = this->ult & 0x7F;
    this->ult++;
}

void ConsoleLogo_Draw(ConsoleLogoState* this) {
    static const char kTitle[] = "FLUTE OF AGE";
    Gfx2D_Begin(0, 0, 0);
    // Original: GfxPrint at x = (SCREEN_WIDTH - len*8)/2, y = (SCREEN_HEIGHT - 8)/2, colour (255,215,80)
    Gfx2D_Text((SCREEN_WIDTH - (sizeof(kTitle) - 1) * GFX_CHAR_X_SPACING) / 2,
               (SCREEN_HEIGHT - GFX_CHAR_Y_SPACING) / 2, 255, 215, 80, 255, kTitle);
    // Original: Environment_FillScreen(gfxCtx, 0, 0, 0, coverAlpha, FILL_SCREEN_XLU)
    Gfx2D_FillScreen(0, 0, 0, this->coverAlpha);
}

void ConsoleLogo_Main(GameState* thisx) {
    ConsoleLogoState* this = (ConsoleLogoState*)thisx;

    ConsoleLogo_Calc(this);
    ConsoleLogo_Draw(this);

    if (this->exit) {
        gSaveContext.seqId = (u8)NA_BGM_DISABLED;
        gSaveContext.natureAmbienceId = 0xFF;
        gSaveContext.gameMode = GAMEMODE_TITLE_SCREEN;
        this->state.running = false;
        log_printf("[state] ConsoleLogo: exit at frame %lu\n", (unsigned long)this->state.frames);
        SET_NEXT_GAMESTATE(&this->state, TitleSetup_Init, TitleSetupState);
    }
}

void ConsoleLogo_Destroy(GameState* thisx) {
    ConsoleLogoState* this = (ConsoleLogoState*)thisx;

    Sram_InitSram(&this->state, &this->sramCtx);
}

void ConsoleLogo_Init(GameState* thisx) {
    ConsoleLogoState* this = (ConsoleLogoState*)thisx;

    log_printf("[state] ConsoleLogo: init\n");
    this->staticSegment = NULL;
    R_UPDATE_RATE = 1;
    Matrix_Init(&this->state);
    View_Init(&this->view, this->state.gfxCtx);
    this->state.main = ConsoleLogo_Main;
    this->state.destroy = ConsoleLogo_Destroy;
    this->exit = false;

    // OOT_VERSION < GC_US: file number depends on controller 1 being present (always true here)
    gSaveContext.fileNum = 0xFF;

    Sram_Alloc(&this->state, &this->sramCtx);
    this->ult = 0;
    this->unk_1D4 = 0x14;
    this->coverAlpha = 255;
    this->addAlpha = -3;
    this->visibleDuration = 0x3C;
}
