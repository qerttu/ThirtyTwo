#include "sequence.h"
#include "app.h"
#include "surface.h"

u8 seqPlay;
u8 seqPlayHeads[TRACK_COUNT];
CLOCK_STATE clockState;
s8 trackSelectStart;
u8 trackSelectEnd;
u32 clearTrackArm;
SETUP_PAGE setupPage;
u8 track;
u8 scene;
u32 stepRepeat;
s8 repeatStart;
u8 repeatLength;
MIDI_NOTE currentNotes[TRACK_COUNT];
u8 seqDiv[TRACK_COUNT];
s8 newStepSize[TRACK_COUNT];
u8 clockDiv;
s8 repeatPlayHeads[TRACK_COUNT];

u32 trackMute[SCENE_COUNT];
u32 trackRepeat;
u8 clockOut;
u8 tempo;

MIDI_NOTE notes[TRACK_COUNT][MAX_SEQ_LENGTH];
MIDI_NOTE inputNotes[TRACK_COUNT];
u8 octave[TRACK_COUNT];
u8 channel[TRACK_COUNT];
u8 seqLength[TRACK_COUNT];
u8 midiPort[TRACK_COUNT];
u8 stepSize[TRACK_COUNT];
u8 drumTrack[TRACK_COUNT]; // 0 = keyboard, 1 = drumpads


u8 trackContainsNotes(u8 i) {
  for (u8 j = 0; j < MAX_SEQ_LENGTH; j++) {
    if (notes[i][j].velocity) {
      return 1;
    }
  }
  return 0;
}

void clearTrack(u8 trk) {
  if (trackContainsNotes(trk)) {
    if (clearTrackArm & (1 << trk)) {
      if (seqPlay) {
        stopPlayedNote(trk);
      }
      for (u8 i = 0; i < MAX_SEQ_LENGTH; i++) {
        notes[trk][i].velocity = 0;
        notes[trk][i].gate = GATE_FULL;
      }
    } else {
      clearTrackArm |= 1 << trk;
    }
  }
}

void updateMidiPort(u8 trk, u8 mp) {
  if (midiPort[trk] != mp) {
    if (seqPlay) {
      stopPlayedNote(trk);
    }
    midiPort[trk] = mp;
  }
}

void toggleClockOutPort(u8 mp) {
  if (clockOut & (1 << mp)) {
    clockOut &= ~(1 << mp);
  } else {
    clockOut |= 1 << mp;
  }
}

void updateTrackChannel(u8 trk, u8 newChannel) {
  if (seqPlay) {
    stopPlayedNote(trk);
  }
  channel[trk] = newChannel;
}

void rotateTrackLeft(u8 trk) {
  MIDI_NOTE firstNote = notes[trk][0];
  for (u8 i = 0; i < (MAX_SEQ_LENGTH - 1); i++) {
    if (seqPlay && seqPlayHeads[trk] == i) {
      stopPlayedNote(trk);
    }
    notes[trk][i] = notes[trk][i + 1];
  }
  if (seqPlay && seqPlayHeads[trk] == MAX_SEQ_LENGTH - 1) {
    stopPlayedNote(trk);
  }
  notes[trk][MAX_SEQ_LENGTH - 1] = firstNote;
}

void rotateTrackRight(u8 trk) {
  MIDI_NOTE lastNote = notes[trk][MAX_SEQ_LENGTH - 1];
  for (u8 i = MAX_SEQ_LENGTH - 1; i > 0; i--) {
    if (seqPlay && seqPlayHeads[trk] == i) {
      stopPlayedNote(trk);
    }
    notes[trk][i] = notes[trk][i - 1];
  }
  if (seqPlay && seqPlayHeads[trk] == 0) {
    stopPlayedNote(trk);
  }
  notes[trk][0] = lastNote;
}

void updateRepeat() {
  s8 start = -1;
  s8 end = -1;
  if (stepRepeat) {
    for (u8 i = 0; i < MAX_SEQ_LENGTH; i++) {
      if (stepRepeat & 1 << i) {
        if (start < 0) {
          start = end = i;
        } else {
          end = i;
        }
      }
    }
  }
  repeatStart = start;
  repeatLength = end - start + 1;
  if (repeatStart >= 0) {
    for (u8 i = 0; i < TRACK_COUNT; i++) {
      if (repeatPlayHeads[i] >= 0) {
        repeatPlayHeads[i] = (repeatPlayHeads[i] - repeatStart) % repeatLength + repeatStart;
      }
    }
  } else {
    for (u8 i = 0; i < TRACK_COUNT; i++) {
      repeatPlayHeads[i] = -1;
    }
  }
}

