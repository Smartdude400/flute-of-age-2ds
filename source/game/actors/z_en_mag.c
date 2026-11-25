// adapted from flute-of-age/src/overlays/actors/ovl_En_Mag/z_en_mag.c
/*
 * File: z_en_mag.c
 * Overlay: ovl_En_Mag
 * Description: Title Screen Manager & Logo
 */

#include "src/overlays/actors/ovl_En_Mag/z_en_mag.h"
#include "engine/rdp_disps.h"
#include "dma.h"

#include "array_count.h"
#include "controller.h"
#include "gfx.h"
#include "gfx_setupdl.h"
#include "gfxalloc.h"
#include "libu64/gfxprint.h"
#include "printf.h"
#include "regs.h"
#include "sfx.h"
#include "stack_pad.h"
#include "translation.h"
#include "versions.h"
#include "audio.h"
#include "cutscene_flags.h"
#include "play_state.h"
#include "save.h"
#include "ss_sram.h"

#include "assets/objects/object_mag/object_mag.h"

#define FLAGS (ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED)

void EnMag_Init(Actor* thisx, PlayState* play);
void EnMag_Destroy(Actor* thisx, PlayState* play);
void EnMag_Update(Actor* thisx, PlayState* play);
void EnMag_Draw(Actor* thisx, PlayState* play);

ActorProfile En_Mag_Profile = {
    /**/ ACTOR_EN_MAG,
    /**/ ACTORCAT_PROP,
    /**/ FLAGS,
    /**/ OBJECT_MAG,
    /**/ sizeof(EnMag),
    /**/ EnMag_Init,
    /**/ EnMag_Destroy,
    /**/ EnMag_Update,
    /**/ EnMag_Draw,
};

static s16 sDelayTimer = 0;

#if OOT_VERSION < GC_US || PLATFORM_IQUE
void EnMag_ResetSram(void) {
    static u8 buffer[0x2000];

    PRINTF(T("\n\n\n＝＝＝＝＝＝＝＝＝＝\n  ＳＲＡＭ初期化\n\n＝＝＝＝＝＝＝＝＝＝\n\n\n",
             "\n\n\n==========\n  SRAM initialization\n\n==========\n\n\n"));

    memset(buffer, 0, 0x800);
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000), buffer, 0x800, 1);
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000800), buffer, 0x800, 1);
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8001000), buffer, 0x800, 1);
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8001800), buffer, 0x800, 1);
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8002000), buffer, 0x800, 1);
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8002800), buffer, 0x800, 1);
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8003000), buffer, 0x800, 1);
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8003800), buffer, 0x800, 1);
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8004000), buffer, 0x800, 1);
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8004800), buffer, 0x800, 1);
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8005000), buffer, 0x800, 1);
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8005800), buffer, 0x800, 1);
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8006000), buffer, 0x800, 1);
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8006800), buffer, 0x800, 1);
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8007000), buffer, 0x800, 1);
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8007800), buffer, 0x800, 1);

    gSaveContext.soundSetting = gSaveContext.zTargetSetting = 0; // SOUND_SETTING_STEREO/Z_TARGET_SETTING_SWITCH
    Audio_SetSoundOutputMode(gSaveContext.soundSetting);
}
#endif

