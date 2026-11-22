// adapted from flute-of-age/include/message_data_static.h (ultra64.h -> game_types.h)
#ifndef MESSAGE_DATA_STATIC_H
#define MESSAGE_DATA_STATIC_H

#include "game_types.h"
#include "message_data_fmt.h"
#include "versions.h"

typedef enum TextBoxType {
    /*  0 */ TEXTBOX_TYPE_BLACK,
    /*  1 */ TEXTBOX_TYPE_WOODEN,
    /*  2 */ TEXTBOX_TYPE_BLUE,
    /*  3 */ TEXTBOX_TYPE_FLUTE,
    /*  4 */ TEXTBOX_TYPE_NONE_BOTTOM,
    /*  5 */ TEXTBOX_TYPE_NONE_NO_SHADOW,
    /* 11 */ TEXTBOX_TYPE_CREDITS = 11
} TextBoxType;

typedef enum TextBoxBackground {
    /* 0 */ TEXTBOX_BG_CROSS
} TextBoxBackground;

typedef enum TextBoxPosition {
    /* 0 */ TEXTBOX_POS_VARIABLE,
    /* 1 */ TEXTBOX_POS_TOP,
    /* 2 */ TEXTBOX_POS_MIDDLE,
    /* 3 */ TEXTBOX_POS_BOTTOM
} TextBoxPosition;

typedef struct MessageTableEntry {
    u16 textId;
    u8 typePos;
    const char* segment;
} MessageTableEntry;

/*
 *  Message Symbol Declarations
 *  native: the original declared one linker symbol per message; here each symbol is the message's
 *  original segment address, generated from the ROM tables (tools/build_text.py).
 */
#include "text/message_symbols.h"

#endif
