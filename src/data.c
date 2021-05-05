#include "data.h"
#include "app.h"
#include "sequence.h"
#include "surface.h"

void formatSysexU32(u32 ul, u8* data) {
  data[0] = ul >> 25;
  data[1] = (ul >> 18) & 0x7F;
  data[2] = (ul >> 11) & 0x7F;
  data[3] = (ul >> 4) & 0x7F;
  data[4] = ul & 0x0F;
}

u32 parseSysexU32(u8* data) {
  return 0xFFFFFFFF & (data[0] << 25 | data[1] << 18 | data[2] << 11 | data[3] << 4 | data[4]);
}

void formatGlobalSettings(u8 *data) {
  //MK: to-do--> support for all scenes
  formatSysexU32(trackMute[0], data);
  formatSysexU32(trackRepeat, data + SYSEX_U32_SIZE);
}

void parseGlobalSettings(u8 *data) {
  //MK: to-do--> support for all scenes
  trackMute[0] = parseSysexU32(data);
  trackRepeat = parseSysexU32(data + SYSEX_U32_SIZE);
}

void formatTrackSettings(u8 tr, u8 *data) {
  data[0] = inputNotes[tr].value;
  data[1] = inputNotes[tr].velocity;
  data[2] = drumTrack[tr] << 6 | stepSize[tr] << 3 | inputNotes[tr].gate;
  data[3] = octave[tr] << 4 | channel[tr];
  data[4] = midiPort[tr] << 5 | (seqLength[tr] - 1);
}

void parseTrackSettings(u8 tr, u8 *data) {
  inputNotes[tr].value = data[0];
  inputNotes[tr].velocity = data[1];
  inputNotes[tr].gate = data[2] & 0x07;
  stepSize[tr] = (data[2] >> 3) & 0x07;
  drumTrack[tr] = data[2] >> 6;
  octave[tr] = data[3] >> 4;
  channel[tr] = data[3] & 0x0F;
  midiPort[tr] = data[4] >> 5;
  seqLength[tr] = (data[4] & 0X1F) + 1;
}

u8 formatTrackSequence(u8 tr, u8 *data) {
  u32 steps = 0;
  u8 size = 0;
  for (u8 i = 0; i < MAX_SEQ_LENGTH; i++) {
    MIDI_NOTE note = notes[tr][i];
    if (note.velocity) {
      steps |= 1 << i;
      data[SYSEX_U32_SIZE + size] = note.value;
      data[SYSEX_U32_SIZE + size + 1] = note.velocity;
      data[SYSEX_U32_SIZE + size + 2] = note.gate;
      size += 3;
    }
  }
  formatSysexU32(steps, data);
  return SYSEX_U32_SIZE + size;
}

void parseTrackSequence(u8 tr, u8 *data) {
  u32 steps = parseSysexU32(data);
  u8 index = 0;
  for (u8 i = 0; i < MAX_SEQ_LENGTH; i++) {
    MIDI_NOTE note = (MIDI_NOTE){.value = 0, .velocity = 0, .gate = GATE_FULL};
    if (steps & (1 << i)) {
      note.value = data[SYSEX_U32_SIZE + index];
      note.velocity = data[SYSEX_U32_SIZE + index + 1];
      note.gate = data[SYSEX_U32_SIZE + index + 2];
      index += 3;
    }
    notes[tr][i] = note;
  }
}

void loadSettings(u8 slot) {
  u8 settings[ SETTINGS_SIZE ];
  hal_read_flash(slot * SETTINGS_SIZE, settings, SETTINGS_SIZE);
  for (u8 i = 0; i < TRACK_COUNT; i++) {
    parseTrackSettings(i, &settings[i * TRACK_SETTINGS_SIZE]);
  }
  parseGlobalSettings(&settings[TRACK_COUNT * TRACK_SETTINGS_SIZE]);
}

