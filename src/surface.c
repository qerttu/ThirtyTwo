#include "surface.h"
#include "app.h"
#include "sequence.h"
#include "data.h"

u32 stepPress = 0;
u16 stepPressTimes[MAX_SEQ_LENGTH] = {0};
u16 settingsSlotPressTimes[8] = {0};
u8 setupMode = 0;
u8 setupButtonPressed = 0;
u8 tempoSelect = 0;
u8 sysexMidiPort = USBSTANDALONE;


s8 indexOf(u8 element, const u8 array[], u8 arraysize) {
  for (u8 i = 0; i < arraysize; i++) {
    if (array[i] == element) {
      return i;
    }
  }
  return -1;
}

s8 noteToIndex(u8 note) {
  if (drumTrack[track]) {
    u8 noteOctave = note / 16;
    u8 relNote = note % 16;
    if (noteOctave >= octave[track] && noteOctave <= octave[track] + 1) {
      return 13 + 10 * (relNote / 4) + (relNote % 4) + (noteOctave - octave[track]) * 40;
    }
  } else {
    u8 noteOctave = note / 12;
    u8 relNote = note % 12;
    if (noteOctave >= octave[track] && noteOctave <= octave[track] + 3) {
      return KEYS_INDEXES[relNote] + (noteOctave - octave[track]) * 20;
    }
  }
  return -1;
}

s8 indexToNote(u8 index) {
  if (drumTrack[track]) {
    u8 row = index / 10;
    u8 column = index % 10;
    if (row >= 1 && row <= 8 && column >= 3 && column <= 6) {
      u8 note = 16 * octave[track] + (row - 1) * 4 + (column - 3);
      return note > 127 ? -1 : note;
    }

  } else {
    u8 relOctave = ((index + 10) / 20) - 1;
    s8 relNote = indexOf(index - relOctave * 20, KEYS_INDEXES, 12);
    if (relNote >= 0) {
      u8 note = 12 * (octave[track] + relOctave) + relNote;
      return note > 127 ? -1 : note;
    }
  }
  return -1;
}



u8 velocityFade(u8 led, u8 velocity) {
  return led ? led * velocity >> 7 | 3 : 0;
}

void clearRect(u8 x1, u8 y1, u8 x2, u8 y2) {
  for (u8 i = y1; i <= y2; i++) {
    for (u8 j = x1; j <= x2; j++) {
      hal_plot_led(TYPEPAD, i * 10 + j, 0, 0, 0);
    }
  }
}

