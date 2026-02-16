/*
 * MidiFile.cpp - support for exporting MIDI files
 *
 * Copyright (c) 2009 Mark Conway Wirt <emergentmusics) at (gmail . com>
 * Copyright (c) 2015 Mohamed Abdel Maksoud <mohamed at amaksoud.com>
 * Copyright (c) 2020 EmoonX
 *
 * This file was originally based on the Python module MidiFile.py from MIDIUtil
 * by Mark Conway Wirt, which was later rewritten in C++ by Mohamed Abdel Maksoud.
 *
 * --------------------------------------------------------------------------
 * MIDUTIL, Copyright (c) 2009, Mark Conway Wirt
 *                        <emergentmusics) at (gmail . com>
 *
 * This software is distributed under an Open Source license, the
 * details of which follow.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 * --------------------------------------------------------------------------
 *
 */

#include "MidiFile.h"

#include <algorithm>
#include <cassert>
#include <stack>
#include <string>
#include <vector>

#include <QDataStream>
#include <QFile>
#include <QString>

namespace lmms
{

MidiFile::MidiFile(const QString& filename, int numTracks)
	: m_file{filename}
	, m_header{numTracks}
{
	// Open designated blank MIDI file (and data stream) for writing
	m_file.open(QIODevice::WriteOnly);
	m_stream.setDevice(&m_file);

	// Resize track list
	m_tracks.resize(numTracks);
}

void MidiFile::writeAllToStream()
{
	m_stream.writeRawData(
		reinterpret_cast<char*>(m_header.m_buffer.data()),
		m_header.m_buffer.size()
	);

	for (Track& track : m_tracks)
	{
		m_stream.writeRawData(
			reinterpret_cast<char*>(track.m_buffer.data()),
			track.m_buffer.size()
		);
	}
}

MidiFile::Section::Section()
{
	m_buffer.reserve(BUFFER_SIZE);
}

void MidiFile::Section::writeBytes(std::span<const std::uint8_t> bytes,
	std::vector<std::uint8_t>* v)
{
	// Append `bytes` content to the end of *v
	if (!v) { v = &m_buffer; }
	v->insert(v->end(), bytes.begin(), bytes.end());
}

void MidiFile::Section::writeBytes(std::initializer_list<std::uint8_t> bytes,
	std::vector<std::uint8_t>* v)
{
	writeBytes(std::span{bytes.begin(), bytes.size()});
}

void MidiFile::Section::writeVarLength(std::uint32_t val)
{
	// Build little endian stack from 7-bit packs
	std::uint8_t result = val & 0x7F;
	auto littleEndian = std::stack<std::uint8_t>({result});
	val >>= 7;
	while (val > 0)
	{
		result = val & 0x7F;
		result |= 0x80;
		littleEndian.push(result);
		val >>= 7;
	}
	// Add packs in reverse order to actual buffer
	while (!littleEndian.empty())
	{
		m_buffer.push_back(littleEndian.top());
		littleEndian.pop();
	}
}

void MidiFile::Section::writeBigEndian4(std::uint32_t val,
	std::vector<std::uint8_t>* v)
{
	std::vector<std::uint8_t> bytes;
	bytes.push_back(val >> 24);
	bytes.push_back((val >> 16) & 0xff),
	bytes.push_back((val >> 8) & 0xff);
	bytes.push_back(val & 0xff);
	writeBytes(bytes, v);
}

void MidiFile::Section::writeBigEndian2(std::uint16_t val,
	std::vector<std::uint8_t>* v)
{
	std::vector<std::uint8_t> bytes;
	bytes.push_back(val >> 8);
	bytes.push_back(val & 0xff);
	writeBytes(bytes, v);
}

MidiFile::Header::Header(int numTracks, int ticksPerBeat)
	: m_numTracks{numTracks}
	, m_ticksPerBeat{ticksPerBeat}
{}

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

void MidiFile::Track::addEvent(Event event, std::uint32_t time)
{
	event.time = time;
	event.channel = channel;
	m_events.push_back(event);
}

void MidiFile::Track::addNote(std::uint8_t pitch, std::uint8_t volume,
	double realTime, double duration)
{
	Event event;
	event.note.volume = volume;

	// Add start of note
	event.type = Event::NoteOn;
	event.note.pitch = pitch;
	std::uint32_t time = realTime * TICKS_PER_BEAT;
	addEvent(event, time);

	// Add end of note
	event.type = Event::NoteOff;
	event.note.pitch = pitch;
	time = (realTime + duration) * TICKS_PER_BEAT;
	addEvent(event, time);
}

void MidiFile::Track::addTempo(std::uint32_t tempo, std::uint32_t time)
{
	Event event;
	event.type = Event::Tempo;
	event.tempo = tempo;
	addEvent(event, time);
}

void MidiFile::Track::addProgramChange(std::uint8_t prog, std::uint32_t time)
{
	Event event;
	event.type = Event::ProgramChange;
	event.programNumber = prog;
	addEvent(event, time);
}

void MidiFile::Track::addName(const std::string& name, std::uint32_t time)
{
	Event event;
	event.type = Event::TrackName;
	event.trackName = name;
	addEvent(event, time);
}

void MidiFile::Track::writeToBuffer()
{
	// Chunk ID
	writeBytes({'M', 'T', 'r', 'k'});

	// Chunk size placeholder
	writeBigEndian4(0);
	std::size_t idx = m_buffer.size();

	// Write all events to buffer
	writeMIDIToBuffer();

	// Write correct size in placeholder place
	std::size_t size = m_buffer.size() - idx;
	std::vector<std::uint8_t> v;
	writeBigEndian4(size, &v);
	for (std::size_t i = 0; i < 4; ++i)
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
	for (Event& event : eventsSorted)
	{
		// If something went wrong on sorting, maybe?
		if (event.time < timeLast)
		{
			std::fprintf(stderr, "error: event.m_time=%d timeLast=%d\n", event.time, timeLast);
			assert(false);
		}
		auto tmp = event.time;
		event.time -= timeLast;
		timeLast = tmp;

		// Write event to buffer
		writeSingleEventToBuffer(event);

		// In case of exceding maximum size, go away
		if (m_buffer.size() >= BUFFER_SIZE) { break; }
	}
}

void MidiFile::Track::writeSingleEventToBuffer(Event& event)
{
	// First of all, write event time
	writeVarLength(event.time);

	std::vector<std::uint8_t> fourBytes;
	switch (event.type)
	{
		case MidiFile::Event::NoteOn:
		{
			// A note starts playing
			std::uint8_t code = (0x9 << 4) | channel;
			writeBytes({code, event.note.pitch, event.note.volume});
			break;
		}
		case MidiFile::Event::NoteOff:
		{
			// A note finishes playing
			std::uint8_t code = (0x8 << 4) | channel;
			writeBytes({code, event.note.pitch, event.note.volume});
			break;
		}
		case MidiFile::Event::Tempo:
		{
			// A tempo measure
			std::uint8_t code = 0xFF;
			writeBytes({code, 0x51, 0x03});

			// Convert to microseconds before writing
			writeBigEndian4(6e7 / event.tempo, &fourBytes);
			writeBytes({fourBytes[1], fourBytes[2], fourBytes[3]});
			break;
		}
		case MidiFile::Event::ProgramChange:
		{
			// Change patch number
			std::uint8_t code = (0xC << 4) | channel;
			writeBytes({code, event.programNumber});
			break;
		}
		case MidiFile::Event::TrackName:
		{
			// Name of current track
			writeBytes({0xFF, 0x03});

			// Write name string size and then copy it's content
			// to the following size bytes of buffer
			auto bytes = std::vector<std::uint8_t>(event.trackName.begin(), event.trackName.end());
			writeVarLength(event.trackName.size());
			writeBytes(bytes);
			break;
		}
	}
}

} // namespace lmms
