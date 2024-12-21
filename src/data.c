#include "data.h"
#include "app.h"
#include "sequence.h"
#include "surface.h"

// CC sends for debugging
//#define DEBUG

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

//MK: function for formating mute data
void formatMuteData(u8 *data, u8 mt) {
	  formatSysexU32(trackMute[mt], data);
}

void formatGlobalSettings(u8 *data) {
  //MK: to-do--> support for all scenes
  formatSysexU32(trackMute[0], data);
  formatSysexU32(trackRepeat, data + SYSEX_U32_SIZE);
}

void parseGlobalSettings(u8 *data) {
  //MK: to-do--> support for all scenes
  //trackMute[0] = parseSysexU32(data);
  trackRepeat = parseSysexU32(data + SYSEX_U32_SIZE);
}

//MK: function to parse mute settings scene
void parseMuteSettings(u8 mt, u8 *data) {
	  trackMute[mt] = parseSysexU32(data);
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

      //MK: added offset to export
      if (!note.offset) {
      	  note.offset = GATE_TIE;
      }
      data[SYSEX_U32_SIZE + size + 3] = note.offset;

      // increase the size by one to allow offset
      size += 4;
    }
  }
  formatSysexU32(steps, data);
  return SYSEX_U32_SIZE + size;
}

void parseTrackSequence(u8 tr, u8 *data) {
  u32 steps = parseSysexU32(data);
  u8 index = 0;
  for (u8 i = 0; i < MAX_SEQ_LENGTH; i++) {
    MIDI_NOTE note = (MIDI_NOTE){.value = 0, .velocity = 0, .gate = GATE_FULL, .offset = GATE_TIE};
    if (steps & (1 << i)) {
      note.value = data[SYSEX_U32_SIZE + index];
      note.velocity = data[SYSEX_U32_SIZE + index + 1];
      note.gate = data[SYSEX_U32_SIZE + index + 2];

      //MK: added offset to the parse
      note.offset = data[SYSEX_U32_SIZE + index + 3];

      // increase index by one to parse also offset
      index += 4;
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
    octave[i] = 0;
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


  //SCENE TYPE
  if (tr == 0x7A) {
	 u8 sc = data[5];
	 u8 sctype = data[6];
	 scene_type[sc] = sctype;
  }

  //PCs for scenes
  if (tr == 0x7B) {
	  u8 sc = data[5];
	  u8 instrument = data[6];
	  u8 pc = data[7];
	  if (sc<SCENE_COUNT) {
		  pc_message[sc][instrument].value = pc;

		#ifdef DEBUG

		  hal_send_midi(DINMIDI, CC, 1, sc);
		  hal_send_midi(DINMIDI, CC, 2, instrument);
		  hal_send_midi(DINMIDI, CC, 3, pc_message[sc][instrument].value);

		#endif
	  }
  }

  //MK: set project
  if (tr == 0x7C) {
	  u8 pt = data[5];
	  if (pt<PROJECT_COUNT) {
		 project = pt;
	  }
  }

  //MK: project file status
  if (tr == 0x7D) {
  u8 pt = data[5];
  u8 status = data[6];
	  if (pt<PROJECT_COUNT) {
		// on/off: 0/1
		if (status==0) {
			hal_plot_led(TYPEPAD, 81 + pt - 18 * (pt / 8), 0, 0, 0);
		}
		// highlight current project
		else if (pt==project) {
			hal_plot_led(TYPEPAD, 81 + pt - 18 * (pt / 8), SYSEX_DUMP_COLOR_R, SYSEX_DUMP_COLOR_G, SYSEX_DUMP_COLOR_B);
		}
		else {
		  hal_plot_led(TYPEPAD, 81 + pt - 18 * (pt / 8), SYSEX_DUMP_COLOR_R >> 3, SYSEX_DUMP_COLOR_G >> 3, SYSEX_DUMP_COLOR_B >> 3);
		}
	  }
  }

  //MK: mutesettings
  if (tr == 0x7E) {
  	u8 mt = data[5];
  	parseMuteSettings(mt,&data[6]);
    }

  if (tr == 0x7F) {
    parseGlobalSettings(&data[5]);
  }
  else if (tr >= MAX_SEQ_LENGTH) {
    return;
  }
  else {
    parseTrackSettings(tr, &data[5]);
    parseTrackSequence(tr, &data[5 + TRACK_SETTINGS_SIZE]);
    if (tr == track) {
      drawSeqSteps();
      drawNotePads();
    }
  }
  drawSetupMode();
}

void sendScenePC(u8 port, u8 sc, u8 instrument) {
	  u8 data[9];

	  data[0] = 0xF0;
	  // 00H 20H 29H -> Focusrite/Novation manufacturer ID
	  data[1] = 0x00;
	  data[2] = 0x20;
	  data[3] = 0x29;

	  // track number 123 for comm messages
	  data[4] = 0x7B;

	  //scene
	  data[5] = sc;

	  //instrument
	  data[6] = instrument;

	  //pc value
	  data[7] = pc_message[sc][instrument].value;

	  data[8] = 0xF7;
	  hal_send_sysex(port, data, 9);

#ifdef DEBUG
	  //hal_send_sysex(DINMIDI, data, 9);

 hal_send_midi(DINMIDI, CC, 1, sc);
 hal_send_midi(DINMIDI, CC, 2, instrument);
 hal_send_midi(DINMIDI, CC, 3, pc_message[sc][instrument].value);
#endif

}

void sendSceneType(u8 port, u8 sc) {
	  u8 data[8];

	  data[0] = 0xF0;
	  // 00H 20H 29H -> Focusrite/Novation manufacturer ID
	  data[1] = 0x00;
	  data[2] = 0x20;
	  data[3] = 0x29;

	  // track number 122 for comm message
	  data[4] = 0x7A;

	  //scene
	  data[5] = sc;

	  //type
	  data[6] = scene_type[sc];

	  data[8] = 0xF7;
	  hal_send_sysex(port, data, 8);
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

//MK: start message for sendAllSysexData function
void sendStartMessage(u8 port) {
  u8 data[8];
  data[0] = 0xF0;
  // 00H 20H 29H -> Focusrite/Novation manufacturer ID
  data[1] = 0x00;
  data[2] = 0x20;
  data[3] = 0x29;
  // track number 246 for start/end messages
  data[4] = 0xF6;
  // 1 for start, 2 for end
  data[5] = project;
  data[6] = 0x01;
  data[7] = 0xF7;
  hal_send_sysex(port, data, 8);
}

void sendEndMessage(u8 port) {
	  u8 data[8];
	  data[0] = 0xF0;
	  // 00H 20H 29H -> Focusrite/Novation manufacturer ID
	  data[1] = 0x00;
	  data[2] = 0x20;
	  data[3] = 0x29;
	  // track number 246 for start/end messages
	  data[4] = 0xF6;
	  data[5] = project;
	  // 1 for start, 2 for end
	  data[6] = 0x02;
	  data[7] = 0xF7;
	  hal_send_sysex(port, data, 8);

}


void sendTrackMuteData(u8 port, u8 mt) {
  u8 data[7 + MUTE_SETTINGS_SIZE];
  data[0] = 0xF0;
  // 00H 20H 29H -> Focusrite/Novation manufacturer ID
  data[1] = 0x00;
  data[2] = 0x20;
  data[3] = 0x29;
  // track number 126 for mute data
  data[4] = 0x7E;
  // mute place number
  data[5] = mt;
  formatMuteData(&data[6],mt);
  data[6 + MUTE_SETTINGS_SIZE] = 0xF7;
  hal_send_sysex(port, data, 7 + MUTE_SETTINGS_SIZE);
}

void loadProject(u8 port, u8 pt) {

	// clean PC's for scenes
	 // for (u8 i=0; i<SCENE_COUNT;i++){
		  // clear PC's for scenes
	//	  scene_pc[i] = 255;
	//	  scene_pc[i+SCENE_COUNT] = 255;
	 // }

	  for (u8 i=0; i<(SCENE_COUNT);i++){
		  for (u8 j=0;j<PC_COUNT;j++) {
			  pc_message[i][j].value = 255;
	  	  }
	  }

	  u8 data[8];
	  data[0] = 0xF0;
	  // 00H 20H 29H -> Focusrite/Novation manufacturer ID
	  data[1] = 0x00;
	  data[2] = 0x20;
	  data[3] = 0x29;
	  // track number 246 for comm messages
	  data[4] = 0xF6;
	  data[5] = pt;
	  // 3 for loading project
	  data[6] = 0x03;
	  data[7] = 0xF7;
	  hal_send_sysex(port, data, 8);
}
void requestProjectStatus(u8 port) {
	  u8 data[8];
	  data[0] = 0xF0;
	  // 00H 20H 29H -> Focusrite/Novation manufacturer ID
	  data[1] = 0x00;
	  data[2] = 0x20;
	  data[3] = 0x29;
	  // track number 246 for comm messages
	  data[4] = 0xF6;
	  data[5] = 0x00;
	  // 4 for project status
	  data[6] = 0x04;
	  data[7] = 0xF7;
	  hal_send_sysex(port, data, 8);
}

void requestMidiClockStart(u8 port) {
	  u8 data[8];
	  data[0] = 0xF0;
	  // 00H 20H 29H -> Focusrite/Novation manufacturer ID
	  data[1] = 0x00;
	  data[2] = 0x20;
	  data[3] = 0x29;
	  // track number 246 for start/end messages
	  data[4] = 0xF6;
	  data[5] = 0x00;
	  // 5 for Start clock, 6 for Stop clock
	  data[6] = 0x05;
	  data[7] = 0xF7;
	  hal_send_sysex(port, data, 8);
}

void requestMidiClockStop(u8 port) {
	  u8 data[8];
	  data[0] = 0xF0;
	  // 00H 20H 29H -> Focusrite/Novation manufacturer ID
	  data[1] = 0x00;
	  data[2] = 0x20;
	  data[3] = 0x29;
	  // track number 246 for comm messages
	  data[4] = 0xF6;
	  data[5] = 0x00;
	  // 5 for Start clock, 6 for Stop clock
	  data[6] = 0x06;
	  data[7] = 0xF7;
	  hal_send_sysex(port, data, 8);
}

void requestSceneTempo(u8 port, u8 sc) {
	  u8 data[8];
	  data[0] = 0xF0;
	  // 00H 20H 29H -> Focusrite/Novation manufacturer ID
	  data[1] = 0x00;
	  data[2] = 0x20;
	  data[3] = 0x29;
	  // track number 246 for comm messages
	  data[4] = 0xF6;
	  // session number
	  data[5] = sc;
	  // 7 for Tempo request
	  data[6] = 0x07;
	  data[7] = 0xF7;
	  hal_send_sysex(port, data, 8);
}

void viewProjectName(u8 port, u8 pt) {
	  u8 data[8];
	  data[0] = 0xF0;
	  // 00H 20H 29H -> Focusrite/Novation manufacturer ID
	  data[1] = 0x00;
	  data[2] = 0x20;
	  data[3] = 0x29;
	  // track number 246 for comm messages
	  data[4] = 0xF6;
	  data[5] = pt;
	  // 8 for view project name
	  data[6] = 0x08;
	  data[7] = 0xF7;
	  hal_send_sysex(port, data, 8);

}

void deleteProject(u8 port, u8 pt) {
	  u8 data[8];
	  data[0] = 0xF0;
	  // 00H 20H 29H -> Focusrite/Novation manufacturer ID
	  data[1] = 0x00;
	  data[2] = 0x20;
	  data[3] = 0x29;
	  // track number 246 for comm messages
	  data[4] = 0xF6;
	  data[5] = pt;
	  // 9 for delete project
	  data[6] = 0x09;
	  data[7] = 0xF7;
	  hal_send_sysex(port, data, 8);

}

void copyTrack(u8 source_track, u8 target_track) {

	// copy track settings
	  inputNotes[target_track].value = inputNotes[source_track].value;
	  inputNotes[target_track].velocity = inputNotes[source_track].velocity;
	  inputNotes[target_track].gate = inputNotes[source_track].gate;
	  stepSize[target_track] = stepSize[source_track];
	  drumTrack[target_track] = drumTrack[source_track];
	  octave[target_track] = octave[source_track];
	  channel[target_track] = channel[source_track];
	  midiPort[target_track] = midiPort[source_track];
	  seqLength[target_track] = seqLength[source_track];

	// copy sequence data
	  for (u8 i = 0; i < MAX_SEQ_LENGTH; i++) {
	    notes[target_track][i] = notes[source_track][i];
	  }


}

