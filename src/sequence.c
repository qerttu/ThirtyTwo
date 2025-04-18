#include "sequence.h"
#include "app.h"
#include "surface.h"
#include "data.h"

// for CC message debuging
//#define DEBUG

u8 seqPlay;
u8 seqPlayHeads[TRACK_COUNT];
CLOCK_STATE clockState;
s8 trackSelectStart;
u8 trackSelectEnd;
u32 clearTrackArm;
SETUP_PAGE setupPage;
SEQ_MODE seqMode;
u8 track;
u8 scene;
u8 project;
u8 recordArm = 0;
u32 offsetTimer = 0;
u8 offsetArm = 0;

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
u8 requestClock; //0=false,  1=true,

MIDI_NOTE notes[TRACK_COUNT][MAX_SEQ_LENGTH];
MIDI_NOTE inputNotes[TRACK_COUNT];
MIDI_NOTE notesBuffer[TRACK_COUNT];

u8 octave[TRACK_COUNT];
u8 channel[TRACK_COUNT];
u8 seqLength[TRACK_COUNT];
u8 midiPort[TRACK_COUNT];
u8 stepSize[TRACK_COUNT];
u8 drumTrack[TRACK_COUNT]; // 0 = keyboard, 1 = drumpads
u8 drumMachine[TRACK_COUNT]; // 0=valca beats, 1 = volca sample, 2 = MPC...

// new PC variables
PC_MESSAGE pc_message[SCENE_COUNT][PC_COUNT]; // new struct for PCs
PC_MESSAGE current_pc_message[SCENE_COUNT][PC_COUNT]; // array for current pc messages
PC_SLOT pc_slot[PC_COUNT];

// old PC variables
u8 current_pc; // current program, set 1
u8 current_pc2; // current program, set 2
u8 scene_pc[SCENE_COUNT+SCENE_COUNT]; // for two set of PCs

u8 scene_type[SCENE_COUNT]; // push=0, momentary=1
u8 pc_set; // pc screens

u8 resetPlayheadTrack; // 0 -> TRACK_COUNT-1; TRACK_COUNT = false
u8 resetPlayHead; // 0 = false; 1 = true;
u8 resetTrackNextPlayHead;


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
        notes[trk][i].offset = 0;
      }
    } else {
      clearTrackArm |= 1 << trk;
    }
  }
}

