/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    This is a modified version of a source file from the 
    Rosegarden MIDI and audio sequencer and notation editor.
    This file copyright 2000-2010 Richard Bown and Chris Cannam.
  
    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use, copy,
    modify, merge, publish, distribute, sublicense, and/or sell copies
    of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR
    ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
    CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

    Except as contained in this notice, the names of the authors
    shall not be used in advertising or otherwise to promote the sale,
    use or other dealings in this Software without prior written
    authorization.
*/


#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>

#include "MIDIFileReader.h"
#include "MIDIEvent.h"

#include <sstream>

using std::string;
using std::ifstream;
using std::stringstream;
using std::cerr;
using std::endl;
using std::ends;
using std::ios;

using namespace MIDIConstants;

//#define DEBUG_MIDI_FILE_READER 1

#define throw_exception(...) do { \
        char message[128]; \
        snprintf(message, 128, __VA_ARGS__); \
        throw MIDIException(std::string(message)); \
    } while (0)
    


MIDIFileReader::MIDIFileReader(std::string path) :
    m_timingDivision(0),
    m_format(MIDI_FILE_BAD_FORMAT),
    m_numberOfTracks(0),
    m_trackByteCount(0),
    m_decrementCount(false),
    m_path(path),
    m_midiFile(0),
    m_fileSize(0)
{
    if (parseFile()) {
	m_error = "";
    }
}

MIDIFileReader::~MIDIFileReader()
{
}

bool
MIDIFileReader::isOK() const
{
    return (m_error == "");
}

std::string
MIDIFileReader::getError() const
{
    return m_error;
}

long
MIDIFileReader::midiBytesToLong(const string& bytes)
{
    if (bytes.length() != 4) {
	throw_exception("Wrong length for long data in MIDI stream (%d, should be %d)", (int)bytes.length(), 4);
    }

    long longRet = ((long)(((MIDIByte)bytes[0]) << 24)) |
                   ((long)(((MIDIByte)bytes[1]) << 16)) |
                   ((long)(((MIDIByte)bytes[2]) << 8)) |
                   ((long)((MIDIByte)(bytes[3])));

    return longRet;
}

int
MIDIFileReader::midiBytesToInt(const string& bytes)
{
    if (bytes.length() != 2) {
	throw_exception("Wrong length for int data in MIDI stream (%d, should be %d)", (int)bytes.length(), 2);
    }

    int intRet = ((int)(((MIDIByte)bytes[0]) << 8)) |
                 ((int)(((MIDIByte)bytes[1])));
    return(intRet);
}


// Gets a single byte from the MIDI byte stream.  For each track
// section we can read only a specified number of bytes held in
// m_trackByteCount.
//
MIDIByte
MIDIFileReader::getMIDIByte()
{
    if (!m_midiFile) {
	throw_exception("getMIDIByte called but no MIDI file open");
    }

    if (m_midiFile->eof()) {
        throw_exception("End of MIDI file encountered while reading");
    }

    if (m_decrementCount && m_trackByteCount <= 0) {
        throw_exception("Attempt to get more bytes than expected on Track");
    }

    char byte;
    if (m_midiFile->read(&byte, 1)) {
	--m_trackByteCount;
	return (MIDIByte)byte;
    }

    throw_exception("Attempt to read past MIDI file end");
}


// Gets a specified number of bytes from the MIDI byte stream.  For
// each track section we can read only a specified number of bytes
// held in m_trackByteCount.
//
string
MIDIFileReader::getMIDIBytes(unsigned long numberOfBytes)
{
    if (!m_midiFile) {
	throw_exception("getMIDIBytes called but no MIDI file open");
    }

    if (m_midiFile->eof()) {
        throw_exception("End of MIDI file encountered while reading");
    }

    if (m_decrementCount && (numberOfBytes > (unsigned long)m_trackByteCount)) {
        throw_exception("Attempt to get more bytes than available on Track (%lu, only have %ld)", numberOfBytes, m_trackByteCount);
    }

    string stringRet;
    char fileMIDIByte;

    while (stringRet.length() < numberOfBytes &&
           m_midiFile->read(&fileMIDIByte, 1)) {
        stringRet += fileMIDIByte;
    }

    // if we've reached the end of file without fulfilling the
    // quota then panic as our parsing has performed incorrectly
    //
    if (stringRet.length() < numberOfBytes) {
        stringRet = "";
        throw_exception("Attempt to read past MIDI file end");
    }

    // decrement the byte count
    if (m_decrementCount)
        m_trackByteCount -= stringRet.length();

    return stringRet;
}


