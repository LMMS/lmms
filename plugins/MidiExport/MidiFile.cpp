/*
 * MidiFile.cpp - support for exporting MIDI files
 *
 * Copyright (c) 2009 Mark Conway Wirt <emergentmusics) at (gmail . com>
 * Copyright (c) 2015 Mohamed Abdel Maksoud <mohamed at amaksoud.com>
 * Copyright (c) 2020 EmoonX
 * Copyright (c) 2026 Dalton Messmer <messmer.dalton/at/gmail.com>
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
#include <array>
#include <cassert>
#include <iostream>
#include <stack>

#include "endian_handling.h"

namespace lmms
{

MidiFile::MidiFile(const std::filesystem::path& file, int numTracks)
	: m_header{numTracks}
{
	// Open designated blank MIDI file as a data stream for writing
	m_stream.open(file, std::ios_base::binary);

	// Resize track list
	m_tracks.resize(numTracks);
}

void MidiFile::writeAllToStream()
{
	m_stream.write(reinterpret_cast<const char*>(m_header.m_buffer.data()), m_header.m_buffer.size());

	for (const Track& track : m_tracks)
	{
		m_stream.write(reinterpret_cast<const char*>(track.m_buffer.data()), track.m_buffer.size());
	}
}

MidiFile::Section::Section()
{
	m_buffer.reserve(BufferSize);
}

void MidiFile::Section::writeBytes(std::span<const std::uint8_t> bytes)
{
	// Append content to the end of m_buffer
	m_buffer.insert(m_buffer.end(), bytes.begin(), bytes.end());
}

void MidiFile::Section::writeBytes(std::initializer_list<std::uint8_t> bytes)
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

void MidiFile::Section::writeBigEndian4(std::uint32_t val)
{
	const auto valBigEndian = std::bit_cast<std::array<std::uint8_t, 4>>(swap32IfLE(val));
	writeBytes(valBigEndian);
}

void MidiFile::Section::writeBigEndian2(std::uint16_t val)
{
	const auto valBigEndian = std::bit_cast<std::array<std::uint8_t, 2>>(swap16IfLE(val));
	writeBytes(valBigEndian);
}

MidiFile::Header::Header(int numTracks, tick_t ticksPerBeat)
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
	writeBigEndian2(static_cast<std::uint16_t>(m_ticksPerBeat));
}

void MidiFile::Track::addEvent(Event&& event, tick_t time)
{
	event.time = time;
	event.channel = m_channel;
	m_events.push_back(std::move(event));
}

void MidiFile::Track::addNote(std::uint8_t pitch, std::uint8_t volume,
	double realTime, double duration)
{
	// Add start of note
	tick_t time = realTime * TicksPerBeat;
	addEvent({
		.type = Event::NoteOn,
		.note = {.pitch = pitch, .volume = volume}
	}, time);

	// Add end of note
	time = (realTime + duration) * TicksPerBeat;
	addEvent({
		.type = Event::NoteOff,
		.note = {.pitch = pitch, .volume = volume}
	}, time);
}

void MidiFile::Track::addTempo(std::uint32_t tempo, tick_t time)
{
	addEvent({.type = Event::Tempo, .tempo = tempo}, time);
}

void MidiFile::Track::addPatch(MidiPatch patch, tick_t time)
{
	addEvent({.type = Event::BankSelectMSB, .patch = patch}, time);
	addEvent({.type = Event::BankSelectLSB, .patch = patch}, time);
	addEvent({.type = Event::ProgramChange, .patch = patch}, time);
}

void MidiFile::Track::addName(std::string name, tick_t time)
{
	addEvent({.type = Event::TrackName, .trackName = std::move(name)}, time);
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
	const auto size = static_cast<std::uint32_t>(m_buffer.size() - idx);
	const auto sizeBigEndian = std::bit_cast<std::array<std::uint8_t, 4>>(swap32IfLE(size));
	for (std::size_t i = 0; i < 4; ++i)
	{
		m_buffer[idx - 4 + i] = sizeBigEndian[i];
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
	std::sort(m_events.begin(), m_events.end());

	tick_t timeLast = 0;
	for (Event& event : m_events)
	{
		// If something went wrong on sorting, maybe?
		if (event.time < timeLast)
		{
			std::cerr << "MidiExport: error: event.time=" << event.time << ", timeLast=" << timeLast << '\n';
			assert(false);
		}
		auto tmp = event.time;
		event.time -= timeLast;
		timeLast = tmp;

		// Write event to buffer
		writeSingleEventToBuffer(event);

		// In case of exceding maximum size, go away
		if (m_buffer.size() >= BufferSize) { break; }
	}
}

void MidiFile::Track::writeSingleEventToBuffer(Event& event)
{
	// First of all, write event time
	writeVarLength(event.time);

	switch (event.type)
	{
		case MidiFile::Event::NoteOn:
		{
			// A note starts playing
			std::uint8_t code = 0x90 | m_channel;
			writeBytes({code, event.note.pitch, event.note.volume});
			break;
		}
		case MidiFile::Event::NoteOff:
		{
			// A note finishes playing
			std::uint8_t code = 0x80 | m_channel;
			writeBytes({code, event.note.pitch, event.note.volume});
			break;
		}
		case MidiFile::Event::Tempo:
		{
			// A tempo measure
			std::uint8_t code = 0xFF;
			writeBytes({code, 0x51, 0x03});

			// Convert to microseconds before writing
			const auto usec = static_cast<std::uint32_t>(6e7 / event.tempo);
			const auto usecBigEndian = std::bit_cast<std::array<std::uint8_t, 4>>(swap32IfLE(usec));
			writeBytes({usecBigEndian[1], usecBigEndian[2], usecBigEndian[3]});
			break;
		}
		case MidiFile::Event::BankSelectMSB:
		{
			// Bank select (MSB)
			writeBytes({static_cast<std::uint8_t>(0xB0 | m_channel), 0x00, event.patch.bankMSB()});
			break;
		}
		case MidiFile::Event::BankSelectLSB:
		{
			// Bank select (LSB)
			writeBytes({static_cast<std::uint8_t>(0xB0 | m_channel), 0x20, event.patch.bankLSB()});
			break;
		}
		case MidiFile::Event::ProgramChange:
		{
			// Program change
			writeBytes({static_cast<std::uint8_t>(0xC0 | m_channel), event.patch.program});
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
