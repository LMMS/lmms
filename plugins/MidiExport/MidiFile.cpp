/**
 * Name:        MidiFile.cpp
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

#include "MidiFile.h"

#include <algorithm>
#include <stack>
#include <string>
#include <vector>
#include <cassert>

#include <QDataStream>
#include <QFile>
#include <QSharedPointer>
#include <QString>

namespace lmms
{

/*---------------------------------------------------------------------------*/

MidiFile::MidiFile(const QString &filename, int nTracks):
		m_file(filename),
		m_header(nTracks)
{
	// Open designated blank MIDI file (and data stream) for writing
	m_file.open(QIODevice::WriteOnly);
	m_stream = QSharedPointer<QDataStream>(new QDataStream(&m_file));

	// Resize track list
	m_tracks.resize(nTracks);
}

void MidiFile::writeAllToStream()
{
	// reinterpret_cast is used to convert raw uint8_t data to char
	m_stream->writeRawData(
			reinterpret_cast<char *>(m_header.m_buffer.data()),
			m_header.m_buffer.size());
	for (Track &track : m_tracks)
	{
		m_stream->writeRawData(
				reinterpret_cast<char *>(track.m_buffer.data()),
				track.m_buffer.size());
	}
}

/*---------------------------------------------------------------------------*/

MidiFile::Section::Section()
{
	// Increases allocated capacity (not size)
	m_buffer.reserve(BUFFER_SIZE);
}

void MidiFile::Section::writeBytes(std::vector<uint8_t> bytes,
		std::vector<uint8_t> *v)
{
	// Insert <bytes> content to the end of *v
	if (!v) v = &m_buffer;
	v->insert(v->end(), bytes.begin(), bytes.end());
}

void MidiFile::Section::writeVarLength(uint32_t val)
{
	// Build little endian stack from 7-bit packs
	uint8_t result = val & 0x7F;
	std::stack<uint8_t> little_endian({result});
	val >>= 7;
	while (val > 0)
	{
		result = val & 0x7F;
		result |= 0x80;
		little_endian.push(result);
		val >>= 7;
	}
	// Add packs in reverse order to actual buffer
	while (!little_endian.empty())
	{
		m_buffer.push_back(little_endian.top());
		little_endian.pop();
	}
}

void MidiFile::Section::writeBigEndian4(uint32_t val,
		std::vector<uint8_t> *v)
{
	std::vector<uint8_t> bytes;
	bytes.push_back(val >> 24);
	bytes.push_back((val >> 16) & 0xff),
	bytes.push_back((val >> 8) & 0xff);
	bytes.push_back(val & 0xff);
	writeBytes(bytes, v);
}

void MidiFile::Section::writeBigEndian2(uint16_t val,
		std::vector<uint8_t> *v)
{
	std::vector<uint8_t> bytes;
	bytes.push_back(val >> 8);
	bytes.push_back(val & 0xff);
	writeBytes(bytes, v);
}

/*---------------------------------------------------------------------------*/

MidiFile::Header::Header(int numTracks, int ticksPerBeat):
		m_numTracks(numTracks),
		m_ticksPerBeat(ticksPerBeat) {}

void MidiFile::Header::writeToBuffer()
{
	// Chunk ID
	writeBytes({'M', 'T', 'h', 'd'});

	// Chunk size (6 bytes always)
	writeBytes({0, 0, 0, 0x06});

	// Format: 1 (multitrack)
	writeBytes({0, 0x01});

	// Track and ticks info
	writeBigEndian2(m_numTracks);
	writeBigEndian2(m_ticksPerBeat);
}

/*---------------------------------------------------------------------------*/

void MidiFile::Track::addEvent(Event event, uint32_t time)
{
	event.m_time = time;
	event.m_channel = m_channel;
	m_events.push_back(event);
}

void MidiFile::Track::addNote(uint8_t pitch, uint8_t volume,
		double realTime, double duration)
{
	Event event;
	event.m_note.volume = volume;

	// Add start of note
	event.m_type = Event::NOTE_ON;
	event.m_note.pitch = pitch;
	uint32_t time = realTime * TICKS_PER_BEAT;
	addEvent(event, time);

	// Add end of note
	event.m_type = Event::NOTE_OFF;
	event.m_note.pitch = pitch;
	time = (realTime + duration) * TICKS_PER_BEAT;
	addEvent(event, time);
}