void drawNotePads() {
  if (setupMode)
    return;

  clearRect(1, 1, 8, 8);

  if (drumTrack[track]) {
    u8 evenOctave = (octave[track] % 2 == 0);
    for (u8 i = 0; i < 8; i++) {
      if ((evenOctave && (i == 0 || i > 4)) || (!evenOctave && i >= 1 && i <= 4)) {
        for (u8 j = 0; j < 4; j++) {
          hal_plot_led(TYPEPAD, 13 + i * 10 + j, BLACK_KEY_COLOR_R, BLACK_KEY_COLOR_G, BLACK_KEY_COLOR_B);
        }
      } else {
        for (u8 j = 0; j < 4; j++) {
          hal_plot_led(TYPEPAD, 13 + i * 10 + j, WHITE_KEY_COLOR_R, WHITE_KEY_COLOR_G, WHITE_KEY_COLOR_B);
        }
      }
    }
    if (octave[track] == 7) {
      clearRect(3, 5, 6, 8);
    }
  } else {
    for (u8 i = 0; i < 4; i++) {
      for (u8 j = 0; j < 7; j++) {
        hal_plot_led(TYPEPAD, i * 20 + WHITE_KEYS_INDEXES[j], WHITE_KEY_COLOR_R, WHITE_KEY_COLOR_G, WHITE_KEY_COLOR_B);
      }
      for (u8 j = 0; j < 5; j++) {
        hal_plot_led(TYPEPAD, i * 20 + BLACK_KEYS_INDEXES[j], BLACK_KEY_COLOR_R, BLACK_KEY_COLOR_G, BLACK_KEY_COLOR_B);
      }
    }
    if (octave[track] == 7) {
      clearRect(6, 7, 7, 8);
    }
  }

  if (stepPress) {
    u8 i = 0;
    while (i < MAX_SEQ_LENGTH && !(stepPress & (1 << i))) i++;
    u8 gate = notes[track][i].gate;
    for (u8 j = 0; j < 8; j++) {
      if (j == gate) {
        hal_plot_led(TYPEPAD, (j + 1) * 10 + 8, CHANNEL_COLORS[channel[track]][0],
                                                CHANNEL_COLORS[channel[track]][1],
                                                CHANNEL_COLORS[channel[track]][2]);
      } else if (j == 7) {
        hal_plot_led(TYPEPAD, (j + 1) * 10 + 8, WHITE_KEY_COLOR_R, WHITE_KEY_COLOR_G, WHITE_KEY_COLOR_B);
      } else {
        hal_plot_led(TYPEPAD, (j + 1) * 10 + 8, BLACK_KEY_COLOR_R, BLACK_KEY_COLOR_G, BLACK_KEY_COLOR_B);
      }
    }
  } else { // octave selector
    hal_plot_led(TYPEPAD, (octave[track] + 1) * 10 + 8, CHANNEL_COLORS[channel[track]][0],
                                                        CHANNEL_COLORS[channel[track]][1],
                                                        CHANNEL_COLORS[channel[track]][2]);
  }

  for (u8 i = 0; i < MAX_SEQ_LENGTH; i++) {
    if (stepPress & (1 << i)) {
      MIDI_NOTE note = notes[track][i];
      if (note.velocity) {
        s8 index = noteToIndex(note.value);
        if (index >= 0) {
          hal_plot_led(TYPEPAD, index,
                       velocityFade(CHANNEL_COLORS[channel[track]][0], note.velocity),
                       velocityFade(CHANNEL_COLORS[channel[track]][1], note.velocity),
                       velocityFade(CHANNEL_COLORS[channel[track]][2], note.velocity));
        }
      }
    }
  }
}