void EnMag_Init(Actor* thisx, PlayState* play) {
    EnMag* this = (EnMag*)thisx;
    Font* font = &this->font;

    YREG(1) = 63;
    YREG(3) = 80;
    YREG(4) = 255;
    YREG(5) = 30;
    YREG(6) = 30;
    YREG(7) = 119;
    YREG(8) = 7;
    YREG(9) = 5;
    YREG(10) = 3;

    VREG(4) = 1;
    VREG(5) = 6;
    VREG(6) = 2;

    this->copyrightAlphaStep = 6;
    this->fadeOutAlphaStep = 10;

    VREG(19) = 99;
    VREG(21) = 9;
    VREG(23) = 10;
    VREG(24) = 8;

    this->effectScroll = 0;
    this->unk_E30C = 0;

    this->effectPrimColor[0] = 0.0f;
    this->effectPrimColor[1] = 100.0f;
    this->effectPrimColor[2] = 170.0f;
    this->effectEnvColor[0] = 0.0f;
    this->effectEnvColor[1] = 100.0f;
    this->effectEnvColor[2] = 0.0f;

    this->effectFadeInTimer = 40;

    this->effectFadeInState = this->effectPrimLodFrac = this->globalState = this->effectAlpha = this->mainAlpha =
        this->subAlpha = this->copyrightAlpha = 0.0f;

    if (gSaveContext.forceRisingButtonAlphas) {
        this->mainAlpha = 210;
        this->subAlpha = 255;
        this->copyrightAlpha = 255;

        this->effectPrimLodFrac = 128.0f;
        this->effectAlpha = 255.0f;

#if !OOT_MQ
        this->effectPrimColor[0] = 255.0f;
        this->effectPrimColor[1] = 255.0f;
        this->effectPrimColor[2] = 170;
        this->effectEnvColor[0] = 255.0f;
        this->effectEnvColor[1] = 100;
#else
        this->effectPrimColor[0] = 170;
        this->effectPrimColor[1] = 255.0f;
        this->effectPrimColor[2] = 255.0f;
        this->effectEnvColor[0] = 200.0f;
        this->effectEnvColor[1] = 255.0f;
        this->effectEnvColor[2] = 0;
#endif

        gSaveContext.forceRisingButtonAlphas = false;
        this->globalState = MAG_STATE_DISPLAY;
        sDelayTimer = 20;
        gSaveContext.transFadeDuration = 1;
        gSaveContext.transWipeSpeed = 255;
    }

    Font_LoadOrderedFont(font);

    this->unk_E316 = 0;
    this->unk_E318 = 0;
    this->unk_E31C = 0;
    this->unk_E320 = 0;
}

void EnMag_Destroy(Actor* thisx, PlayState* play) {
}

#if OOT_VERSION < GC_US || PLATFORM_IQUE
void EnMag_CheckSramResetCode(PlayState* play, EnMag* this) {
    static s32 sSramResetCode[] = {
        BTN_DUP, BTN_DDOWN,  BTN_DLEFT, BTN_DRIGHT, BTN_START, BTN_B, BTN_CDOWN,
        BTN_L,   BTN_CRIGHT, BTN_CLEFT, BTN_A,      BTN_CUP,   BTN_R, BTN_Z,
    };
    Input* input = &play->state.input[2];
    s32 var_v1;

    var_v1 =
        play->state.input[2].cur.button & (BTN_A | BTN_B | BTN_Z | BTN_START | BTN_DUP | BTN_DDOWN | BTN_DLEFT |
                                           BTN_DRIGHT | BTN_L | BTN_R | BTN_CUP | BTN_CDOWN | BTN_CLEFT | BTN_CRIGHT);
    if (var_v1 == this->unk_E31C) {
        this->unk_E320--;
        if (this->unk_E320 < 0) {
            this->unk_E320 = 1;
        } else {
            var_v1 ^= this->unk_E31C;
        }
    } else {
        this->unk_E320 = 16;
        this->unk_E31C = var_v1;
    }

    if (this->unk_E316 < 4) {
        if (var_v1 & sSramResetCode[this->unk_E316]) {
            this->unk_E316++;
        } else if (var_v1 != 0) {
            this->unk_E316 = 0;
        }
    } else {
        if (CHECK_BTN_ALL(input->press.button, sSramResetCode[this->unk_E316])) {
            this->unk_E316++;
        } else if (var_v1 != 0) {
            this->unk_E316 = 0;
        }
    }

    if (this->unk_E316 == ARRAY_COUNT(sSramResetCode)) {
        EnMag_ResetSram();
        this->unk_E316 = 0;
    }
}
#endif

