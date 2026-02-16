/*
 * MidiFile.h - support for exporting MIDI files
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

#ifndef LMMS_MIDI_FILE_H
#define LMMS_MIDI_FILE_H

#include <string>
#include <vector>

#include <QDataStream>
#include <QFile>
#include <QString>

namespace lmms
{

/*---------------------------------------------------------------------------*/

//! MIDI file class for exporting purposes
class MidiFile
{
public:
	//! Default number of ticks per single beat
	static constexpr int TICKS_PER_BEAT = 128;

	//! Maximum size of buffers used for each section
	static constexpr size_t BUFFER_SIZE = 50 * 1024;

private:
	//! Base class for sections of MIDI file
	class Section
	{
	public:
		//! @brief Constant-capacity buffer vector to be written to stream
		//!
		//! (should be better than `std::array` as it provides
		//! `size()` and `push_back()` funcionalities)
		std::vector<std::uint8_t> m_buffer;

	protected:
		//! Reserve constant space for @ref BUFFER_SIZE capacity vector
		Section();

		//! @brief Write bytes to vector (or buffer by default)
		//! @param bytes List of bytes to be written
		//! @param v Pointer to vector (if none, use @ref m_buffer)
		void writeBytes(std::vector<std::uint8_t> bytes,
			std::vector<std::uint8_t>* v = nullptr);

		//! @brief Write a MIDI-compatible variable length stream
		//! @param val A four-byte value
		//!
		//! The MIDI format is a little strange, and makes use of so-called
		//! variable length quantities. These quantities are a stream of bytes.
		//! If the most significant bit is 1, then more bytes follow. If it is
		//! zero, then the byte in question is the last in the stream.
		void writeVarLength(std::uint32_t val);

		//! Buffer gets four 8-bit values from left to right
		void writeBigEndian4(std::uint32_t val, std::vector<std::uint8_t>* v = nullptr);

		//! Buffer gets two 8-bit values from left to right
		void writeBigEndian2(std::uint16_t val, std::vector<std::uint8_t>* v = nullptr);

		//! Write section info to buffer
		virtual void writeToBuffer() {}
	};

public:
	//! Represents MIDI header info
	class Header : public Section
	{
	private:
		//! Number of tracks in MIDI file
		const int m_numTracks;

		//! How many ticks each beat has
		const int m_ticksPerBeat;

	public:
		//! Constructor
		Header(int numTracks, int ticksPerBeat = TICKS_PER_BEAT);

		//! Write header info to buffer
		void writeToBuffer();
	};

private:
	//! Represents a track event. See @ref Event::Type for more info
	struct Event
	{
		//! Possible event types, ordered most important to least important
		enum Type
		{
			NOTE_ON, NOTE_OFF, TEMPO, PROG_CHANGE, TRACK_NAME
		}
		type = Type::NOTE_ON;

		//! Time count when event happens
		int time = 0; // FIXME: was std::uint32_t

		//! Channel number where event is
		std::uint8_t channel = 0;

		//! Union for saving space
		union
		{
			//! Note properties
			struct
			{
				//! Note pitch
				std::uint8_t pitch;

				//! Note volume
				std::uint8_t volume;
			}
			note;

			//! Tempo of event (in BPM)
			std::uint32_t tempo;

			//! Program (patch) number of instrument
			std::uint8_t programNumber = 0;
		};

		//! Name of track where event is
		// (too much trouble putting it inside union...)
		std::string trackName;

		//! @brief Comparison operator
		//! Events are sorted by their time (or, in case of tie, type priority)
		bool operator<(const Event& b) const
		{
			if (time < b.time) { return true; }
			return (time == b.time && type > b.type);
		}
	};

public:
	//! Represents a MIDI track
	class Track : public Section
	{
	private:
		//! Variable-length vector of events
		std::vector<Event> m_events;

	public:
		//! Track channel number
		std::uint8_t channel;

	private:
		//! Append a single event to vector
		void addEvent(Event event, std::uint32_t time);

	public:
		//! @brief Add both NOTE_ON and NOTE_OFF effects
		//! @param pitch Note pitch
		//! @param volume Note volume
		//! @param realTime Time of note start
		//! @param duration How long the note lasts
		void addNote(std::uint8_t pitch, std::uint8_t volume,
			double realTime, double duration);

		//! Add a tempo mark
		void addTempo(std::uint32_t tempo, std::uint32_t time);

		//! Add a program (patch) change event
		void addProgramChange(std::uint8_t prog, std::uint32_t time);

		//! Add a track name event
		void addName(const std::string& name, std::uint32_t time);

		//! Write MIDI track to buffer
		void writeToBuffer();

	private:
		//! Write the meta data and note data to buffer
		void writeMIDIToBuffer();

		//! Write the sorted events in @ref m_events to buffer
		void writeEventsToBuffer();

		//! Write a single event to buffer
		void writeSingleEventToBuffer(Event& event);
	};

private:
	//! Qt file to be opened
	QFile m_file;

	//! Write-only Qt data stream
	QDataStream m_stream;

public:
	//! The sole file header
	Header m_header;

	//! List of tracks
	std::vector<Track> m_tracks;

	//! @brief Open data stream for writing to file and create list of tracks
	//! @param filename Name of file to be opened
	//! @param numTracks Number of instrument (pattern and non-pattern) tracks
	MidiFile(const QString& filename, int numTracks);

	//! Write all data (both header and tracks) to stream
	void writeAllToStream();
};

} // namespace lmms

#endif // LMMS_MIDI_FILE_H