void drawSetupMode() {
  if (!setupMode)
    return;

  clearRect(1, 1, 8, 8);

  switch (setupPage) {
  case MUTE:
    for (u8 i = 0; i < TRACK_COUNT; i++) {
      if (trackMute & (1 << i)) {
        hal_plot_led(TYPEPAD, 81 + i - 18 * (i / 8), MUTE_COLOR_R >> 3, MUTE_COLOR_G >> 3, MUTE_COLOR_B >> 3);
      } else {
        hal_plot_led(TYPEPAD, 81 + i - 18 * (i / 8), MUTE_COLOR_R, MUTE_COLOR_G, MUTE_COLOR_B);
      }
    }
    break;

  case REPEAT:
    for (u8 i = 0; i < TRACK_COUNT; i++) {
      if (!trackRepeat || (trackRepeat & (1 << i))) {
        hal_plot_led(TYPEPAD, 81 + i - 18 * (i / 8), REPEAT_COLOR_R, REPEAT_COLOR_G, REPEAT_COLOR_B);
      } else {
        hal_plot_led(TYPEPAD, 81 + i - 18 * (i / 8), REPEAT_COLOR_R >> 3, REPEAT_COLOR_G >> 3, REPEAT_COLOR_B >> 3);
      }
    }
    break;

  case SAVE:
    for (u8 i = 0; i < TRACK_COUNT; i++) {
      if (trackContainsNotes(i)) {
        hal_plot_led(TYPEPAD, 81 + i - 18 * (i / 8), SYSEX_DUMP_COLOR_R, SYSEX_DUMP_COLOR_G, SYSEX_DUMP_COLOR_B);
      } else {
        hal_plot_led(TYPEPAD, 81 + i - 18 * (i / 8), SYSEX_DUMP_COLOR_R >> 3, SYSEX_DUMP_COLOR_G >> 3, SYSEX_DUMP_COLOR_B >> 3);
      }
    }
    hal_plot_led(TYPEPAD, 41, SYSEX_DUMP_COLOR_R, SYSEX_DUMP_COLOR_G, SYSEX_DUMP_COLOR_B);

    for (u8 i = 0; i < 3; i++) {
      if (sysexMidiPort == i) {
        hal_plot_led(TYPEPAD, 46 + i, MIDI_PORT_COLOR_R, MIDI_PORT_COLOR_G, MIDI_PORT_COLOR_B);
      } else {
        hal_plot_led(TYPEPAD, 46 + i, MIDI_PORT_COLOR_R >> 3, MIDI_PORT_COLOR_G >> 3, MIDI_PORT_COLOR_B >> 3);
      }
    }

    for (u8 i = 0; i < SETTINGS_SLOT_COUNT; i++) {
      if (isSettingsEmpty(i)) {
        hal_plot_led(TYPEPAD, 31 + i, SETTINGS_COLOR_R >> 3,
                                      SETTINGS_COLOR_G >> 3,
                                      SETTINGS_COLOR_B >> 3);
      } else {
        hal_plot_led(TYPEPAD, 31 + i, SETTINGS_COLOR_R,
                                      SETTINGS_COLOR_G,
                                      SETTINGS_COLOR_B);
      }
    }
    break;

  case CLEAR:
    for (u8 i = 0; i < TRACK_COUNT; i++) {
      if (trackContainsNotes(i)) {
        if (clearTrackArm & (1 << i)) {
          hal_plot_led(TYPEPAD, 81 + i - 18 * (i / 8), CLEAR_TRACK_ARM_COLOR_R, CLEAR_TRACK_ARM_COLOR_G, CLEAR_TRACK_ARM_COLOR_B);
        } else {
          hal_plot_led(TYPEPAD, 81 + i - 18 * (i / 8), CLEAR_TRACK_COLOR_R, CLEAR_TRACK_COLOR_G, CLEAR_TRACK_COLOR_B);
        }
      } else {
        if (clearTrackArm & (1 << i)) {
          clearTrackArm &= ~(1 << i);
        }
        hal_plot_led(TYPEPAD, 81 + i - 18 * (i / 8), CLEAR_TRACK_COLOR_R >> 3, CLEAR_TRACK_COLOR_G >> 3, CLEAR_TRACK_COLOR_B >> 3);
      }
    }
    break;

  case EDIT:
  default:
    for (u8 i = 0; i < TRACK_COUNT; i++) {
      if (track == i) {
        hal_plot_led(TYPEPAD, 81 + i - 18 * (i / 8), CHANNEL_COLORS[channel[i]][0], CHANNEL_COLORS[channel[i]][1],
                     CHANNEL_COLORS[channel[i]][2]);
      } else if (trackSelectStart < 0 || (trackSelectStart <= i && i <= trackSelectEnd)) {
        hal_plot_led(TYPEPAD, 81 + i - 18 * (i / 8), CHANNEL_COLORS[channel[i]][0] >> 3, CHANNEL_COLORS[channel[i]][1] >> 3,
                     CHANNEL_COLORS[channel[i]][2] >> 3);
      }
    }

    if (trackSelectStart >= 0) {
      // track channel
      for (u8 i = 0; i < 16; i++) {
        if (channel[track] == i) {
          hal_plot_led(TYPEPAD, 41 + i - 18 * (i / 8), CHANNEL_COLORS[i][0], CHANNEL_COLORS[i][1], CHANNEL_COLORS[i][2]);
        } else {
          hal_plot_led(TYPEPAD, 41 + i - 18 * (i / 8), CHANNEL_COLORS[i][0] >> 3, CHANNEL_COLORS[i][1] >> 3, CHANNEL_COLORS[i][2] >> 3);
        }
      }

      // track speed
      for (u8 i = 0; i < 8; i++) {
        if (i == stepSize[track]) {
          hal_plot_led(TYPEPAD, 21 + i, STEP_SIZE_COLOR_R, STEP_SIZE_COLOR_G,
                       STEP_SIZE_COLOR_B);
        } else {
          hal_plot_led(TYPEPAD, 21 + i, STEP_SIZE_COLOR_R >> 3,
                       STEP_SIZE_COLOR_G >> 3, STEP_SIZE_COLOR_B >> 3);
        }
      }

      // rotate track
      hal_plot_led(TYPEPAD, 11, ROTATE_LEFT_COLOR_R, ROTATE_LEFT_COLOR_G, ROTATE_LEFT_COLOR_B);
      hal_plot_led(TYPEPAD, 12, ROTATE_RIGHT_COLOR_R, ROTATE_RIGHT_COLOR_G, ROTATE_RIGHT_COLOR_B);

      // track mode
      if (drumTrack[track]) {
        hal_plot_led(TYPEPAD, 14, TRACK_MODE_COLOR_R, TRACK_MODE_COLOR_G, TRACK_MODE_COLOR_B);
      } else {
        hal_plot_led(TYPEPAD, 14, TRACK_MODE_COLOR_R >> 3, TRACK_MODE_COLOR_G >> 3, TRACK_MODE_COLOR_B >> 3);
      }

      // MIDI out selection
      for (u8 i = 0; i < 3; i++) {
        if (midiPort[track] == i) {
          hal_plot_led(TYPEPAD, 16 + i, MIDI_PORT_COLOR_R, MIDI_PORT_COLOR_G,
                       MIDI_PORT_COLOR_B);
        } else {
          hal_plot_led(TYPEPAD, 16 + i, MIDI_PORT_COLOR_R >> 3,
                       MIDI_PORT_COLOR_G >> 3, MIDI_PORT_COLOR_B >> 3);
        }
      }

    } else {
      // MIDI clock out
      for (u8 i = 0; i < 3; i++) {
        if (clockOut & (1 << i)) {
          hal_plot_led(TYPEPAD, 26 + i, MIDI_PORT_COLOR_R, MIDI_PORT_COLOR_G, MIDI_PORT_COLOR_B);
        } else {
          hal_plot_led(TYPEPAD, 26 + i, MIDI_PORT_COLOR_R >> 3, MIDI_PORT_COLOR_G >> 3, MIDI_PORT_COLOR_B >> 3);
        }
      }
    }
  }

  if (trackSelectStart < 0) {
    // settings page
    for (u8 i = 0; i < 5; i++) {
      if (setupPage == i) {
        hal_plot_led(TYPEPAD, 11 + i, SETUP_PAGE_COLOR_R, SETUP_PAGE_COLOR_G,
                     SETUP_PAGE_COLOR_B);
      } else {
        hal_plot_led(TYPEPAD, 11 + i, SETUP_PAGE_COLOR_R >> 3,
                     SETUP_PAGE_COLOR_G >> 3, SETUP_PAGE_COLOR_B >> 3);
      }
    }

    // internal clock
    if (clockState == STOPPED) {
      hal_plot_led(TYPEPAD, 17, CLOCK_STATE_COLOR_R >> 3, CLOCK_STATE_COLOR_G >> 3, CLOCK_STATE_COLOR_B >> 3);
    } else {
      hal_plot_led(TYPEPAD, 17, CLOCK_STATE_COLOR_R, CLOCK_STATE_COLOR_G, CLOCK_STATE_COLOR_B);
    }

    // tempo select
    hal_plot_led(TYPEPAD, 18, TEMPO_SELECT_COLOR_R, TEMPO_SELECT_COLOR_G, TEMPO_SELECT_COLOR_B);
  }
}

