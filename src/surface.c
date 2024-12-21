#include <string.h>
#include "surface.h"
#include "app.h"
#include "sequence.h"
#include "data.h"

// CC sends for debugging
//#define DEBUG

u32 stepPress = 0;
u16 stepPressTimes[MAX_SEQ_LENGTH] = {0};
u16 settingsSlotPressTimes[8] = {0};
u8 setupMode = 1;
u8 setupButtonPressed = 0;
u8 tempoSelect = 0;
u8 muteTrackSelect = 0;
u8 midiTrackSelect = 0;
u8 sysexMidiPort = USBSTANDALONE;
u8 saveSelect = 0;
u8 loadSelect = 0;
u8 deleteSelect = 0;
u8 sceneSelect = 0;

// old pc variables
u8 sceneShift = 0;

// new pc variables
u8 pcSlotSelect = 0;



// MK: status for sendSysex time function
u8 sendSysex = 0;

//MK: default seq indexes
u8 SEQ_INDEXES[32] = {95, 96, 97, 98, 89, 79, 69, 59,
        49, 39, 29, 19,  8,  7,  6,  5,
         4,  3,  2,  1, 10, 20, 30, 40,
        50, 60, 70, 80, 91, 92, 93, 94};

s8 indexOf(u8 element, const u8 array[], u8 arraysize) {
  for (u8 i = 0; i < arraysize; i++) {
    if (array[i] == element) {
      return i;
    }
  }
  return -1;
}

