// printf.h (rewritten: native). Original debug printing is compiled out in non-debug builds,
// exactly as in the original (PRINTF -> (void)0). Translation helper kept for string arguments.
#ifndef PRINTF_H
#define PRINTF_H
#define PRINTF(format, ...) (void)0
#define PRINTF_COLOR_BLACK() (void)0
#define PRINTF_COLOR_RED() (void)0
#define PRINTF_COLOR_GREEN() (void)0
#define PRINTF_COLOR_YELLOW() (void)0
#define PRINTF_COLOR_BLUE() (void)0
#define PRINTF_COLOR_MAGENTA() (void)0
#define PRINTF_COLOR_CYAN() (void)0
#define PRINTF_COLOR_WHITE() (void)0
#define PRINTF_COLOR_WARNING() (void)0
#define PRINTF_COLOR_ERROR() (void)0
#define PRINTF_RST() (void)0
#ifndef T
#define T(jp, en) en
#endif
#endif
