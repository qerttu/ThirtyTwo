#ifndef SEQUENCE_H
#define SEQUENCE_H

#include "app_defs.h"

#define TRACK_COUNT 32
#define MAX_SEQ_LENGTH 32

//MK: scene count
#define SCENE_COUNT 6

#define GATE_FULL 6
#define GATE_TIE 7

#define FAKE_PORT 255

typedef struct {
  u8 value;
  u8 velocity;
  u8 gate;
} MIDI_NOTE;

typedef enum { STOPPED, STARTING, PLAYING, STOPPING } CLOCK_STATE;

typedef enum { EDIT, MUTE, REPEAT, SAVE, CLEAR } SETUP_PAGE;

static const u8 AVAILABLE_STEP_SIZES[8][7] = {//10% 25% 33% 50% 66% 75% 100%
                                               { 1,  1,  1,  2,  2,  2,  3},
                                               { 1,  1,  1,  2,  3,  3,  4},
                                               { 1,  2,  2,  3,  4,  5,  6},
                                               { 1,  2,  3,  4,  5,  6,  8},
                                               { 1,  3,  4,  6,  8,  9, 12},
                                               { 2,  4,  5,  8, 11, 12, 16},
                                               { 2,  6,  8, 12, 16, 18, 24},
                                               { 5, 12, 16, 24, 32, 36, 48}};

// tempi are actually periods in milliseconds between each 24ppqn clock event
static const u8 AVAILABLE_TEMPI[MAX_SEQ_LENGTH] = {83, 71, 63, 56, 50, 45, 42, 38,
                                                   36, 33, 31, 29, 28, 26, 25, 24,
                                                   23, 22, 21, 20, 19, 18, 17, 16,
                                                   15, 14, 13, 12, 11, 10,  9,  8};

extern u8 seqPlay;
extern u8 seqPlayHeads[TRACK_COUNT];
extern CLOCK_STATE clockState;
extern s8 trackSelectStart;
extern u8 trackSelectEnd;
extern u32 clearTrackArm;
extern SETUP_PAGE setupPage;
extern u8 track;

//MK: added scene for mutes
extern u8 scene;
extern u32 stepRepeat;
extern s8 repeatStart;
extern u8 repeatLength;

//MK: trackmutes for scenes
extern u32 trackMute[SCENE_COUNT];
extern u32 trackRepeat;
extern u8 clockOut;
extern u8 tempo;

extern MIDI_NOTE notes[TRACK_COUNT][MAX_SEQ_LENGTH];
extern MIDI_NOTE inputNotes[TRACK_COUNT];
extern u8 octave[TRACK_COUNT];
extern u8 channel[TRACK_COUNT];
extern u8 seqLength[TRACK_COUNT];
extern u8 midiPort[TRACK_COUNT];
extern u8 stepSize[TRACK_COUNT];
extern u8 drumTrack[TRACK_COUNT]; // 0 = keyboard, 1 = drumpads



//////// EDIT ////////

u8 trackContainsNotes(u8 i);

void clearTrack(u8 trk);

void updateMidiPort(u8 trk, u8 mp);

void toggleClockOutPort(u8 mp);

void updateTrackChannel(u8 trk, u8 newChannel);

void rotateTrackLeft(u8 trk);

void rotateTrackRight(u8 trk);

void updateRepeat();

void updateTrackMute(u8 trk);

void updateTrackRepeat(u8 trk);

void updateTrackSelect(u8 trk, u8 value);

void updateTrackStepSize(u8 trk, u8 stepSizeIndex);


//////// PLAY ////////

void playLiveNote(u8 index, u8 value);

void stopPlayedNote(u8 i);

void onMidiReceive(u8 port, u8 status, u8 d1, u8 d2);

void handleInternalClock();

void initSequence();

#endif
