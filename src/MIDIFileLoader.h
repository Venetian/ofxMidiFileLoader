/*
 *  MIDIFileLoader.h
 *  ofxMidiFileLoader
 *
 *  Created by Andrew on 20/11/2013.
 *  Copyright 2013 QMUL. All rights reserved.
 *
 */

#ifndef MIDI_FILE_LOADER
#define MIDI_FILE_LOADER

#include "MIDIFileReader.h"
using namespace MIDIConstants;
#include "vector.h"

struct noteData {
	float beatPosition;//in beats from beginning
	int pitch;//as MIDI note number
	double timeMillis;
	int ticks;
	int velocity;
	long  durationTicks;
	double durationMillis;
};

class MIDIFileLoader{
public:
	MIDIFileLoader();
	
	int loadFile(std::string& filename);
	
	double updateElapsedTime(int ticksNow);
	double ticksToMillis(int ticks);
	
	//newTimeSignature(int ticks, int numerator, int denominator);
	//where we store the info
	std::vector<noteData> midiEvents;
	
	int lastTick;
	double lastMillis;
	
	double beatPeriod;
	bool printMidiInfo;
	int pulsesPerQuarternote;
	
	//	int lastMeasurePosition;
};
#endif

/*

#ifndef CANNAM_MIDI_FILE_LOADER
#define  CANNAM_MIDI_FILE_LOADER

#include "MIDIFileReader.h"
//#include "MIDIEvent.h"
#include "midiEventHolder.h"
using namespace MIDIConstants;

class CannamMidiFileLoader{
	
public:
	CannamMidiFileLoader();
	
	typedef std::vector<double> DoubleVector;
	
	int loadFile(std::string& filename, midiEventHolder& myMidiEvents);
	
	void createEventTiming( midiEventHolder& myMidiEvents);
	void setTempoFromMidiValue(long tempo,  midiEventHolder& myMidiEvents);
	double firstNoteTime;
	int firstTickTime;
	bool chopBeginning;
	void chopBeginningfromEvents(midiEventHolder&  myMidiEvents);
	
	typedef std::vector<int> IntVector;
	IntVector v;
	int noteOnIndex;
	
	int ticksPerMeasure;
	void newTimeSignature(int ticks, int numerator, int denominator, midiEventHolder& myMidiEvents);
	void updateMeasureToTickPosition(int ticks,  midiEventHolder& myMidiEvents);
	bool printMidiInfo;
	void printMeasuresSoFar(midiEventHolder& myMidiEvents);
	void correctMeasuresTiming(midiEventHolder& myMidiEvents);
	double fileDuration;
	float beatsPerMeasure;
	float numberOfBeatsAtLastPosition;
	int lastBeatPosition;
	double getBeatPositionForTickCount(long t, midiEventHolder& myMidiEvents);
	void printUpToIndex(const int& index, midiEventHolder& midiEvents);
};
#endif
*/