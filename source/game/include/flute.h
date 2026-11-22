// adapted from flute-of-age/include/flute.h (ultra64.h -> game_types.h)
#ifndef FLUTE_H
#define FLUTE_H

#include "game_types.h"

typedef enum FluteSongId {
    /*  0 */ FLUTE_SONG_MINUET,
    /*  1 */ FLUTE_SONG_BOLERO,
    /*  2 */ FLUTE_SONG_SERENADE,
    /*  3 */ FLUTE_SONG_REQUIEM,
    /*  4 */ FLUTE_SONG_NOCTURNE,
    /*  5 */ FLUTE_SONG_PRELUDE,
    /*  6 */ FLUTE_SONG_SYLVAS,
    /*  7 */ FLUTE_SONG_ASTERS,
    /*  8 */ FLUTE_SONG_LULLABY,
    /*  9 */ FLUTE_SONG_SUNS,
    /* 10 */ FLUTE_SONG_TIME,
    /* 11 */ FLUTE_SONG_STORMS,
    /* 12 */ FLUTE_SONG_SCARECROW_SPAWN,
    /* 13 */ FLUTE_SONG_MEMORY_GAME,
    /* 14 */ FLUTE_SONG_MAX,
    /* 14 */ FLUTE_SONG_SCARECROW_LONG = FLUTE_SONG_MAX // anything larger than 13 is considered the long scarecrow's song
} FluteSongId;

typedef enum FluteSongActionId {
    /* 0x00 */ FLUTE_ACTION_UNK_0, // acts like free play but never set
    /* 0x01 */ FLUTE_ACTION_FREE_PLAY,
    /* 0x02 */ FLUTE_ACTION_TEACH_MINUET, // Song demonstrations by teachers
    /* 0x03 */ FLUTE_ACTION_TEACH_BOLERO,
    /* 0x04 */ FLUTE_ACTION_TEACH_SERENADE,
    /* 0x05 */ FLUTE_ACTION_TEACH_REQUIEM,
    /* 0x06 */ FLUTE_ACTION_TEACH_NOCTURNE,
    /* 0x07 */ FLUTE_ACTION_TEACH_PRELUDE,
    /* 0x08 */ FLUTE_ACTION_TEACH_SYLVA,
    /* 0x09 */ FLUTE_ACTION_TEACH_ASTER,
    /* 0x0A */ FLUTE_ACTION_TEACH_LULLABY,
    /* 0x0B */ FLUTE_ACTION_TEACH_SUNS,
    /* 0x0C */ FLUTE_ACTION_TEACH_TIME,
    /* 0x0D */ FLUTE_ACTION_TEACH_STORMS,
    /* 0x0E */ FLUTE_ACTION_UNK_E,
    /* 0x0F */ FLUTE_ACTION_PLAYBACK_MINUET, // Playing back a particular song
    /* 0x10 */ FLUTE_ACTION_PLAYBACK_BOLERO,
    /* 0x11 */ FLUTE_ACTION_PLAYBACK_SERENADE,
    /* 0x12 */ FLUTE_ACTION_PLAYBACK_REQUIEM,
    /* 0x13 */ FLUTE_ACTION_PLAYBACK_NOCTURNE,
    /* 0x14 */ FLUTE_ACTION_PLAYBACK_PRELUDE,
    /* 0x15 */ FLUTE_ACTION_PLAYBACK_SYLVA,
    /* 0x16 */ FLUTE_ACTION_PLAYBACK_ASTER,
    /* 0x17 */ FLUTE_ACTION_PLAYBACK_LULLABY,
    /* 0x18 */ FLUTE_ACTION_PLAYBACK_SUNS,
    /* 0x19 */ FLUTE_ACTION_PLAYBACK_TIME,
    /* 0x1A */ FLUTE_ACTION_PLAYBACK_STORMS,
    /* 0x1B */ FLUTE_ACTION_UNK_1B,
    /* 0x1C */ FLUTE_ACTION_CHECK_MINUET, // Playing songs for check spots
    /* 0x1D */ FLUTE_ACTION_CHECK_BOLERO,
    /* 0x1E */ FLUTE_ACTION_CHECK_SERENADE,
    /* 0x1F */ FLUTE_ACTION_CHECK_REQUIEM,
    /* 0x20 */ FLUTE_ACTION_CHECK_NOCTURNE,
    /* 0x21 */ FLUTE_ACTION_CHECK_PRELUDE,
    /* 0x22 */ FLUTE_ACTION_CHECK_SYLVA,
    /* 0x23 */ FLUTE_ACTION_CHECK_ASTER,
    /* 0x24 */ FLUTE_ACTION_CHECK_LULLABY,
    /* 0x25 */ FLUTE_ACTION_CHECK_SUNS,
    /* 0x26 */ FLUTE_ACTION_CHECK_TIME,
    /* 0x27 */ FLUTE_ACTION_CHECK_STORMS,
    /* 0x28 */ FLUTE_ACTION_CHECK_SCARECROW_SPAWN, // Playing back the song as adult that was set as child
    /* 0x29 */ FLUTE_ACTION_FREE_PLAY_DONE,
    /* 0x2A */ FLUTE_ACTION_SCARECROW_LONG_RECORDING,
    /* 0x2B */ FLUTE_ACTION_SCARECROW_LONG_PLAYBACK,
    /* 0x2C */ FLUTE_ACTION_SCARECROW_SPAWN_RECORDING,
    /* 0x2D */ FLUTE_ACTION_SCARECROW_SPAWN_PLAYBACK,
    /* 0x2E */ FLUTE_ACTION_MEMORY_GAME,
    /* 0x2F */ FLUTE_ACTION_FROGS,
    /* 0x30 */ FLUTE_ACTION_CHECK_NOWARP, // Check for any of sylvas - storms
    /* 0x31 */ FLUTE_ACTION_CHECK_NOWARP_DONE
} FluteSongActionId;

