// audio.h (rewritten: native)
// Game-facing audio request API only (sound effects, sequences, reverb, ambience), with the original
// signatures so adapted game code calls it unchanged. The original N64 audio engine (RSP synthesis,
// sample DMA, sequence player internals) is NOT ported. These requests are served by the native
// ARM7 audio driver (engine/audio_*.c, Phase 5).
#ifndef AUDIO_H
#define AUDIO_H

#include "game_types.h"
#include "sequence.h"
#include "z_math.h"

// (from the original audio.h) the sound setting stored in the save (options menu)
typedef enum SoundSetting {
    /* 0 */ SOUND_SETTING_STEREO,
    /* 1 */ SOUND_SETTING_MONO,
    /* 2 */ SOUND_SETTING_HEADSET,
    /* 3 */ SOUND_SETTING_SURROUND
} SoundSetting;

void Audio_Update(void);
void Audio_SetSfxProperties(u8 bankId, u8 entryIdx, u8 channelIndex);
void Audio_PlayCutsceneEffectsSequence(u8 csEffectType);
void func_800F4010(Vec3f* pos, u16 sfxId, f32);
void Audio_PlaySfxRandom(Vec3f* pos, u16 baseSfxId, u8 randLim);
void func_800F4138(Vec3f* pos, u16 sfxId, f32);
void func_800F4190(Vec3f* pos, u16 sfxId);
void func_800F436C(Vec3f* pos, u16 sfxId, f32 arg2);
void func_800F4414(Vec3f* pos, u16 sfxId, f32);
void func_800F44EC(s8 arg0, s8 arg1);
void func_800F4524(Vec3f* pos, u16 sfxId, s8 arg2);
void Audio_PlaySwordChargeSfx(Vec3f* pos, u8 level);
void Audio_PlaySfxRiver(Vec3f* pos, f32 freqScale);
void Audio_PlaySfxWaterfall(Vec3f* pos, f32 freqScale);
void Audio_SetBgmVolumeOffDuringFanfare(void);
void Audio_SetBgmVolumeOnDuringFanfare(void);
void Audio_SetMainBgmVolume(u8 targetVol, u8 volFadeTimer);
void Audio_SetVarkonsTowerBgmVolumeLevel(u8 varkonsTowerLevel);
void Audio_LowerMainBgmVolume(u8 volume);
void Audio_PlaySfxIncreasinglyTransposed(Vec3f* pos, s16 sfxId, u8* semitones);
void Audio_ResetIncreasingTranspose(void);
void Audio_PlaySfxTransposed(Vec3f* pos, u16 sfxId, s8 semitone);
void func_800F4C58(Vec3f* pos, u16 sfxId, u8);
void func_800F4E30(Vec3f* pos, f32);
void Audio_ClearSylvaBgm(void);
void Audio_ClearSylvaBgmAtPos(Vec3f* pos);
void Audio_PlaySylvaBgm(Vec3f* pos, u16 seqId, u16 distMax);
void Audio_ClearSylvaBgm2(void);
void Audio_PlayMorningSceneSequence(u16 seqId);
void Audio_PlaySceneSequence(u16 seqId);
void Audio_SetMainBgmTempoFreqAfterFanfare(f32 scaleTempoAndFreq, u8 duration);
void Audio_PlayWindmillBgm(void);
void Audio_SetFastTempoForTimedMinigame(void);
void Audio_PlaySequenceInCutscene(u16 seqId);
void Audio_StopSequenceInCutscene(u16 seqId);
s32 Audio_IsSequencePlaying(u16 seqId);
void func_800F5ACC(u16 seqId);
void func_800F5B58(void);
void func_800F5BF0(u8 natureAmbienceId);
void Audio_PlayFanfare(u16);
void func_800F5C2C(void);
void Audio_PlaySequenceWithSeqPlayerIO(u8 seqPlayerIndex, u16 seqId, u8 fadeInDuration, s8 ioPort, s8 ioData);
void Audio_SetSequenceMode(u8 seqMode);
void Audio_SetBgmEnemyVolume(f32 dist);
void Audio_UpdateMerylSinging(f32 dist, u16 seqId);
void func_800F64E0(u8 arg0);
void Audio_ToggleMerylSinging(u8 merylSingingDisabled);
void Audio_SetEnvReverb(s8 reverb);
void Audio_SetCodeReverb(s8 reverb);
void Audio_SetSoundOutputMode(s8 soundSetting);
void Audio_SetBaseFilter(u8);
void Audio_SetExtraFilter(u8);
void Audio_SetCutsceneFlag(s8 flag);
void Audio_PlaySfxIfNotInCutscene(u16 sfxId);
void func_800F6964(u16);
void Audio_StopBgmAndFanfare(u16 fadeOutDuration);
void func_800F6B3C(void);
void func_800F6BDC(void);
void Audio_PreNMI(void);
void Audio_SetNatureAmbienceChannelIO(u8 channelIdxRange, u8 ioPort, u8 ioData);
void Audio_PlayNatureAmbienceSequence(u8 natureAmbienceId);
void Audio_Init(void);
void Audio_InitSound(void);
void func_800F7170(void);
void func_800F71BC(s32 arg0);

// Commonly referenced game-side audio globals
extern u8 gSoundOutputModes[];
extern u8 gAudioSpecId;

#endif
