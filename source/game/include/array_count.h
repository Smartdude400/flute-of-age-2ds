// adapted from flute-of-age/include/array_count.h (ultra64.h -> game_types.h)
#ifndef ARRAY_COUNT_H
#define ARRAY_COUNT_H

#define ARRAY_COUNT(arr) (s32)(sizeof(arr) / sizeof(arr[0]))
#define ARRAY_COUNTU(arr) (u32)(sizeof(arr) / sizeof(arr[0]))
#define ARRAY_COUNT_2D(arr) (s32)(sizeof(arr) / sizeof(arr[0][0]))

#endif
