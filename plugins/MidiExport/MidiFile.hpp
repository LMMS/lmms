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

#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cassert>

using std::string;
using std::vector;
using std::sort;

/*---------------------------------------------------------------------------*/

namespace MidiFile
{

//! Default number of ticks per single beat
static constexpr u_int16_t TICKS_PER_BEAT = 128;

/*! \brief Accept an input, and write a MIDI-compatible variable length stream.
 *
 *  The MIDI format is a little strange, and makes use of so-called
 *  variable length quantities. These quantities are a stream of bytes.
 *  If the most significant bit is 1, then more bytes follow. If it is
 *  zero, then the byte in question is the last in the stream.
 */
size_t writeVarLength(uint32_t val, uint8_t *buffer)
{
	size_t size = 0;
	uint8_t result, little_endian[4];

	// Build little endian array from 7-bit packs
	result = val & 0x7F;
	little_endian[size++] = result;
	val >>= 7;
	while (val > 0)
	{
		result = val & 0x7F;
		result |= 0x80;
		little_endian[size++] = result;
		val >>= 7;
	}
	// Reverse it for the actual buffer
	for (int i = 0; i < size; i++)
	{
		buffer[i] = little_endian[size-i-1];
	}
	return size;
}

//! Buffer gets four 8-bit values from left to right
const size_t writeBigEndian4(uint32_t val, uint8_t *buffer)
{
	buffer[0] = val >> 24;
	buffer[1] = (val >> 16) & 0xff;
	buffer[2] = (val >> 8) & 0xff;
	buffer[3] = val & 0xff;
	return 4;
}

//! Buffer gets two 8-bit values from left to right
const size_t writeBigEndian2(uint16_t val, uint8_t *buffer)
{
	buffer[0] = (val >> 8) & 0xff;
	buffer[1] = val & 0xff;
	return 2;
}

/*---------------------------------------------------------------------------*/

//! Class to encapsulate MIDI header structure
class MIDIHeader
{
private:
	//! Number of tracks in MIDI file
	uint16_t numTracks;

	//! How many ticks each beat has
	uint16_t ticksPerBeat;

	/*-----------------------------------------------------------------------*/

public:
	MIDIHeader(uint16_t numTracks, uint16_t ticksPerBeat=TICKS_PER_BEAT):
			numTracks(numTracks),
			ticksPerBeat(ticksPerBeat) {}

	//! Write header info to buffer
	inline size_t writeToBuffer(uint8_t *buffer, size_t start=0) const
	{
		// Chunk ID
		buffer[start++] = 'M';
		buffer[start++] = 'T';
		buffer[start++] = 'h';
		buffer[start++] = 'd';

		// Chunk size (6 bytes always)
		buffer[start++] = 0;
		buffer[start++] = 0;
		buffer[start++] = 0;
		buffer[start++] = 0x06;

		// Format: 1 (multitrack)
		buffer[start++] = 0;
		buffer[start++] = 0x01;

		// Write chunks to buffer
		start += writeBigEndian2(numTracks, buffer + start);
		start += writeBigEndian2(ticksPerBeat, buffer + start);
		return start;
	}
};

/*---------------------------------------------------------------------------*/

//! Represents a single MIDI event
struct Event
{
	//! Possible event types
	enum { NOTE_ON, NOTE_OFF, TEMPO, PROG_CHANGE, TRACK_NAME } type;

	//! Time count when event happens
	uint32_t time;

	//! Channel number where event is
	uint8_t channel;

	// Union for saving space
	union
	{
		struct
		{
			//! Note pitch
			uint8_t pitch;

			//! Note volume
			uint8_t volume;
		}
		note;

		//! Tempo of event (in BPM)
		uint32_t tempo;

		//! Program number where event is
		uint8_t programNumber;
	};

	//! Name of track where event is
	//  (too much trouble putting it inside union...)
	string trackName;

	/*-----------------------------------------------------------------------*/

	//! Write MIDI event info to buffer
	inline size_t writeToBuffer(uint8_t *buffer) const
	{
		uint8_t code, fourBytes[4];
		size_t size = 0;

		switch (type)
		{
			case NOTE_ON:
				// A note starts playing
				code = (0x9 << 4) | channel;
				size = writeVarLength(time, buffer);
				buffer[size++] = code;
				buffer[size++] = note.pitch;
				buffer[size++] = note.volume;
				break;

			case NOTE_OFF:
				// A note finishes playing
				code = (0x8 << 4) | channel;
				size = writeVarLength(time, buffer);
				buffer[size++] = code;
				buffer[size++] = note.pitch;
				buffer[size++] = note.volume;
				break;

			case TEMPO:
				// A tempo measure
				code = 0xFF;
				size = writeVarLength(time, buffer);
				buffer[size++] = code;
				buffer[size++] = 0x51;
				buffer[size++] = 0x03;

				// Convert to microseconds before writting
				writeBigEndian4(6e7 / tempo, fourBytes);
				buffer[size++] = fourBytes[1];
				buffer[size++] = fourBytes[2];
				buffer[size++] = fourBytes[3];
				break;

			case PROG_CHANGE:
				// Change to another numbered program
				code = (0xC << 4) | channel;
				size = writeVarLength(time, buffer);
				buffer[size++] = code;
				buffer[size++] = programNumber;
				break;

			case TRACK_NAME:
				// Name of current track
				size = writeVarLength(time, buffer);
				buffer[size++] = 0xFF;
				buffer[size++] = 0x03;

				// Write name string size and then copy it's content
				// to the following size bytes of buffer
				size += writeVarLength(trackName.size(), buffer + size);
				trackName.copy((char *) &buffer[size], trackName.size());
				size += trackName.size();
				break;
		}
		return size;
	}

