#ifndef _MIDI_FILE_H
#define _MIDI_FILE_H

/**
 * Name:        MidiFile.h
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

#include <QDataStream>
#include <QFile>
#include <QSharedPointer>
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

	/*-----------------------------------------------------------------------*/

private:
	//! Base class for sections of MIDI file
	class Section
	{
	public:
		//! \brief Constant-capacity buffer vector to be written to stream
		//!
		//! (should be better than `std::array` as it provides
		//! `size()` and `push_back()` funcionalities)
		std::vector<uint8_t> m_buffer;

	protected:
		//! Reserve constant space for \ref BUFFER_SIZE capacity vector
		Section();

		//! \brief Write bytes to vector (or buffer by default)
		//! \param bytes List of bytes to be written
		//! \param v Pointer to vector (if none, use \ref m_buffer)
		void writeBytes(std::vector<uint8_t> bytes,
				std::vector<uint8_t> *v=nullptr);

		/*! \brief Write a MIDI-compatible variable length stream
		 *  \param val A four-byte value
		 *
		 *  The MIDI format is a little strange, and makes use of so-called
		 *  variable length quantities. These quantities are a stream of bytes.
		 *  If the most significant bit is 1, then more bytes follow. If it is
		 *  zero, then the byte in question is the last in the stream. */
		void writeVarLength(uint32_t val);

		//! Buffer gets four 8-bit values from left to right
		void writeBigEndian4(uint32_t val, std::vector<uint8_t> *v=nullptr);

		//! Buffer gets two 8-bit values from left to right
		void writeBigEndian2(uint16_t val, std::vector<uint8_t> *v=nullptr);

		//! Write section info to buffer
		virtual void writeToBuffer() {}
	};

	/*-----------------------------------------------------------------------*/

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
		Header(int numTracks, int ticksPerBeat=TICKS_PER_BEAT);

		//! Write header info to buffer
		void writeToBuffer();
	};

	/*-----------------------------------------------------------------------*/

private:
	//! Represents a track event. See \ref Event::Type for more info
	struct Event
	{
		//! Possible event types
		enum Type
		{
			NOTE_ON, NOTE_OFF, TEMPO, PROG_CHANGE, TRACK_NAME
		}
		m_type;

		//! Time count when event happens
		int m_time; // FIXME: was uint32_t

		//! Channel number where event is
		uint8_t m_channel;

		//! Union for saving space
		union
		{
			//! Note properties
			struct
			{
				//! Note pitch
				uint8_t pitch;

				//! Note volume
				uint8_t volume;
			}
			m_note;

			//! Tempo of event (in BPM)
			uint32_t m_tempo;

			//! Program (patch) number of instrument
			uint8_t m_programNumber;
		};

		//! Name of track where event is
		// (too much trouble putting it inside union...)
		std::string m_trackName;

		//! \brief Comparison operator
		//! Events are sorted by their time (or, in case of tie, type priority)
		inline bool operator<(const Event& b) const
		{
			if (m_time < b.m_time) { return true; }
			return (m_time == b.m_time && m_type > b.m_type);
		}
	};

	/*-----------------------------------------------------------------------*/

public:
	//! Represents a MIDI track
	class Track : public Section
	{
	private:
		//! Variable-length vector of events
		std::vector<Event> m_events;

	public:
		//! Track channel number
		uint8_t m_channel;

	private:
		//! Append a single event to vector
		void addEvent(Event event, uint32_t time);

	public:
		//! \brief Add both NOTE_ON and NOTE_OFF effects
		//! \param pitch Note pitch
		//! \param volume Note volume
		//! \param realTime Time of note start
		//! \param duration How long the note lasts
		void addNote(uint8_t pitch, uint8_t volume,
				double realTime, double duration);

		//! Add a tempo mark
		void addTempo(uint32_t tempo, uint32_t time);

		//! Add a program (patch) change event
		void addProgramChange(uint8_t prog, uint32_t time);

		//! Add a track name event
		void addName(const std::string &name, uint32_t time);

		//! Write MIDI track to buffer
		void writeToBuffer();

	private:
		//! Write the meta data and note data to buffer
		void writeMIDIToBuffer();

		//! Write the sorted events in \ref m_events to buffer
		void writeEventsToBuffer();

		//! Write a single event to buffer
		void writeSingleEventToBuffer(Event &event);
	};

	/*-----------------------------------------------------------------------*/

private:
	//! Qt file to be opened
	QFile m_file;

	//! \brief Smart pointer to write-only Qt data stream
	//! (this should automatically be freed upon lifetime end)
	QSharedPointer<QDataStream> m_stream;

public:
	//! The sole file header
	Header m_header;

	//! List of tracks
	std::vector<Track> m_tracks;

	//! \brief Open data stream for writing to file and create list of tracks
	//! \param filename Name of file to be opened
	//! \param nTracks Number of instrument (BB and non BB) tracks
	MidiFile(const QString &filename, int nTracks);

	//! Write all data (both header and tracks) to stream
	void writeAllToStream();
};

} // namespace lmms

/*---------------------------------------------------------------------------*/

#endif