void drawSeqDepSetupPage() {
  if (setupPage == SAVE || setupPage == CLEAR) {
    drawSetupMode();
  }
}

void refreshGrid() {
  if (setupMode) {
    drawSetupMode();
  } else {
    drawNotePads();
  }
}

void updateLed(u8 index) {
  if (tempoSelect) {
    if (index == tempo) {
      hal_plot_led(TYPEPAD, SEQ_INDEXES[index], TEMPO_SELECT_COLOR_R, TEMPO_SELECT_COLOR_G,
                   TEMPO_SELECT_COLOR_B);
    } else {
      hal_plot_led(TYPEPAD, SEQ_INDEXES[index], 62 - index * 2, index * 2, 0);
    }
    return;
  } else if (setupMode && setupPage == REPEAT) {
    if (index == repeatStart || index == repeatStart + repeatLength - 1) {
      hal_plot_led(TYPEPAD, SEQ_INDEXES[index], MAXLED, MAXLED, MAXLED);
      return;
    }
  }

  u8 velocity = notes[track][index].velocity;
  if (trackSelectStart >= 0 && index < seqLength[track]) {
    if (velocity) {
      hal_plot_led(TYPEPAD, SEQ_INDEXES[index],
                   MAXLED - velocityFade(CHANNEL_COLORS[channel[track]][0], velocity),
                   MAXLED - velocityFade(CHANNEL_COLORS[channel[track]][1], velocity),
                   MAXLED - velocityFade(CHANNEL_COLORS[channel[track]][2], velocity));
    } else {
      hal_plot_led(TYPEPAD, SEQ_INDEXES[index], MAXLED, MAXLED, MAXLED);
    }
  } else {
    if (velocity) {
      hal_plot_led(TYPEPAD, SEQ_INDEXES[index],
                   velocityFade(CHANNEL_COLORS[channel[track]][0], velocity),
                   velocityFade(CHANNEL_COLORS[channel[track]][1], velocity),
                   velocityFade(CHANNEL_COLORS[channel[track]][2], velocity));
    } else {
      hal_plot_led(TYPEPAD, SEQ_INDEXES[index], 0, 0, 0);
    }
  }
}

