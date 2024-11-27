#ifndef SEQUENCE_H
#define SEQUENCE_H

#include "app_defs.h"

//#define TRACK_COUNT 32
#define TRACK_COUNT 32
#define MAX_SEQ_LENGTH 32
#define SCENE_COUNT 8
#define PC_COUNT 3
#define PROJECT_COUNT 32
#define DRUMPAD_COUNT 16

#define GATE_FULL 6
#define GATE_TIE 7
#define OFFSET_MP 8

#define FAKE_PORT 255

typedef struct {
  u8 value;
  u8 velocity;
  u8 gate;
  u8 offset;
} MIDI_NOTE;

typedef struct {
	u8 value; // 0-127
	u8 bank; // 0-127
} PC_MESSAGE;

typedef struct {
	u8 channel; // 1-16
	u8 status; // 0 = inactive, 1 = active
} PC_SLOT;

typedef enum { STOPPED, STARTING, PLAYING, STOPPING } CLOCK_STATE;

typedef enum {SAVE, EDIT, SEQ, MUTE, CLEAR, REPEAT } SETUP_PAGE;

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

//MK: added project
extern u8 project;

//MK: flag for arming record during playl tive
extern u8 recordArm;

//timer for offset during play time
extern u32 offsetTimer;

// flag for offset record during live record
extern u8 offsetArm;

//MK: added requestClock;
extern u8 requestClock;



extern u32 stepRepeat;
extern s8 repeatStart;
extern u8 repeatLength;

extern u32 trackMute[SCENE_COUNT];
extern u32 trackRepeat;
extern u8 clockOut;
extern u8 tempo;

extern MIDI_NOTE notes[TRACK_COUNT][MAX_SEQ_LENGTH];
extern MIDI_NOTE inputNotes[TRACK_COUNT];

// MK: buffer for one note/step/track
extern MIDI_NOTE notesBuffer[TRACK_COUNT];

extern u8 octave[TRACK_COUNT];
extern u8 channel[TRACK_COUNT];
extern u8 seqLength[TRACK_COUNT];
extern u8 midiPort[TRACK_COUNT];
extern u8 stepSize[TRACK_COUNT];
extern u8 drumTrack[TRACK_COUNT]; // 0 = keyboard, 1 = drumpads
extern u8 drumMachine[TRACK_COUNT]; // 0 = volca beats, 1 = volca sample, 2 = MPC
extern u8 scene_type[SCENE_COUNT]; // push=0, momentary=1

// new PC variables
extern PC_MESSAGE pc_message[SCENE_COUNT][PC_COUNT]; // using new
extern PC_MESSAGE current_pc_message[SCENE_COUNT][PC_COUNT];
extern PC_SLOT pc_slot[PC_COUNT];

// old PC variables
extern u8 current_pc; // current program, set 1
extern u8 current_pc2; // current program, set 2
extern u8 scene_pc[SCENE_COUNT+SCENE_COUNT]; // program change message for scenes

extern u8 pc_set; // pc screens



//////// EDIT ////////

u8 trackContainsNotes(u8 i);

void clearTrack(u8 trk);

void deleteAllTracks();

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

void playLiveDrumNote(u8 index, u8 value, u8 machine);

void playMidiNote(u8 status, u8 note, u8 value);

void stopPlayedNote(u8 i);

void onMidiReceive(u8 port, u8 status, u8 d1, u8 d2);

void playNotesInBuffer();

void handleInternalClock();

void initSequence();

#endif
