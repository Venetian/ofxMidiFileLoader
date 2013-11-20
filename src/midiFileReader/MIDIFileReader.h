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

#ifndef _MIDI_FILE_READER_H_
#define _MIDI_FILE_READER_H_

#include "MIDIComposition.h"

#include <set>
#include <iostream>

typedef unsigned char MIDIByte;

class MIDIFileReader
{
public:
    MIDIFileReader(std::string path);
    virtual ~MIDIFileReader();

    virtual bool isOK() const;
    virtual std::string getError() const;

    virtual MIDIComposition load() const;

    MIDIConstants::MIDIFileFormatType getFormat() const { return m_format; }
    int getTimingDivision() const { return m_timingDivision; }

protected:

    bool parseFile();
    bool parseHeader(const std::string &midiHeader);
    bool parseTrack(unsigned int trackNum);
    bool consolidateNoteOffEvents(unsigned int track);

    // Internal convenience functions
    //
    int  midiBytesToInt(const std::string &bytes);
    long midiBytesToLong(const std::string &bytes);

    long getNumberFromMIDIBytes(int firstByte = -1);

    MIDIByte getMIDIByte();
    std::string getMIDIBytes(unsigned long bytes);

    bool skipToNextTrack();

    int                    m_timingDivision;   // pulses per quarter note
    MIDIConstants::MIDIFileFormatType m_format;
    unsigned int           m_numberOfTracks;

    long                   m_trackByteCount;
    bool                   m_decrementCount;

    std::map<int, std::string> m_trackNames;
    MIDIComposition        m_midiComposition;

    std::string            m_path;
    std::ifstream         *m_midiFile;
    size_t                 m_fileSize;
    std::string            m_error;
};


#endif // _MIDI_FILE_READER_H_