//MK: modified for scenes
void updateTrackMute(u8 trk) {
  if (trackMute[scene] & (1 << trk)) {
    trackMute[scene] &= ~(1 << trk);
  } else {
    trackMute[scene] |= 1 << trk;
    if (seqPlay) {
      stopPlayedNote(trk);
    }
  }
}

void updateTrackRepeat(u8 trk) {
  if (trackRepeat & (1 << trk)) {
    trackRepeat &= ~(1 << trk);
  } else {
    trackRepeat |= 1 << trk;
  }
}

void updateTrackSelect(u8 trk, u8 value) {
  if (value) {
    if (trackSelectStart < 0) {
      trackSelectStart = trackSelectEnd = track = trk;
    } else {
      if (trk > trackSelectStart) {
        trackSelectEnd = trk;
      } else {
        trackSelectEnd = trackSelectStart;
      }
    }
  } else if (trk == trackSelectStart) {
    trackSelectStart = -1;
  }
}

void updateTrackStepSize(u8 trk, u8 stepSizeIndex) {
  if (stepSize[trk] != stepSizeIndex && newStepSize[trk] != stepSizeIndex) {
    if (seqPlay) {
      newStepSize[trk] = stepSizeIndex;
    } else {
      stepSize[trk] = stepSizeIndex;
      newStepSize[trk] = -1;
      if (trk == track) {
        drawSetupMode();
      }
    }
  }
}



void playLiveNote(u8 index, u8 value) {
  s8 note = indexToNote(index);
  if (note >= 0) {
    hal_send_midi(midiPort[track],
                  (value ? NOTEON : NOTEOFF) + channel[track], note,
                  value);
    if (value) {
      inputNotes[track].value = note;
      inputNotes[track].velocity = value;
    }

//    if (!stepPress) {
      if (value) { // note on
        hal_plot_led(TYPEPAD, index,
                     velocityFade(CHANNEL_COLORS[channel[track]][0], value),
                     velocityFade(CHANNEL_COLORS[channel[track]][1], value),
                     velocityFade(CHANNEL_COLORS[channel[track]][2], value));
      } else { // note off
        if (drumTrack[track]) {
          u8 evenOctave = (octave[track] % 2 == 0);
          u8 row = index / 10;
          if ((evenOctave && (row == 1 || row > 5)) || (!evenOctave && row >= 2 && row <= 5)) {
            hal_plot_led(TYPEPAD, index, BLACK_KEY_COLOR_R, BLACK_KEY_COLOR_G, BLACK_KEY_COLOR_B);
          } else {
            hal_plot_led(TYPEPAD, index, WHITE_KEY_COLOR_R, WHITE_KEY_COLOR_G, WHITE_KEY_COLOR_B);
          }
        } else {
          if (indexOf(note % 12, BLACK_NOTES, 5) >= 0) {
            hal_plot_led(TYPEPAD, index, BLACK_KEY_COLOR_R, BLACK_KEY_COLOR_G, BLACK_KEY_COLOR_B);
          } else {
            hal_plot_led(TYPEPAD, index, WHITE_KEY_COLOR_R, WHITE_KEY_COLOR_G, WHITE_KEY_COLOR_B);
          }
        }
      }
//    }
  }
}

void stopPlayedNote(u8 i) {
  if (currentNotes[i].velocity > 0) {
    hal_send_midi(midiPort[i], NOTEOFF + channel[i], currentNotes[i].value, 0);
    currentNotes[i].velocity = 0;
  }
}

