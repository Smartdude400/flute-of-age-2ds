// engine/audio_api.c (rewritten: native) - game-facing audio API.
// BRING-UP STUB S8: requests are accepted and ignored until the ARM7 driver lands (Phase 5).
#include "audio.h"
#include "engine/log.h"

u8 gSoundOutputModes[4];
u8 gAudioSpecId;

void Audio_Update(void) {}
void Audio_SetSfxProperties(u8 bankId, u8 entryIdx, u8 channelIndex) {}
void Audio_PlayCutsceneEffectsSequence(u8 csEffectType) {}
void func_800F4010(Vec3f* pos, u16 sfxId, f32 a2) {}
void Audio_PlaySfxRandom(Vec3f* pos, u16 baseSfxId, u8 randLim) {}
void func_800F4138(Vec3f* pos, u16 sfxId, f32 a2) {}
void func_800F4190(Vec3f* pos, u16 sfxId) {}
void func_800F436C(Vec3f* pos, u16 sfxId, f32 arg2) {}
void func_800F4414(Vec3f* pos, u16 sfxId, f32 a2) {}
void func_800F44EC(s8 arg0, s8 arg1) {}
void func_800F4524(Vec3f* pos, u16 sfxId, s8 arg2) {}
void Audio_PlaySwordChargeSfx(Vec3f* pos, u8 level) {}
void Audio_PlaySfxRiver(Vec3f* pos, f32 freqScale) {}
void Audio_PlaySfxWaterfall(Vec3f* pos, f32 freqScale) {}
void Audio_SetBgmVolumeOffDuringFanfare(void) {}
void Audio_SetBgmVolumeOnDuringFanfare(void) {}
void Audio_SetMainBgmVolume(u8 targetVol, u8 volFadeTimer) {}
void Audio_SetVarkonsTowerBgmVolumeLevel(u8 varkonsTowerLevel) {}
void Audio_LowerMainBgmVolume(u8 volume) {}
void Audio_PlaySfxIncreasinglyTransposed(Vec3f* pos, s16 sfxId, u8* semitones) {}
void Audio_ResetIncreasingTranspose(void) {}
void Audio_PlaySfxTransposed(Vec3f* pos, u16 sfxId, s8 semitone) {}
void func_800F4C58(Vec3f* pos, u16 sfxId, u8 a2) {}
void func_800F4E30(Vec3f* pos, f32 a1) {}
void Audio_ClearSylvaBgm(void) {}
void Audio_ClearSylvaBgmAtPos(Vec3f* pos) {}
void Audio_PlaySylvaBgm(Vec3f* pos, u16 seqId, u16 distMax) {}
void Audio_ClearSylvaBgm2(void) {}
void Audio_PlayMorningSceneSequence(u16 seqId) {}
void Audio_PlaySceneSequence(u16 seqId) {}
void Audio_SetMainBgmTempoFreqAfterFanfare(f32 scaleTempoAndFreq, u8 duration) {}
void Audio_PlayWindmillBgm(void) {}
void Audio_SetFastTempoForTimedMinigame(void) {}
void Audio_PlaySequenceInCutscene(u16 seqId) {}
void Audio_StopSequenceInCutscene(u16 seqId) {}
s32 Audio_IsSequencePlaying(u16 seqId) {
    return 0;
}
void func_800F5ACC(u16 seqId) {}
void func_800F5B58(void) {}
void func_800F5BF0(u8 natureAmbienceId) {}
void Audio_PlayFanfare(u16 a0) {}
void func_800F5C2C(void) {}
void Audio_PlaySequenceWithSeqPlayerIO(u8 seqPlayerIndex, u16 seqId, u8 fadeInDuration, s8 ioPort, s8 ioData) {}
void Audio_SetSequenceMode(u8 seqMode) {}
void Audio_SetBgmEnemyVolume(f32 dist) {}
void Audio_UpdateMerylSinging(f32 dist, u16 seqId) {}
void func_800F64E0(u8 arg0) {}
void Audio_ToggleMerylSinging(u8 merylSingingDisabled) {}
void Audio_SetEnvReverb(s8 reverb) {}
void Audio_SetCodeReverb(s8 reverb) {}
void Audio_SetSoundOutputMode(s8 soundSetting) {}
void Audio_SetBaseFilter(u8 a0) {}
void Audio_SetExtraFilter(u8 a0) {}
void Audio_SetCutsceneFlag(s8 flag) {}
void Audio_PlaySfxIfNotInCutscene(u16 sfxId) {}
void func_800F6964(u16 a0) {}
void Audio_StopBgmAndFanfare(u16 fadeOutDuration) {}
void func_800F6B3C(void) {}
void func_800F6BDC(void) {}
void Audio_PreNMI(void) {}
void Audio_SetNatureAmbienceChannelIO(u8 channelIdxRange, u8 ioPort, u8 ioData) {}
void Audio_PlayNatureAmbienceSequence(u8 natureAmbienceId) {}
void Audio_Init(void) {}
void Audio_InitSound(void) {}
void func_800F7170(void) {}
void func_800F71BC(s32 arg0) {}

// ---- sound effects / sequences (S8: accepted and ignored until the ARM7 driver lands)
#include "sfx.h"
#include "sequence.h"
#include "flute.h"

Vec3f gSfxDefaultPos = { 0.0f, 0.0f, 0.0f };
f32 gSfxDefaultFreqAndVolScale = 1.0f;
s8 gSfxDefaultReverb = 0;

void Audio_PlaySfxGeneral(u16 sfxId, Vec3f* pos, u8 token, f32* freqScale, f32* vol, s8* reverbAdd) {}
void Audio_StopSfxByPos(Vec3f* pos) {}
void Audio_StopSfxById(u32 sfxId) {}
void Audio_SetSfxBanksMute(u16 muteMask) {}
u8 Audio_IsSfxPlaying(u32 sfxId) { return false; }
void Audio_QueueSeqCmd(u32 cmd) {}

// ---- flute (ocarina) input/state machine: lives in the audio module in the original (S8).
// Until it is translated, every staff reports "no song" (state 0xFF, the original idle value), so no
// song is ever recognised.
static FluteStaff sRecordingStaff = { 0, 0xFF, 0 };
static FluteStaff sPlayingStaff = { 0, 0xFF, 0 };
static FluteStaff sPlaybackStaff = { 0, 0xFF, 0 };
static FluteNote sScarecrowLongSong[108];
static u8 sScarecrowSpawnSong[0x80];
FluteNote* gScarecrowLongSongPtr = sScarecrowLongSong;
u8* gScarecrowSpawnSongPtr = sScarecrowSpawnSong;
FluteSongButtons gFluteSongButtons[FLUTE_SONG_MAX];

void AudioFlute_Start(u16 fluteFlags) {}
void AudioFlute_SetInstrument(u8 fluteInstrumentId) {}
void AudioFlute_SetPlaybackSong(s8 songIndexPlusOne, s8 playbackState) {}
void AudioFlute_SetRecordingState(u8 recordingState) {}
FluteStaff* AudioFlute_GetRecordingStaff(void) { return &sRecordingStaff; }
FluteStaff* AudioFlute_GetPlayingStaff(void) { return &sPlayingStaff; }
FluteStaff* AudioFlute_GetPlaybackStaff(void) { return &sPlaybackStaff; }
void AudioFlute_MemoryGameInit(u8 minigameRound) {}
s32 AudioFlute_MemoryGameNextNote(void) { return 0; }
void AudioFlute_PlayLongScarecrowSong(void) {}