void EnMag_Update(Actor* thisx, PlayState* play) {
    STACK_PAD(s32);
    Input* input = &play->state.input[0];
    EnMag* this = (EnMag*)thisx;

#if OOT_VERSION < GC_US || PLATFORM_IQUE
    EnMag_CheckSramResetCode(play, this);
#endif

    if (gSaveContext.fileNum != 0xFEDC) {
        if (this->globalState < MAG_STATE_DISPLAY) {
            if (CHECK_BTN_ALL(input->press.button, BTN_START) || CHECK_BTN_ALL(input->press.button, BTN_A) ||
                CHECK_BTN_ALL(input->press.button, BTN_B)) {

                SFX_PLAY_CENTERED(NA_SE_SY_PIECE_OF_HEART);

                this->mainAlpha = 210;
                this->subAlpha = 255;
                this->copyrightAlpha = 255;

                this->effectPrimLodFrac = 128.0f;
                this->effectAlpha = 255.0f;

#if !OOT_MQ
                this->effectPrimColor[0] = 255.0f;
                this->effectPrimColor[1] = 255.0f;
                this->effectPrimColor[2] = 170;
                this->effectEnvColor[0] = 255.0f;
                this->effectEnvColor[1] = 100;
#else
                this->effectPrimColor[0] = 170;
                this->effectPrimColor[1] = 255.0f;
                this->effectPrimColor[2] = 255.0f;
                this->effectEnvColor[0] = 200.0f;
                this->effectEnvColor[1] = 255.0f;
                this->effectEnvColor[2] = 0;
#endif

                this->globalState = MAG_STATE_DISPLAY;
                sDelayTimer = 20;
                gSaveContext.transFadeDuration = 1;
                gSaveContext.transWipeSpeed = 255;
            }
        } else if (this->globalState >= MAG_STATE_DISPLAY) {
            if (sDelayTimer == 0) {
                if (CHECK_BTN_ALL(input->press.button, BTN_START) || CHECK_BTN_ALL(input->press.button, BTN_A) ||
                    CHECK_BTN_ALL(input->press.button, BTN_B)) {

                    if (play->transitionTrigger != TRANS_TRIGGER_START) {
                        Audio_SetCutsceneFlag(0);

                        SFX_PLAY_CENTERED(NA_SE_SY_PIECE_OF_HEART);

                        gSaveContext.gameMode = GAMEMODE_FILE_SELECT;
                        play->transitionTrigger = TRANS_TRIGGER_START;
                        play->transitionType = TRANS_TYPE_FADE_BLACK;
                    }

                    this->copyrightAlphaStep = 15;
                    this->fadeOutAlphaStep = 25;
                    this->globalState = MAG_STATE_FADE_OUT;
                }
            } else {
                sDelayTimer--;
            }
        }
    }

    if (this->globalState == MAG_STATE_FADE_IN) {
        if (this->effectFadeInState == 0) {
#if !PLATFORM_GC
            this->effectPrimLodFrac += 0.8f;
            this->effectAlpha += 6.375f;
#else
            this->effectAlpha += 6.375f;
            this->effectPrimLodFrac += 0.8f;
#endif

            this->effectPrimColor[0] += 6.375f;
            this->effectPrimColor[1] += 3.875f;
            this->effectPrimColor[2] += 2.125f;
            this->effectEnvColor[0] += 6.375f;
            this->effectEnvColor[1] += 3.875f;

            this->effectFadeInTimer--;

            if (this->effectFadeInTimer == 0) {
                this->effectPrimLodFrac = 32.0f;
                this->effectAlpha = 255.0f;

                this->effectPrimColor[0] = 255.0f;
                this->effectPrimColor[1] = 255.0f;
                this->effectPrimColor[2] = 255.0f;
                this->effectEnvColor[0] = 255.0f;
                this->effectEnvColor[1] = 255.0f;

                this->effectFadeInTimer = 40;
                this->effectFadeInState = 1;
            }
        } else if (this->effectFadeInState == 1) {
#if !PLATFORM_GC
            this->effectPrimLodFrac += 2.4f;
#endif
#if !OOT_MQ
            this->effectPrimColor[2] += -2.125f;
            this->effectEnvColor[1] += -3.875f;
#else
            this->effectPrimColor[0] += -2.125f;
            this->effectEnvColor[0] += -1.375f;
#endif
#if PLATFORM_GC
            this->effectPrimLodFrac += 2.4f;
#endif

            this->effectFadeInTimer--;

            if (this->effectFadeInTimer == 0) {
                this->effectPrimLodFrac = 128.0f;

#if !OOT_MQ
                this->effectPrimColor[2] = 170.0f;
                this->effectEnvColor[1] = 100.0f;
#else
                this->effectPrimColor[0] = 170.0f;
                this->effectEnvColor[0] = 200.0f;
#endif

                this->effectFadeInTimer = 32;
                this->effectFadeInState = 2;
            }
        }

        if (this->effectAlpha > 160) {
            this->mainAlpha += VREG(5);
            if (this->mainAlpha >= 210.0f) {
                this->mainAlpha = 210.0f;
            }

            if (this->mainAlpha >= 210) {
                this->subAlpha += VREG(6);
                if (this->subAlpha >= 255.0f) {
                    this->subAlpha = 255.0f;
                }

                if (this->subAlpha >= 200) {
                    this->copyrightAlpha += this->copyrightAlphaStep;
                    if (this->copyrightAlpha >= 255.0f) {
                        this->copyrightAlpha = 255.0f;
                        this->globalState = MAG_STATE_DISPLAY;
                        sDelayTimer = 20;
                    }
                }
            }
        }
    } else if (this->globalState == MAG_STATE_FADE_OUT) {
        this->effectAlpha -= this->fadeOutAlphaStep;
        if (this->effectAlpha < 0.0f) {
            this->effectAlpha = 0.0f;
        }

        this->mainAlpha -= this->fadeOutAlphaStep;
        if (this->mainAlpha < 0.0f) {
            this->mainAlpha = 0.0f;
        }

        this->subAlpha -= this->fadeOutAlphaStep;
        if (this->subAlpha < 0.0f) {
            this->subAlpha = 0.0f;
        }

        this->copyrightAlpha -= this->copyrightAlphaStep;
        if (this->copyrightAlpha < 0.0f) {
            this->copyrightAlpha = 0.0f;
            this->globalState = MAG_STATE_POST_DISPLAY;
        }
    }

    if (this->globalState == MAG_STATE_INITIAL) {
        if (CutsceneFlags_Get(play, 3)) {
            this->effectFadeInTimer = 40;
            this->globalState = MAG_STATE_FADE_IN;
        }
    } else if (this->globalState == MAG_STATE_DISPLAY) {
        if (CutsceneFlags_Get(play, 4)) {
            this->globalState = MAG_STATE_FADE_OUT;
        }
    }
}

