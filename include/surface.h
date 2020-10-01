#ifndef SURFACE_H
#define SURFACE_H

#include "app_defs.h"

#define WHITE_KEY_COLOR_R 47
#define WHITE_KEY_COLOR_G 47
#define WHITE_KEY_COLOR_B 47

#define BLACK_KEY_COLOR_R 15
#define BLACK_KEY_COLOR_G 0
#define BLACK_KEY_COLOR_B 55

#define STEP_SIZE_COLOR_R 47
#define STEP_SIZE_COLOR_G 47
#define STEP_SIZE_COLOR_B 15

#define TRACK_MODE_COLOR_R 47
#define TRACK_MODE_COLOR_G 31
#define TRACK_MODE_COLOR_B 0

#define SETUP_PAGE_COLOR_R 47
#define SETUP_PAGE_COLOR_G 15
#define SETUP_PAGE_COLOR_B 15

#define MIDI_PORT_COLOR_R 15
#define MIDI_PORT_COLOR_G 31
#define MIDI_PORT_COLOR_B 47

#define ROTATE_RIGHT_COLOR_R 31
#define ROTATE_RIGHT_COLOR_G 0
#define ROTATE_RIGHT_COLOR_B 0

#define ROTATE_LEFT_COLOR_R 0
#define ROTATE_LEFT_COLOR_G 31
#define ROTATE_LEFT_COLOR_B 0

#define CLOCK_STATE_COLOR_R 47
#define CLOCK_STATE_COLOR_G 15
#define CLOCK_STATE_COLOR_B 0

#define TEMPO_SELECT_COLOR_R 0
#define TEMPO_SELECT_COLOR_G 15
#define TEMPO_SELECT_COLOR_B 47

#define MUTE_COLOR_R 47
#define MUTE_COLOR_G 47
#define MUTE_COLOR_B 7

#define CLEAR_TRACK_ARM_COLOR_R 63
#define CLEAR_TRACK_ARM_COLOR_G 63
#define CLEAR_TRACK_ARM_COLOR_B 63

#define CLEAR_TRACK_COLOR_R 63
#define CLEAR_TRACK_COLOR_G 7
#define CLEAR_TRACK_COLOR_B 0

#define SETTINGS_COLOR_R 0
#define SETTINGS_COLOR_G 7
#define SETTINGS_COLOR_B 63

#define SYSEX_DUMP_COLOR_R 0
#define SYSEX_DUMP_COLOR_G 63
#define SYSEX_DUMP_COLOR_B 31

#define REPEAT_COLOR_R 0
#define REPEAT_COLOR_G 63
#define REPEAT_COLOR_B 7

static const u8 CHANNEL_COLORS[16][3] = {{63,  0,  0}, {63, 15,  0}, {63, 31,  0}, {63, 47,  0},
                                         {63, 63,  0}, {31, 63,  0}, {15, 63,  0}, { 0, 63,  0},
                                         { 0, 63, 15}, { 0, 63, 31}, { 0, 63, 63}, { 0, 31, 63},
                                         { 0,  0, 63}, {15,  0, 31}, {31,  0, 31}, {31,  0, 15}};

static const u8 SEQ_INDEXES[32] = {95, 96, 97, 98, 89, 79, 69, 59,
                                   49, 39, 29, 19,  8,  7,  6,  5,
                                    4,  3,  2,  1, 10, 20, 30, 40,
                                   50, 60, 70, 80, 91, 92, 93, 94};

static const u8 KEYS_INDEXES[12] = {11, 22, 12, 23, 13, 14, 25, 15, 26, 16, 27, 17};

static const u8 BLACK_KEYS_INDEXES[5] = {22, 23, 25, 26, 27};

static const u8 WHITE_KEYS_INDEXES[7] = {11, 12, 13, 14, 15, 16, 17};

static const u8 BLACK_NOTES[5] = {1, 3, 6, 8, 10};

static const u8 WHITE_NOTES[7] = {0, 2, 4, 5, 7, 9, 11};



//////// UTILS ////////

s8 indexOf(u8 element, const u8 array[], u8 arraysize);

s8 noteToIndex(u8 note);

s8 indexToNote(u8 index);


//////// DRAW ////////

u8 velocityFade(u8 led, u8 velocity);

void clearRect(u8 x1, u8 y1, u8 x2, u8 y2);

void drawNotePads();

void drawSetupMode();

void drawSeqDepSetupPage();

void refreshGrid();

void updateLed(u8 index);

void updatePlayHeadLed(u8 index);

void drawSeqSteps();


//////// TOUCH ////////

void onSeqTouch(u8 index, u8 value);

void onTrackSettingsGridTouch(u8 index, u8 value);

void onSetupGridTouch(u8 index, u8 value);

void onAnyTouch(u8 type, u8 index, u8 value);

void onAftertouch(u8 index, u8 value);

void updatePressTimes();

#endif