void onMidiReceive(u8 port, u8 status, u8 d1, u8 d2) {
  u8 clockEvent = 0;
  if (status == MIDISTART) {
    for (u8 i = 0; i < TRACK_COUNT; i++) {
      seqDiv[i] = 0;
      seqPlayHeads[i] = seqLength[i] - 1;
    }
    seqPlay = 1;
    clockEvent = 1;
  } else if (status == MIDICONTINUE) {
    seqPlay = 1;
    clockEvent = 1;
  } else if (status == MIDISTOP) {
    updateLed(seqPlayHeads[track]);

    for (u8 i = 0; i < TRACK_COUNT; i++) {
      stopPlayedNote(i);
    }
    seqPlay = 0;
    clockEvent = 1;
  } else if (status == SONGPOSITIONPOINTER) {
    for (u8 i = 0; i < TRACK_COUNT; i++) {
      seqDiv[i] = 0;
      seqPlayHeads[i] = d1 % seqLength[i] - 1;
    }
    clockEvent = 1;
  } else if (status == MIDITIMINGCLOCK) {
    if (seqPlay) {
      for (u8 i = 0; i < TRACK_COUNT; i++) {
        u8 repeat = (!trackRepeat || (trackRepeat & (1 << i))) && stepRepeat;
        u8 prevSeqPlayHead = seqPlayHeads[i];
        u8 prevPlayHead = !repeat || repeatPlayHeads[i] < 0 ? prevSeqPlayHead : repeatPlayHeads[i];
        u8 prevGate = notes[i][prevPlayHead].gate;
        if (prevGate != GATE_TIE
            && (seqDiv[i] == 0 || seqDiv[i] == AVAILABLE_STEP_SIZES[stepSize[i]][currentNotes[i].gate])) {
          stopPlayedNote(i);
        }

        if (seqDiv[i] == 0) {
          seqPlayHeads[i] = (seqPlayHeads[i] + 1) % seqLength[i];
          u8 newPlayHead;
          if (repeat) {
            if (repeatPlayHeads[i] < 0) {
              repeatPlayHeads[i] = repeatStart;
            } else {
              repeatPlayHeads[i] = (repeatPlayHeads[i] - repeatStart + 1) % repeatLength + repeatStart;
            }
            newPlayHead = repeatPlayHeads[i];
          } else {
            newPlayHead = seqPlayHeads[i];
          }

          MIDI_NOTE note = notes[i][newPlayHead];
          //MK: modified for scenes
          if (!(trackMute[scene] & (1 << i))) {
            if (note.velocity && (prevGate != GATE_TIE || note.value != currentNotes[i].value)) {
              hal_send_midi(midiPort[i], NOTEON + channel[i], note.value, note.velocity);
            }
            if (prevGate == GATE_TIE && (!note.velocity || note.value != currentNotes[i].value)) {
              stopPlayedNote(i);
            }
            currentNotes[i] = note;
          }
          if (i == track) {
            if (repeat) {
              updateLed(prevSeqPlayHead);
            }
            updateLed(prevPlayHead);
            updatePlayHeadLed(newPlayHead);
          }
          if (seqPlayHeads[i] == 0) {
            if (newStepSize[i] >= 0) {
              stepSize[i] = newStepSize[i];
              newStepSize[i] = -1;
              refreshGrid();
            }
          }
        }
        seqDiv[i] = (seqDiv[i] + 1) % AVAILABLE_STEP_SIZES[stepSize[i]][GATE_FULL];
      }
      clockEvent = 1;
    }
  }

  //MK: pass thru also other stuff than clock events

  //if (clockEvent) {
    for (u8 i = 0; i < 3; i++) {
      if (clockOut & (1 << i)) {
        hal_send_midi(i, status, d1, d2);
      }
    }
  //}
}

void handleInternalClock() {
  switch (clockState) {
  case STOPPED:
    break;

  case STARTING:
    onMidiReceive(FAKE_PORT, MIDISTART, 0, 0);
    clockState = PLAYING;
    break;

  case PLAYING:
    if (clockDiv == 0) {
      onMidiReceive(FAKE_PORT, MIDITIMINGCLOCK, 0, 0);
    }
    clockDiv = (clockDiv + 1) % AVAILABLE_TEMPI[tempo];
    break;

  case STOPPING:
    onMidiReceive(FAKE_PORT, MIDISTOP, 0, 0);
    clockDiv = 0;
    clockState = STOPPED;
    drawSetupMode();
    break;
  }
}

void initSequence() {
  seqPlay = 0;
  clockState = STOPPED;
  trackSelectStart = -1;
  trackSelectEnd = 0;
  clearTrackArm = 0;
  setupPage = EDIT;
  track = 0;
  scene = 0;
  stepRepeat = 0;
  repeatStart = -1;
  repeatLength = 0;
  clockDiv = 0;

  //MK: init trackmutes for all scenes
  for (u8 i=0; i<SCENE_COUNT;i++){
	  trackMute[i] = 0;
  }

  trackRepeat = 0;
  clockOut = 1;
  tempo = 19; // index for 20ms

  for (u8 i = 0; i < TRACK_COUNT; i++) {
    seqPlayHeads[i] = seqLength[i] - 1;
    currentNotes[i] = (MIDI_NOTE){.value = 0, .velocity = 0, .gate = 0};
    newStepSize[i] = -1;
    repeatPlayHeads[i] = -1;
  }
}