void saveSettings(u8 slot) {
  u8 settings[ SETTINGS_SLOT_COUNT * SETTINGS_SIZE ];
  hal_read_flash(0, settings, SETTINGS_SLOT_COUNT * SETTINGS_SIZE);
  for (u8 i = 0; i < TRACK_COUNT; i++) {
    formatTrackSettings(i, &settings[slot * SETTINGS_SIZE + i * TRACK_SETTINGS_SIZE]);
  }
  formatGlobalSettings(&settings[slot * SETTINGS_SIZE + TRACK_COUNT * TRACK_SETTINGS_SIZE]);
  hal_write_flash(0, settings, SETTINGS_SLOT_COUNT * SETTINGS_SIZE);
}

void deleteSettings(u8 slot) {
  u8 settings[ SETTINGS_SLOT_COUNT * SETTINGS_SIZE ];
  hal_read_flash(0, settings, SETTINGS_SLOT_COUNT * SETTINGS_SIZE);
  settings[slot * SETTINGS_SIZE] |= 0x80;
  hal_write_flash(0, settings, SETTINGS_SLOT_COUNT * SETTINGS_SIZE);
}

u8 isSettingsEmpty(u8 slot) {
  u8 settings[1];
  hal_read_flash(slot * SETTINGS_SIZE, settings, 1);
  return settings[0] >> 7;
}

void loadLastSettings() {
  int i = SETTINGS_SLOT_COUNT - 1;
  while (i >= 0) {
    if (!isSettingsEmpty(i)) {
      loadSettings(i);
      return;
    }
    i--;
  }
  // default settings
  for (u8 i = 0; i < TRACK_COUNT; i++) {
    seqLength[i] = MAX_SEQ_LENGTH;
    inputNotes[i] = (MIDI_NOTE){.value = 60, .velocity = 100, .gate = GATE_FULL};
    octave[i] = 3;
    channel[i] = i % 16;
    stepSize[i] = 2;
    midiPort[i] = i < 16 ? USBSTANDALONE : USBMIDI;
    drumTrack[i] = 0;
  }
}


void onDataReceive(u8 *data, u16 count) {
  if (data[1] != 0x00 || data[2] != 0x20 || data[3] != 0x29) { // 00H 20H 29H -> Focusrite/Novation manufacturer ID
    return;
  }
  u8 tr = data[4];
  if (tr == 0x7F) {
    parseGlobalSettings(&data[5]);
  } else if (tr >= MAX_SEQ_LENGTH) {
    return;
  } else {
    parseTrackSettings(tr, &data[5]);
    parseTrackSequence(tr, &data[5 + TRACK_SETTINGS_SIZE]);
    if (tr == track) {
      drawSeqSteps();
      drawNotePads();
    }
  }
  drawSetupMode();
}

void sendTrackData(u8 port, u8 tr) {
  u8 data[6 + TRACK_SETTINGS_SIZE + SEQ_SAVE_MAX_SIZE];
  data[0] = 0xF0;
  // 00H 20H 29H -> Focusrite/Novation manufacturer ID
  data[1] = 0x00;
  data[2] = 0x20;
  data[3] = 0x29;
  data[4] = tr;
  formatTrackSettings(tr, &data[5]);
  u16 length = 6 + TRACK_SETTINGS_SIZE + formatTrackSequence(tr, &data[5 + TRACK_SETTINGS_SIZE]);
  data[length - 1] = 0xF7;
  hal_send_sysex(port, data, length);
}

void sendGlobalSettingsData(u8 port) {
  u8 data[6 + GLOBAL_SETTINGS_SIZE];
  data[0] = 0xF0;
  // 00H 20H 29H -> Focusrite/Novation manufacturer ID
  data[1] = 0x00;
  data[2] = 0x20;
  data[3] = 0x29;
  data[4] = 0x7F;
  formatGlobalSettings(&data[5]);
  data[5 + GLOBAL_SETTINGS_SIZE] = 0xF7;
  hal_send_sysex(port, data, 6 + GLOBAL_SETTINGS_SIZE);
}