// Get a long number of variable length from the MIDI byte stream.
//
long
MIDIFileReader::getNumberFromMIDIBytes(int firstByte)
{
    if (!m_midiFile) {
	throw_exception("getNumberFromMIDIBytes called but no MIDI file open");
    }

    long longRet = 0;
    MIDIByte midiByte;

    if (firstByte >= 0) {
	midiByte = (MIDIByte)firstByte;
    } else if (m_midiFile->eof()) {
	return longRet;
    } else {
	midiByte = getMIDIByte();
    }

    longRet = midiByte;
    if (midiByte & 0x80) {
	longRet &= 0x7F;
	do {
	    midiByte = getMIDIByte();
	    longRet = (longRet << 7) + (midiByte & 0x7F);
	} while (!m_midiFile->eof() && (midiByte & 0x80));
    }

    return longRet;
}


// Seek to the next track in the midi file and set the number
// of bytes to be read in the counter m_trackByteCount.
//
bool
MIDIFileReader::skipToNextTrack()
{
    if (!m_midiFile) {
	throw_exception("skipToNextTrack called but no MIDI file open");
    }

    string buffer, buffer2;
    m_trackByteCount = -1;
    m_decrementCount = false;

    while (!m_midiFile->eof() && (m_decrementCount == false)) {
        buffer = getMIDIBytes(4); 
	if (buffer.compare(0, 4, MIDI_TRACK_HEADER) == 0) {
	    m_trackByteCount = midiBytesToLong(getMIDIBytes(4));
	    m_decrementCount = true;
	}
    }

    if (m_trackByteCount == -1) { // we haven't found a track
        return false;
    } else {
        return true;
    }
}


// Read in a MIDI file.  The parsing process throws exceptions back up
// here if we run into trouble which we can then pass back out to
// whoever called us using a nice bool.
//
bool
MIDIFileReader::parseFile()
{
    m_error = "";

#ifdef DEBUG_MIDI_FILE_READER
    cerr << "MIDIFileReader::open() : fileName = " << m_path.toStdString() << endl;
#endif

    // Open the file
    m_midiFile = new ifstream(m_path.c_str(), ios::in | ios::binary);

    if (!*m_midiFile) {
	m_error = "File not found or not readable.";
	m_format = MIDI_FILE_BAD_FORMAT;
	delete m_midiFile;
        m_midiFile = 0;
	return false;
    }

    bool retval = false;

    try {

	// Set file size so we can count it off
	//
	m_midiFile->seekg(0, ios::end);
	m_fileSize = m_midiFile->tellg();
	m_midiFile->seekg(0, ios::beg);

	// Parse the MIDI header first.  The first 14 bytes of the file.
	if (!parseHeader(getMIDIBytes(14))) {
	    m_format = MIDI_FILE_BAD_FORMAT;
	    m_error = "Not a MIDI file.";
	    goto done;
	}

	for (unsigned int j = 0; j < m_numberOfTracks; ++j) {

#ifdef DEBUG_MIDI_FILE_READER
	    cerr << "Parsing Track " << j << endl;
#endif

	    if (!skipToNextTrack()) {
#ifdef DEBUG_MIDI_FILE_READER
		cerr << "Couldn't find Track " << j << endl;
#endif
		m_error = "File corrupted or in non-standard format?";
		m_format = MIDI_FILE_BAD_FORMAT;
		goto done;
	    }

#ifdef DEBUG_MIDI_FILE_READER
	    cerr << "Track has " << m_trackByteCount << " bytes" << endl;
#endif

	    // Run through the events taking them into our internal
	    // representation.
	    if (!parseTrack(j)) {
#ifdef DEBUG_MIDI_FILE_READER
		cerr << "Track " << j << " parsing failed" << endl;
#endif
		m_error = "File corrupted or in non-standard format?";
		m_format = MIDI_FILE_BAD_FORMAT;
		goto done;
	    }
	}
	
	retval = true;

    } catch (MIDIException e) {

        cerr << "MIDIFileReader::open() - caught exception - " << e.what() << endl;
	m_error = e.what();
    }
    
done:
    m_midiFile->close();
    delete m_midiFile;

    for (unsigned int track = 0; track < m_numberOfTracks; ++track) {

        // Convert the deltaTime to an absolute time since the track
        // start.  The addTime method returns the sum of the current
        // MIDI Event delta time plus the argument.

	unsigned long acc = 0;

        for (MIDITrack::iterator i = m_midiComposition[track].begin();
             i != m_midiComposition[track].end(); ++i) {
#ifdef DEBUG_MIDI_FILE_READER
            cerr << "converting delta time " << i->getTime();
#endif
            acc = i->addTime(acc);
#ifdef DEBUG_MIDI_FILE_READER
            cerr << " to " << i->getTime() << endl;
#endif
        }

        consolidateNoteOffEvents(track);
    }

    return retval;
}

