/*
 *  MIDIFileLoader.cpp
 *  ofxMidiFileLoader
 *
 *  Created by Andrew on 20/11/2013.
 *  Copyright 2013 QMUL. All rights reserved.
 *
 */

#include "MIDIFileLoader.h"


const bool overrideTempo = true;//for Andrew R's use with Logic exported files
const int repeatCutoff = 150;//msec when looking for repeated midi events (post-filtering)

MIDIFileLoader:: MIDIFileLoader(){
	printMidiInfo = true;
}


int MIDIFileLoader::loadFile(std::string& filename){
	midiEvents.clear();//empty vector where we will hold the pitch and event time 
	
	pulsesPerQuarternote = 240;
	
	lastTick = 0;
	lastMillis = 0;
	
	beatPeriod = 500;///guessing
	printf("FIRST BEAT PERIOD %f\n", beatPeriod);
	//lastMeasurePosition = 0;
	/*
	noteOnIndex = 0;
	firstTickTime = 0;
	beatsPerMeasure = 4;
	numberOfBeatsAtLastPosition = 0;
	lastBeatPosition = 0;
	*/
	//setTempoFromMidiValue(500000, myMidiEvents);//default is 120bpm
	
	MIDIFileReader fr(filename);
	
	if (!fr.isOK()) {
		std::cerr << "Error: " << fr.getError().c_str() << std::endl;
		return 1;
	}
	
	MIDIComposition c = fr.load();
	
	switch (fr.getFormat()) {
		case MIDI_SINGLE_TRACK_FILE: cout << "Format: MIDI Single Track File" << endl; break;
		case MIDI_SIMULTANEOUS_TRACK_FILE: cout << "Format: MIDI Simultaneous Track File" << endl; break;
		case MIDI_SEQUENTIAL_TRACK_FILE: cout << "Format: MIDI Sequential Track File" << endl; break;
		default: cout << "Format: Unknown MIDI file format?" << endl; break;
	}
	
	std::cout << "Tracks: " << c.size() << endl;
	
	int td = fr.getTimingDivision();
	if (td < 32768) {
		if (printMidiInfo)
			std::cout << "Timing division: " << fr.getTimingDivision() << " ppq" << endl;
		
		pulsesPerQuarternote = fr.getTimingDivision();		
		
		//myMidiEvents.pulsesPerQuarternote = fr.getTimingDivision();
		//ticksPerMeasure = myMidiEvents.pulsesPerQuarternote * 4;//default setting
		
	} else {
		int frames = 256 - (td >> 8);
		int subframes = td & 0xff;
		if (printMidiInfo)
			std::cout << "SMPTE timing: " << frames << " fps, " << subframes << " subframes" << endl;
	}
	
	for (MIDIComposition::const_iterator i = c.begin(); i != c.end(); ++i) {
		if (printMidiInfo)
			std::cout << "Start of track: " << i->first+1 << endl;
		
		for (MIDITrack::const_iterator j = i->second.begin(); j != i->second.end(); ++j) {
			
			unsigned int t = j->getTime();
			int ch = j->getChannelNumber();
			
			if (j->isMeta()) {
				int code = j->getMetaEventCode();
				std::string name;
				bool printable = true;
				switch (code) {
						
					case MIDI_END_OF_TRACK:
						std::cout << t << ": End of track" << endl;
						break;
						
					case MIDI_TEXT_EVENT: name = "Text"; break;
					case MIDI_COPYRIGHT_NOTICE: name = "Copyright"; break;
					case MIDI_TRACK_NAME: name = "Track name"; break;
					case MIDI_INSTRUMENT_NAME: name = "Instrument name"; break;
					case MIDI_LYRIC: name = "Lyric"; break;
					case MIDI_TEXT_MARKER: name = "Text marker"; break;
					case MIDI_SEQUENCE_NUMBER: name = "Sequence number"; printable = false; break;
					case MIDI_CHANNEL_PREFIX_OR_PORT: name = "Channel prefix or port"; printable = false; break;
					case MIDI_CUE_POINT: name = "Cue point"; break;
					case MIDI_CHANNEL_PREFIX: name = "Channel prefix"; printable = false; break;
					case MIDI_SEQUENCER_SPECIFIC: name = "Sequencer specific"; printable = false; break;
					case MIDI_SMPTE_OFFSET: name = "SMPTE offset"; printable = false; break;
						
					case MIDI_SET_TEMPO:
					{
						int m0 = j->getMetaMessage()[0];
						int m1 = j->getMetaMessage()[1];
						int m2 = j->getMetaMessage()[2];
						long tempo = (((m0 << 8) + m1) << 8) + m2;
						//if (printMidiInfo)
						std::cout << "tempo data: " << tempo << endl;
						std::cout << t << ": Tempo(BPM): " << 60000000.0 / double(tempo) << endl;
						
						// The 3 data bytes of tt tt tt are the tempo in microseconds per quarter note
						
						//Joel - this bit needs checking
						//Andrew - yes, tempo above is actually period
						//however, from Logic when exporting at 120 BPM I get 138.101 here
						//so needs more checking
						
						if (!overrideTempo){
							beatPeriod = tempo/1000.0;
						} else {
							printf("WARNING! - Tempo message overriden here");
						}
						
						printf("BPM %.2f\n", 60000./beatPeriod);
					
						
						double tmp = updateElapsedTime(t);
						/*
						DoubleVector tmp;
						
						double lastTickInMillis = 0;
						double millisTimeNow = lastTickInMillis;
						int tickInterval = 0;
						if (myMidiEvents.periodValues.size() > 0){
							lastTickInMillis = myMidiEvents.periodValues[myMidiEvents.periodValues.size()-1][2];
							tickInterval = t  - myMidiEvents.periodValues[myMidiEvents.periodValues.size()-1][0];
							millisTimeNow = lastTickInMillis + (myMidiEvents.periodValues[myMidiEvents.periodValues.size()-1][1]*tickInterval);
							
						}
						
						tmp.push_back(t);
						
						
						tmp.push_back(60000000.0 / double(tempo));	
						double tmpTempoVal = 60000000.0 / double(tempo);
						tmp.push_back(millisTimeNow);
						
						myMidiEvents.periodValues.push_back(tmp);
						
						printf("tick[%i]: TEMPO %d tempoVal %f : time now %f\n", t, tempo, tmpTempoVal, millisTimeNow);
						*/
					}
						break;
						
					case MIDI_TIME_SIGNATURE:
					{
						int numerator = j->getMetaMessage()[0];
						int denominator = 1 << (int)j->getMetaMessage()[1];
						
						//newTimeSignature(t, numerator, denominator);
						
						//if (printMidiInfo)
						std::cout << t << ": Time signature: " << numerator << "/" << denominator << endl;
						printf(" ticks %i Time signature: %i by %i \n", t,  numerator , denominator );
					}
						
					case MIDI_KEY_SIGNATURE:
					{
						int accidentals = j->getMetaMessage()[0];
						int isMinor = j->getMetaMessage()[1];
						bool isSharp = accidentals < 0 ? false : true;
						accidentals = accidentals < 0 ? -accidentals : accidentals;
						if (printMidiInfo)
							std::cout << t << ": Key signature: " << accidentals << " "
							<< (isSharp ?
								(accidentals > 1 ? "sharps" : "sharp") :
								(accidentals > 1 ? "flats" : "flat"))
							<< (isMinor ? ", minor" : ", major") << endl;
					}
						
				}
				
				
				if (name != "") {
					if (printable) {
						std::cout << t << ": File meta event: code " << code
						<< ": " << name << ": \"" << j->getMetaMessage()
						<< "\"" << endl;
					} else {
						std::cout << t << ": File meta event: code " << code
						<< ": " << name << ": ";
						for (int k = 0; k < j->getMetaMessage().length(); ++k) {
							std::cout << (int)j->getMetaMessage()[k] << " ";
						}
					}
				}
				continue;
			}
			double newBeatLocation = 0;
			switch (j->getMessageType()) {
					
				case MIDI_NOTE_ON:
					if (printMidiInfo)
						std::cout << t << ": Note: channel " << ch
						<< " duration " << j->getDuration()
						<< " pitch " << j->getPitch()
						<< " velocity " << j->getVelocity() << endl;
//						<< "event time " << myMidiEvents.getEventTimeMillis(t) << endl;
					
					noteData newNote;
					newNote.pitch = j->getPitch();
					newNote.ticks = t;
					newNote.velocity = j->getVelocity();
					newNote.durationTicks = j->getDuration();
					newNote.durationMillis = ticksToMillis(newNote.durationTicks);
					double millis;
					millis = updateElapsedTime(t);
					printf("ticks %i event time %f dur %f\n", t, millis, newNote.durationMillis);
					//millis = (beatPeriod * newNote.ticks / (double) pulsesPerQuarternote);
					
					
					newNote.timeMillis = millis;
				
					midiEvents.push_back(newNote);
					
				
					
					//newBeatLocation = getBeatPositionForTickCount(t, myMidiEvents);
					
					//	printf("%i channel %i durn %i pitch %i vel %i event time %f beat pos %f\n", t, ch, (int)j->getDuration(), (int)j->getPitch(), (int)j->getVelocity(), myMidiEvents.getEventTimeMillis(t)
					//		   , newBeatLocation);
					
					
					
					//	printf("Beat location %3.2f\n", newBeatLocation);
					
				/*
					v.clear();
					
					//	printf("note on at %i\n", t);
					
					//if (!chopBeginning)
					v.push_back(t);
					//else
					//	v.push_back(t - firstTickTime);
					
					v.push_back(j->getPitch());
					v.push_back(j->getVelocity());
					v.push_back(j->getDuration());
				 */
					/*
					myMidiEvents.recordedNoteOnMatrix.push_back(v);
					myMidiEvents.noteOnMatches.push_back(false);
					myMidiEvents.beatPositions.push_back(newBeatLocation);
					*/
					
					break;
					
				case MIDI_POLY_AFTERTOUCH:
					if (printMidiInfo)
						std::cout << t << ": Polyphonic aftertouch: channel " << ch
						<< " pitch " << j->getPitch()
						<< " pressure " << j->getData2() << endl;
					break;
					
				case MIDI_CTRL_CHANGE:
				{
					int controller = j->getData1();
					std::string name;
					switch (controller) {
						case MIDI_CONTROLLER_BANK_MSB: name = "Bank select MSB"; break;
						case MIDI_CONTROLLER_VOLUME: name = "Volume"; break;
						case MIDI_CONTROLLER_BANK_LSB: name = "Bank select LSB"; break;
						case MIDI_CONTROLLER_MODULATION: name = "Modulation wheel"; break;
						case MIDI_CONTROLLER_PAN: name = "Pan"; break;
						case MIDI_CONTROLLER_SUSTAIN: name = "Sustain"; break;
						case MIDI_CONTROLLER_RESONANCE: name = "Resonance"; break;
						case MIDI_CONTROLLER_RELEASE: name = "Release"; break;
						case MIDI_CONTROLLER_ATTACK: name = "Attack"; break;
						case MIDI_CONTROLLER_FILTER: name = "Filter"; break;
						case MIDI_CONTROLLER_REVERB: name = "Reverb"; break;
						case MIDI_CONTROLLER_CHORUS: name = "Chorus"; break;
						case MIDI_CONTROLLER_NRPN_1: name = "NRPN 1"; break;
						case MIDI_CONTROLLER_NRPN_2: name = "NRPN 2"; break;
						case MIDI_CONTROLLER_RPN_1: name = "RPN 1"; break;
						case MIDI_CONTROLLER_RPN_2: name = "RPN 2"; break;
						case MIDI_CONTROLLER_SOUNDS_OFF: name = "All sounds off"; break;
						case MIDI_CONTROLLER_RESET: name = "Reset"; break;
						case MIDI_CONTROLLER_LOCAL: name = "Local"; break;
						case MIDI_CONTROLLER_ALL_NOTES_OFF: name = "All notes off"; break;
					}
					if (printMidiInfo)
						std::cout << t << ": Controller change: channel " << ch
						<< " controller " << j->getData1();
					if (name != "") std::cout << " (" << name << ")";
					std::cout << " value " << j->getData2() << endl;
				}
					break;
					
				case MIDI_PROG_CHANGE:
					if (printMidiInfo)
						std::cout << t << ": Program change: channel " << ch
						<< " program " << j->getData1() << endl;
					break;
					
				case MIDI_CHNL_AFTERTOUCH:
					if (printMidiInfo)
						std::cout << t << ": Channel aftertouch: channel " << ch
						<< " pressure " << j->getData1() << endl;
					break;
					
				case MIDI_PITCH_BEND:
					if (printMidiInfo)
						std::cout << t << ": Pitch bend: channel " << ch
						<< " value " << (int)j->getData2() * 128 + (int)j->getData1() << endl;
					break;
					
				case MIDI_SYSTEM_EXCLUSIVE:
					if (printMidiInfo)
						std::cout << t << ": System exclusive: code "
						<< (int)j->getMessageType() << " message length " <<
						j->getMetaMessage().length() << endl;
					break;
					
					
			}
			
			
		}
		
		
	}

}//end midi main reading


