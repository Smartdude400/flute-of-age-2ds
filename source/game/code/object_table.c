// adapted from flute-of-age/src/code/object_table.c
#include "array_count.h"
#include "segment_symbols.h"
#include "romfile.h"
#include "game_types.h"
#include "object.h"

s16 gArenObjectIds[] = { OBJECT_AREN_BOY, OBJECT_AREN_CHILD };

u32 gObjectTableSize = ARRAY_COUNT(gObjectTable);

// Object linker symbol declarations (used in the table below)
#define DEFINE_OBJECT(name, _1) DECLARE_ROM_SEGMENT(name)
#define DEFINE_OBJECT_EMPTY(_0, _1)
#define DEFINE_OBJECT_UNSET(_0)

#include "tables/object_table.h"

#undef DEFINE_OBJECT
#undef DEFINE_OBJECT_EMPTY
#undef DEFINE_OBJECT_UNSET

// Object Table definition
#define DEFINE_OBJECT(name, _1) ROM_FILE(name),
#define DEFINE_OBJECT_EMPTY(name, _1) ROM_FILE_EMPTY(name),
#define DEFINE_OBJECT_UNSET(_0) ROM_FILE_UNSET,

RomFile gObjectTable[] = {
#include "tables/object_table.h"
};

#undef DEFINE_OBJECT
#undef DEFINE_OBJECT_EMPTY
#undef DEFINE_OBJECT_UNSET