// Parse and ensure the MIDI Header is legitimate
//
bool
MIDIFileReader::parseHeader(const string &midiHeader)
{
    if (midiHeader.size() < 14) {
#ifdef DEBUG_MIDI_FILE_READER
        cerr << "MIDIFileReader::parseHeader() - file header undersized" << endl;
#endif
        return false;
    }

    if (midiHeader.compare(0, 4, MIDI_FILE_HEADER) != 0) {
#ifdef DEBUG_MIDI_FILE_READER
	cerr << "MIDIFileReader::parseHeader()"
	     << "- file header not found or malformed"
	     << endl;
#endif
	return false;
    }

    if (midiBytesToLong(midiHeader.substr(4,4)) != 6L) {
#ifdef DEBUG_MIDI_FILE_READER
        cerr << "MIDIFileReader::parseHeader()"
	     << " - header length incorrect"
	     << endl;
#endif
        return false;
    }

    m_format = (MIDIFileFormatType) midiBytesToInt(midiHeader.substr(8,2));
    m_numberOfTracks = midiBytesToInt(midiHeader.substr(10,2));
    m_timingDivision = midiBytesToInt(midiHeader.substr(12,2));

#ifdef DEBUG_MIDI_FILE_READER
    if (m_timingDivision < 0) {
        cerr << "MIDIFileReader::parseHeader()"
                  << " - file uses SMPTE timing"
                  << endl;
    }
#endif

    return true; 
}

// Extract the contents from a MIDI file track and places it into
// our local map of MIDI events.
//
bool
MIDIFileReader::parseTrack(unsigned int trackNum)
{
    MIDIByte midiByte, metaEventCode, data1, data2;
    MIDIByte eventCode = 0x80;
    string metaMessage;
    unsigned int messageLength;
    unsigned long deltaTime;
    unsigned long accumulatedTime = 0;

    // Remember the last non-meta status byte (-1 if we haven't seen one)
    int runningStatus = -1;

    while (!m_midiFile->eof() && (m_trackByteCount > 0)) {

	if (eventCode < 0x80) {
#ifdef DEBUG_MIDI_FILE_READER
	    cerr << "WARNING: Invalid event code " << eventCode
		 << " in MIDI file" << endl;
#endif
	    throw_exception("Invalid event code %d found", int(eventCode));
	}

        deltaTime = getNumberFromMIDIBytes();

#ifdef DEBUG_MIDI_FILE_READER
	cerr << "read delta time " << deltaTime << endl;
#endif

        // Get a single byte
        midiByte = getMIDIByte();

        if (!(midiByte & MIDI_STATUS_BYTE_MASK)) {

	    if (runningStatus < 0) {
		throw_exception("Running status used for first event in track");
	    }

	    eventCode = (MIDIByte)runningStatus;
	    data1 = midiByte;

#ifdef DEBUG_MIDI_FILE_READER
	    cerr << "using running status (byte " << int(midiByte) << " found)" << endl;
#endif
        } else {
#ifdef DEBUG_MIDI_FILE_READER
	    cerr << "have new event code " << int(midiByte) << endl;
#endif
            eventCode = midiByte;
	    data1 = getMIDIByte();
	}

        if (eventCode == MIDI_FILE_META_EVENT) {

	    metaEventCode = data1;
            messageLength = getNumberFromMIDIBytes();

#ifdef DEBUG_MIDI_FILE_READER
		cerr << "Meta event of type " << int(metaEventCode) << " and " << messageLength << " bytes found, putting on track " << metaTrack << endl;
#endif
            metaMessage = getMIDIBytes(messageLength);

	    accumulatedTime += deltaTime;

            MIDIEvent e(deltaTime,
                        MIDI_FILE_META_EVENT,
                        metaEventCode,
                        metaMessage);

	    m_midiComposition[trackNum].push_back(e);

	    if (metaEventCode == MIDI_TRACK_NAME) {
		m_trackNames[trackNum] = metaMessage.c_str();
	    }

        } else { // non-meta events

	    runningStatus = eventCode;

	    int channel = (eventCode & MIDI_CHANNEL_NUM_MASK);
	    
	    accumulatedTime += deltaTime;

            switch (eventCode & MIDI_MESSAGE_TYPE_MASK) {

            case MIDI_NOTE_ON:
            case MIDI_NOTE_OFF:
            case MIDI_POLY_AFTERTOUCH:
            case MIDI_CTRL_CHANGE:
                data2 = getMIDIByte();

                {
                // create and store our event
                MIDIEvent midiEvent(deltaTime, eventCode, data1, data2);

#ifdef DEBUG_MIDI_FILE_READER
		cerr << "MIDI event for channel " << channel << " (track "
                     << trackNum << ") with delta time " << deltaTime << endl;
#endif

                m_midiComposition[trackNum].push_back(midiEvent);
                }
                break;

            case MIDI_PITCH_BEND:
                data2 = getMIDIByte();

                {
                // create and store our event
                MIDIEvent midiEvent(deltaTime, eventCode, data1, data2);
                m_midiComposition[trackNum].push_back(midiEvent);
                }
                break;

            case MIDI_PROG_CHANGE:
            case MIDI_CHNL_AFTERTOUCH:
                
                {
                // create and store our event
                MIDIEvent midiEvent(deltaTime, eventCode, data1);
                m_midiComposition[trackNum].push_back(midiEvent);
                }
                break;

            case MIDI_SYSTEM_EXCLUSIVE:
                messageLength = getNumberFromMIDIBytes(data1);

#ifdef DEBUG_MIDI_FILE_READER
		cerr << "SysEx of " << messageLength << " bytes found" << endl;
#endif

                metaMessage= getMIDIBytes(messageLength);

                if (MIDIByte(metaMessage[metaMessage.length() - 1]) !=
                        MIDI_END_OF_EXCLUSIVE)
                {
#ifdef DEBUG_MIDI_FILE_READER
                    cerr << "MIDIFileReader::parseTrack() - "
                              << "malformed or unsupported SysEx type"
                              << endl;
#endif
                    continue;
                }

                // chop off the EOX 
                // length fixed by Pedro Lopez-Cabanillas (20030523)
                //
                metaMessage = metaMessage.substr(0, metaMessage.length()-1);

                {
                MIDIEvent midiEvent(deltaTime,
                                    MIDI_SYSTEM_EXCLUSIVE,
                                    metaMessage);
                m_midiComposition[trackNum].push_back(midiEvent);
                }
                break;

            case MIDI_END_OF_EXCLUSIVE:
#ifdef DEBUG_MIDI_FILE_READER
                cerr << "MIDIFileReader::parseTrack() - "
                          << "Found a stray MIDI_END_OF_EXCLUSIVE" << endl;
#endif
                break;

            default:
#ifdef DEBUG_MIDI_FILE_READER
                cerr << "MIDIFileReader::parseTrack()" 
                          << " - Unsupported MIDI Event Code:  "
                          << (int)eventCode << endl;
#endif
                break;
            } 
        }
    }

    return true;
}