void updatePlayHeadLed(u8 index) {
  if (tempoSelect) {
    hal_plot_led(TYPEPAD, SEQ_INDEXES[index], MAXLED, MAXLED, MAXLED);
    return;
  }

  u8 velocity = notes[track][index].velocity;
  if (trackSelectStart >= 0 && index < seqLength[track]) {
    if (velocity) {
      hal_plot_led(TYPEPAD, SEQ_INDEXES[index],
                   velocityFade(CHANNEL_COLORS[channel[track]][0], velocity),
                   velocityFade(CHANNEL_COLORS[channel[track]][1], velocity),
                   velocityFade(CHANNEL_COLORS[channel[track]][2], velocity));
    } else {
      hal_plot_led(TYPEPAD, SEQ_INDEXES[index], 0, 0, 0);
    }
  } else {
    if (velocity) {
      hal_plot_led(TYPEPAD, SEQ_INDEXES[index],
                   MAXLED - velocityFade(CHANNEL_COLORS[channel[track]][0], velocity),
                   MAXLED - velocityFade(CHANNEL_COLORS[channel[track]][1], velocity),
                   MAXLED - velocityFade(CHANNEL_COLORS[channel[track]][2], velocity));
    } else {
      hal_plot_led(TYPEPAD, SEQ_INDEXES[index], MAXLED, MAXLED, MAXLED);
    }
  }
}

void drawSeqSteps() {
  for (u8 i = 0; i < MAX_SEQ_LENGTH; i++) {
    updateLed(i);
  }
  if (seqPlay) {
    updatePlayHeadLed(seqPlayHeads[track]);
  }
}



void onSeqTouch(u8 index, u8 value) {
  if (tempoSelect && value) {
    tempo = index;
    drawSeqSteps();
  } else if (setupMode && setupPage == REPEAT) {
    if (value) {
      stepRepeat |= 1 << index;
    } else {
      stepRepeat &= ~(1 << index);
    }
    updateRepeat();
    drawSeqSteps();
  } else if (trackSelectStart >= 0 && value) {
    for (u8 i = trackSelectStart; i <= trackSelectEnd; i++) {
      seqLength[i] = index + 1;
    }
    drawSeqSteps();
  } else {
    if (value) { // pressed
      if (notes[track][index].velocity) {
        stepPressTimes[index] = 500;
        inputNotes[track] = notes[track][index];
      } else {
        notes[track][index] = inputNotes[track];
        drawSeqSteps();
        drawSeqDepSetupPage();
      }
      stepPress |= 1 << index;
      drawNotePads();
    } else { // released
      if (stepPressTimes[index]) {
        if (seqPlay && seqPlayHeads[track] == index) {
          stopPlayedNote(track);
        }
        notes[track][index].velocity = 0;
        notes[track][index].gate = GATE_FULL;
        drawSeqSteps();
        drawSeqDepSetupPage();
      }
      stepPress &= ~(1 << index);
      drawNotePads();
    }
  }
}

