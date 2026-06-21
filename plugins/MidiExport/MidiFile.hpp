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
#include <array>
#include <limits>

using std::string;
using std::vector;
using std::set;

namespace MidiFile 
{

const int TICKSPERBEAT = 128;


enum class MIDIFormat : uint16_t
{
	Type0 = 0,
	Type1 = 1,
	Type2 = 2,
};


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
	uint8_t little_endian[4];
	uint8_t result = val & 0x7F;
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
	MIDIFormat format;
	uint16_t numTracks;
	uint16_t ticksPerBeat;
	
	public:
	
	MIDIHeader(uint16_t nTracks, MIDIFormat fileFormat = MIDIFormat::Type1,
			uint16_t ticksPB = TICKSPERBEAT) :
		format(fileFormat), numTracks(nTracks), ticksPerBeat(ticksPB) {}
	
	inline int writeToBuffer(uint8_t *buffer, int start=0) const
	{
		// chunk ID
		buffer[start++] = 'M'; buffer[start++] = 'T'; buffer[start++] = 'h'; buffer[start++] = 'd';
		// chunk size (6 bytes always)
		buffer[start++] = 0; buffer[start++] = 0; buffer[start++] = 0; buffer[start++] = 0x06;
		// format: Type 1 is a synchronous multitrack Standard MIDI File.
		start += writeBigEndian2(static_cast<uint16_t>(format), buffer + start);
		
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
		int size = 0;
		switch (type)
		{
			case NOTE_ON:
			{
				uint8_t code = 0x9 << 4 | channel;
				size += writeVarLength(time, buffer+size);
				buffer[size++] = code;
				buffer[size++] = pitch;
				buffer[size++] = volume;
				break;
			}
			case NOTE_OFF:
			{
				uint8_t code = 0x8 << 4 | channel;
				size += writeVarLength(time, buffer+size);
				buffer[size++] = code;
				buffer[size++] = pitch;
				buffer[size++] = volume;
				break;
			}
			case TEMPO:
			{
				uint8_t code = 0xFF;
				size += writeVarLength(time, buffer+size);
				buffer[size++] = code;
				buffer[size++] = 0x51;
				buffer[size++] = 0x03;

				std::array<uint8_t, 4> fourbytes;
				writeBigEndian4(int(60000000.0 / tempo), fourbytes.data());
				
				//printf("tempo of %x translates to ", tempo);
				/*
				for (int i=0; i<3; i++) printf("%02x ", fourbytes[i+1]);
				printf("\n");
				*/
				buffer[size++] = fourbytes[1];
				buffer[size++] = fourbytes[2];
				buffer[size++] = fourbytes[3];
				break;
			}
			case PROG_CHANGE:
			{
				uint8_t code = 0xC << 4 | channel;
				size += writeVarLength(time, buffer+size);
				buffer[size++] = code;
				buffer[size++] = programNumber;
				break;
			}
			case TRACK_NAME:
			{
				size += writeVarLength(time, buffer+size);
				buffer[size++] = 0xFF;
				buffer[size++] = 0x03;
				size += writeVarLength(trackName.size(), buffer+size);
				trackName.copy((char *)(&buffer[size]), trackName.size());
				size += trackName.size();
				break;
				//				 buffer[size++] = '\0';
				//				 buffer[size++] = '\0';
			}
		}
		return size;
	} // writeEventsToBuffer

