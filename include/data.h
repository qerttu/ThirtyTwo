#ifndef DATA_H
#define DATA_H

#include "app_defs.h"

#define VERSION "1.16.1"
#define TRACK_SETTINGS_SIZE 5
#define SYSEX_U32_SIZE 5
#define GLOBAL_SETTINGS_SIZE 10 // 2 * SYSEX_U32_SIZE
#define SETTINGS_SIZE 170 // TRACK_COUNT * TRACK_SETTINGS_SIZE + GLOBAL_SETTINGS_SIZE
#define SETTINGS_SLOT_COUNT 6
//#define SEQ_SAVE_MAX_SIZE 101 // SYSEX_U32_SIZE + MAX_SEQ_LENGTH * 3
#define SEQ_SAVE_MAX_SIZE 133 // SYSEX_U32_SIZE + MAX_SEQ_LENGTH * 4 (include offset)
#define MUTE_SETTINGS_SIZE 5


void formatSysexU32(u32 ul, u8* data);

u32 parseSysexU32(u8* data);

void formatMuteData(u8 *data, u8 mt);

void formatGlobalSettings(u8 *data);

void parseGlobalSettings(u8 *data);

void parseMuteSettings(u8 mt, u8 *data);

void formatTrackSettings(u8 tr, u8 *data);

void parseTrackSettings(u8 tr, u8 *data);

u8 formatTrackSequence(u8 tr, u8 *data);

void parseTrackSequence(u8 tr, u8 *data);

void loadSettings(u8 slot);

void saveSettings(u8 slot);

void deleteSettings(u8 slot);

u8 isSettingsEmpty(u8 slot);

void loadLastSettings();

void onDataReceive(u8 *data, u16 count);

void sendTrackData(u8 port, u8 tr);

void sendGlobalSettingsData(u8 port);

void sendStartMessage(u8 port);

void sendEndMessage(u8 port);

void sendTrackMuteData(u8 port, u8 mt);

void loadProject(u8 port, u8 pt);

void requestProjectStatus(u8 port);

void requestMidiClockStart(u8 port);

void requestMidiClockStop(u8 port);

void requestSceneTempo(u8 port, u8 sc);

void viewProjectName(u8 port, u8 sc);

void deleteProject(u8 port, u8 pt);

#endif