void onTrackSettingsGridTouch(u8 index, u8 value) {
  if (value && index >= 31 && index <= 48) {
    for (u8 i = trackSelectStart; i <= trackSelectEnd; i++) {
      updateTrackChannel(i, 31 + index - 18 * (index / 10));
    }
    drawSetupMode();
  } else if (value && index >= 21 && index <= 28) {
    for (u8 i = trackSelectStart; i <= trackSelectEnd; i++) {
      updateTrackStepSize(i, index - 21);
    }
  } else if (value && index == 11) {
    for (u8 i = trackSelectStart; i <= trackSelectEnd; i++) {
      rotateTrackLeft(i);
    }
    drawSeqSteps();
  } else if (value && index == 12) {
    for (u8 i = trackSelectStart; i <= trackSelectEnd; i++) {
      rotateTrackRight(i);
    }
    drawSeqSteps();
  } else if (value && index == 14) {
    u8 newDrumTrack = drumTrack[track] ? 0 : 1;
    for (u8 i = trackSelectStart; i <= trackSelectEnd; i++) {
      drumTrack[i] = newDrumTrack;
    }
    drawSetupMode();
  } else if (value && index >= 16 && index <= 18) {
    for (u8 i = trackSelectStart; i <= trackSelectEnd; i++) {
      updateMidiPort(i, index - 16);
    }
    drawSetupMode();
  }
}

void onSetupGridTouch(u8 index, u8 value) {
  if (value && setupButtonPressed) {
    setupMode |= 0x80;
  }

  if (trackSelectStart < 0) {
    // Bottom row
    if (value && index >= 11 && index <= 15) {
      setupPage = index - 11;
      if (setupPage == CLEAR) {
        clearTrackArm = 0;
      }
      if (setupPage != REPEAT && stepRepeat) {
        stepRepeat = 0;
        updateRepeat();
        drawSeqSteps();
      }
      drawSetupMode();
    } else if (value && index == 17) {
      if (!seqPlay && clockState == STOPPED) {
        clockState = STARTING;
        drawSetupMode();
      } else if (clockState == PLAYING) {
        clockState = STOPPING;
      }
    } else if (index == 18) {
      tempoSelect = value;
      drawSeqSteps();
    }
  }

  if (tempoSelect) return;

  switch (setupPage) {
  case MUTE:
    if (value && index >= 51 && index <= 88) {
      updateTrackMute(63 + index - 18 * (index / 10));
      drawSetupMode();
    }
    break;

  case REPEAT:
    if (value && index >= 51 && index <= 88) {
      updateTrackRepeat(63 + index - 18 * (index / 10));
      drawSetupMode();
    }
    break;

  case SAVE:
    if (value && index >= 51 && index <= 88) {
      sendTrackData(sysexMidiPort, 63 + index - 18 * (index / 10));
    } else if (value && index == 41) {
      sendGlobalSettingsData(sysexMidiPort);
    } else if (value && index >= 46 && index <= 48) {
      sysexMidiPort = index - 46;
      drawSetupMode();
    } else if (index >= 31 && index <= 36) { // 36 = 31 + SETTINGS_SLOT_COUNT - 1
      u8 slot = index - 31;
      if (value) {
        settingsSlotPressTimes[slot] = 3000;
      } else if (settingsSlotPressTimes[slot] == 0) {
        deleteSettings(slot);
        drawSetupMode();
      } else if (settingsSlotPressTimes[slot] < 2000) {
        saveSettings(slot);
        settingsSlotPressTimes[slot] = 0;
        drawSetupMode();
      } else if (!isSettingsEmpty(slot)) {
        loadSettings(slot);
        settingsSlotPressTimes[slot] = 0;
        drawSetupMode();
      }
    }
    break;

  case CLEAR:
    if (value && index >= 51 && index <= 88) {
      clearTrack(63 + index - 18 * (index / 10));
      drawSetupMode();
    }
    break;

  case EDIT:
  default:
    if (index >= 51 && index <= 88) {
      updateTrackSelect(63 + index - 18 * (index / 10), value);
      drawSetupMode();
      drawSeqSteps();
    } else if (trackSelectStart >= 0) {
      onTrackSettingsGridTouch(index, value);
    } else if (value && index >= 26 && index <= 28) {
      toggleClockOutPort(index - 26);
      drawSetupMode();
    }
  }
}

