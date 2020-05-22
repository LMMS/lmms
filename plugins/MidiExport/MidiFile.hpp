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
#include <array>
#include <stack>
#include <vector>
#include <initializer_list>
#include <algorithm>
#include <cassert>

#include <QString>
#include <QFile>
#include <QDataStream>

using std::string;
using std::array;
using std::stack;
using std::vector;
using std::initializer_list;
using std::sort;

/*---------------------------------------------------------------------------*/

//! MIDI file class used in exporting
class MidiFile
{
public:
	//! Default number of ticks per single beat
	static constexpr u_int16_t TICKS_PER_BEAT = 128;

	//! Maximum size of buffers used for each section
	static constexpr size_t BUFFER_SIZE = 50 * 1024;

private:
	//! Base class for sections of MIDI file
	class Section
	{
	public:
		//! Constant-capacity vector to serve as buffer before writting
		//  to stream (should be better than std::array as it provides
		//  size() and push_back() funcionalities)
		vector<uint8_t> m_buffer;

		//! Reserve constant space for BUFFER_SIZE capacity vector
		Section();

	protected:
		//! Write bytes from initializer list to vector (or buffer by default)
		void writeBytes(vector<uint8_t> bytes, vector<uint8_t> *v=nullptr);

		/*! \brief Write a MIDI-compatible variable length stream
		 *  \param val A four-byte value
		 *
		 *  The MIDI format is a little strange, and makes use of so-called
		 *  variable length quantities. These quantities are a stream of bytes.
		 *  If the most significant bit is 1, then more bytes follow. If it is
		 *  zero, then the byte in question is the last in the stream.
		 */
		void writeVarLength(uint32_t val);

		//! Buffer gets four 8-bit values from left to right
		void writeBigEndian4(uint32_t val, vector<uint8_t> *v=nullptr);

		//! Buffer gets two 8-bit values from left to right
		void writeBigEndian2(uint16_t val, vector<uint8_t> *v=nullptr);

		//! Write section info to buffer
		virtual void writeToBuffer();
	};

	/*-----------------------------------------------------------------------*/

	//! Represents MIDI header info
	class Header : public Section
	{
	private:
		//! Number of tracks in MIDI file
		uint16_t m_numTracks;

		//! How many ticks each beat has
		uint16_t m_ticksPerBeat;

	public:
		//! Constructor
		Header(uint16_t numTracks, uint16_t ticksPerBeat=TICKS_PER_BEAT);

		//! Write header info to buffer
		void writeToBuffer();
	};

	/*-----------------------------------------------------------------------*/

public:
	//! Represents a MIDI track
	class Track : public Section
	{
	private:
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

				//! Program (patch) number of instrument
				uint8_t programNumber;
			};

			//! Name of track where event is
			//  (too much trouble putting it inside union...)
			string trackName;

			//! Write MIDI event info to track buffer
			inline void writeToBuffer(Track &parent) const;

			//! \brief Comparison operator
			//  Events are sorted by their time
			inline bool operator<(const Event& b) const;
		};

		/*-------------------------------------------------------------------*/

		//! Variable-length vector of events
		vector<Event> events;

		//! Append a single event to vector
		inline void addEvent(Event event, uint32_t time);

	public:
		//! Track channel number
		uint8_t channel;

		//! Constructor
		Track(uint8_t channel);

		//! Add both NOTE_ON and NOTE_OFF effects
		//  \param realTime Time of note start
		//  \param duration How long the note lasts
		inline void addNote(uint8_t pitch, uint8_t volume,
				double realTime, double duration);

		//! Add a tempo mark
		inline void addTempo(uint8_t tempo, uint32_t time);

		//! Add a program (patch) change event
		inline void addProgramChange(uint8_t prog, uint32_t time);

		//! Add a track name event
		inline void addName(const string &name, uint32_t time);

		//! Write MIDI track to buffer
		inline void writeToBuffer();

		//! Write the meta data and note data to the packed MIDI stream
		inline void writeMIDIToBuffer();

		//! Write the events in MIDIEvents to the MIDI stream
		inline void writeEventsToBuffer();
	};

	/*-----------------------------------------------------------------------*/

private:
	//! Qt file to be opened
	QFile m_file;

	//! Qt data stream for writing
	QDataStream *m_stream;

public:
	//! The sole file header
	Header m_header;

	//! List of tracks
	vector<Track> m_tracks;

	MidiFile(const QString &filename, uint16_t nTracks);

	~MidiFile();

	void writeAllToStream();
};

/*---------------------------------------------------------------------------*/

MidiFile::MidiFile(const QString &filename, uint16_t nTracks):
		m_file(filename), m_header(nTracks)
{
	// Open designated blank MIDI file (and data stream) for writing
	m_file.open(QIODevice::WriteOnly);
	m_stream = new QDataStream(&m_file);

	// Reserve space for track list
	m_tracks.reserve(nTracks);
	for (size_t i = 0; i < nTracks; i++)
	{
		m_tracks.push_back(Track(i));
	}
}

MidiFile::~MidiFile()
{
	delete m_stream;
}

void MidiFile::writeAllToStream()
{
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
	m_buffer.reserve(BUFFER_SIZE);
}