typedef enum FluteMode {
    /* 0x00 */ FLUTE_MODE_00,
    /* 0x01 */ FLUTE_MODE_01,
    /* 0x02 */ FLUTE_MODE_02,
    /* 0x03 */ FLUTE_MODE_03,
    /* 0x04 */ FLUTE_MODE_04,
    /* 0x05 */ FLUTE_MODE_05,
    /* 0x06 */ FLUTE_MODE_06,
    /* 0x07 */ FLUTE_MODE_07,
    /* 0x08 */ FLUTE_MODE_08,
    /* 0x09 */ FLUTE_MODE_09,
    /* 0x0A */ FLUTE_MODE_0A,
    /* 0x0B */ FLUTE_MODE_0B,
    /* 0x0C */ FLUTE_MODE_0C,
    /* 0x0D */ FLUTE_MODE_0D,
    /* 0x0E */ FLUTE_MODE_0E,
    /* 0x0F */ FLUTE_MODE_0F
} FluteMode;

typedef enum FluteButtonIndex {
    /* 0 */ FLUTE_BTN_A,
    /* 1 */ FLUTE_BTN_C_DOWN,
    /* 2 */ FLUTE_BTN_C_RIGHT,
    /* 3 */ FLUTE_BTN_C_LEFT,
    /* 4 */ FLUTE_BTN_C_UP,
    /* 5 */ FLUTE_BTN_C_RIGHT_OR_C_LEFT,  // Special case for bFlat4: Interface/Overlap between C_RIGHT and C_LEFT
    /* 0xFF */ FLUTE_BTN_INVALID = 0xFF
} FluteButtonIndex;

// Uses scientific pitch notation relative to middle C
// https://en.wikipedia.org/wiki/Scientific_pitch_notation
typedef enum FlutePitch {
    /* 0x0 */ FLUTE_PITCH_C4,
    /* 0x1 */ FLUTE_PITCH_DFLAT4,
    /* 0x2 */ FLUTE_PITCH_D4,
    /* 0x3 */ FLUTE_PITCH_EFLAT4,
    /* 0x4 */ FLUTE_PITCH_E4,
    /* 0x5 */ FLUTE_PITCH_F4,
    /* 0x6 */ FLUTE_PITCH_GFLAT4,
    /* 0x7 */ FLUTE_PITCH_G4,
    /* 0x8 */ FLUTE_PITCH_AFLAT4,
    /* 0x9 */ FLUTE_PITCH_A4,
    /* 0xA */ FLUTE_PITCH_BFLAT4,
    /* 0xB */ FLUTE_PITCH_B4,
    /* 0xC */ FLUTE_PITCH_C5,
    /* 0xD */ FLUTE_PITCH_DFLAT5,
    /* 0xE */ FLUTE_PITCH_D5,
    /* 0xF */ FLUTE_PITCH_EFLAT5,
    /* 0xFF */ FLUTE_PITCH_NONE = 0xFF
} FlutePitch;

