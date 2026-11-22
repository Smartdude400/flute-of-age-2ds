// asset_sizes.h (rewritten: native)
// Texture dimension/size constants that the original took from extracted asset headers (not part of
// the inputs). Derived from the XML specs in flute-of-age/assets/xml (Width/Height/Format, confirmed
// by the spacing of consecutive Offset values). Used for game-arena allocation sizes and offsets.
#ifndef ASSET_SIZES_H
#define ASSET_SIZES_H
#define DO_ACTION_TEX_WIDTH 48   // do_action_static: ia4 48x16 (offset step 0x180)
#define DO_ACTION_TEX_HEIGHT 16
#define DO_ACTION_TEX_SIZE ((DO_ACTION_TEX_WIDTH * DO_ACTION_TEX_HEIGHT) / 2)
#define ITEM_ICON_WIDTH 32       // icon_item_static: rgba32 32x32 (step 0x1000)
#define ITEM_ICON_HEIGHT 32
#define ITEM_ICON_SIZE (ITEM_ICON_WIDTH * ITEM_ICON_HEIGHT * 4)
#define QUEST_ICON_WIDTH 24      // icon_item_24_static: rgba32 24x24 (step 0x900)
#define QUEST_ICON_HEIGHT 24
#define QUEST_ICON_SIZE (QUEST_ICON_WIDTH * QUEST_ICON_HEIGHT * 4)
#define FONT_CHAR_TEX_WIDTH 16   // nes_font_static: i4 16x16 (step 0x80)
#define FONT_CHAR_TEX_HEIGHT 16
#define FONT_CHAR_TEX_SIZE ((FONT_CHAR_TEX_WIDTH * FONT_CHAR_TEX_HEIGHT) / 2)
#define MESSAGE_STATIC_TEX_SIZE 0x1000          // message_static: 128x64 4bpp (step 0x1000)
#define MESSAGE_TEXTURE_STATIC_TEX_SIZE 0x900   // message_texture_static: i4 96x48 (step 0x900)
#define MAP_I_TEX_WIDTH 96       // map_i_static: i4 96x85 (step 0xFF0)
#define MAP_I_TEX_HEIGHT 85
#define MAP_I_TEX_SIZE ((MAP_I_TEX_WIDTH * MAP_I_TEX_HEIGHT) / 2)
#define PLACE_NAME_TEX_WIDTH 144 // place_title_cards: ia8 144x24 (step 0xD80)
#define PLACE_NAME_TEX_HEIGHT 24
#endif
