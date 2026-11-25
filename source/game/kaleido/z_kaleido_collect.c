// adapted from flute-of-age/src/overlays/misc/ovl_kaleido_scope/z_kaleido_collect.c
#include "src/overlays/misc/ovl_kaleido_scope/z_kaleido_scope.h"
#include "engine/rdp_disps.h"
#include "dma.h"

#include "array_count.h"
#include "attributes.h"
#include "controller.h"
#include "gfx.h"
#include "printf.h"
#include "regs.h"
#include "sfx.h"
#include "flute.h"
#include "play_state.h"
#include "save.h"
#include "stack_pad.h"

#include "assets/textures/parameter_static/parameter_static.h"
#include "assets/textures/icon_item_static/icon_item_static.h"

#define SONG_MAX_LENGTH 8

#if !PLATFORM_GC
#define QUEST_FLUTE_BTN_A_COLOR_R 80
#define QUEST_FLUTE_BTN_A_COLOR_G 150
#define QUEST_FLUTE_BTN_A_COLOR_B 255
#else
#define QUEST_FLUTE_BTN_A_COLOR_R 80
#define QUEST_FLUTE_BTN_A_COLOR_G 255
#define QUEST_FLUTE_BTN_A_COLOR_B 150
#endif

void KaleidoScope_DrawQuestStatus(PlayState* play, GraphicsContext* gfxCtx) {
    static s16 sHpPrimColors[][4] = {
        { 255, 0, 0, 255 },
        { 255, 70, 0, 150 },
        { 255, 70, 0, 150 },
        { 255, 0, 0, 255 },
    };

    // Shining medallions
    static s16 sMedallionsEnvColors[6 + 6][3] = {
        // Target env color when sMedallionsEnvShineState == 0
        { 0, 0, 0 }, // QUEST_MEDALLION_FOREST
        { 0, 0, 0 }, // QUEST_MEDALLION_FIRE
        { 0, 0, 0 }, // QUEST_MEDALLION_WATER
        { 0, 0, 0 }, // QUEST_MEDALLION_SPIRIT
        { 0, 0, 0 }, // QUEST_MEDALLION_SHADOW
        { 0, 0, 0 }, // QUEST_MEDALLION_LIGHT

        // Target env color when sMedallionsEnvShineState == 2
        { 0, 60, 0 },   // QUEST_MEDALLION_FOREST
        { 90, 0, 0 },   // QUEST_MEDALLION_FIRE
        { 0, 40, 110 }, // QUEST_MEDALLION_WATER
        { 80, 40, 0 },  // QUEST_MEDALLION_SPIRIT
        { 70, 0, 90 },  // QUEST_MEDALLION_SHADOW
        { 90, 90, 0 },  // QUEST_MEDALLION_LIGHT
    };
    // Current (animated) env color for each medallion
    static s16 sMedallionsEnvRed[6] = { 255, 255, 255, 255, 255, 255 };
    static s16 sMedallionsEnvGreen[6] = { 255, 255, 255, 255, 255, 255 };
    static s16 sMedallionsEnvBlue[6] = { 150, 150, 150, 150, 150, 150 };
    static s16 sMedallionsEnvTimer = 20;
    static s16 sMedallionsEnvShineState = 0;

    static s16 sHpPrimRed = 0;
    static s16 sHpPrimGreen = 0;
    static s16 sHpPrimBlue = 0;
    static s16 sHpPrimAlpha = 0;
    static s16 sHpPrimTimer = 20;
    static s16 sHpPrimState = 0;

    static s16 sPlayedSongBtnsNum = 0;
    static s16 sPlaybackSongStartDelayTimer = 0;
    static u8 sPlayedSongBtns[SONG_MAX_LENGTH + 1] = { 0 };
    static void* sFluteBtnTextures[] = {
        gFluteBtnIconATex,      // FLUTE_BTN_A
        gFluteBtnIconCDownTex,  // FLUTE_BTN_C_DOWN
        gFluteBtnIconCRightTex, // FLUTE_BTN_C_RIGHT
        gFluteBtnIconCLeftTex,  // FLUTE_BTN_C_LEFT
        gFluteBtnIconCUpTex,    // FLUTE_BTN_C_UP
    };
    UNUSED static u16 D_8082A144[] = {
        0xFFCC, 0xFFCC, 0xFFCC, 0xFFCC, 0xFFCC,
    };
    static s16 sPlayedSongBtnsAlpha[SONG_MAX_LENGTH] = { 0 };
    UNUSED static s32 sUnused1 = 0;

    static s16 sSongsPrimRed[] = {
        150, // QUEST_SONG_MINUET
        255, // QUEST_SONG_BOLERO
        100, // QUEST_SONG_SERENADE
        255, // QUEST_SONG_REQUIEM
        255, // QUEST_SONG_NOCTURNE
        255, // QUEST_SONG_PRELUDE
        255, // QUEST_SONG_LULLABY
        255, // QUEST_SONG_ASTER
        255, // QUEST_SONG_SYLVA
        255, // QUEST_SONG_SUN
        255, // QUEST_SONG_TIME
        255, // QUEST_SONG_STORMS
    };
    static s16 sSongsPrimGreen[] = {
        255, // QUEST_SONG_MINUET
        80,  // QUEST_SONG_BOLERO
        150, // QUEST_SONG_SERENADE
        160, // QUEST_SONG_REQUIEM
        100, // QUEST_SONG_NOCTURNE
        240, // QUEST_SONG_PRELUDE
        255, // QUEST_SONG_LULLABY
        255, // QUEST_SONG_ASTER
        255, // QUEST_SONG_SYLVA
        255, // QUEST_SONG_SUN
        255, // QUEST_SONG_TIME
        255, // QUEST_SONG_STORMS
    };
    static s16 sSongsPrimBlue[] = {
        100, // QUEST_SONG_MINUET
        40,  // QUEST_SONG_BOLERO
        255, // QUEST_SONG_SERENADE
        0,   // QUEST_SONG_REQUIEM
        255, // QUEST_SONG_NOCTURNE
        100, // QUEST_SONG_PRELUDE
        255, // QUEST_SONG_LULLABY
        255, // QUEST_SONG_ASTER
        255, // QUEST_SONG_SYLVA
        255, // QUEST_SONG_SUN
        255, // QUEST_SONG_TIME
        255, // QUEST_SONG_STORMS
    };

    enum {
        /* -3 */ CURSOR_TO_LEFT = -3, // Cursor on the "scroll to left page" position
        /* -2 */ CURSOR_TO_RIGHT,     // Cursor on the "scroll to right page" position
        /* -1 */ CURSOR_NONE          // No position in that direction, cursor stays where it is
    };
    // Each {up, down, left, right} entry defines where the cursor can move next
    static s8 sCursorPointArens[][4] = {
        /* QUEST_MEDALLION_FOREST */
        { QUEST_MEDALLION_LIGHT, QUEST_MEDALLION_FIRE, QUEST_MEDALLION_LIGHT, CURSOR_TO_RIGHT },
        /* QUEST_MEDALLION_FIRE */
        { QUEST_MEDALLION_FOREST, QUEST_MEDALLION_WATER, QUEST_MEDALLION_WATER, CURSOR_TO_RIGHT },
        /* QUEST_MEDALLION_WATER */ { CURSOR_NONE, QUEST_CAIRN_RUBY, QUEST_MEDALLION_SPIRIT, QUEST_MEDALLION_FIRE },
        /* QUEST_MEDALLION_SPIRIT */
        { QUEST_MEDALLION_SHADOW, QUEST_MEDALLION_WATER, QUEST_SONG_STORMS, QUEST_MEDALLION_WATER },
        /* QUEST_MEDALLION_SHADOW */
        { QUEST_MEDALLION_LIGHT, QUEST_MEDALLION_SPIRIT, QUEST_HEART_PIECE, QUEST_MEDALLION_LIGHT },
        /* QUEST_MEDALLION_LIGHT */ { CURSOR_NONE, CURSOR_NONE, QUEST_MEDALLION_SHADOW, QUEST_MEDALLION_FOREST },
        /* QUEST_SONG_MINUET */ { QUEST_SONG_LULLABY, CURSOR_NONE, CURSOR_TO_LEFT, QUEST_SONG_BOLERO },
        /* QUEST_SONG_BOLERO */ { QUEST_SONG_ASTER, CURSOR_NONE, QUEST_SONG_MINUET, QUEST_SONG_SERENADE },
        /* QUEST_SONG_SERENADE */ { QUEST_SONG_SYLVA, CURSOR_NONE, QUEST_SONG_BOLERO, QUEST_SONG_REQUIEM },
        /* QUEST_SONG_REQUIEM */ { QUEST_SONG_SUN, CURSOR_NONE, QUEST_SONG_SERENADE, QUEST_SONG_NOCTURNE },
        /* QUEST_SONG_NOCTURNE */ { QUEST_SONG_TIME, CURSOR_NONE, QUEST_SONG_REQUIEM, QUEST_SONG_PRELUDE },
        /* QUEST_SONG_PRELUDE */ { QUEST_SONG_STORMS, CURSOR_NONE, QUEST_SONG_NOCTURNE, QUEST_SYLVAN_EMERALD },
        /* QUEST_SONG_LULLABY */ { QUEST_SKULL_TOKEN, QUEST_SONG_MINUET, CURSOR_TO_LEFT, QUEST_SONG_ASTER },
        /* QUEST_SONG_ASTER */ { QUEST_SKULL_TOKEN, QUEST_SONG_BOLERO, QUEST_SONG_LULLABY, QUEST_SONG_SYLVA },
        /* QUEST_SONG_SYLVA */ { QUEST_SKULL_TOKEN, QUEST_SONG_SERENADE, QUEST_SONG_ASTER, QUEST_SONG_SUN },
        /* QUEST_SONG_SUN */ { QUEST_HEART_PIECE, QUEST_SONG_REQUIEM, QUEST_SONG_SYLVA, QUEST_SONG_TIME },
        /* QUEST_SONG_TIME */ { QUEST_HEART_PIECE, QUEST_SONG_NOCTURNE, QUEST_SONG_SUN, QUEST_SONG_STORMS },
        /* QUEST_SONG_STORMS */ { QUEST_HEART_PIECE, QUEST_SONG_PRELUDE, QUEST_SONG_TIME, QUEST_MEDALLION_SPIRIT },
        /* QUEST_SYLVAN_EMERALD */ { QUEST_MEDALLION_WATER, CURSOR_NONE, QUEST_SONG_PRELUDE, QUEST_CAIRN_RUBY },
        /* QUEST_CAIRN_RUBY */ { QUEST_MEDALLION_WATER, CURSOR_NONE, QUEST_SYLVAN_EMERALD, QUEST_NERI_SAPPHIRE },
        /* QUEST_NERI_SAPPHIRE */ { QUEST_MEDALLION_WATER, CURSOR_NONE, QUEST_CAIRN_RUBY, CURSOR_TO_RIGHT },
        /* QUEST_STONE_OF_ECHOES */ { CURSOR_NONE, QUEST_SKULL_TOKEN, CURSOR_TO_LEFT, QUEST_DUNEANS_CARD },
        /* QUEST_DUNEANS_CARD */ { CURSOR_NONE, QUEST_SKULL_TOKEN, QUEST_STONE_OF_ECHOES, QUEST_HEART_PIECE },
        /* QUEST_SKULL_TOKEN */ { QUEST_STONE_OF_ECHOES, QUEST_SONG_LULLABY, CURSOR_TO_LEFT, QUEST_HEART_PIECE },
        /* QUEST_HEART_PIECE */ { CURSOR_NONE, QUEST_SONG_TIME, QUEST_DUNEANS_CARD, QUEST_MEDALLION_SHADOW },
    };
    UNUSED static s32 sUnused2 = 0;

    PauseContext* pauseCtx = &play->pauseCtx;
    Input* input = &play->state.input[0];
    s16 stepRed;
    s16 stepGreen;
    s16 stepBlue;
    s16 stepAlpha;
    s16 nextCursorPoint;
    s16 prevCursorPoint;
    s16 bufI;
    s16 j;
    s16 cursor;
    s16 i;
    s16 targetColorIndex;
    STACK_PAD(s16);
    s16 cursorItem;
    s16 gsTokenDigits[3];

    OPEN_DISPS(gfxCtx, "../z_kaleido_collect.c", 248);

    if ((((u32)pauseCtx->mainState == PAUSE_MAIN_STATE_IDLE) || (pauseCtx->mainState == PAUSE_MAIN_STATE_SONG_PROMPT) ||
         (pauseCtx->mainState == PAUSE_MAIN_STATE_IDLE_CURSOR_ON_SONG)) &&
        (pauseCtx->pageIndex == PAUSE_QUEST)) {

        pauseCtx->cursorColorSet = 0;

        if (pauseCtx->cursorSpecialPos == 0) {
            pauseCtx->nameColorSet = 0;

            if ((pauseCtx->state != PAUSE_STATE_MAIN) || ((pauseCtx->stickAdjX == 0) && (pauseCtx->stickAdjY == 0))) {
                // No cursor movement
                cursor = pauseCtx->cursorSlot[PAUSE_QUEST];
            } else {
                // Move cursor based on stick input

                prevCursorPoint = pauseCtx->cursorPoint[PAUSE_QUEST];

                if (pauseCtx->stickAdjX < -30) {
                    // Move cursor left
                    nextCursorPoint = sCursorPointArens[prevCursorPoint][2];
                    if (nextCursorPoint == CURSOR_TO_LEFT) {
                        KaleidoScope_MoveCursorToSpecialPos(play, PAUSE_CURSOR_PAGE_LEFT);
                        pauseCtx->mainState = PAUSE_MAIN_STATE_IDLE;
                    } else {
                        while (nextCursorPoint >= 0) {
                            if ((s16)KaleidoScope_UpdateQuestStatusPoint(pauseCtx, nextCursorPoint)) {
                                break;
                            }
                            nextCursorPoint = sCursorPointArens[nextCursorPoint][2];
                        }
                    }
                } else if (pauseCtx->stickAdjX > 30) {
                    // Move cursor right
                    nextCursorPoint = sCursorPointArens[prevCursorPoint][3];
                    if (nextCursorPoint == CURSOR_TO_RIGHT) {
                        KaleidoScope_MoveCursorToSpecialPos(play, PAUSE_CURSOR_PAGE_RIGHT);
                        pauseCtx->mainState = PAUSE_MAIN_STATE_IDLE;
                    } else {
                        while (nextCursorPoint >= 0) {
                            if ((s16)KaleidoScope_UpdateQuestStatusPoint(pauseCtx, nextCursorPoint)) {
                                break;
                            }
                            nextCursorPoint = sCursorPointArens[nextCursorPoint][3];
                        }
                    }
                }

                if (pauseCtx->stickAdjY < -30) {
                    // Move cursor down
                    nextCursorPoint = sCursorPointArens[prevCursorPoint][1];
                    while (nextCursorPoint >= 0) {
                        if ((s16)KaleidoScope_UpdateQuestStatusPoint(pauseCtx, nextCursorPoint)) {
                            break;
                        }
                        nextCursorPoint = sCursorPointArens[nextCursorPoint][1];
                    }
                } else if (pauseCtx->stickAdjY > 30) {
                    // Move cursor up
                    nextCursorPoint = sCursorPointArens[prevCursorPoint][0];
                    while (nextCursorPoint >= 0) {
                        if ((s16)KaleidoScope_UpdateQuestStatusPoint(pauseCtx, nextCursorPoint)) {
                            break;
                        }
                        nextCursorPoint = sCursorPointArens[nextCursorPoint][0];
                    }
                }

                // if the cursor point changed
                if (pauseCtx->cursorPoint[PAUSE_QUEST] != prevCursorPoint) {
                    pauseCtx->mainState = PAUSE_MAIN_STATE_IDLE;
                    SFX_PLAY_CENTERED(NA_SE_SY_CURSOR);
                }

                // Update cursor item and slot

                if (pauseCtx->cursorPoint[PAUSE_QUEST] != QUEST_HEART_PIECE) {
                    if (CHECK_QUEST_ITEM(pauseCtx->cursorPoint[PAUSE_QUEST])) {
                        if (pauseCtx->cursorPoint[PAUSE_QUEST] < QUEST_SONG_MINUET) {
                            cursorItem =
                                ITEM_MEDALLION_FOREST - QUEST_MEDALLION_FOREST + pauseCtx->cursorPoint[PAUSE_QUEST];
                            PRINTF("000 ccc=%d\n", cursorItem);
                        } else if (pauseCtx->cursorPoint[PAUSE_QUEST] < QUEST_SYLVAN_EMERALD) {
                            cursorItem = ITEM_SONG_MINUET - QUEST_SONG_MINUET + pauseCtx->cursorPoint[PAUSE_QUEST];
                            PRINTF("111 ccc=%d\n", cursorItem);
                        } else {
                            cursorItem =
                                ITEM_SYLVAN_EMERALD - QUEST_SYLVAN_EMERALD + pauseCtx->cursorPoint[PAUSE_QUEST];
                            PRINTF("222 ccc=%d (%d, %d, %d)\n", cursorItem, pauseCtx->cursorPoint[PAUSE_QUEST],
                                   QUEST_SYLVAN_EMERALD, ITEM_SYLVAN_EMERALD);
                        }
                    } else {
                        cursorItem = PAUSE_ITEM_NONE;
                        PRINTF("999 ccc=%d (%d,  %d)\n", PAUSE_ITEM_NONE, pauseCtx->cursorPoint[PAUSE_QUEST], 0x18);
                    }
                } else {
                    if ((gSaveContext.save.info.inventory.questItems & 0xF0000000) != 0) {
                        cursorItem = ITEM_HEART_CONTAINER;
                    } else {
                        cursorItem = PAUSE_ITEM_NONE;
                    }
                    PRINTF("888 ccc=%d (%d,  %d,  %x)\n", cursorItem, pauseCtx->cursorPoint[PAUSE_QUEST],
                           ITEM_HEART_CONTAINER, gSaveContext.save.info.inventory.questItems & 0xF0000000);
                }

                cursor = pauseCtx->cursorPoint[PAUSE_QUEST];
                pauseCtx->cursorItem[pauseCtx->pageIndex] = cursorItem;
                pauseCtx->cursorSlot[pauseCtx->pageIndex] = cursor;
            }

            KaleidoScope_SetCursorPos(pauseCtx, cursor * 4, pauseCtx->questVtx);

            if ((pauseCtx->state == PAUSE_STATE_MAIN) && (pauseCtx->mainState == PAUSE_MAIN_STATE_IDLE) &&
                (pauseCtx->cursorSpecialPos == 0)) {
                if ((cursor >= QUEST_SONG_MINUET) && (cursor < QUEST_SYLVAN_EMERALD)) {
                    if (CHECK_QUEST_ITEM(pauseCtx->cursorPoint[PAUSE_QUEST])) {
                        cursor = pauseCtx->cursorSlot[PAUSE_QUEST];
                        pauseCtx->fluteSongIdx = gFluteSongItemMap[cursor - QUEST_SONG_MINUET];
                        sPlaybackSongStartDelayTimer = 10;

                        for (i = 0; i < SONG_MAX_LENGTH; i++) {
                            sPlayedSongBtns[i] = FLUTE_BTN_INVALID;
                            sPlayedSongBtnsAlpha[i] = 0;
                        }
                        sPlayedSongBtnsNum = 0;

                        AudioFlute_SetInstrument(FLUTE_INSTRUMENT_DEFAULT);
                        AudioFlute_Start((1 << pauseCtx->fluteSongIdx) + 0x8000);
                        pauseCtx->fluteStaff = AudioFlute_GetPlaybackStaff();
                        pauseCtx->fluteStaff->pos = 0;
                        pauseCtx->fluteStaff->state = 0xFF;

                        R_PAUSE_SONG_OCA_BTN_Y(FLUTE_BTN_A) = -62;
                        R_PAUSE_SONG_OCA_BTN_Y(FLUTE_BTN_C_DOWN) = -56;
                        R_PAUSE_SONG_OCA_BTN_Y(FLUTE_BTN_C_RIGHT) = -49;
                        R_PAUSE_SONG_OCA_BTN_Y(FLUTE_BTN_C_LEFT) = -46;
                        R_PAUSE_SONG_OCA_BTN_Y(FLUTE_BTN_C_UP) = -41;

                        pauseCtx->mainState = PAUSE_MAIN_STATE_IDLE_CURSOR_ON_SONG;

                        AudioFlute_SetInstrument(FLUTE_INSTRUMENT_OFF);
                    }
                }
            } else if (pauseCtx->mainState == PAUSE_MAIN_STATE_SONG_PROMPT) {
                // Abort having the player play the song if the stick is moved
                if ((pauseCtx->stickAdjX != 0) || (pauseCtx->stickAdjY != 0)) {
                    pauseCtx->mainState = PAUSE_MAIN_STATE_IDLE;

                    AudioFlute_SetInstrument(FLUTE_INSTRUMENT_OFF);
                }
            } else if (pauseCtx->mainState == PAUSE_MAIN_STATE_IDLE_CURSOR_ON_SONG) {
                if (CHECK_BTN_ALL(input->press.button, BTN_A) && (cursor >= QUEST_SONG_MINUET) &&
                    (cursor < QUEST_SYLVAN_EMERALD)) {

                    pauseCtx->mainState = PAUSE_MAIN_STATE_SONG_PLAYBACK_START;

                    sPlaybackSongStartDelayTimer = 10;
                }
            }
        } else if (pauseCtx->cursorSpecialPos == PAUSE_CURSOR_PAGE_LEFT) {
            if (pauseCtx->stickAdjX > 30) {
                // Move cursor right from the "scroll to left page" position

                pauseCtx->cursorPoint[PAUSE_QUEST] = QUEST_STONE_OF_ECHOES;
                pauseCtx->nameDisplayTimer = 0;
                pauseCtx->cursorSpecialPos = 0;

                cursor = pauseCtx->cursorPoint[PAUSE_QUEST];
                KaleidoScope_SetCursorPos(pauseCtx, cursor * 4, pauseCtx->questVtx);
                SFX_PLAY_CENTERED(NA_SE_SY_CURSOR);

                if (CHECK_QUEST_ITEM(pauseCtx->cursorPoint[PAUSE_QUEST])) {
                    cursorItem = ITEM_STONE_OF_ECHOES - QUEST_STONE_OF_ECHOES + pauseCtx->cursorPoint[PAUSE_QUEST];
                } else {
                    cursorItem = PAUSE_ITEM_NONE;
                }
                cursor = pauseCtx->cursorPoint[PAUSE_QUEST];
                pauseCtx->cursorItem[pauseCtx->pageIndex] = cursorItem;
                pauseCtx->cursorSlot[pauseCtx->pageIndex] = cursor;
            }
        } else { // cursorSpecialPos == PAUSE_CURSOR_PAGE_RIGHT
            if (pauseCtx->stickAdjX < -30) {
                // Move cursor left from the "scroll to right page" position

                pauseCtx->cursorPoint[PAUSE_QUEST] = QUEST_MEDALLION_FOREST;
                pauseCtx->nameDisplayTimer = 0;
                pauseCtx->cursorSpecialPos = 0;

                cursor = pauseCtx->cursorPoint[PAUSE_QUEST];
                KaleidoScope_SetCursorPos(pauseCtx, cursor * 4, pauseCtx->questVtx);
                SFX_PLAY_CENTERED(NA_SE_SY_CURSOR);

                if (CHECK_QUEST_ITEM(pauseCtx->cursorPoint[PAUSE_QUEST])) {
                    // Only the first if may be reached, as QUEST_MEDALLION_FOREST is set above
                    if (pauseCtx->cursorPoint[PAUSE_QUEST] < QUEST_SONG_MINUET) {
                        cursorItem =
                            ITEM_MEDALLION_FOREST - QUEST_MEDALLION_FOREST + pauseCtx->cursorPoint[PAUSE_QUEST];
                    } else if (pauseCtx->cursorPoint[PAUSE_QUEST] < QUEST_SONG_LULLABY) {
                        // this would result in the wrong item
                        cursorItem = ITEM_SCALE_GOLDEN - QUEST_SONG_MINUET + pauseCtx->cursorPoint[PAUSE_QUEST];
                    } else {
                        // this would result in the wrong item
                        cursorItem = ITEM_DUNGEON_COMPASS - QUEST_SONG_LULLABY + pauseCtx->cursorPoint[PAUSE_QUEST];
                    }
                } else {
                    cursorItem = PAUSE_ITEM_NONE;
                }
                cursor = pauseCtx->cursorPoint[PAUSE_QUEST];
                pauseCtx->cursorItem[pauseCtx->pageIndex] = cursorItem;
                pauseCtx->cursorSlot[pauseCtx->pageIndex] = cursor;
            }
        }

    } else if (pauseCtx->mainState == PAUSE_MAIN_STATE_SONG_PLAYBACK_START) {
        // After a short delay, start playing the selected song back to the player

        pauseCtx->cursorColorSet = 8;

        if (--sPlaybackSongStartDelayTimer == 0) {
            for (i = 0; i < SONG_MAX_LENGTH; i++) {
                sPlayedSongBtns[i] = FLUTE_BTN_INVALID;
                sPlayedSongBtnsAlpha[i] = 0;
            }
            sPlayedSongBtnsNum = 0;

            R_PAUSE_SONG_OCA_BTN_Y(FLUTE_BTN_A) = -62;
            R_PAUSE_SONG_OCA_BTN_Y(FLUTE_BTN_C_DOWN) = -56;
            R_PAUSE_SONG_OCA_BTN_Y(FLUTE_BTN_C_RIGHT) = -49;
            R_PAUSE_SONG_OCA_BTN_Y(FLUTE_BTN_C_LEFT) = -46;
            R_PAUSE_SONG_OCA_BTN_Y(FLUTE_BTN_C_UP) = -41;

            cursor = pauseCtx->cursorSlot[PAUSE_QUEST];
            AudioFlute_SetInstrument(FLUTE_INSTRUMENT_DEFAULT);
            AudioFlute_SetInstrument(FLUTE_INSTRUMENT_DEFAULT);
            pauseCtx->fluteSongIdx = gFluteSongItemMap[cursor - QUEST_SONG_MINUET];
            AudioFlute_SetPlaybackSong(pauseCtx->fluteSongIdx + 1, 1);

            pauseCtx->mainState = PAUSE_MAIN_STATE_SONG_PLAYBACK;

            pauseCtx->fluteStaff = AudioFlute_GetPlaybackStaff();
            pauseCtx->fluteStaff->pos = 0;

            cursor = pauseCtx->cursorSlot[PAUSE_QUEST];
            KaleidoScope_SetCursorPos(pauseCtx, cursor * 4, pauseCtx->questVtx);
        }
    } else {
        cursor = pauseCtx->cursorSlot[PAUSE_QUEST];
        KaleidoScope_SetCursorPos(pauseCtx, cursor * 4, pauseCtx->questVtx);
    }

    // Draw medallions
    // QUEST_MEDALLION_FOREST to QUEST_MEDALLION_LIGHT

    gDPPipeSync(POLY_OPA_DISP++);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, pauseCtx->alpha);
    gDPSetCombineLERP(POLY_OPA_DISP++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0, PRIMITIVE,
                      ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0);

    sMedallionsEnvTimer--;

    for (j = 0, bufI = 0; j < QUEST_SONG_MINUET - QUEST_MEDALLION_FOREST; j++, bufI += 4) {
        if ((sMedallionsEnvShineState != 1) && (sMedallionsEnvShineState != 3)) {
            targetColorIndex = (sMedallionsEnvShineState != 0) ? j + 6 : j;

            if (sMedallionsEnvTimer != 0) {
                stepRed = ABS(sMedallionsEnvRed[j] - sMedallionsEnvColors[targetColorIndex][0]) / sMedallionsEnvTimer;
                stepGreen =
                    ABS(sMedallionsEnvGreen[j] - sMedallionsEnvColors[targetColorIndex][1]) / sMedallionsEnvTimer;
                stepBlue = ABS(sMedallionsEnvBlue[j] - sMedallionsEnvColors[targetColorIndex][2]) / sMedallionsEnvTimer;
                if (sMedallionsEnvRed[j] >= sMedallionsEnvColors[targetColorIndex][0]) {
                    sMedallionsEnvRed[j] -= stepRed;
                } else {
                    sMedallionsEnvRed[j] += stepRed;
                }
                if (sMedallionsEnvGreen[j] >= sMedallionsEnvColors[targetColorIndex][1]) {
                    sMedallionsEnvGreen[j] -= stepGreen;
                } else {
                    sMedallionsEnvGreen[j] += stepGreen;
                }
                if (sMedallionsEnvBlue[j] >= sMedallionsEnvColors[targetColorIndex][2]) {
                    sMedallionsEnvBlue[j] -= stepBlue;
                } else {
                    sMedallionsEnvBlue[j] += stepBlue;
                }
            } else {
                sMedallionsEnvRed[j] = sMedallionsEnvColors[targetColorIndex][0];
                sMedallionsEnvGreen[j] = sMedallionsEnvColors[targetColorIndex][1];
                sMedallionsEnvBlue[j] = sMedallionsEnvColors[targetColorIndex][2];
            }
        }

        if (CHECK_QUEST_ITEM(QUEST_MEDALLION_FOREST + j)) {
            gDPPipeSync(POLY_OPA_DISP++);
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, pauseCtx->alpha);
            gDPSetEnvColor(POLY_OPA_DISP++, sMedallionsEnvRed[j], sMedallionsEnvGreen[j], sMedallionsEnvBlue[j], 0);
            gSPVertex(POLY_OPA_DISP++, &pauseCtx->questVtx[bufI], 4, 0);

            KaleidoScope_DrawQuadTextureRGBA32(gfxCtx, gItemIcons[ITEM_MEDALLION_FOREST + j], QUEST_ICON_WIDTH,
                                               QUEST_ICON_HEIGHT, 0);
        }
    }

    if (sMedallionsEnvTimer == 0) {
        sMedallionsEnvTimer = R_PAUSE_QUEST_MEDALLION_SHINE_TIME(sMedallionsEnvShineState);
        if (++sMedallionsEnvShineState >= 4) {
            sMedallionsEnvShineState = 0;
        }
    }

    // Draw songs
    // QUEST_SONG_MINUET to QUEST_SONG_STORMS

    gDPPipeSync(POLY_OPA_DISP++);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, pauseCtx->alpha);
    gDPSetEnvColor(POLY_OPA_DISP++, 0, 0, 0, 255);

    gDPLoadTextureBlock(POLY_OPA_DISP++, gSongNoteTex, G_IM_FMT_IA, G_IM_SIZ_8b, gSongNoteTex_WIDTH,
                        gSongNoteTex_HEIGHT, 0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK,
                        G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

    for (j = 0; j < QUEST_SYLVAN_EMERALD - QUEST_SONG_MINUET; j++, bufI += 4) {
        if (CHECK_QUEST_ITEM(QUEST_SONG_MINUET + j)) {
            if ((QUEST_SONG_MINUET + j) == cursor) {
                pauseCtx->questVtx[bufI + 0].v.ob[0] = pauseCtx->questVtx[bufI + 2].v.ob[0] =
                    pauseCtx->questVtx[bufI + 0].v.ob[0] - 2;

                pauseCtx->questVtx[bufI + 1].v.ob[0] = pauseCtx->questVtx[bufI + 3].v.ob[0] =
                    pauseCtx->questVtx[bufI + 1].v.ob[0] + 4;

                pauseCtx->questVtx[bufI + 0].v.ob[1] = pauseCtx->questVtx[bufI + 1].v.ob[1] =
                    pauseCtx->questVtx[bufI + 0].v.ob[1] + 2;

                pauseCtx->questVtx[bufI + 2].v.ob[1] = pauseCtx->questVtx[bufI + 3].v.ob[1] =
                    pauseCtx->questVtx[bufI + 2].v.ob[1] - 4;
            }

            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, sSongsPrimRed[j], sSongsPrimGreen[j], sSongsPrimBlue[j],
                            pauseCtx->alpha);
            gSPVertex(POLY_OPA_DISP++, &pauseCtx->questVtx[bufI], 4, 0);
            gSP1Quadrangle(POLY_OPA_DISP++, 0, 2, 3, 1, 0);
        }
    }

    // Draw primal stones
    // QUEST_SYLVAN_EMERALD, QUEST_CAIRN_RUBY, QUEST_NERI_SAPPHIRE

    gDPPipeSync(POLY_OPA_DISP++);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, pauseCtx->alpha);
    gDPSetEnvColor(POLY_OPA_DISP++, 0, 0, 0, 255);

    for (j = 0; j < QUEST_STONE_OF_ECHOES - QUEST_SYLVAN_EMERALD; j++, bufI += 4) {
        if (CHECK_QUEST_ITEM(QUEST_SYLVAN_EMERALD + j)) {
            gSPVertex(POLY_OPA_DISP++, &pauseCtx->questVtx[bufI], 4, 0);
            KaleidoScope_DrawQuadTextureRGBA32(gfxCtx, gItemIcons[ITEM_SYLVAN_EMERALD + j], QUEST_ICON_WIDTH,
                                               QUEST_ICON_HEIGHT, 0);
        }
    }

    // Draw QUEST_STONE_OF_ECHOES, QUEST_DUNEANS_CARD, QUEST_SKULL_TOKEN

    gDPPipeSync(POLY_OPA_DISP++);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, pauseCtx->alpha);

    for (j = 0; j < QUEST_HEART_PIECE - QUEST_STONE_OF_ECHOES; j++, bufI += 4) {
        if (CHECK_QUEST_ITEM(QUEST_STONE_OF_ECHOES + j)) {
            gSPVertex(POLY_OPA_DISP++, &pauseCtx->questVtx[bufI], 4, 0);
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, pauseCtx->alpha);
            KaleidoScope_DrawQuadTextureRGBA32(gfxCtx, gItemIcons[ITEM_STONE_OF_ECHOES + j], QUEST_ICON_WIDTH,
                                               QUEST_ICON_HEIGHT, 0);
        }
    }

    // Draw heart pieces
    // QUEST_HEART_PIECE

    stepRed = ABS(sHpPrimRed - sHpPrimColors[sHpPrimState][0]) / sHpPrimTimer;
    stepGreen = ABS(sHpPrimGreen - sHpPrimColors[sHpPrimState][1]) / sHpPrimTimer;
    stepBlue = ABS(sHpPrimBlue - sHpPrimColors[sHpPrimState][2]) / sHpPrimTimer;
    stepAlpha = ABS(sHpPrimAlpha - sHpPrimColors[sHpPrimState][3]) / sHpPrimTimer;
    if (sHpPrimRed >= sHpPrimColors[sHpPrimState][0]) {
        sHpPrimRed -= stepRed;
    } else {
        sHpPrimRed += stepRed;
    }
    if (sHpPrimGreen >= sHpPrimColors[sHpPrimState][1]) {
        sHpPrimGreen -= stepGreen;
    } else {
        sHpPrimGreen += stepGreen;
    }
    if (sHpPrimBlue >= sHpPrimColors[sHpPrimState][2]) {
        sHpPrimBlue -= stepBlue;
    } else {
        sHpPrimBlue += stepBlue;
    }
    if (sHpPrimAlpha >= sHpPrimColors[sHpPrimState][3]) {
        sHpPrimAlpha -= stepAlpha;
    } else {
        sHpPrimAlpha += stepAlpha;
    }

    if (--sHpPrimTimer == 0) {
        sHpPrimRed = sHpPrimColors[sHpPrimState][0];
        sHpPrimGreen = sHpPrimColors[sHpPrimState][1];
        sHpPrimBlue = sHpPrimColors[sHpPrimState][2];
        sHpPrimAlpha = sHpPrimColors[sHpPrimState][3];
        sHpPrimTimer = ZREG(24 + sHpPrimState);
        if (++sHpPrimState >= ARRAY_COUNT(sHpPrimColors)) {
            sHpPrimState = 0;
        }
    }

    if ((gSaveContext.save.info.inventory.questItems >> QUEST_HEART_PIECE_COUNT) != 0) {
        gDPPipeSync(POLY_OPA_DISP++);
        gDPSetCombineLERP(POLY_OPA_DISP++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0,
                          PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0);

        if ((pauseCtx->state == PAUSE_STATE_OPENING_1) || (pauseCtx->state == PAUSE_STATE_CLOSING)) {
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, sHpPrimColors[0][0], sHpPrimColors[0][1], sHpPrimColors[0][2],
                            pauseCtx->alpha);
        } else {
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, sHpPrimRed, sHpPrimGreen, sHpPrimBlue, sHpPrimAlpha);
        }

        gDPSetEnvColor(POLY_OPA_DISP++, 0, 0, 0, 255);
        gSPVertex(POLY_OPA_DISP++, &pauseCtx->questVtx[bufI], 4, 0);

        POLY_OPA_DISP = KaleidoScope_QuadTextureIA8(
            POLY_OPA_DISP,
            gItemIcons[ITEM_HEART_PIECE_2 - 1 +
                       (((gSaveContext.save.info.inventory.questItems & 0xF0000000) & 0xF0000000) >>
                        QUEST_HEART_PIECE_COUNT)],
            48, 48, 0);
    }

    //

    if (pauseCtx->state == PAUSE_STATE_MAIN) {
        bufI += (QUEST_QUAD_SONG_NOTE_A1 - QUEST_HEART_PIECE) * 4;

        gDPPipeSync(POLY_OPA_DISP++);
        gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);

        // Update cursor color
        if ((pauseCtx->cursorSpecialPos == 0) && (cursor >= QUEST_SONG_MINUET) && (cursor < QUEST_SYLVAN_EMERALD)) {
            if ((pauseCtx->mainState < PAUSE_MAIN_STATE_3) || (pauseCtx->mainState == PAUSE_MAIN_STATE_SONG_PROMPT) ||
                (pauseCtx->mainState == PAUSE_MAIN_STATE_IDLE_CURSOR_ON_SONG)) {
                if (pauseCtx->cursorItem[pauseCtx->pageIndex] != PAUSE_ITEM_NONE) {
                    pauseCtx->cursorColorSet = 8;
                    if ((pauseCtx->mainState >= PAUSE_MAIN_STATE_SONG_PLAYBACK) &&
                        (pauseCtx->mainState <= PAUSE_MAIN_STATE_SONG_PROMPT_DONE)) {
                        pauseCtx->cursorColorSet = 0;
                    }
                }
            }
        }

        if (pauseCtx->mainState == PAUSE_MAIN_STATE_SONG_PLAYBACK) {
            // Draw flute buttons as the song playback progresses
            // QUEST_QUAD_SONG_NOTE_A1 to QUEST_QUAD_SONG_NOTE_A8

            pauseCtx->fluteStaff = AudioFlute_GetPlaybackStaff();

            if (pauseCtx->fluteStaff->pos != 0) {
                if (sPlayedSongBtnsNum == (pauseCtx->fluteStaff->pos - 1)) {
                    sPlayedSongBtnsNum++;
                    sPlayedSongBtns[pauseCtx->fluteStaff->pos - 1] = pauseCtx->fluteStaff->buttonIndex;
                }

                for (j = 0, i = 0; j < SONG_MAX_LENGTH; j++, i += 4, bufI += 4) {
                    if (sPlayedSongBtns[j] == FLUTE_BTN_INVALID) {
                        break;
                    }

                    if (sPlayedSongBtnsAlpha[j] != 255) {
                        sPlayedSongBtnsAlpha[j] += R_FLUTE_BUTTONS_APPEAR_ALPHA_STEP;
                        if (sPlayedSongBtnsAlpha[j] >= 255) {
                            sPlayedSongBtnsAlpha[j] = 255;
                        }
                    }

                    pauseCtx->questVtx[bufI + 0].v.ob[1] = pauseCtx->questVtx[bufI + 1].v.ob[1] =
                        R_PAUSE_SONG_OCA_BTN_Y(sPlayedSongBtns[j]);

                    pauseCtx->questVtx[bufI + 2].v.ob[1] = pauseCtx->questVtx[bufI + 3].v.ob[1] =
                        pauseCtx->questVtx[bufI + 0].v.ob[1] - 12;

                    gDPPipeSync(POLY_OPA_DISP++);

                    if (sPlayedSongBtns[j] == FLUTE_BTN_A) {
                        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, QUEST_FLUTE_BTN_A_COLOR_R, QUEST_FLUTE_BTN_A_COLOR_G,
                                        QUEST_FLUTE_BTN_A_COLOR_B, sPlayedSongBtnsAlpha[j]);
                    } else {
                        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 50, sPlayedSongBtnsAlpha[j]);
                    }

                    gDPSetEnvColor(POLY_OPA_DISP++, 10, 10, 10, 0);
                    gSPVertex(POLY_OPA_DISP++, &pauseCtx->questVtx[bufI], 4, 0);

                    gDPLoadTextureBlock(POLY_OPA_DISP++, sFluteBtnTextures[sPlayedSongBtns[j]], G_IM_FMT_IA,
                                        G_IM_SIZ_8b, 16, 16, 0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP,
                                        G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

                    gSP1Quadrangle(POLY_OPA_DISP++, 0, 2, 3, 1, 0);
                }
            }
        } else if (((pauseCtx->mainState >= PAUSE_MAIN_STATE_SONG_PROMPT_INIT) &&
                    (pauseCtx->mainState <= PAUSE_MAIN_STATE_SONG_PROMPT_DONE)) ||
                   (pauseCtx->mainState == PAUSE_MAIN_STATE_IDLE_CURSOR_ON_SONG)) {
            // Draw the buttons for playing a song
            // QUEST_QUAD_SONG_NOTE_A1 to QUEST_QUAD_SONG_NOTE_A8

            // temps reused, fake?
            stepGreen = pauseCtx->fluteSongIdx;
            stepRed = gFluteSongButtons[stepGreen].numButtons;

            j = bufI;

            for (i = 0; i < stepRed; i++, bufI += 4) {
                pauseCtx->questVtx[bufI + 0].v.ob[1] = pauseCtx->questVtx[bufI + 1].v.ob[1] =
                    R_PAUSE_SONG_OCA_BTN_Y(gFluteSongButtons[stepGreen].buttonsIndex[i]);

                pauseCtx->questVtx[bufI + 2].v.ob[1] = pauseCtx->questVtx[bufI + 3].v.ob[1] =
                    pauseCtx->questVtx[bufI + 0].v.ob[1] - 12;

                gDPPipeSync(POLY_OPA_DISP++);

                if (pauseCtx->mainState == PAUSE_MAIN_STATE_IDLE_CURSOR_ON_SONG) {
                    if (gFluteSongButtons[stepGreen].buttonsIndex[i] == FLUTE_BTN_A) {
                        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, QUEST_FLUTE_BTN_A_COLOR_R, QUEST_FLUTE_BTN_A_COLOR_G,
                                        QUEST_FLUTE_BTN_A_COLOR_B, 200);
                    } else {
                        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 50, 200);
                    }
                } else {
                    // Gray out buttons during the player playing the song
                    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 150, 150, 150, 150);
                }

                gDPSetEnvColor(POLY_OPA_DISP++, 10, 10, 10, 0);

                gSPVertex(POLY_OPA_DISP++, &pauseCtx->questVtx[bufI], 4, 0);

                gDPLoadTextureBlock(POLY_OPA_DISP++,
                                    sFluteBtnTextures[gFluteSongButtons[stepGreen].buttonsIndex[i]], G_IM_FMT_IA,
                                    G_IM_SIZ_8b, 16, 16, 0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP,
                                    G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

                gSP1Quadrangle(POLY_OPA_DISP++, 0, 2, 3, 1, 0);
            }

            if (pauseCtx->mainState != PAUSE_MAIN_STATE_IDLE_CURSOR_ON_SONG) {
                // Draw the buttons colored as the player plays the song
                // QUEST_QUAD_SONG_NOTE_B1 to QUEST_QUAD_SONG_NOTE_B8

                pauseCtx->fluteStaff = AudioFlute_GetPlayingStaff();

                if (pauseCtx->fluteStaff->pos != 0) {
                    if (sPlayedSongBtnsNum == (pauseCtx->fluteStaff->pos - 1)) {
                        if ((pauseCtx->fluteStaff->buttonIndex >= FLUTE_BTN_A) &&
                            (pauseCtx->fluteStaff->buttonIndex <= FLUTE_BTN_C_UP)) {
                            sPlayedSongBtns[pauseCtx->fluteStaff->pos - 1] = pauseCtx->fluteStaff->buttonIndex;
                            sPlayedSongBtns[pauseCtx->fluteStaff->pos] = FLUTE_BTN_INVALID;
                            sPlayedSongBtnsNum++;
                        }
                    }
                }

                bufI = j + ((QUEST_QUAD_SONG_NOTE_B1 - QUEST_QUAD_SONG_NOTE_A1) * 4);

                for (i = 0; i < SONG_MAX_LENGTH; i++, bufI += 4) {
                    if (sPlayedSongBtns[i] == FLUTE_BTN_INVALID) {
                        continue;
                    }

                    if (sPlayedSongBtnsAlpha[i] != 255) {
                        sPlayedSongBtnsAlpha[i] += R_FLUTE_BUTTONS_APPEAR_ALPHA_STEP;
                        if (sPlayedSongBtnsAlpha[i] >= 255) {
                            sPlayedSongBtnsAlpha[i] = 255;
                        }
                    }

                    pauseCtx->questVtx[bufI + 0].v.ob[1] = pauseCtx->questVtx[bufI + 1].v.ob[1] =
                        R_PAUSE_SONG_OCA_BTN_Y(sPlayedSongBtns[i]);

                    pauseCtx->questVtx[bufI + 2].v.ob[1] = pauseCtx->questVtx[bufI + 3].v.ob[1] =
                        pauseCtx->questVtx[bufI + 0].v.ob[1] - 12;

                    gDPPipeSync(POLY_OPA_DISP++);

                    if (sPlayedSongBtns[i] == FLUTE_BTN_A) {
                        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, QUEST_FLUTE_BTN_A_COLOR_R, QUEST_FLUTE_BTN_A_COLOR_G,
                                        QUEST_FLUTE_BTN_A_COLOR_B, sPlayedSongBtnsAlpha[i]);
                    } else {
                        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 50, sPlayedSongBtnsAlpha[i]);
                    }

                    gDPSetEnvColor(POLY_OPA_DISP++, 10, 10, 10, 0);

                    gSPVertex(POLY_OPA_DISP++, &pauseCtx->questVtx[bufI], 4, 0);

                    gDPLoadTextureBlock(POLY_OPA_DISP++, sFluteBtnTextures[sPlayedSongBtns[i]], G_IM_FMT_IA,
                                        G_IM_SIZ_8b, 16, 16, 0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP,
                                        G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

                    gSP1Quadrangle(POLY_OPA_DISP++, 0, 2, 3, 1, 0);
                }

                if (pauseCtx->mainState == PAUSE_MAIN_STATE_SONG_PROMPT_INIT) {
                    for (i = 0; i < SONG_MAX_LENGTH; i++) {
                        sPlayedSongBtns[i] = FLUTE_BTN_INVALID;
                        sPlayedSongBtnsAlpha[i] = 0;
                    }
                    sPlayedSongBtnsNum = 0;

                    AudioFlute_SetInstrument(FLUTE_INSTRUMENT_DEFAULT);
                    AudioFlute_Start((1 << pauseCtx->fluteSongIdx) + 0x8000);
                    pauseCtx->fluteStaff = AudioFlute_GetPlaybackStaff();
                    pauseCtx->fluteStaff->pos = 0;
                    pauseCtx->fluteStaff->state = 0xFE;

                    pauseCtx->mainState = PAUSE_MAIN_STATE_SONG_PROMPT;
                }
            }
        }
    }

    // Draw amount of gold webguard tokens
    // QUEST_QUAD_SKULL_TOKENS_DIGIT1_SHADOW to QUEST_QUAD_SKULL_TOKENS_DIGIT3

    if (CHECK_QUEST_ITEM(QUEST_SKULL_TOKEN)) {
        gDPPipeSync(POLY_OPA_DISP++);
        gDPSetCombineLERP(POLY_OPA_DISP++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0,
                          PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0);
        gDPSetEnvColor(POLY_OPA_DISP++, 0, 0, 0, 0);

        gsTokenDigits[0] = gsTokenDigits[1] = 0;
        gsTokenDigits[2] = gSaveContext.save.info.inventory.gsTokens;

        while (gsTokenDigits[2] >= 100) {
            gsTokenDigits[0]++;
            gsTokenDigits[2] -= 100;
        }

        while (gsTokenDigits[2] >= 10) {
            gsTokenDigits[1]++;
            gsTokenDigits[2] -= 10;
        }

        gSPVertex(POLY_OPA_DISP++, &pauseCtx->questVtx[QUEST_QUAD_SKULL_TOKENS_DIGIT1_SHADOW * 4], 6 * 4, 0);

        for (i = 0, j = 0; i < 2; i++) {
            if (i == 0) {
                // Text shadow
                gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 0, 0, 0, pauseCtx->alpha);
            } else {
                // Text color
                if (gSaveContext.save.info.inventory.gsTokens == 100) {
                    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 200, 50, 50, pauseCtx->alpha);
                } else {
                    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, pauseCtx->alpha);
                }
            }

            // Variable reused as a flag indicating all digits onwards should be displayed
            cursorItem = false;

            for (bufI = 0; bufI < 3; bufI++, j += 4) {
                if ((bufI >= 2) || (gsTokenDigits[bufI] != 0) || cursorItem) {
                    gDPLoadTextureBlock(POLY_OPA_DISP++, ((u8*)gCounterDigit0Tex + (8 * 16 * gsTokenDigits[bufI])),
                                        G_IM_FMT_I, G_IM_SIZ_8b, 8, 16, 0, G_TX_NOMIRROR | G_TX_WRAP,
                                        G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

                    gSP1Quadrangle(POLY_OPA_DISP++, j, j + 2, j + 3, j + 1, 0);

                    cursorItem = true;
                }
            }
        }
    }

    CLOSE_DISPS(gfxCtx, "../z_kaleido_collect.c", 863);
}

s32 KaleidoScope_UpdateQuestStatusPoint(PauseContext* pauseCtx, s32 point) {
    pauseCtx->cursorPoint[PAUSE_QUEST] = point;

    return true;
}