void onAnyTouch(u8 type, u8 index, u8 value) {
  switch (type) {
  case TYPEPAD: {
    s8 i = indexOf(index, SEQ_INDEXES, MAX_SEQ_LENGTH);
    if (i >= 0) { // round pad / seq step
      onSeqTouch(i, value);
    } else { // grid pad pressed/released
      if (setupMode) {
        onSetupGridTouch(index, value);
      } else { // note input mode
        if (index % 10 == 8) {  // select octave or gate
          if (value) {
            if (stepPress) {
              u8 gate = (index / 10) - 1;
              inputNotes[track].gate = gate;
              for (u8 i = 0; i < MAX_SEQ_LENGTH; i++) {
                if (stepPress & (1 << i)) {
                  notes[track][i].gate = gate;
                }
              }
            } else {
              u8 newOctave = (index / 10) - 1;
              if (!drumTrack[track] || newOctave != 7) {
                octave[track] = newOctave;
              }
            }
            drawNotePads();
          }
        } else { // note inputs
          if (stepPress) { // step record
            if (value) {
              s8 note = indexToNote(index);
              if (note >= 0) {
                inputNotes[track].value = note;
                inputNotes[track].velocity = value;
                for (u8 i = 0; i < MAX_SEQ_LENGTH; i++) {
                  if (stepPress & (1 << i)) {
                    if (seqPlay && seqPlayHeads[track] == i) {
                      stopPlayedNote(track);
                    }
                    notes[track][i] = inputNotes[track];
                  }
                }
                drawSeqSteps();
                drawNotePads();
              }
            }
          }
          if (!stepPress || !seqPlay) { // play note
            playLiveNote(index, value);
          }
        }
      }
    }
    break;
  }

  case TYPESETUP:
    if (value) {
      setupMode = setupMode ? 0 : 1;
    } else if (setupMode & 0x80) { // quick setup
      setupMode = 0;
    }
    if (setupMode && setupPage == CLEAR) {
      clearTrackArm = 0;
    }
    if (!setupMode && stepRepeat) {
      stepRepeat = 0;
      updateRepeat();
      drawSeqSteps();
    }
    refreshGrid();
    if (!setupMode && (trackSelectStart >= 0 || tempoSelect)) {
      trackSelectStart = -1;
      tempoSelect = 0;
      drawSeqSteps();
    }
    setupButtonPressed = value;
    break;
  }
}

void onAftertouch(u8 index, u8 value) {
  if (!setupMode && stepPress && value) {
    s8 note = indexToNote(index);
    if (inputNotes[track].value == note) {
      inputNotes[track].velocity = value;
      for (u8 i = 0; i < MAX_SEQ_LENGTH; i++) {
        if (stepPress & (1 << i)) {
          notes[track][i] = inputNotes[track];
        }
      }
      hal_plot_led(TYPEPAD, index, velocityFade(CHANNEL_COLORS[channel[track]][0], value),
                   velocityFade(CHANNEL_COLORS[channel[track]][1], value),
                   velocityFade(CHANNEL_COLORS[channel[track]][2], value));
      drawSeqSteps();
    }
  }
}

void updatePressTimes() {
  for (u8 i = 0; i < MAX_SEQ_LENGTH; i++) {
    if (stepPressTimes[i]) {
      stepPressTimes[i]--;
    }
  }
  for (u8 i = 0; i < 8; i++) {
    if (settingsSlotPressTimes[i]) {
      settingsSlotPressTimes[i]--;
    }
  }

}