	//! \brief Comparison operator
	//  Events are sorted by their time
	inline bool operator<(const Event& b) const
	{
		if (time < b.time)
		{
			return true;
		}
		// In case of same time events, sort by event priority
		return (time == b.time and type > b.type);
	}
};

/*---------------------------------------------------------------------------*/

//! Class that encapsulates a MIDI track.
//  Maximum available track size defined in template
template<const uint32_t MAX_TRACK_SIZE>
class MIDITrack
{
private:
	//! Variable-length vector of events
	vector<Event> events;

	//! Append a single event to vector
	inline void addEvent(const Event &e, uint32_t time)
	{
		Event event = e;
		event.time = time;
		event.channel = channel;
		events.push_back(event);
	}

public:
	//! Channel number corresponding to self
	uint8_t channel = 0;

	// TODO: Constructor?

	/*-----------------------------------------------------------------------*/

	//! Add both NOTE_ON and NOTE_OFF effects, starting at
	//  <realTime> continuous time and separated by a <duration> delay
	inline void addNote(uint8_t pitch, uint8_t volume,
			double realTime, double duration)
	{
		Event event;
		event.note.volume = volume;

		// Add start of note
		event.type = Event::NOTE_ON;
		event.note.pitch = pitch;
		uint32_t time = realTime * TICKS_PER_BEAT;
		addEvent(event, time);

		// Add end of note
		event.type = Event::NOTE_OFF;
		event.note.pitch = pitch;
		time = (realTime + duration) * TICKS_PER_BEAT;
		addEvent(event, time);
	}

	//! Add a tempo mark
	inline void addTempo(uint8_t tempo, uint32_t time)
	{
		Event event;
		event.type = Event::TEMPO;
		event.tempo = tempo;
		addEvent(event, time);
	}

	//! Add a program change event
	inline void addProgramChange(uint8_t prog, uint32_t time)
	{
		Event event;
		event.type = Event::PROG_CHANGE;
		event.programNumber = prog;
		addEvent(event, time);
	}

	//! Add a track name event
	inline void addName(const string &name, uint32_t time)
	{
		Event event;
		event.type = Event::TRACK_NAME;
		event.trackName = name;
		addEvent(event, time);
	}

	/*-----------------------------------------------------------------------*/

	//! Write the meta data and note data to the packed MIDI stream
	inline size_t writeMIDIToBuffer(uint8_t *buffer, size_t start=0) const
	{
		// Process events in the eventList
		start += writeEventsToBuffer(buffer, start);

		// Write MIDI close event
		buffer[start++] = 0x00;
		buffer[start++] = 0xFF;
		buffer[start++] = 0x2F;
		buffer[start++] = 0x00;

		// Return the entire length of the data
		return start;
	}

	//! Write the events in MIDIEvents to the MIDI stream
	inline size_t writeEventsToBuffer(uint8_t *buffer, size_t start=0) const
	{
		// Created sorted vector of events
		vector<Event> eventsSorted = events;
		sort(eventsSorted.begin(), eventsSorted.end());

		uint32_t timeLast = 0;
		for (Event &e : eventsSorted)
		{
			// If something went wrong on sorting, maybe?
			if (e.time < timeLast)
			{
				fprintf(stderr, "error: e.time=%d timeLast=%d\n",
						e.time, timeLast);
				assert(false);
			}
			uint32_t tmp = e.time;
			e.time -= timeLast;
			timeLast = tmp;

			// Write event to buffer
			start += e.writeToBuffer(buffer + start);

			// In case of exceding maximum size, go away
			if (start >= MAX_TRACK_SIZE) {
				break;
			}
		}
		return start;
	}

	//! Write MIDI track to buffer
	inline size_t writeToBuffer(uint8_t *buffer, size_t start=0) const
	{
		// Write all events to buffer
		uint8_t eventsBuffer[MAX_TRACK_SIZE];
		uint32_t eventsSize = writeMIDIToBuffer(eventsBuffer);

		// Chunk ID
		buffer[start++] = 'M';
		buffer[start++] = 'T';
		buffer[start++] = 'r';
		buffer[start++] = 'k';

		// Chunk size
		start += writeBigEndian4(eventsSize, buffer + start);

		// Copy events data
		memmove(buffer + start, eventsBuffer, eventsSize);
		start += eventsSize;
		return start;
	}
};

/*---------------------------------------------------------------------------*/

}; // namespace

#endif