	inline void appendTo(vector<uint8_t>& buffer) const
	{
		std::array<uint8_t, 4> variableLength;
		const auto appendVariableLength = [&buffer, &variableLength](uint32_t value)
		{
			const int length = writeVarLength(value, variableLength.data());
			buffer.insert(buffer.end(), variableLength.begin(),
				variableLength.begin() + length);
		};

		appendVariableLength(time);
		switch (type)
		{
			case NOTE_ON:
				buffer.push_back(0x9 << 4 | channel);
				buffer.push_back(pitch);
				buffer.push_back(volume);
				break;
			case NOTE_OFF:
				buffer.push_back(0x8 << 4 | channel);
				buffer.push_back(pitch);
				buffer.push_back(volume);
				break;
			case TEMPO:
			{
				buffer.push_back(0xFF);
				buffer.push_back(0x51);
				buffer.push_back(0x03);

				std::array<uint8_t, 4> fourbytes;
				writeBigEndian4(static_cast<uint32_t>(60000000.0 / tempo), fourbytes.data());
				buffer.insert(buffer.end(), fourbytes.begin() + 1, fourbytes.end());
				break;
			}
			case PROG_CHANGE:
				buffer.push_back(0xC << 4 | channel);
				buffer.push_back(programNumber);
				break;
			case TRACK_NAME:
				buffer.push_back(0xFF);
				buffer.push_back(0x03);
				appendVariableLength(static_cast<uint32_t>(trackName.size()));
				buffer.insert(buffer.end(), trackName.begin(), trackName.end());
				break;
		}
	}
	
	
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
		addNoteAtTick(pitch, volume,
			static_cast<uint32_t>(time * TICKSPERBEAT),
			static_cast<uint32_t>((time + duration) * TICKSPERBEAT));
	}

	inline void addNoteAtTick(uint8_t pitch, uint8_t volume,
			uint32_t startTime, uint32_t endTime)
	{
		Event event; event.channel = channel;
		event.volume = volume;
		
		event.type = Event::NOTE_ON; event.pitch = pitch; event.time = startTime;
		addEvent(event);
		
		event.type = Event::NOTE_OFF; event.pitch = pitch; event.time = endTime;
		addEvent(event);
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
	
	inline void addTempo(uint32_t tempo, uint32_t time)
	{
		Event event;
		event.channel = channel;

		event.type = Event::TEMPO;
		event.time = time;
		event.tempo = tempo;

		addEvent(event);
	}

	inline vector<uint8_t> writeToVector() const
	{
		vector<Event> sortedEvents = events;
		std::sort(sortedEvents.begin(), sortedEvents.end());

		vector<uint8_t> eventBytes;
		eventBytes.reserve(sortedEvents.size() * 4 + 4);
		uint32_t timeLast = 0;
		for (Event event : sortedEvents)
		{
			assert(event.time >= timeLast);
			const uint32_t eventTime = event.time;
			event.time -= timeLast;
			timeLast = eventTime;
			event.appendTo(eventBytes);
		}

		// End of track meta event.
		eventBytes.insert(eventBytes.end(), {0x00, 0xFF, 0x2F, 0x00});
		if (eventBytes.size() > std::numeric_limits<uint32_t>::max())
		{
			return {};
		}

		vector<uint8_t> trackBytes;
		trackBytes.reserve(eventBytes.size() + 8);
		trackBytes.insert(trackBytes.end(), {'M', 'T', 'r', 'k'});
		std::array<uint8_t, 4> length;
		writeBigEndian4(static_cast<uint32_t>(eventBytes.size()), length.data());
		trackBytes.insert(trackBytes.end(), length.begin(), length.end());
		trackBytes.insert(trackBytes.end(), eventBytes.begin(), eventBytes.end());
		return trackBytes;
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
		uint32_t time_last = 0;
		for (it = _events.begin(); it!=_events.end(); ++it)
		{
			Event e = *it;
			if (e.time < time_last){
				printf("error: e.time=%d  time_last=%d\n", e.time, time_last);
				assert(false);
			}
			uint32_t tmp = e.time;
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
		const auto trackBytes = writeToVector();
		if (trackBytes.empty() || start < 0 || start > MAX_TRACK_SIZE ||
			trackBytes.size() > static_cast<size_t>(MAX_TRACK_SIZE - start))
		{
			return 0;
		}
		memmove(buffer + start, trackBytes.data(), trackBytes.size());
		return start + static_cast<int>(trackBytes.size());
	}
};

}; // namespace

#endif