void MidiFile::Section::writeBytes(vector<uint8_t> bytes, vector<uint8_t> *v)
{
	if (not v) v = &m_buffer;
	v->insert(v->end(), bytes.begin(), bytes.end());
}

void MidiFile::Section::writeVarLength(uint32_t val)
{
	// Build little endian stack from 7-bit packs
	uint8_t result = val & 0x7F;
	stack<uint8_t> little_endian({result});
	val >>= 7;
	while (val > 0)
	{
		result = val & 0x7F;
		result |= 0x80;
		little_endian.push(result);
		val >>= 7;
	}
	// Add packs in reverse order to actual buffer
	while (not little_endian.empty())
	{
		m_buffer.push_back(little_endian.top());
		little_endian.pop();
	}
}

void MidiFile::Section::writeBigEndian4(uint32_t val, vector<uint8_t> *v)
{
	vector<uint8_t> bytes;
 	bytes.push_back(val >> 24);
	bytes.push_back((val >> 16) & 0xff),
	bytes.push_back((val >> 8) & 0xff);
	bytes.push_back(val & 0xff);
	writeBytes(bytes, v);
}

void MidiFile::Section::writeBigEndian2(uint16_t val, vector<uint8_t> *v)
{
	vector<uint8_t> bytes;
	bytes.push_back(val >> 8);
	bytes.push_back(val & 0xff);
	writeBytes(bytes, v);
}

/*---------------------------------------------------------------------------*/

MidiFile::Header::Header(uint16_t numTracks, uint16_t ticksPerBeat):
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

	// Extra info
	writeBigEndian2(m_numTracks);
	writeBigEndian2(m_ticksPerBeat);
}

/*---------------------------------------------------------------------------*/

inline void MidiFile::Track::Event::writeToBuffer(Track &parent) const
{
	// First of all, write event time
	parent.writeVarLength(time);

	uint8_t code;
	vector<uint8_t> fourBytes;
	switch (type)
	{
	case NOTE_ON:
		// A note starts playing
		code = (0x9 << 4) | channel;
		parent.writeBytes({code, note.pitch, note.volume});
		break;

	case NOTE_OFF:
		// A note finishes playing
		code = (0x8 << 4) | channel;
		parent.writeBytes({code, note.pitch, note.volume});
		break;

	case TEMPO:
		// A tempo measure
		code = 0xFF;
		parent.writeBytes({code, 0x51, 0x03});

		// Convert to microseconds before writting
		parent.writeBigEndian4(6e7 / tempo, &fourBytes);
		parent.writeBytes({fourBytes[1], fourBytes[2], fourBytes[3]});
		break;

	case PROG_CHANGE:
		// Change patch number
		code = (0xC << 4) | channel;
		parent.writeBytes({code, programNumber});
		break;

	case TRACK_NAME:
		// Name of current track
		parent.writeBytes({0xFF, 0x03});

		// Write name string size and then copy it's content
		// to the following size bytes of buffer
		vector<uint8_t> bytes(trackName.begin(), trackName.end());
		parent.writeVarLength(trackName.size());
		parent.writeBytes(bytes);
		break;
	}
}

inline bool MidiFile::Track::Event::operator<(const Event& b) const
{
	if (time < b.time) { return true; }
	return (time == b.time and type > b.type);
}

/*---------------------------------------------------------------------------*/

inline void MidiFile::Track::addEvent(Event event, uint32_t time)
{
	event.time = time;
	event.channel = channel;
	events.push_back(event);
}

MidiFile::Track::Track(uint8_t channel):
		channel(channel) {}

inline void MidiFile::Track::addNote(uint8_t pitch, uint8_t volume,
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

inline void MidiFile::Track::addTempo(uint8_t tempo, uint32_t time)
{
	Event event;
	event.type = Event::TEMPO;
	event.tempo = tempo;
	addEvent(event, time);
}

inline void MidiFile::Track::addProgramChange(uint8_t prog, uint32_t time)
{
	Event event;
	event.type = Event::PROG_CHANGE;
	event.programNumber = prog;
	addEvent(event, time);
}

inline void MidiFile::Track::addName(const string &name, uint32_t time)
{
	Event event;
	event.type = Event::TRACK_NAME;
	event.trackName = name;
	addEvent(event, time);
}

inline void MidiFile::Track::writeToBuffer()
{
	// Chunk ID
	writeBytes({'M', 'T', 'r', 'k'});

	// Chunk size placeholder
	size_t idx = m_buffer.size();
	writeBigEndian4(9);

	// Write all events to buffer
	writeMIDIToBuffer();

	// Write correct size
	size_t size = m_buffer.size() - idx;
	vector<uint8_t> v;
	writeBigEndian4(size, &v);
	for (size_t i = idx; i < idx + 4; idx++)
	{
		m_buffer[i] = v[i];
	}
}

inline void MidiFile::Track::writeMIDIToBuffer()
{
	// Process events in the eventList
	writeEventsToBuffer();

	// Write MIDI close event
	writeBytes({0x00, 0xFF, 0x2F, 0x00});
}

inline void MidiFile::Track::writeEventsToBuffer()
{
	// Create sorted vector of events
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
		e.writeToBuffer(*this);

		// In case of exceding maximum size, go away
		if (m_buffer.size() >= BUFFER_SIZE) { break; }
	}
}
/*---------------------------------------------------------------------------*/

#endif