// Delete dead NOTE OFF and NOTE ON/Zero Velocity Events after
// reading them and modifying their relevant NOTE ONs.  Return true
// if there are some notes in this track.
//
bool
MIDIFileReader::consolidateNoteOffEvents(unsigned int track)
{
    bool notesOnTrack = false;
    bool noteOffFound;

    MIDITrack &t = m_midiComposition[track];

    for (MIDITrack::iterator i = t.begin(); i != t.end(); ++i) {

        if (i->getMessageType() == MIDI_NOTE_ON && i->getVelocity() > 0) {

#ifdef DEBUG_MIDI_FILE_READER
            cerr << "Looking for note-offs for note at " << i->getTime() << " (pitch " << (int)i->getPitch() << ")" <<  endl;
#endif

	    notesOnTrack = true;
            noteOffFound = false;

            for (MIDITrack::iterator j = i; j != t.end(); ++j) {

                if ((j->getChannelNumber() == i->getChannelNumber()) &&
		    (j->getPitch() == i->getPitch()) &&
                    (j->getMessageType() == MIDI_NOTE_OFF ||
                    (j->getMessageType() == MIDI_NOTE_ON &&
                     j->getVelocity() == 0x00))) {

#ifdef DEBUG_MIDI_FILE_READER
                    cerr << "Found note-off at " << j->getTime() << " for note at " << i->getTime() << endl;
#endif

                    i->setDuration(j->getTime() - i->getTime());

#ifdef DEBUG_MIDI_FILE_READER
                    cerr << "Duration is now " << i->getDuration() << endl;
#endif

                    t.erase(j);

                    noteOffFound = true;
                    break;
                }
            }

            // If no matching NOTE OFF has been found then set
            // Event duration to length of track
            //
            if (!noteOffFound) {
#ifdef DEBUG_MIDI_FILE_READER
                cerr << "Failed to find note-off for note at " << i->getTime() << endl;
#endif
		MIDITrack::iterator j = t.end();
		--j;
                i->setDuration(j->getTime() - i->getTime());
	    }
        }
    }

    return notesOnTrack;
}

MIDIComposition
MIDIFileReader::load() const
{
    return m_midiComposition;
}