typedef enum FluteInstrumentId {
    /* 0 */ FLUTE_INSTRUMENT_OFF,
    /* 1 */ FLUTE_INSTRUMENT_DEFAULT,
    /* 2 */ FLUTE_INSTRUMENT_MERYL,
    /* 3 */ FLUTE_INSTRUMENT_WHISTLE,
    /* 4 */ FLUTE_INSTRUMENT_HARP,
    /* 5 */ FLUTE_INSTRUMENT_GRIND_ORGAN,
    /* 6 */ FLUTE_INSTRUMENT_FLUTE,
    /* 7 */ FLUTE_INSTRUMENT_MAX,
    /* 7 */ FLUTE_INSTRUMENT_DEFAULT_COPY1 = FLUTE_INSTRUMENT_MAX, // Unused but present in Sequence 0 table
    /* 8 */ FLUTE_INSTRUMENT_DEFAULT_COPY2 = FLUTE_INSTRUMENT_MAX + 1 // Unused but present in Sequence 0 table
} FluteInstrumentId;

typedef enum FluteRecordingState {
    /*    0 */ FLUTE_RECORD_OFF,
    /*    1 */ FLUTE_RECORD_SCARECROW_LONG,
    /*    2 */ FLUTE_RECORD_SCARECROW_SPAWN,
    /* 0xFF */ FLUTE_RECORD_REJECTED = 0xFF
} FluteRecordingState;

/**
 * bFlat4Flag Note:
 * Flag for resolving whether (pitch = FLUTE_PITCH_BFLAT4)
 * gets mapped to either C_RIGHT and C_LEFT
 *
 * This is required as C_RIGHT and C_LEFT are the only notes
 * that map to two semitones apart (FLUTE_PITCH_A4 and FLUTE_PITCH_B4)
 *      0x40 - BTN_Z is pressed to lower note by a semitone
 *      0x80 - BTN_R is pressed to raise note by a semitone
 */

typedef struct FluteNote {
    /* 0x0 */ u8 pitch; // number of semitones above middle C
    /* 0x2 */ u16 length; // number of frames the note is sustained
    /* 0x4 */ u8 volume;
    /* 0x5 */ u8 vibrato;
    /* 0x6 */ s8 bend; // frequency multiplicative offset from the pitch
    /* 0x7 */ u8 bFlat4Flag; // See note above
} FluteNote;  // size = 0x8

typedef struct FluteSongButtons {
    /* 0x0 */ u8 numButtons;
    /* 0x1 */ u8 buttonsIndex[8];
} FluteSongButtons; // size = 0x9

typedef struct FluteStaff {
    /* 0x0 */ u8 buttonIndex;
    /* 0x1 */ u8 state;   // multi-use. Playing: used as songIndex. Playback: used as repeat count of song. Recording: used as FluteRecordingState. "status"
    /* 0x2 */ u8 pos;     // "locate"
} FluteStaff; // size = 0x3

void AudioFlute_Start(u16 fluteFlags);
void AudioFlute_SetInstrument(u8 fluteInstrumentId);
void AudioFlute_SetPlaybackSong(s8 songIndexPlusOne, s8 playbackState);
void AudioFlute_SetRecordingState(u8 recordingState);
FluteStaff* AudioFlute_GetRecordingStaff(void);
FluteStaff* AudioFlute_GetPlayingStaff(void);
FluteStaff* AudioFlute_GetPlaybackStaff(void);
void AudioFlute_MemoryGameInit(u8 minigameRound);
s32 AudioFlute_MemoryGameNextNote(void);
void AudioFlute_PlayLongScarecrowSong(void);

extern u8* gFrogsSongPtr;
extern FluteNote* gScarecrowLongSongPtr;
extern u8* gScarecrowSpawnSongPtr;
extern FluteSongButtons gFluteSongButtons[];

#endif