s8 noteToDrumIndex(u8 note, u8 machine) {
	if (drumTrack[track]) {

		u8 indx = -1;
		s8 relIndex = 0;

		switch (machine) {

		case 1: // sample
			relIndex = indexOf(note, VOLCA_SAMPLE_NOTES, 10);
			if (relIndex>=0){
				indx = VOLCA_SAMPLE_INDEXES[relIndex];
			}
		break;

		case 2: // MPC
			relIndex = indexOf(note, MPC2_NOTES, 16);
			if (relIndex>=0){
				indx = MPC2_INDEXES[relIndex];
			}
		break;

		case 0: // beats
		default:
			relIndex = indexOf(note, VOLCA_BEATS_NOTES, 7);
			if (relIndex>=0){
				indx = VOLCA_BEATS_INDEXES[relIndex];
			}
			break;
		}
		return indx > 127 ? -1 : indx;
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

s8 indexToDrumNote(u8 index, u8 machine) {

		if (drumTrack[track]) {
			u8 note = -1;
			s8 relNote = 0;
			switch (machine) {

			case 1: // sample
				relNote = indexOf(index,VOLCA_SAMPLE_INDEXES, 10);
				if (relNote>=0) {
					note = VOLCA_SAMPLE_NOTES[relNote];
				}
			break;

			case 2: // MPC
				relNote = indexOf(index,MPC_INDEXES, 16);
				if (relNote>=0) {
					note = MPC_NOTES[relNote];
				}
			break;

			case 0: // mpc jjos2xl
			default:
				relNote = indexOf(index,MPC2_INDEXES, 16);
				if (relNote>=0) {
					note = (12 *octave[track]) + MPC2_NOTES[relNote];
				}

			break;
			}
			return note > 127 ? -1 : note;
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

void clearRoundedPads() {
    for (u8 i = 0; i < 32; i++) {
      hal_plot_led(TYPEPAD, SEQ_INDEXES_RND[i], 0, 0, 0);
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

  if (stepPress) { // gate and offset
    u8 i = 0;
    while (i < MAX_SEQ_LENGTH && !(stepPress & (1 << i))) i++;

    u8 gate = notes[track][i].gate;
    u8 offset = notes[track][i].offset;

    for (u8 j = 0; j < 8; j++) {

      // gate
      if (j == gate) {
        hal_plot_led(TYPEPAD, (j + 1) * 10 + 8, CHANNEL_COLORS[channel[track]][0],
                                                CHANNEL_COLORS[channel[track]][1],
                                                CHANNEL_COLORS[channel[track]][2]);
      } else if (j == 7) {
        hal_plot_led(TYPEPAD, (j + 1) * 10 + 8, WHITE_KEY_COLOR_R, WHITE_KEY_COLOR_G, WHITE_KEY_COLOR_B);
      } else {
        hal_plot_led(TYPEPAD, (j + 1) * 10 + 8, BLACK_KEY_COLOR_R, BLACK_KEY_COLOR_G, BLACK_KEY_COLOR_B);
      }

      // offset
     if (drumTrack[track]) {
      if ((j*OFFSET_MP) == offset) {
        hal_plot_led(TYPEPAD, (j + 1) * 10 + 7, CHANNEL_COLORS[channel[track]][0],
                                                CHANNEL_COLORS[channel[track]][1],
                                                CHANNEL_COLORS[channel[track]][2]);
      } else if (j == 7) {
        hal_plot_led(TYPEPAD, (j + 1) * 10 + 7, WHITE_KEY_COLOR_R, WHITE_KEY_COLOR_G, WHITE_KEY_COLOR_B);
      } else {
        hal_plot_led(TYPEPAD, (j + 1) * 10 + 7, OFFSET_COLOR_R, OFFSET_COLOR_G, OFFSET_COLOR_B);
      }
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


	  // DRAW MUTE BUTTONS
		clearRect(1, 1, 4, 4);

		//highlight tracks with channel color & have data
	    for (u8 i = 0; i < TRACK_COUNT; i++) {
	      if (trackContainsNotes(i)) {
	    	  if (trackMute[scene] & (1 << i)) {
	    		  hal_plot_led(TYPEPAD, 81 + i - 18 * (i / 8), CHANNEL_COLORS[channel[i]][0] >> 3, CHANNEL_COLORS[channel[i]][1] >> 3,
	    		 	                     CHANNEL_COLORS[channel[i]][2] >> 3);
	    	  }
	    	  else {
	    		  hal_plot_led(TYPEPAD, 81 + i - 18 * (i / 8), CHANNEL_COLORS[channel[i]][0], CHANNEL_COLORS[channel[i]][1],
	    		  	    		 	                     CHANNEL_COLORS[channel[i]][2]);

	    	  }


	      }
		  //if (i==track) {
		  //      hal_plot_led(TYPEPAD, 81 + i - 18 * (i / 8), WHITE_KEY_COLOR_R,
		  //                                    	  	  	  	 WHITE_KEY_COLOR_G,
		  //                                    	  	  	  	 WHITE_KEY_COLOR_B);
		  //}
	     }

	  //MK: muteTrackSelect button
		if (muteTrackSelect >0) {
			// highlight muteSelect button
			hal_plot_led(TYPEPAD, 15, CLOCK_STATE_COLOR_R, CLOCK_STATE_COLOR_G, CLOCK_STATE_COLOR_B);

		}
			else {
			// mute select button default
			hal_plot_led(TYPEPAD, 15, CLOCK_STATE_COLOR_R >> 3, CLOCK_STATE_COLOR_G >> 3, CLOCK_STATE_COLOR_B >> 3);
		}


    //MK: scene buttons, in 3x2 matrix
    for (u8 i = 0; i < sizeof(SCENE_INDEXES); i++) {

		if (i==scene) {
			if (scene_type[i]==1) {
				hal_plot_led(TYPEPAD, SCENE_INDEXES[i], SCENE_TYPE_R,SCENE_TYPE_G,SCENE_TYPE_B);
			}
			else {
				hal_plot_led(TYPEPAD, SCENE_INDEXES[i], WHITE_KEY_COLOR_R,WHITE_KEY_COLOR_G,WHITE_KEY_COLOR_B);
			}
		}
		else
		{
			if (scene_type[i]==1) {
				hal_plot_led(TYPEPAD, SCENE_INDEXES[i], SCENE_TYPE_R >> 3,SCENE_TYPE_G >> 3,SCENE_TYPE_B >> 3);
			}
			else {
				hal_plot_led(TYPEPAD, SCENE_INDEXES[i], WHITE_KEY_COLOR_R >> 3,
				                                      WHITE_KEY_COLOR_G >> 3,
				                                      WHITE_KEY_COLOR_B >> 3);
			}
		}

    }

    // draw drum buttons
	for (u8 i = 0; i < 4; i++) {
		for (u8 j = 0; j < 4; j++) {

			// highlight active track
		 if (track == (i*4+j)) {
			 hal_plot_led(TYPEPAD, 11 + i * 10 + j, WHITE_KEY_COLOR_R, WHITE_KEY_COLOR_G, WHITE_KEY_COLOR_B);
		 }
		 else {
			 hal_plot_led(TYPEPAD, 11 + i * 10 + j, SETUP_PAGE_COLORS[MUTE][0], SETUP_PAGE_COLORS[MUTE][1],
			        		SETUP_PAGE_COLORS[MUTE][2]);
		 }
		}
	}

	// play drums
	if (drumTrack[track]) {

	  for (u8 i = 0; i < MAX_SEQ_LENGTH; i++) {
	    if (stepPress & (1 << i)) {
	      MIDI_NOTE note = notes[track][i];
	      if (note.velocity) {
	        s8 index = noteToDrumIndex(note.value,drumMachine[track]);
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



    // button to switch between seq and mute screen
    //hal_plot_led(TYPEPAD,28, SETUP_PAGE_COLOR_R, SETUP_PAGE_COLOR_G,
     //                    SETUP_PAGE_COLOR_B);


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

  //MK: changed track SAVE to project load/save
  case SAVE:
    // request project status
	//requestProjectStatus(sysexMidiPort);

	for (u8 i = 0; i < PROJECT_COUNT; i++) {
      if (project==i) {
        hal_plot_led(TYPEPAD, 81 + i - 18 * (i / 8), SYSEX_DUMP_COLOR_R, SYSEX_DUMP_COLOR_G, SYSEX_DUMP_COLOR_B);
      }

    //  else {
    //    hal_plot_led(TYPEPAD, 81 + i - 18 * (i / 8), SYSEX_DUMP_COLOR_R >> 3, SYSEX_DUMP_COLOR_G >> 3, SYSEX_DUMP_COLOR_B >> 3);
    //  }


    }
    //hal_plot_led(TYPEPAD, 41, SYSEX_DUMP_COLOR_R, SYSEX_DUMP_COLOR_G, SYSEX_DUMP_COLOR_B);

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

    //MK: loadSelect button
	if (loadSelect >0) {
		hal_plot_led(TYPEPAD, 41, REPEAT_COLOR_R, REPEAT_COLOR_G, REPEAT_COLOR_B);
	}
		else {
		hal_plot_led(TYPEPAD, 41, REPEAT_COLOR_R >> 3, REPEAT_COLOR_G >> 3, REPEAT_COLOR_B >> 3);
	}

    //MK: saveSelect button
	if (saveSelect >0) {
		hal_plot_led(TYPEPAD, 42, CLOCK_STATE_COLOR_R, CLOCK_STATE_COLOR_G, CLOCK_STATE_COLOR_B);
	}
		else {
		hal_plot_led(TYPEPAD, 42, CLOCK_STATE_COLOR_R >> 3, CLOCK_STATE_COLOR_G >> 3, CLOCK_STATE_COLOR_B >> 3);
	}

	//MK: deleteSelect button
	if (deleteSelect >0) {
		hal_plot_led(TYPEPAD, 44, CLEAR_TRACK_COLOR_R, CLEAR_TRACK_COLOR_G, CLEAR_TRACK_COLOR_B);
	}
		else {
		hal_plot_led(TYPEPAD, 44, CLEAR_TRACK_COLOR_R >> 3, CLEAR_TRACK_COLOR_G >> 3, CLEAR_TRACK_COLOR_B >> 3);
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

    // clear all button
    hal_plot_led(TYPEPAD, 41, CLEAR_TRACK_ARM_COLOR_R, CLEAR_TRACK_ARM_COLOR_G, CLEAR_TRACK_ARM_COLOR_B);


    break;

  case SEQ:

	  // record arm
	  if (recordArm>0) {
		  hal_plot_led(TYPEPAD, 1, CLEAR_TRACK_COLOR_R, CLEAR_TRACK_COLOR_G, CLEAR_TRACK_COLOR_B);
	  }
	  else
	  {
		  hal_plot_led(TYPEPAD, 1, CLEAR_TRACK_COLOR_R >> 5, CLEAR_TRACK_COLOR_G >> 5, CLEAR_TRACK_COLOR_B >> 5);
	  }

	  // offset arm
	  if (offsetArm>0) {
		  hal_plot_led(TYPEPAD, 2, OFFSET_COLOR_R, OFFSET_COLOR_G, OFFSET_COLOR_B);
	  }
	  else
	  {
		  hal_plot_led(TYPEPAD, 2, OFFSET_COLOR_R >> 5, OFFSET_COLOR_G >> 5, OFFSET_COLOR_B >> 5);
	  }



	   // track select
		if (muteTrackSelect >0) {

			clearRect(1, 1, 4, 4);


			//highlight tracks with channel color & have data
		    for (u8 i = 0; i < TRACK_COUNT; i++) {
		      if (trackContainsNotes(i)) {
		        hal_plot_led(TYPEPAD, 81 + i - 18 * (i / 8), CHANNEL_COLORS[channel[i]][0] >> 3, CHANNEL_COLORS[channel[i]][1] >> 3,
		                     CHANNEL_COLORS[channel[i]][2] >> 3);
		      }
			  if (i==track) {
			        hal_plot_led(TYPEPAD, 81 + i - 18 * (i / 8), CHANNEL_COLORS[channel[i]][0], CHANNEL_COLORS[channel[i]][1],
			                     CHANNEL_COLORS[channel[i]][2]);
			  }
		     }

			hal_plot_led(TYPEPAD, 80, CLOCK_STATE_COLOR_R, CLOCK_STATE_COLOR_G, CLOCK_STATE_COLOR_B);
		}
			else {
			hal_plot_led(TYPEPAD, 80, CLOCK_STATE_COLOR_R >> 3, CLOCK_STATE_COLOR_G >> 3, CLOCK_STATE_COLOR_B >> 3);
		}

		// midi and track length select
		if (midiTrackSelect>0) {

		  // track channel
		  for (u8 i = 0; i < 16; i++) {
			if (channel[track] == i) {
			  hal_plot_led(TYPEPAD, 41 + i - 18 * (i / 8), CHANNEL_COLORS[i][0], CHANNEL_COLORS[i][1], CHANNEL_COLORS[i][2]);
			} else {
			  hal_plot_led(TYPEPAD, 41 + i - 18 * (i / 8), CHANNEL_COLORS[i][0] >> 3, CHANNEL_COLORS[i][1] >> 3, CHANNEL_COLORS[i][2] >> 3);
			}
		  }

		}



	  // draw drumpads
		if ((muteTrackSelect ==0 && (midiTrackSelect==0)) ) {
			for (u8 i = 0; i < 4; i++) {
				for (u8 j = 0; j < 4; j++) {

					// highlight active track
				 if (track == (i*4+j)) {
					 hal_plot_led(TYPEPAD, 11 + i * 10 + j, WHITE_KEY_COLOR_R, WHITE_KEY_COLOR_G, WHITE_KEY_COLOR_B);
				 }
				 else {
					 hal_plot_led(TYPEPAD, 11 + i * 10 + j, SETUP_PAGE_COLORS[SEQ][0], SETUP_PAGE_COLORS[SEQ][1],
					 			        		SETUP_PAGE_COLORS[SEQ][2]);
				 }
				}
			}

		  for (u8 i = 0; i < MAX_SEQ_LENGTH; i++) {
		    if (stepPress & (1 << i)) {
		      MIDI_NOTE note = notes[track][i];
		      if (note.velocity) {
		        s8 index = noteToDrumIndex(note.value,drumMachine[track]);
		        // testing with a blank spot....
		    	//s8 index = 42;
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

	  // draw octave
		for (u8 i = 0; i < 5; i++) {
		 if (i==octave[track]+1)
		 	 {
			hal_plot_led(TYPEPAD, i * 10, CHANNEL_COLORS[channel[track]][0],
					                                                        CHANNEL_COLORS[channel[track]][1],
		 	 		                                                        		CHANNEL_COLORS[channel[track]][2]);
		 	 }
			else
			{
				hal_plot_led(TYPEPAD, i *10, CHANNEL_COLORS[channel[track]][0]>> 3,
						                                                        CHANNEL_COLORS[channel[track]][1]>> 3,
			 	 		                                                        		CHANNEL_COLORS[channel[track]][2]>> 3);
			}
		}

	  // back to mute screen button
	  //hal_plot_led(TYPEPAD, 28, SETUP_PAGE_COLOR_R, SETUP_PAGE_COLOR_G,
	   //                    SETUP_PAGE_COLOR_B);


	  break;

  case EDIT:
  default:

   // track area
    for (u8 i = 0; i < TRACK_COUNT; i++) {

	   // scene PC's
	   if (sceneSelect>0) {

		  if (pc_message[sceneSelect-1][pcSlotSelect].value ==i) {
				  hal_plot_led(TYPEPAD, 81 + i - 18 * (i / 8), PC_SLOT_COLORS[pcSlotSelect][0], PC_SLOT_COLORS[pcSlotSelect][1],
						  PC_SLOT_COLORS[pcSlotSelect][2]);
			  }

		// OLD SCENE PC
		//if (sceneShift) {
		//  if (scene_pc[sceneSelect+SCENE_COUNT-1]==i+1) {
		//		  hal_plot_led(TYPEPAD, 81 + i - 18 * (i / 8), SCENE_2_R, SCENE_2_G,
		//				  SCENE_2_B);
		//	  }
		//  }
		//  else {
		//	  if(scene_pc[sceneSelect-1]==i+1) {
		//		  hal_plot_led(TYPEPAD, 81 + i - 18 * (i / 8), SCENE_1_R, SCENE_1_G,
		//				  SCENE_1_B);
		//	  }
    	//}
	   }
	   // track colors
	   else {
		for (u8 i = 0; i < TRACK_COUNT; i++) {
		  if (track == i) {
			hal_plot_led(TYPEPAD, 81 + i - 18 * (i / 8), CHANNEL_COLORS[channel[i]][0], CHANNEL_COLORS[channel[i]][1],
						 CHANNEL_COLORS[channel[i]][2]);
		  } else if (trackSelectStart < 0 || (trackSelectStart <= i && i <= trackSelectEnd)) {
			hal_plot_led(TYPEPAD, 81 + i - 18 * (i / 8), CHANNEL_COLORS[channel[i]][0] >> 3, CHANNEL_COLORS[channel[i]][1] >> 3,
						 CHANNEL_COLORS[channel[i]][2] >> 3);
		  }
		 }
	   }
	  }


    // track specific configurations,e.g midi channels etc
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
    	  //MK: select drummachine layout
    	  for(u8 i =0; i<3;i++){
			  if (drumMachine[track]==i) {
				hal_plot_led(TYPEPAD, 13+i, TRACK_MODE_COLOR_R, TRACK_MODE_COLOR_G, TRACK_MODE_COLOR_B);
			 }
			  else {
				hal_plot_led(TYPEPAD, 13+i, TRACK_MODE_COLOR_R >> 3, TRACK_MODE_COLOR_G >> 3, TRACK_MODE_COLOR_B >> 3);
			  }
    	  }
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

     // MK: MIDI clock receive setting
     if (requestClock==1) {
    	 hal_plot_led(TYPEPAD, 14, CLOCK_STATE_COLOR_MIDI_R, CLOCK_STATE_COLOR_MIDI_G, CLOCK_STATE_COLOR_MIDI_B);
     }
     else
     {
    	 hal_plot_led(TYPEPAD, 14, CLOCK_STATE_COLOR_MIDI_R >> 3, CLOCK_STATE_COLOR_MIDI_G >> 3, CLOCK_STATE_COLOR_MIDI_B >> 3);
     }

     // scene buttons
     for (u8 i = 0; i < sizeof(SCENE_INDEXES); i++) {

 		if (sceneSelect && (sceneSelect-1)==i) {

 		   if (scene_type[i]==1) {
  			  hal_plot_led(TYPEPAD, SCENE_INDEXES[i], SCENE_TYPE_R,SCENE_TYPE_G,SCENE_TYPE_B);
 		   }
 		   else
 		   {
 			  hal_plot_led(TYPEPAD, SCENE_INDEXES[i], WHITE_KEY_COLOR_R,
 			   		                                      WHITE_KEY_COLOR_G,
 			   		                                      WHITE_KEY_COLOR_B);
 		   }

 		}
 		else
 		{

  		   if (scene_type[i]==1) {
  			 hal_plot_led(TYPEPAD, SCENE_INDEXES[i], SCENE_TYPE_R >> 3,SCENE_TYPE_G >> 3,SCENE_TYPE_B >> 3);
  		   }
  		   else
  		   {
  	 			hal_plot_led(TYPEPAD, SCENE_INDEXES[i], WHITE_KEY_COLOR_R >> 3,
  	 			                                      WHITE_KEY_COLOR_G >> 3,
  	 			                                      WHITE_KEY_COLOR_B >> 3);
  		   }

 		}

     }

		 // scene slot select
     	 for (u8 i=0;i<PC_COUNT;i++) {
     		if (i==pcSlotSelect) {
     			hal_plot_led(TYPEPAD, PC_SLOT_INDEXES[i], PC_SLOT_COLORS[i][0],PC_SLOT_COLORS[i][1],PC_SLOT_COLORS[i][2]);
     		}
     		else
     		{
     			hal_plot_led(TYPEPAD, PC_SLOT_INDEXES[i], PC_SLOT_COLORS[i][0] >> 3,PC_SLOT_COLORS[i][1] >> 3,PC_SLOT_COLORS[i][2] >> 3);
     		}
     	 }


	  // scene type button
		if (sceneSelect) {

			//momentary
			if (scene_type[sceneSelect-1]==1) {
				hal_plot_led(TYPEPAD, 34, SCENE_TYPE_R,SCENE_TYPE_G,SCENE_TYPE_B);
			}
			//push (default)
			else
			{
				hal_plot_led(TYPEPAD, 34, SCENE_TYPE_R >> 3,SCENE_TYPE_G >> 3,SCENE_TYPE_B >> 3);
			}
		}


      // MIDI clock out
      for (u8 i = 0; i < 3; i++) {
        if (clockOut & (1 << i)) {
          hal_plot_led(TYPEPAD, 11 + i, MIDI_PORT_COLOR_R, MIDI_PORT_COLOR_G, MIDI_PORT_COLOR_B);
        } else {
          hal_plot_led(TYPEPAD, 11 + i, MIDI_PORT_COLOR_R >> 3, MIDI_PORT_COLOR_G >> 3, MIDI_PORT_COLOR_B >> 3);
        }
      }
    }
  }

  if ((trackSelectStart < 0)) {
    // settings pages
    for (u8 i = 0; i < 4; i++) {
      if (setupPage == i) {
        hal_plot_led(TYPEPAD, 25 + i, SETUP_PAGE_COLORS[i][0], SETUP_PAGE_COLORS[i][1],
        		SETUP_PAGE_COLORS[i][2]);
      } else {
        hal_plot_led(TYPEPAD, 25 + i, SETUP_PAGE_COLORS[i][0] >> 3,
        		SETUP_PAGE_COLORS[i][1] >> 3, SETUP_PAGE_COLORS[i][2] >> 3);
      }
     }
  	}

  if (trackSelectStart < 0) {


	// tempo select
	if (setupPage == EDIT) {
		hal_plot_led(TYPEPAD, 17, TEMPO_SELECT_COLOR_R, TEMPO_SELECT_COLOR_G, TEMPO_SELECT_COLOR_B);
	}

	  // internal clock
    if (clockState == STOPPED) {
      hal_plot_led(TYPEPAD, 18, CLOCK_STATE_COLOR_R >> 3, CLOCK_STATE_COLOR_G >> 3, CLOCK_STATE_COLOR_B >> 3);
    } else {
      hal_plot_led(TYPEPAD, 18, CLOCK_STATE_COLOR_R, CLOCK_STATE_COLOR_G, CLOCK_STATE_COLOR_B);
    }


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

  // active track
 //	for (u8 i = 0; i < TRACK_COUNT; i++) {
//		  if (i==track) {
//				hal_plot_led(TYPEPAD, 81 + i - 18 * (i / 8), WHITE_KEY_COLOR_R >>3,
//														 WHITE_KEY_COLOR_G >>3,
//															 WHITE_KEY_COLOR_B >>3);
//		  }
//	}

  if (seqPlay) {
    updatePlayHeadLed(seqPlayHeads[track]);
  }

}



void onSeqTouch(u8 index, u8 value) {

 // allow sequence on other mods than EDIT

  //

  if (tempoSelect && value) {
    tempo = index;
    drawSeqSteps();
  }

  // if on repeat mode
  else if (setupMode && setupPage == MUTE) {
    if (value) {
      stepRepeat |= 1 << index;
    } else {
      stepRepeat &= ~(1 << index);
    }
    updateRepeat();
    drawSeqSteps();
  }

  // if on edit mode
  /*
  else if(setupMode && setupPage == EDIT) {
	  if (trackSelectStart >= 0 && value) {
	    for (u8 i = trackSelectStart; i <= trackSelectEnd; i++) {
	      seqLength[i] = index + 1;
	    }
	    drawSeqSteps();
  	  }
  }
  */

  else if (trackSelectStart >= 0 && value) {
    for (u8 i = trackSelectStart; i <= trackSelectEnd; i++) {
      seqLength[i] = index + 1;
    }
    drawSeqSteps();
  }

  else {
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
  } else if (value && index >=13 && index <=15) {
    u8 newDrumTrack = drumTrack[track] ? 0 : 1;
    u8 newDrumMachine = index-13;
    for (u8 i = trackSelectStart; i <= trackSelectEnd; i++) {
      drumTrack[i] = newDrumTrack;
      drumMachine[i] = newDrumMachine;
    }
    drawSetupMode();
  } else if (value && index >= 16 && index <= 18) {
    for (u8 i = trackSelectStart; i <= trackSelectEnd; i++) {
      updateMidiPort(i, index - 16);
    }
    drawSetupMode();
  }
}

void onScenePCGridTouch(u8 index, u8 value) {

	// set PC messages
	if (value && index >= 51 && index <= 88 && sceneSelect>0) {

		//if (sceneShift) {

			// if value already set, add 255 (not set)
			if (pc_message[sceneSelect-1][pcSlotSelect].value==(64 + index - 18 * (index / 10))-1) {
				pc_message[sceneSelect-1][pcSlotSelect].value = 255;
			}
			else {
				// add value to the select scene/slot
				pc_message[sceneSelect-1][pcSlotSelect].value = (64 + index - 18 * (index / 10))-1;

				// send program message got the select slot as well, using track 1 port
				hal_send_midi(midiPort[0],PC + pc_slot[pcSlotSelect].channel,pc_message[sceneSelect-1][pcSlotSelect].value,0);
			}
		//}
		//else {

			// if value already set, add 255 (not set)
		//	if (scene_pc[sceneSelect-1]==(64 + index - 18 * (index / 10))) {
		//		scene_pc[sceneSelect-1] = 255;
		//	}
		//	else {
				// add pc number as PC2 value
		//		scene_pc[sceneSelect-1] = 64 + index - 18 * (index / 10);
		//		// send PC2 as well
		//		hal_send_midi(midiPort[TRACK_COUNT-1],PC + channel[TRACK_COUNT-1],scene_pc[sceneSelect-1],0);
		//	}
		//
		//}
	}

	// set scene TYPE
	else if (value && index == 34 && sceneSelect>0) {

		// if momentary, set to 255 (not set)
		if (scene_type[sceneSelect-1] == 1) {
			scene_type[sceneSelect-1] = 255;
		}
		// if push, set to momentary
		else
		{
			scene_type[sceneSelect-1] = 1;
		}
	}
	drawSetupMode();
}

void onSetupGridTouch(u8 index, u8 value) {
  if (value && setupButtonPressed) {
    setupMode |= 0x80;
  }

  if (trackSelectStart < 0) {


	// setup page selection pressed
	if (value && index >= 25 && index <= 28) {
	  setupPage = index - 25;

	  if (setupPage == CLEAR) {
	  }


	  if (setupPage != REPEAT && stepRepeat) {
		stepRepeat = 0;
		updateRepeat();
		drawSeqSteps();
	  }

	  if (setupPage != SEQ) {
		  memcpy(SEQ_INDEXES, SEQ_INDEXES_RND, MAX_SEQ_LENGTH);
		  clearRoundedPads();
	  }
	  else
	  {
		  memcpy(SEQ_INDEXES, SEQ_INDEXES_SQR, MAX_SEQ_LENGTH);
		  clearRoundedPads();
	  }

	  if (setupPage == MUTE) {
		  drawSeqSteps();
	  }


	  drawSetupMode();
	}



	 /*
    // SEQ mode back button to mute screen
   if (value && index==28 && setupPage == SEQ) {
    	memcpy(SEQ_INDEXES, SEQ_INDEXES_RND, 32);
    	setupPage = MUTE;
    	drawSetupMode();
    	drawSeqSteps();
    }


    // MUTE mode back button to seq screen
    else if (value && index ==28 && setupPage == MUTE) {
    	memcpy(SEQ_INDEXES, SEQ_INDEXES_SQR, 32);
    	setupPage = SEQ;
    	drawSetupMode();
    	drawSeqSteps();
    }
	*/

    // start button
    else if (value && index == 18) {
      if (!seqPlay && clockState == STOPPED) {
        clockState = STARTING;
        drawSetupMode();
      } else if (clockState == PLAYING) {
        clockState = STOPPING;
      }
    }

    // tempo select
    else if (index == 17) {
      tempoSelect = value;
      drawSeqSteps();
    }
  }

  /*
  if (trackSelectStart >= 0 && value)
  {

	    s8 i2 = indexOf(index, SEQ_INDEXES, MAX_SEQ_LENGTH);
		for (u8 i = trackSelectStart; i <= trackSelectEnd; i++) {
		  seqLength[i] = i2 + 1;
		}
		drawSeqSteps();
  }
  */

  if (tempoSelect) return;

  switch (setupPage) {
  case MUTE:


	// change track in drum pads
   if ((muteTrackSelect==0)) {
	for (u8 i = 0; i < 4; i++) {
		for (u8 j = 0; j < 4; j++) {
		  if (value && index==(11 + i * 10 + j))
		  {
			  if (track != (i*4+j))
			  {
			  track = i*4+j;
			  drawSeqSteps();
			  drawSetupMode();
			  }
		  }
		}
	  }
    }

	//MK: if not mute track select pressed
	if (value && index >= 51 && index <= 88 && (muteTrackSelect==0)) {
	  updateTrackMute(63 + index - 18 * (index / 10));
	  drawSetupMode();
	}

	//MK: if mute track select pressed
	if (value && index >= 51 && index <= 88 && (muteTrackSelect>0)) {
	      // copy track
		  copyTrack(track,63 + index - 18 * (index / 10));
		  // change to target track
		  track = 63 + index - 18 * (index / 10);
	      drawSetupMode();
	      drawSeqSteps();
	}

	//MK: mute track select
	if (index==15) {
		muteTrackSelect=value;
		drawSetupMode();
	}


	//MK: scene buttons
	if (((index >= 35 && index <= 38) || (index >= 45 && index <=48))) {

		// button pushed
		 if (value) {

			// set scene
			for (u8 i = 0; i < sizeof(SCENE_INDEXES); i++) {
			  if (index == SCENE_INDEXES[i]) {
				  scene = i;

				  //if we are not on momentary, store the current scene
				  if (scene_type[scene]!=1) {
					  sceneSelect = i;
				  }

				#ifdef DEBUG
				 hal_send_midi(DINMIDI, CC, 1, sceneSelect);
				#endif
			  }
			}

			// send PC messages
			for (u8 i=0;i<PC_COUNT;i++) {

				// check if an active value
				if (pc_message[scene][i].value<255) {

					// TO DO PLAYING
					// if playing, check first previous pc value
					//if ((seqPlay==1)) {
					//	if ((current_pc_message[scene][i].value!=pc_message[scene][i].value)){
					//		//send PC
					//		hal_send_midi(midiPort[0],PC + pc_slot[i].channel,pc_message[scene][i].value,0);
					//	}
					//}
					// if not playing, just send out the PC's
					//else
					//{
						hal_send_midi(midiPort[0],PC + pc_slot[i].channel,pc_message[scene][i].value,0);
					//}

					//set current program
					current_pc_message[scene][i].value = pc_message[scene][i].value;
					#ifdef DEBUG
					 hal_send_midi(DINMIDI, CC, 1, sceneSelect);
					#endif
				}

			}

			// send request tempo sysx
			requestSceneTempo(sysexMidiPort, scene);

		 }
		 // button released
		 else {
			 //if we are on momentary scene, switch to the previous scene
			 if(scene_type[scene]==1) {
				 scene = sceneSelect;
			 }

		 }


		// draw pads again
		drawSetupMode();
		}

	// change color of the mute track select if pressed
	if (muteTrackSelect >0) {
		hal_plot_led(TYPEPAD, 15, CLOCK_STATE_COLOR_R, CLOCK_STATE_COLOR_G, CLOCK_STATE_COLOR_B);
	}
		else {
		hal_plot_led(TYPEPAD, 15, CLOCK_STATE_COLOR_R >> 3, CLOCK_STATE_COLOR_G >> 3, CLOCK_STATE_COLOR_B >> 3);
	}


	// drum mode buttons
	if ((index >=11 && index <=14) || (index >=21 && index <=24) || (index >=31 && index <=34) || (index >=41 && index <=44)) {

		// if steps are pressed
        if (stepPress) { // step record
          if (value) {
            s8 note = indexToDrumNote(index,drumMachine[track]);
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
              drawSetupMode();
            }
          }
        }

        // MK: play drum notes
        if (!stepPress || !seqPlay) { // play note
          playLiveDrumNote(index, value,drumMachine[track]);
        }

	}




    break;

  case REPEAT:
    if (value && index >= 51 && index <= 88) {
      updateTrackRepeat(63 + index - 18 * (index / 10));
      drawSetupMode();
    }
    break;

  //MK: changed to project load/save
  case SAVE:

	// if loadselect pressed, load project
	if ((loadSelect>0) && (saveSelect==0) && (deleteSelect==0)) {
    	if (project <255) {
		//project = 63 + index - 18 * (index / 10);
    	drawSetupMode();
    	deleteAllTracks();
    	loadProject(sysexMidiPort,project);
    	}
	  }

	// if saveselect pressed, save project
	if (value && index >= 51 && index <= 88 && (saveSelect>0) && (loadSelect==0) && (deleteSelect==0)) {
    	project = 63 + index - 18 * (index / 10);
    	drawSetupMode();
    	sendSysex=1;
	  }
	// if deleteSelect pressed, delete
	if (value && index >= 51 && index <= 88 && (deleteSelect>0) && (loadSelect==0) && (saveSelect==0)) {
    	project = 63 + index - 18 * (index / 10);
    	drawSetupMode();
    	deleteProject(sysexMidiPort,project);
	  }

	// if projects pressed only, send display sysex
    if (value && index >= 51 && index <= 88 && (saveSelect==0) && (loadSelect==0) && (deleteSelect==0)) {
    	project = 63 + index - 18 * (index / 10);
    	drawSetupMode();
    	viewProjectName(sysexMidiPort,project);
    }
    //else if (value && index == 41) {
    //  sendGlobalSettingsData(sysexMidiPort);
    //} else if (value && index ==42) {
    //	sendSysex=1;
    //}
    else if (value && index >= 46 && index <= 48) {
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

    //MK: load select value
	if (index==41) {
		loadSelect=value;
	}

	//MK: save select value
	if (index==42) {
		saveSelect=value;
	}

	//MK: delete select valie
	if (index==44) {
		deleteSelect=value;
	}

	// change color of the save select, load select and delete select if pressed
	if (loadSelect >0) {
		hal_plot_led(TYPEPAD, 41, REPEAT_COLOR_R, REPEAT_COLOR_G, REPEAT_COLOR_B);
	}
		else {
		hal_plot_led(TYPEPAD, 41, REPEAT_COLOR_R >> 3, REPEAT_COLOR_G >> 3, REPEAT_COLOR_B >> 3);
	}

	if (saveSelect >0) {
		hal_plot_led(TYPEPAD, 42, CLOCK_STATE_COLOR_R, CLOCK_STATE_COLOR_G, CLOCK_STATE_COLOR_B);
	}
		else {
		hal_plot_led(TYPEPAD, 42, CLOCK_STATE_COLOR_R >> 3, CLOCK_STATE_COLOR_G >> 3, CLOCK_STATE_COLOR_B >> 3);
	}

	if (deleteSelect >0) {
		hal_plot_led(TYPEPAD, 44, CLEAR_TRACK_COLOR_R, CLEAR_TRACK_COLOR_G, CLEAR_TRACK_COLOR_B);
	}
		else {
		hal_plot_led(TYPEPAD, 44, CLEAR_TRACK_COLOR_R >> 3, CLEAR_TRACK_COLOR_G >> 3, CLEAR_TRACK_COLOR_B >> 3);
	}

    break;

  case CLEAR:

    // clear specific track
	if (value && index >= 51 && index <= 88) {
      clearTrack(63 + index - 18 * (index / 10));
      drawSetupMode();
    }

	// clear all tracks
	if (value && index == 41) {

	    for (u8 i = 0; i < TRACK_COUNT; i++) {
	      if (trackContainsNotes(i)) {
	    	  clearTrack(i);
	      }
	    }

	    drawSetupMode();
	}


    break;

  case SEQ:




	//mute track select pressed
	if (muteTrackSelect>0) {
		if (value && index >= 51 && index <= 88) {
			  track = 63 + index - 18 * (index / 10);
			  drawSeqSteps();
			  drawSetupMode();
		}
	}
	//midi trackselect pressed
	else if(midiTrackSelect>0) {
		if (value && index >= 31 && index <=48 && (midiTrackSelect>0)) {

			trackSelectStart = track;
			trackSelectEnd = track;
			onTrackSettingsGridTouch(index, value);
			drawSeqSteps();
			drawSetupMode();
		}
	}
	else {

		if ((!stepPress && drumTrack[track]) || (!recordArm && !drumTrack[track])) {

		// if drumpad pressed, select track
		for (u8 i = 0; i < 4; i++) {
			for (u8 j = 0; j < 4; j++) {
			  if (value && index==(11 + i * 10 + j))
			  {
				  if (track != (i*4+j))
				  {
				  track = i*4+j;
				  drawSeqSteps();
				  drawSetupMode();
				  }
			  }
			}
		  }
		}
	}




	//MK: mute track select
	if (index==80) {
		muteTrackSelect=value;
		drawSetupMode();

	}

	// midi track select
	if (index==70) {
		midiTrackSelect=value;
		drawSetupMode();

	}

	//step record
     if (midiTrackSelect==0) {
	  if ((index >=11 && index <=14) || (index >=21 && index <=24) || (index >=31 && index <=34) || (index >=41 && index <=44)) {

		// if steps are pressed
		if (stepPress) { // step record
		  if (value) {
			s8 note = 0;

			if (drumTrack[track]) {
				note = indexToDrumNote(index,drumMachine[track]);
			}
			else {
				note = indexToNote(index);
			}

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
				  drawSetupMode();
				}
			  }
			}

			// MK: play drum notes
			if (!stepPress) { // play note
			  playLiveDrumNote(index, value,drumMachine[track]);
			}

		  }
       }

	  	//octave
		if (index==40 || index==30 || index==20 || index==10) {
			u8 newOctave = ((index / 10)-1);
			octave[track] = newOctave;
			drawSetupMode();
		}

	//track arm
	  if(value && index == 1)  {
			if (recordArm==0) {
				recordArm=1;
			}
			else {
				recordArm=0;
			}
			drawSetupMode();
	  }

	//track arm offset
	  if(value && index == 2)  {
			if (offsetArm==0) {
				offsetArm=1;
			}
			else {
				offsetArm=0;
			}
			drawSetupMode();
	  }



		// change color of the mute track select if pressed
		if (muteTrackSelect >0) {
			hal_plot_led(TYPEPAD, 80, CLOCK_STATE_COLOR_R, CLOCK_STATE_COLOR_G, CLOCK_STATE_COLOR_B);
		}
			else {
			hal_plot_led(TYPEPAD, 80, CLOCK_STATE_COLOR_R >> 3, CLOCK_STATE_COLOR_G >> 3, CLOCK_STATE_COLOR_B >> 3);
		}

		// change color of the midi tack select if pressed
		if (midiTrackSelect >0) {
			hal_plot_led(TYPEPAD, 70, CLOCK_STATE_COLOR_MIDI_R, CLOCK_STATE_COLOR_MIDI_G, CLOCK_STATE_COLOR_MIDI_B);
		}
			else {
			hal_plot_led(TYPEPAD, 70, CLOCK_STATE_COLOR_MIDI_R >> 3, CLOCK_STATE_COLOR_MIDI_G >> 3, CLOCK_STATE_COLOR_MIDI_B >> 3);
		}

		if ((midiTrackSelect==0)) {
			trackSelectStart = -1;
			drawSetupMode();
		}

		// if no select button is selected, draw steps
		if ((muteTrackSelect==0)) {
			drawSeqSteps();
		}

	  break;

  case EDIT:
  default:

	//PC slots
	if (index>=PC_SLOT_INDEXES[0] && index<=PC_SLOT_INDEXES[PC_COUNT-1]){
		if (value) {
		 pcSlotSelect = index-41;
		 drawSetupMode();
		}
	}
	//sceneShift
	//if (index==44) {
	//	if (value) {
	//	 sceneShift = value;
	//	}
	//	else {
	//	 sceneShift = 0;
	//	}
	//	drawSetupMode();
	//}


    if ((index >= 51 && index <= 88) && (sceneSelect==0)) {
      updateTrackSelect(63 + index - 18 * (index / 10), value);
      drawSetupMode();
      drawSeqSteps();
    } else if (trackSelectStart >= 0) {
      onTrackSettingsGridTouch(index, value);
    } else if (value && index >= 11 && index <= 13) {
      toggleClockOutPort(index - 11);
      drawSetupMode();
    }
    else if ((index >= 35 && index <= 38) || (index >= 45 && index <=48)) { // scene buttons press
    	if (value) {
    		for (u8 i = 0; i < SCENE_COUNT; i++) {
    			if (SCENE_INDEXES[i] == index) {
    				sceneSelect = i+1;
    			}
    		}
    	}
    	else {
    		sceneSelect = 0;
    	}
        drawSetupMode();
        drawSeqSteps();
    }
    else if (sceneSelect>0) {
    	// PC change for scenes
    	onScenePCGridTouch(index, value);
    }
    // requestMidiClock setting
    else if(value && index == 14) {
    	if (requestClock==0) {
    		requestClock=1;
    	}
    	else {
    		requestClock=0;
    	}
    	drawSetupMode();
    }

  } // end of switch-case
} // end of function

void onAnyTouch(u8 type, u8 index, u8 value) {
  switch (type) {
  case TYPEPAD: {
    s8 i = indexOf(index, SEQ_INDEXES, MAX_SEQ_LENGTH);
    if (i >= 0 && (muteTrackSelect ==0) && (setupPage!=SAVE && setupPage!=CLEAR)) { // round pad / seq step
      onSeqTouch(i, value);
    } else { // grid pad pressed/released
      if (setupMode) {
        onSetupGridTouch(index, value);
      } else {
    	// note input mode
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
        }
        else if (drumTrack[track] && (index % 10 == 7)) { // select offset if drumtrack
            if (value) {
              if (stepPress) {
                u8 offset = (index / 10)-1;
                inputNotes[track].offset = offset*OFFSET_MP;
                for (u8 i = 0; i < MAX_SEQ_LENGTH; i++) {
                  if (stepPress & (1 << i)) {
                    notes[track][i].offset = offset*OFFSET_MP;
                  }
                }
              }
              drawNotePads();
            }
        }
        else { // note inputs
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

	// setup button click handling
	if (value) {
      setupMode = setupMode ? 0 : 1;
    } else if (setupMode & 0x80) { // quick setup
      setupMode = 0;
    }



	// set indexes and clear rounded pads when in note adding mode (not setupmode)
	if (!setupMode) {
		  refreshGrid();
		  memcpy(SEQ_INDEXES, SEQ_INDEXES_RND, MAX_SEQ_LENGTH);
		  clearRoundedPads();
		  drawSeqSteps();
	}

	// if back to setupmode, set square indexes in SEQ page
	if (setupMode && setupPage == SEQ) {
		  clearRoundedPads();
		  refreshGrid();
		  memcpy(SEQ_INDEXES, SEQ_INDEXES_SQR, MAX_SEQ_LENGTH);
		  drawSeqSteps();
	}

	// if back to setupmode, set square indexes in MUTE page
	if (setupMode && setupPage == MUTE) {
		  refreshGrid();
		  memcpy(SEQ_INDEXES, SEQ_INDEXES_RND, MAX_SEQ_LENGTH);
		  clearRoundedPads();
		  drawSeqSteps();
	}

    if (setupMode && setupPage == CLEAR) {
      refreshGrid();
      clearTrackArm = 0;
    }

    if (!setupMode && stepRepeat) {
      stepRepeat = 0;
      updateRepeat();
      drawSeqSteps();
      refreshGrid();
    }

    if (!setupMode && (trackSelectStart >= 0 || tempoSelect)) {
      trackSelectStart = -1;
      tempoSelect = 0;
      drawSeqSteps();
      refreshGrid();
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

//MK: send all tracks with 50ms delay between sends
void sendAllSysexData() {
#define DELAY_MS 50

	static u8 ms = DELAY_MS;
	static u8 tr_sysex = 0;

	if (sendSysex==1) {

		if (++ms >= DELAY_MS) {
			  ms = 0;

			  // sending Start message before first track
			  if (tr_sysex == 0){
				  sendStartMessage(sysexMidiPort);
			  }

			  // send track data and also last track in order to include PC messages midi channel
			  if (trackContainsNotes(tr_sysex) || (tr_sysex==(TRACK_COUNT-1))) {
				  sendTrackData(sysexMidiPort, tr_sysex);
			  }

			  ++tr_sysex;

			  // last, send global settings, track mute data and PC data
			  if (tr_sysex>TRACK_COUNT) {

				  // send global settings
				  sendGlobalSettingsData(sysexMidiPort);

				  // send trackmute and PC data per scene
				  for (u8 i=0; i<(SCENE_COUNT);i++){
					  // send track mute data
					  sendTrackMuteData(sysexMidiPort,i);
				  }

				  // send program change messages
				  for (u8 i=0; i<(SCENE_COUNT);i++){
					  for (u8 j=0;j<PC_COUNT;j++) {
						  if (pc_message[i][j].value <255) {
							  sendScenePC(sysexMidiPort,i,j);
						  }
					  }
				  }

				  // send scene type messages
				  for (u8 i=0; i<(SCENE_COUNT);i++){
					  sendSceneType(sysexMidiPort,i);
				  }


				  // send end message
				  sendEndMessage(sysexMidiPort);
				  sendSysex=0;
				  tr_sysex=0;

			  }

		}
	}
}
