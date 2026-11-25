// engine/rdp_consts.h (imported: include/ultra64/gbi.h lines 349-378, TMEM_SIZE from include/gfx.h:16)
// Texture size constants game headers use for buffer sizes (pause menu, interface).
#ifndef ENGINE_RDP_CONSTS_H
#define ENGINE_RDP_CONSTS_H
#define TMEM_SIZE 0x1000

#define G_IM_SIZ_4b_BYTES       0
#define G_IM_SIZ_4b_TILE_BYTES  G_IM_SIZ_4b_BYTES
#define G_IM_SIZ_4b_LINE_BYTES  G_IM_SIZ_4b_BYTES

#define G_IM_SIZ_8b_BYTES       1
#define G_IM_SIZ_8b_TILE_BYTES  G_IM_SIZ_8b_BYTES
#define G_IM_SIZ_8b_LINE_BYTES  G_IM_SIZ_8b_BYTES

#define G_IM_SIZ_16b_BYTES      2
#define G_IM_SIZ_16b_TILE_BYTES G_IM_SIZ_16b_BYTES
#define G_IM_SIZ_16b_LINE_BYTES G_IM_SIZ_16b_BYTES

#define G_IM_SIZ_32b_BYTES      4
#define G_IM_SIZ_32b_TILE_BYTES 2
#define G_IM_SIZ_32b_LINE_BYTES 2

#define G_IM_SIZ_4b_LOAD_BLOCK  G_IM_SIZ_16b
#define G_IM_SIZ_8b_LOAD_BLOCK  G_IM_SIZ_16b
#define G_IM_SIZ_16b_LOAD_BLOCK G_IM_SIZ_16b
#define G_IM_SIZ_32b_LOAD_BLOCK G_IM_SIZ_32b

#define G_IM_SIZ_4b_SHIFT  2
#define G_IM_SIZ_8b_SHIFT  1
#define G_IM_SIZ_16b_SHIFT 0
#define G_IM_SIZ_32b_SHIFT 0

#define G_IM_SIZ_4b_INCR  3
#define G_IM_SIZ_8b_INCR  1
#define G_IM_SIZ_16b_INCR 0
#define G_IM_SIZ_32b_INCR 0

#endif
