// adapted from flute-of-age/include/avoid_ub.h (ultra64.h -> game_types.h)
#ifndef AVOID_UB_H
#define AVOID_UB_H

/**
 * This macro is used when the return type of a function is incorrect
 */
#ifndef AVOID_UB
#define BAD_RETURN(type) type
#else
#define BAD_RETURN(type) void
#endif

#endif
