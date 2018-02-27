#ifndef MIDIFILE_HPP
#define MIDIFILE_HPP

/**
 * Name:        MidiFile.hpp
 * Purpose:     C++ re-write of the python module MidiFile.py
 * Author:      Mohamed Abdel Maksoud <mohamed at amaksoud.com>
 *-----------------------------------------------------------------------------
 * Name:        MidiFile.py
 * Purpose:     MIDI file manipulation utilities
 *
 * Author:      Mark Conway Wirt <emergentmusics) at (gmail . com>
 *
 * Created:     2008/04/17
 * Copyright:   (c) 2009 Mark Conway Wirt
 * License:     Please see License.txt for the terms under which this
 *              software is distributed.
 *-----------------------------------------------------------------------------
 */

#include <string.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <assert.h>

using std::string;
using std::vector;
using std::set;

namespace MidiFile 
{

const int TICKSPERBEAT = 128;


int writeVarLength(uint32_t val, uint8_t *buffer)
{
	/*
	Accept an input, and write a MIDI-compatible variable length stream
	
	The MIDI format is a little strange, and makes use of so-called variable
	length quantities. These quantities are a stream of bytes. If the most
	significant bit is 1, then more bytes follow. If it is zero, then the
	byte in question is the last in the stream
	*/
	int size = 0;
	uint8_t result, little_endian[4];
	result = val & 0x7F;
	little_endian[size++] = result;
	val = val >> 7;
	while (val > 0)
	{
		result = val & 0x7F;
		result = result | 0x80;
		little_endian[size++] = result;
		val = val >> 7;
	}
	for (int i=0; i<size; i++)
	{
		buffer[i] = little_endian[size-i-1];
	}

	return size;
}

int writeBigEndian4(uint32_t val, uint8_t *buf)
{
	buf[0] = val >> 24;
	buf[1] = val >> 16 & 0xff;
	buf[2] = val >> 8 & 0xff;
	buf[3] = val & 0xff;
	return 4;
}

int writeBigEndian2(uint16_t val, uint8_t *buf)
{
	buf[0] = val >> 8 & 0xff;
	buf[1] = val & 0xff;
	return 2;
}


class MIDIHeader
{
	// Class to encapsulate the MIDI header structure.
	uint16_t numTracks;
	uint16_t ticksPerBeat;
	
	public:
	
	MIDIHeader(uint16_t nTracks, uint16_t ticksPB=TICKSPERBEAT): numTracks(nTracks), ticksPerBeat(ticksPB) {}
	
	inline int writeToBuffer(uint8_t *buffer, int start=0) const
	{
		// chunk ID
		buffer[start++] = 'M'; buffer[start++] = 'T'; buffer[start++] = 'h'; buffer[start++] = 'd';
		// chunk size (6 bytes always)
		buffer[start++] = 0; buffer[start++] = 0; buffer[start++] = 0; buffer[start++] = 0x06;
		// format: 1 (multitrack)
		buffer[start++] = 0; buffer[start++] = 0x01;
		
		start += writeBigEndian2(numTracks, buffer+start);
		
		start += writeBigEndian2(ticksPerBeat, buffer+start);
		
		return start;
	}
	
};


struct Event
{
	uint32_t time;
	uint32_t tempo;
	string trackName;
	enum {NOTE_ON, NOTE_OFF, TEMPO, PROG_CHANGE, TRACK_NAME} type;
	// TODO make a union to save up space
	uint8_t pitch;
	uint8_t programNumber;
	uint8_t duration;
	uint8_t volume;
	uint8_t channel;
	
	Event() {time=tempo=pitch=programNumber=duration=volume=channel=0; trackName="";}
	