void EnMag_DrawCharTexture(Gfx** gfxP, u8* texture, s32 rectLeft, s32 rectTop) {
    Gfx* gfx = *gfxP;

    YREG(0) = 1024.0f / (YREG(1) / 100.0f);
    YREG(2) = 16.0f * (YREG(1) / 100.0f);

    gDPLoadTextureBlock_4b(gfx++, texture, G_IM_FMT_I, 16, 16, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                           G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

    gSPTextureRectangle(gfx++, rectLeft << 2, rectTop << 2, (rectLeft + YREG(2)) << 2, (rectTop + YREG(2)) << 2,
                        G_TX_RENDERTILE, 0, 0, YREG(0), YREG(0));

    *gfxP = gfx;
}

#if PLATFORM_IQUE
#include "assets/overlays/ovl_En_Mag/ovl_En_Mag.c"
#endif

void EnMag_DrawInner(Actor* thisx, PlayState* play, Gfx** gfxP) {
    static s16 textAlpha = 0;
    static s16 textFadeDirection = 0;
    static s16 textFadeTimer = 0;
    static u8 noControllerFontIndices[] = {
        FILENAME_UPPERCASE('N'), FILENAME_UPPERCASE('O'), FILENAME_UPPERCASE('C'), FILENAME_UPPERCASE('O'),
        FILENAME_UPPERCASE('N'), FILENAME_UPPERCASE('T'), FILENAME_UPPERCASE('R'), FILENAME_UPPERCASE('O'),
        FILENAME_UPPERCASE('L'), FILENAME_UPPERCASE('L'), FILENAME_UPPERCASE('E'), FILENAME_UPPERCASE('R'),
    };
    // For iQue, the word "PRESS" is drawn below as a Chinese character instead (gTitlePressCHN)
    static u8 pressStartFontIndices[] = {
#if !PLATFORM_IQUE
        FILENAME_UPPERCASE('P'),
        FILENAME_UPPERCASE('R'),
        FILENAME_UPPERCASE('E'),
        FILENAME_UPPERCASE('S'),
        FILENAME_UPPERCASE('S'),
#endif
        FILENAME_UPPERCASE('S'),
        FILENAME_UPPERCASE('T'),
        FILENAME_UPPERCASE('A'),
        FILENAME_UPPERCASE('R'),
        FILENAME_UPPERCASE('T'),
    };
    EnMag* this = (EnMag*)thisx;
    Font* font = &this->font;
    STACK_PAD(s32);
    Gfx* gfx = *gfxP;
    GfxPrint printer;
    u16 i;
    u16 rectLeft;

    gSPSegment(gfx++, 0x06, play->objectCtx.slots[this->actor.objectSlot].segment);

    this->effectScroll -= 2;
#if OOT_NTSC
#if !PLATFORM_IQUE
    if (gSaveContext.language == LANGUAGE_JPN)
#endif
    {
        this->unk_E30C++;
    }
#endif

    if ((s16)this->mainAlpha != 0) {
        gfx = Gfx_SetupDL_28(gfx);
        GfxPrint_Init(&printer);
        GfxPrint_Open(&printer, gfx);
        // GfxPrint's default combiner ignores primitive alpha; keep the title fade.
        printer.flags &= ~(GFXP_FLAG_UPDATE | GFXP_FLAG_SHADOW);
        gDPSetCombineMode(printer.dList++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
        GfxPrint_SetColor(&printer, 255, 215, 80, (s16)this->mainAlpha);
        GfxPrint_SetPosPx(&printer, (SCREEN_WIDTH - (sizeof("FLUTE OF AGE") - 1) * GFX_CHAR_X_SPACING) / 2, 100);
        GfxPrint_Printf(&printer, "FLUTE OF AGE");
        gfx = GfxPrint_Close(&printer);
        GfxPrint_Destroy(&printer);
    }

    Gfx_SetupDL_39Ptr(&gfx);

    gDPSetAlphaCompare(gfx++, G_AC_NONE);
    gDPSetCombineMode(gfx++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);

    gDPSetPrimColor(gfx++, 0, 0, (s16)this->copyrightAlpha, (s16)this->copyrightAlpha, (s16)this->copyrightAlpha,
                    (s16)this->copyrightAlpha);

    if ((s16)this->copyrightAlpha != 0) {
#if PLATFORM_N64
        gDPLoadTextureBlock(gfx++, gTitleCopyright1998Tex, G_IM_FMT_IA, G_IM_SIZ_8b, 128, 16, 0,
                            G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK,
                            G_TX_NOLOD, G_TX_NOLOD);
        gSPTextureRectangle(gfx++, 94 << 2, 198 << 2, 222 << 2, 214 << 2, G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
#elif PLATFORM_GC && OOT_VERSION < GC_US
        gDPLoadTextureBlock(gfx++, gTitleCopyright19982002Tex, G_IM_FMT_IA, G_IM_SIZ_8b, 160, 16, 0,
                            G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK,
                            G_TX_NOLOD, G_TX_NOLOD);
        gSPTextureRectangle(gfx++, 78 << 2, 198 << 2, 238 << 2, 214 << 2, G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
#elif PLATFORM_GC && OOT_NTSC
        if (gSaveContext.language == LANGUAGE_JPN) {
            gDPLoadTextureBlock(gfx++, gTitleCopyright19982002Tex, G_IM_FMT_IA, G_IM_SIZ_8b, 160, 16, 0,
                                G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK,
                                G_TX_NOLOD, G_TX_NOLOD);
        } else {
            gDPLoadTextureBlock(gfx++, gTitleCopyright19982003Tex, G_IM_FMT_IA, G_IM_SIZ_8b, 160, 16, 0,
                                G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK,
                                G_TX_NOLOD, G_TX_NOLOD);
        }
        gSPTextureRectangle(gfx++, 78 << 2, 198 << 2, 238 << 2, 214 << 2, G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
#elif PLATFORM_GC && OOT_PAL
        gDPLoadTextureBlock(gfx++, gTitleCopyright19982003Tex, G_IM_FMT_IA, G_IM_SIZ_8b, 160, 16, 0,
                            G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK,
                            G_TX_NOLOD, G_TX_NOLOD);
        gSPTextureRectangle(gfx++, 78 << 2, 198 << 2, 238 << 2, 214 << 2, G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
#elif PLATFORM_IQUE
        gDPLoadTextureBlock(gfx++, gTitleCopyright19982003IQueTex, G_IM_FMT_IA, G_IM_SIZ_8b, 128, 32, 0,
                            G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK,
                            G_TX_NOLOD, G_TX_NOLOD);
        gSPTextureRectangle(gfx++, 94 << 2, 198 << 2, 222 << 2, 230 << 2, G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
#endif
    }

    if (gSaveContext.fileNum == 0xFEDC) {
        // Draw "NO CONTROLLER" Text
        textAlpha = textFadeTimer * 10;
        if (textAlpha >= 255) {
            textAlpha = 255;
        }

        // Text Shadow
        gDPPipeSync(gfx++);
        gDPSetCombineLERP(gfx++, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE,
                          0);
        gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, textAlpha);

        rectLeft = VREG(19) + 1;
        for (i = 0; i < ARRAY_COUNT(noControllerFontIndices); i++) {
            EnMag_DrawCharTexture(&gfx, font->fontBuf + noControllerFontIndices[i] * FONT_CHAR_TEX_SIZE, rectLeft,
                                  YREG(10) + 172);
            rectLeft += VREG(21);
            if (i == 1) {
                rectLeft += VREG(23);
            }
        }

        // Actual Text
        gDPPipeSync(gfx++);
        gDPSetPrimColor(gfx++, 0, 0, 100, 255, 255, textAlpha);

        rectLeft = VREG(19);
        for (i = 0; i < ARRAY_COUNT(noControllerFontIndices); i++) {
            EnMag_DrawCharTexture(&gfx, font->fontBuf + noControllerFontIndices[i] * FONT_CHAR_TEX_SIZE, rectLeft,
                                  YREG(10) + 171);
            rectLeft += VREG(21);
            if (i == 1) {
                rectLeft += VREG(23);
            }
        }
    } else if (this->copyrightAlpha >= 200.0f) {
        // Draw "PRESS START" Text
        textAlpha = textFadeTimer * 10;
        if (textAlpha >= 255) {
            textAlpha = 255;
        }

#if !PLATFORM_IQUE
        // Text Shadow
        gDPPipeSync(gfx++);
        gDPSetCombineLERP(gfx++, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE,
                          0);
        gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, textAlpha);

        rectLeft = YREG(7) + 1;
        for (i = 0; i < ARRAY_COUNT(pressStartFontIndices); i++) {
            EnMag_DrawCharTexture(&gfx, font->fontBuf + pressStartFontIndices[i] * FONT_CHAR_TEX_SIZE, rectLeft,
                                  YREG(10) + 172);
            rectLeft += YREG(8);
            if (i == 4) {
                rectLeft += YREG(9);
            }
        }

        // Actual Text
        gDPPipeSync(gfx++);
        gDPSetPrimColor(gfx++, 0, 0, YREG(4), YREG(5), YREG(6), textAlpha);

        rectLeft = YREG(7);
        for (i = 0; i < ARRAY_COUNT(pressStartFontIndices); i++) {
            EnMag_DrawCharTexture(&gfx, font->fontBuf + pressStartFontIndices[i] * FONT_CHAR_TEX_SIZE, rectLeft,
                                  YREG(10) + 171);
            rectLeft += YREG(8);
            if (i == 4) {
                rectLeft += YREG(9);
            }
        }
#else
        // Text Shadow
        gDPPipeSync(gfx++);
        gDPSetCombineLERP(gfx++, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE,
                          0);
        gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, textAlpha);

        rectLeft = YREG(7) + 1;

        rectLeft += YREG(8) * 2;
        EnMag_DrawCharTexture(&gfx, (u8*)gTitlePressCHN, rectLeft, YREG(10) + 172);
        rectLeft += YREG(9) + YREG(8);

        for (i = 0; i < ARRAY_COUNT(pressStartFontIndices); i++) {
            EnMag_DrawCharTexture(&gfx, font->fontBuf + pressStartFontIndices[i] * FONT_CHAR_TEX_SIZE, rectLeft,
                                  YREG(10) + 172);
            rectLeft += YREG(8);
        }

        // Actual Text
        gDPPipeSync(gfx++);
        gDPSetPrimColor(gfx++, 0, 0, YREG(4), YREG(5), YREG(6), textAlpha);

        rectLeft = YREG(7);

        rectLeft += YREG(8) * 2;
        EnMag_DrawCharTexture(&gfx, (u8*)gTitlePressCHN, rectLeft, YREG(10) + 171);
        rectLeft += YREG(9) + YREG(8);

        for (i = 0; i < ARRAY_COUNT(pressStartFontIndices); i++) {
            EnMag_DrawCharTexture(&gfx, font->fontBuf + pressStartFontIndices[i] * FONT_CHAR_TEX_SIZE, rectLeft,
                                  YREG(10) + 171);
            rectLeft += YREG(8);
        }
#endif
    }

    if (textFadeDirection != 0) {
        if (--textFadeTimer == 0) {
            textFadeDirection = 0;
        }
    } else {
        if (++textFadeTimer >= 26) {
            textFadeDirection = 1;
        }
    }

    *gfxP = gfx;
}

void EnMag_Draw(Actor* thisx, PlayState* play) {
    STACK_PAD(s32);
    Gfx* gfx;
    Gfx* gfxRef;

    OPEN_DISPS(play->state.gfxCtx, "../z_en_mag.c", 1151);

    gfxRef = POLY_OPA_DISP;
    gfx = Gfx_Open(gfxRef);
    gSPDisplayList(OVERLAY_DISP++, gfx);

    EnMag_DrawInner(thisx, play, &gfx);

    gSPEndDisplayList(gfx++);
    Gfx_Close(gfxRef, gfx);
    POLY_OPA_DISP = gfx;

    CLOSE_DISPS(play->state.gfxCtx, "../z_en_mag.c", 1161);
}
