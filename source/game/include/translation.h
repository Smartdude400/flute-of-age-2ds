// translation.h (rewritten: native). Debug-string translation helper; strings only feed logging.
#ifndef TRANSLATION_H
#define TRANSLATION_H
#ifndef T
#define T(jp, en) en
#endif
#define LOG_STRING_T(stringJP, stringEN, file, line) ((void)0)
#endif