double MIDIFileLoader::updateElapsedTime(int ticksNow){

	double millisNow = ((beatPeriod * (ticksNow - lastTick)) / (double) pulsesPerQuarternote);//update time elapsed since we last updated
	printf("ticks elapsed %i, last millis %f elapsed millis %f\n", (ticksNow - lastTick), lastMillis, millisNow);
	
	millisNow += lastMillis;
	
	lastTick = ticksNow;
	lastMillis = millisNow;
	
	return millisNow;
}

double MIDIFileLoader::ticksToMillis(int ticks){
	return (beatPeriod * ticks / (double) pulsesPerQuarternote);
}


void MIDIFileLoader::printNoteData(){
	for (int i = 0; i < midiEvents.size(); i++){
		printf("NOTE %i time %.0f vel %i\n", midiEvents[i].pitch, midiEvents[i].timeMillis, midiEvents[i].velocity);
		if (filterEvent(i))
			printf("REPEAT!\n");
	}
}

void MIDIFileLoader::filterMidiEvents(){
	int index = 0;
	while (index < midiEvents.size()){
		if (filterEvent(index))
			midiEvents.erase(midiEvents.begin()+index);
		else 
			index++;
	}
}

bool MIDIFileLoader::filterEvent(int index){
	double cutoffTime = midiEvents[index].timeMillis - repeatCutoff;
	bool repeatEvent = false;
	int tmpIndex = index-1;
	while (tmpIndex >= 0 && midiEvents[tmpIndex].timeMillis > cutoffTime){
		if (midiEvents[tmpIndex].pitch == midiEvents[index].pitch)
			repeatEvent = true;
		tmpIndex--;
	}
	return repeatEvent;
}