	inline int writeToBuffer(uint8_t *buffer) const 
	{
		uint8_t code, fourbytes[4];
		int size=0;
		switch (type) 
		{
			case NOTE_ON:
				code = 0x9 << 4 | channel;
				size += writeVarLength(time, buffer+size);
				buffer[size++] = code;
				buffer[size++] = pitch;
				buffer[size++] = volume;
				break;
			case NOTE_OFF:
				code = 0x8 << 4 | channel;
				size += writeVarLength(time, buffer+size);
				buffer[size++] = code;
				buffer[size++] = pitch;
				buffer[size++] = volume;
				break;
			case TEMPO:
				code = 0xFF;
				size += writeVarLength(time, buffer+size);
				buffer[size++] = code;
				buffer[size++] = 0x51;
				buffer[size++] = 0x03;
				writeBigEndian4(int(60000000.0 / tempo), fourbytes);
				
				//printf("tempo of %x translates to ", tempo);
				/*
				for (int i=0; i<3; i++) printf("%02x ", fourbytes[i+1]);
				printf("\n");
				*/
				buffer[size++] = fourbytes[1];
				buffer[size++] = fourbytes[2];
				buffer[size++] = fourbytes[3];
				break;
			case PROG_CHANGE:
				code = 0xC << 4 | channel;
				size += writeVarLength(time, buffer+size);
				buffer[size++] = code;
				buffer[size++] = programNumber;
				break;
			case TRACK_NAME:
				size += writeVarLength(time, buffer+size);
				buffer[size++] = 0xFF;
				buffer[size++] = 0x03;
				size += writeVarLength(trackName.size(), buffer+size);
				trackName.copy((char *)(&buffer[size]), trackName.size());
				size += trackName.size();
//				 buffer[size++] = '\0';
//				 buffer[size++] = '\0';
				
				break;
		}
		return size;
	} // writeEventsToBuffer
	
	
	// events are sorted by their time
	inline bool operator < (const Event& b) const {
		return this->time < b.time ||
			(this->time == b.time && this->type > b.type);
	}
};

template<const int MAX_TRACK_SIZE>
class MIDITrack
{
	// A class that encapsulates a MIDI track
	// Nested class definitions.
	vector<Event> events;
	
	public:
	uint8_t channel;
	
	MIDITrack(): channel(0) {}
		
	inline void addEvent(const Event &e)
	{
		Event E = e;
		events.push_back(E);
	}
	
	inline void addNote(uint8_t pitch, uint8_t volume, double time, double duration)
	{
		Event event; event.channel = channel;
		event.volume = volume;
		
		event.type = Event::NOTE_ON; event.pitch = pitch; event.time= (uint32_t) (time * TICKSPERBEAT);
		addEvent(event);
		
		event.type = Event::NOTE_OFF; event.pitch = pitch; event.time=(uint32_t) ((time+duration) * TICKSPERBEAT);
		addEvent(event);
		
		//printf("note: %d-%d\n", (uint32_t) time * TICKSPERBEAT, (uint32_t)((time+duration) * TICKSPERBEAT));
	}
	
	inline void addName(const string &name, uint32_t time)
	{
		Event event; event.channel = channel;
		event.type = Event::TRACK_NAME; event.time=time; event.trackName = name;
		addEvent(event);
	}
	
	inline void addProgramChange(uint8_t prog, uint32_t time)
	{
		Event event; event.channel = channel;
		event.type = Event::PROG_CHANGE; event.time=time; event.programNumber = prog;
		addEvent(event);
	}
	
	inline void addTempo(uint8_t tempo, uint32_t time)
	{
		Event event; event.channel = channel;
		event.type = Event::TEMPO; event.time=time; event.tempo = tempo;
		addEvent(event);
	}
	
	inline int writeMIDIToBuffer(uint8_t *buffer, int start=0) const
	{
		// Write the meta data and note data to the packed MIDI stream.
		// Process the events in the eventList

		start += writeEventsToBuffer(buffer, start);

		// Write MIDI close event.
		buffer[start++] = 0x00;
		buffer[start++] = 0xFF;
		buffer[start++] = 0x2F;
		buffer[start++] = 0x00;
		
		// return the entire length of the data and write to the header
		
		return start;
	}

	inline int writeEventsToBuffer(uint8_t *buffer, int start=0) const
	{
		// Write the events in MIDIEvents to the MIDI stream.
		vector<Event> _events = events;
		std::sort(_events.begin(), _events.end());
		vector<Event>::const_iterator it;
		uint32_t time_last = 0, tmp;
		for (it = _events.begin(); it!=_events.end(); ++it)
		{
			Event e = *it;
			if (e.time < time_last){
				printf("error: e.time=%d  time_last=%d\n", e.time, time_last);
				assert(false);
			}
			tmp = e.time;
			e.time -= time_last;
			time_last = tmp;
			start += e.writeToBuffer(buffer+start);
			if (start >= MAX_TRACK_SIZE) {
				break;
			}
		}
		return start;
	}
	
	inline int writeToBuffer(uint8_t *buffer, int start=0) const
	{
		uint8_t eventsBuffer[MAX_TRACK_SIZE];
		uint32_t events_size = writeMIDIToBuffer(eventsBuffer);
		//printf(">> track %lu events took 0x%x bytes\n", events.size(), events_size);
		
		// chunk ID
		buffer[start++] = 'M'; buffer[start++] = 'T'; buffer[start++] = 'r'; buffer[start++] = 'k';
		// chunk size
		start += writeBigEndian4(events_size, buffer+start);
		// copy events data
		memmove(buffer+start, eventsBuffer, events_size);
		start += events_size;
		return start;
	}
};

}; // namespace

#endif