void MidiFile::Track::addTempo(uint32_t tempo, uint32_t time)
{
	Event event;
	event.m_type = Event::TEMPO;
	event.m_tempo = tempo;
	addEvent(event, time);
}

void MidiFile::Track::addProgramChange(uint8_t prog, uint32_t time)
{
	Event event;
	event.m_type = Event::PROG_CHANGE;
	event.m_programNumber = prog;
	addEvent(event, time);
}

void MidiFile::Track::addName(const std::string &name, uint32_t time)
{
	Event event;
	event.m_type = Event::TRACK_NAME;
	event.m_trackName = name;
	addEvent(event, time);
}

void MidiFile::Track::writeToBuffer()
{
	// Chunk ID
	writeBytes({'M', 'T', 'r', 'k'});

	// Chunk size placeholder
	writeBigEndian4(0);
	size_t idx = m_buffer.size();

	// Write all events to buffer
	writeMIDIToBuffer();

	// Write correct size in placeholder place
	size_t size = m_buffer.size() - idx;
	std::vector<uint8_t> v;
	writeBigEndian4(size, &v);
	for (size_t i = 0; i < 4; ++i)
	{
		m_buffer[idx - 4 + i] = v[i];
	}
}

void MidiFile::Track::writeMIDIToBuffer()
{
	// Process events in the eventList
	writeEventsToBuffer();

	// Write MIDI close event
	writeBytes({0x00, 0xFF, 0x2F, 0x00});
}

void MidiFile::Track::writeEventsToBuffer()
{
	// Create sorted vector of events
	std::vector<Event> eventsSorted = m_events;
	std::sort(eventsSorted.begin(), eventsSorted.end());

	int timeLast = 0;
	for (Event &event : eventsSorted)
	{
		// If something went wrong on sorting, maybe?
		if (event.m_time < timeLast)
		{
			fprintf(stderr, "error: event.m_time=%d timeLast=%d\n",
					event.m_time, timeLast);
			assert(false);
		}
		int tmp = event.m_time;
		event.m_time -= timeLast;
		timeLast = tmp;

		// Write event to buffer
		writeSingleEventToBuffer(event);

		// In case of exceding maximum size, go away
		if (m_buffer.size() >= BUFFER_SIZE) { break; }
	}
}

void MidiFile::Track::writeSingleEventToBuffer(Event &event)
{
	// First of all, write event time
	writeVarLength(event.m_time);

	std::vector<uint8_t> fourBytes;
	switch (event.m_type)
	{
	case MidiFile::Event::NOTE_ON:
	{
		// A note starts playing
		uint8_t code = (0x9 << 4) | m_channel;
		writeBytes({code, event.m_note.pitch, event.m_note.volume});
		break;
	}
	case MidiFile::Event::NOTE_OFF:
	{
		// A note finishes playing
		uint8_t code = (0x8 << 4) | m_channel;
		writeBytes({code, event.m_note.pitch, event.m_note.volume});
		break;
	}
	case MidiFile::Event::TEMPO:
	{
		// A tempo measure
		uint8_t code = 0xFF;
		writeBytes({code, 0x51, 0x03});

		// Convert to microseconds before writting
		writeBigEndian4(6e7 / event.m_tempo, &fourBytes);
		writeBytes({fourBytes[1], fourBytes[2], fourBytes[3]});
		break;
	}
	case MidiFile::Event::PROG_CHANGE:
	{
		// Change patch number
		uint8_t code = (0xC << 4) | m_channel;
		writeBytes({code, event.m_programNumber});
		break;
	}
	case MidiFile::Event::TRACK_NAME:
	{
		// Name of current track
		writeBytes({0xFF, 0x03});

		// Write name string size and then copy it's content
		// to the following size bytes of buffer
		std::vector<uint8_t> bytes(event.m_trackName.begin(),
				event.m_trackName.end());
		writeVarLength(event.m_trackName.size());
		writeBytes(bytes);
		break;
	}
	}
}

/*---------------------------------------------------------------------------*/

} // namespace lmms