void deleteAllTracks() {

    for (u8 i = 0; i < TRACK_COUNT; i++) {
      if (trackContainsNotes(i)) {
	      for (u8 j = 0; j < MAX_SEQ_LENGTH; j++) {
	        notes[i][j].velocity = 0;
	        notes[i][j].gate = GATE_FULL;
	        notes[i][i].offset = 0;
	      }
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

void playLiveDrumNote(u8 index, u8 value, u8 machine) {

	s8 note = 0;
	if (drumTrack[track]) {
		note = indexToDrumNote(index,machine);
	}
	else
	{
		note = indexToNote(index);
	}

	#ifdef DEBUG
	 hal_send_midi(DINMIDI, CC, 1, index);
	#endif


	if (note >= 0) {
		hal_send_midi(midiPort[track],
				  (value ? NOTEON : NOTEOFF) + channel[track], note,
				  value);
	if (value) {
	  inputNotes[track].value = note;
	  inputNotes[track].velocity = value;

	  if (recordArm>0) {
		  notes[track][seqPlayHeads[track]] = inputNotes[track];
		  notes[track][seqPlayHeads[track]].offset = 0;
		  if (offsetArm>0) {
			  notes[track][seqPlayHeads[track]].offset = offsetTimer;
		  }
	  }
	}

    if (value) { // note on
      hal_plot_led(TYPEPAD, index,
                   velocityFade(CHANNEL_COLORS[channel[track]][0], value),
                   velocityFade(CHANNEL_COLORS[channel[track]][1], value),
                   velocityFade(CHANNEL_COLORS[channel[track]][2], value));
    	}
      else { // note off

       hal_plot_led(TYPEPAD, index, WHITE_KEY_COLOR_R, WHITE_KEY_COLOR_B, WHITE_KEY_COLOR_G);
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

	  if (recordArm>0) {
		  notes[track][seqPlayHeads[track]] = inputNotes[track];
		  notes[track][seqPlayHeads[track]].offset = 0;
		  if (offsetArm>0) {
			  notes[track][seqPlayHeads[track]].offset = offsetTimer;
		  }
	  }

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

void playMidiNote(u8 status, u8 note, u8 value) {


	if (note >= 0) {
		hal_send_midi(midiPort[track],status + channel[track], note,value);

		// note on
		if (status==NOTEON) {

		  inputNotes[track].value = note;
		  inputNotes[track].velocity = value;

		  if (recordArm>0) {
			  notes[track][seqPlayHeads[track]] = inputNotes[track];
			  notes[track][seqPlayHeads[track]].gate = 0;
			  notes[track][seqPlayHeads[track]].offset = 0;
			  if (offsetArm>0) {
				  notes[track][seqPlayHeads[track]].offset = offsetTimer;
			  }
		   }

		#ifdef DEBUG
		 //hal_send_midi(DINMIDI, CC, 2,notes[track][seqPlayHeads[track]].gate);
		#endif

		 }
		// note off
		else {
			//notes[track][seqPlayHeads[track]].gate = seqDiv[track] - notes[track][seqPlayHeads[track]].gate;
			#ifdef DEBUG
			 //hal_send_midi(DINMIDI, CC, 3,notes[track][seqPlayHeads[track]].gate);
			#endif
		}
	}
}

void stopPlayedNote(u8 i) {
  if (currentNotes[i].velocity > 0) {
    hal_send_midi(midiPort[i], NOTEOFF + channel[i], currentNotes[i].value, 0);
    currentNotes[i].velocity = 0;
  }
}

void onMidiReceive(u8 port, u8 status, u8 d1, u8 d2) {
  //u8 clockEvent = 0;
  if (status == MIDISTART) {
    for (u8 i = 0; i < TRACK_COUNT; i++) {
      seqDiv[i] = 0;
      seqPlayHeads[i] = seqLength[i] - 1;
    }
    seqPlay = 1;
    offsetTimer = 0;
    //clockEvent = 1;
  } else if (status == MIDICONTINUE) {
    seqPlay = 1;
    //clockEvent = 1;
  } else if (status == MIDISTOP) {
    updateLed(seqPlayHeads[track]);

    for (u8 i = 0; i < TRACK_COUNT; i++) {
      stopPlayedNote(i);
    }
    seqPlay = 0;
    recordArm = 0;
    offsetArm = 0;
    //clockEvent = 1;
  } else if (status == SONGPOSITIONPOINTER) {
    for (u8 i = 0; i < TRACK_COUNT; i++) {
      seqDiv[i] = 0;
      seqPlayHeads[i] = d1 % seqLength[i] - 1;
    }
    //clockEvent = 1;
  } else if (status == MIDITIMINGCLOCK) {
    if (seqPlay) {

	// set RESET_TRACK value
      resetTrackNextPlayHead = (seqPlayHeads[resetPlayheadTrack] + 1) % seqLength[resetPlayheadTrack];

      if ((resetPlayHead < SCENE_COUNT) && (resetTrackNextPlayHead==0) && (seqDiv[resetPlayheadTrack]==0))
      {
    	// set scene
      	scene = resetPlayHead;
		// send request tempo sysx
		requestSceneTempo(sysexMidiPort, scene);
		 //send PC messages
		sendScenePCMessages();
      }


      for (u8 i = 0; i < TRACK_COUNT; i++) {
        u8 repeat = (!trackRepeat || (trackRepeat & (1 << i))) && stepRepeat;
        u8 prevSeqPlayHead = seqPlayHeads[i];
        u8 prevPlayHead = !repeat || repeatPlayHeads[i] < 0 ? prevSeqPlayHead : repeatPlayHeads[i];
        u8 prevGate = notes[i][prevPlayHead].gate;

        //u8 prevOffset = notes[i][prevPlayHead].offset;

        // play note if offset is reached
        //if (prevOffset && (prevOffset != GATE_TIE) && (seqDiv[i] == AVAILABLE_STEP_SIZES[stepSize[i]][currentNotes[i].offset])) {
        //	 hal_send_midi(midiPort[i], NOTEON + channel[i], currentNotes[i].value, currentNotes[i].velocity);
        //}

        // stop note when gate value is reached
        if (prevGate != GATE_TIE
            && (seqDiv[i] == 0 || seqDiv[i] == AVAILABLE_STEP_SIZES[stepSize[i]][currentNotes[i].gate])) {
          stopPlayedNote(i);
        }



        // if we are in the beginning of a step
        if (seqDiv[i] == 0) {


          // reset only based on active track
          if (i==track) {
        	  offsetTimer = 0;
          }

          // we increase the playhead of a track
          seqPlayHeads[i] = (seqPlayHeads[i] + 1) % seqLength[i];

		  if ((resetPlayHead < SCENE_COUNT) && (resetTrackNextPlayHead==0)) {
			// reset all playheads
			  seqPlayHeads[i] = 0;
		  }

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

          // we create a note from the new playhead position
          MIDI_NOTE note = notes[i][newPlayHead];

          if (!(trackMute[scene] & (1 << i))) {
            if (note.velocity && (prevGate != GATE_TIE || note.value != currentNotes[i].value)) {
            	//if (note.offset==GATE_TIE || (!note.offset)) {
            		//hal_send_midi(midiPort[i], NOTEON + channel[i], note.value, note.velocity);

            		// put note to notebuffer
            		notesBuffer[i] = note;
            	//}
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
            if(setupPage == SEQ && seqMode == VELO) {
            	drawNoteVelocity(note.velocity);
            }
          }

          // if we are in the beginning of a track sequence
          if (seqPlayHeads[i] == 0) {

           if (newStepSize[i] >= 0) {
              stepSize[i] = newStepSize[i];
              newStepSize[i] = -1;
              refreshGrid();
            }

           // reset resetPlayhead flag and draw pads
           if (i == (TRACK_COUNT-1)) {
        	if (resetPlayHead < SCENE_COUNT) {
        		// reset flag
        		resetPlayHead = SCENE_COUNT;
        		// draw pads and notes
        		drawSetupMode();
           		drawSeqSteps();
        		}
           }
          }
        }

        // increase seq div size by one
        seqDiv[i] = (seqDiv[i] + 1) % AVAILABLE_STEP_SIZES[stepSize[i]][GATE_FULL];

      }
      //clockEvent = 1;
    }
  }

  //MK: pass thru also other stuff than clock events
   //if (clockEvent) {
    for (u8 i = 0; i < 3; i++) {
      if (clockOut & (1 << i)) {


      //TOBETESTED
      // if recording on, trigger playLiveNOte function
      if (recordArm > 0 && ((status==NOTEON || status==NOTEOFF))) {

    		  playMidiNote(status,d1,d2);
       }
      // else pass through as is
       else {
        hal_send_midi(i, status, d1, d2);
       }
      }
    }
  //}

}

void playNotesInBuffer() {
	// go
    for (u8 i = 0; i < TRACK_COUNT; i++) {
       MIDI_NOTE note = notesBuffer[i];

       if (note.value) {
    	 if (note.offset>0) {
    		 note.offset= note.offset-1;
    	 }
    	 else {
    	  hal_send_midi(midiPort[i], NOTEON + channel[i], note.value, note.velocity);
    	  note.value = 0;
    	 }
		 notesBuffer[i] = note;
      }
    }

}

void handleInternalClock() {
  switch (clockState) {
  case STOPPED:
    break;

  case STARTING:
	if (requestClock==0) {
		onMidiReceive(FAKE_PORT, MIDISTART, 0, 0);
		}
	else {
		requestMidiClockStart(sysexMidiPort);
	}
    clockState = PLAYING;
    break;

  case PLAYING:
    if (clockDiv == 0 && requestClock==0) {
      onMidiReceive(FAKE_PORT, MIDITIMINGCLOCK, 0, 0);
    }
    clockDiv = (clockDiv + 1) % AVAILABLE_TEMPI[tempo];
    break;

  case STOPPING:
	if (requestClock==0) {
		   onMidiReceive(FAKE_PORT, MIDISTOP, 0, 0);
		}
	else {
		requestMidiClockStop(sysexMidiPort);
	}
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
  setupPage = MUTE;
  seqMode = NOTES;
  track = 0;
  scene = 0;
  project = 255;
  stepRepeat = 0;
  repeatStart = -1;
  repeatLength = 0;
  clockDiv = 0;
  trackRepeat = 0;
  clockOut = 1;
  tempo = 19; // index for 20ms
  requestClock = 1; // use request midi as default


  // MUTE all tracks in all scenes and set default values for PC messages
  for (u8 i=0; i<SCENE_COUNT;i++){

	  // MUTE all tracks in scene
	  //trackMute[i] = -1;

	  //OLD IMPLEMENTATION clear PC's for scenes
	  //scene_pc[i] = 255;
	  //scene_pc[i+SCENE_COUNT] = 255;

	  // set default values for new PC messages
	  for (u8 j=0;j<PC_COUNT;j++) {
		  pc_message[i][j].bank = 0;
		  pc_message[i][j].value = 255; // 255 = empty

		  current_pc_message[i][j].bank = 0;
		  current_pc_message[i][j].value = 255;

	  }
  }
  // default values for PC slots
  for (u8 i=0; i<PC_COUNT;i++) {

	  // set slots inactive
	  pc_slot[i].status = 0; // inactive

	  // set slot channels
	  switch (i) {

	  // NORD
	  case 0:
		  pc_slot[i].channel = 15;
		  break;

	  //MPC
	  case 1:
		  pc_slot[i].channel = 0;
		  break;
	  //MONSTA
	  case 2:
		  pc_slot[i].channel = 5;
		  break;
	  }
  }


  //set default scene
  scene = 0;
  resetPlayheadTrack = 15; // 0 -> TRACK_COUNT-1; TRACK_COUNT = false
  resetPlayHead = 0; // 0 -> SCENE_COUNT -1; SCENE_COUNT = false

  // init notesbuffer
  for (u8 i = 0; i < TRACK_COUNT; i++) {
     MIDI_NOTE note;
     note.value = 0;
     note.velocity = 0;
     note.gate = 0;
     note.offset = 0;
  	 notesBuffer[i] = note;
    }

  // sequence default values
  for (u8 i = 0; i < TRACK_COUNT; i++) {
    seqPlayHeads[i] = seqLength[i] - 1;
    currentNotes[i] = (MIDI_NOTE){.value = 0, .velocity = 0, .gate = 0};
    newStepSize[i] = -1;
    repeatPlayHeads[i] = -1;
  }


  // initial screen load
  drawSetupMode();
}
