/*
 * Instrument.h - declaration of class Instrument, which provides a
 *                standard interface for all instrument plugins
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#ifndef _INSTRUMENT_H
#define _INSTRUMENT_H

#include <QtGui/QWidget>

#include "Plugin.h"
#include "Mixer.h"


// forward-declarations
class InstrumentTrack;
class InstrumentView;
class midiEvent;
class midiTime;
class notePlayHandle;
class track;


/*! \brief Provides a standard interface for all instrument plugins.
 *
 * All instrument plugins have to derive from this class and implement the
 * according virtual methods (see below). An instrument is instantiated by an
 * InstrumentTrack.
 *
 * Instrument plugins can operate in two modes: process audio per note or
 * process audio per Mixer period (one continuous audio stream).
 * For the latter one, the instrument has to create an InstrumentPlayHandle
 * for itself and re-implement #play( sampleFrame * ). When processing audio
 * per note, overload the #playNote( notePlayHandle *, sampleFrame * ) and
 * #deleteNotePluginData( notePlayHandle * ).
 */
class EXPORT Instrument : public Plugin
{
public:
	/*! \brief Constructs an Instrument object.
	 *
	 * The constructor for Instrument objects.
	 * \param instrumentTrack The InstrumentTrack this Instrument belongs to.
	 * \param descriptor A Plugin::Descriptor holding information about the
	 * instrument plugin.
	 */
	Instrument( InstrumentTrack * instrumentTrack,
					const Descriptor * descriptor );
	virtual ~Instrument();

	/*! \brief Generates audio data for the next mixer period.
	 *
	 * If the instrument only generates one continuous audio stream (i.e. is
	 * not capable of generating individual audio streams for each active
	 * note), it has to overload this method, generate the audio data (should
	 * use working buffer to improve cache hit rate and eliminate memory
	 * de-/allocations) and finally call InstrumentTrack::processAudioBuffer()
	 * and pass NULL for the last parameter.
	 *
	 * \param workingBuf A buffer the instrument should operate on (data in it
	 * is not used after this function returns).
	 */
	virtual void play( sampleFrame * workingBuffer );

	/*! \brief Generates audio data for the given NotePlayHandle.
	 *
	 * When generating audio data per-note (recommended), the instrument has
	 * to do this in an overloaded version of this method. It can allocate
	 * note-specific data objects (sound generators, generator settings etc.)
	 * in the given NotePlayHandle if NotePlayHandle::totalFramesPlayed()==0.
	 * Store this data in NotePlayHandle::m_pluginData. See the
	 * deleteNotePluginData() method below for information how to free the
	 * allocated data.
	 * Like play(), call Instrument::processAudioBuffer() after sound data
	 * has been generated and pass the NotePlayHandle as last parameter.
	 *
	 * \param noteToPlay A NotePlayHandle handle for the note to play
	 * \param workingBuf A buffer the instrument should operate on (data in it
	 * is not used after this function returns).
	 */
	virtual void playNote( notePlayHandle * noteToPlay,
					sampleFrame * workingBuf )
	{
		Q_UNUSED(noteToPlay)
		Q_UNUSED(workingBuf)
	}

	/*! \brief Deletes data allocated for playing a certain note.
	 *
	 * In Instrument::playNote() an instrument usually allocates data for each
	 * note to store current state and or generator objects. After a note has
	 * finished playing, this method is called to free the data that was
	 * allocated for playing this note. Plugin has to cast
	 * NotePlayHandle::m_pluginData to according type and delete it.
	 */
	virtual void deleteNotePluginData( notePlayHandle * noteToPlay );

	/*! \brief Returns number of frames for notes with unspecified length.
	 *
	 * When playing a note with unspecified length (e.g. a step in the
	 * BB-Editor), the sequencer core somehow has to determine for how long
	 * to play this note. Plugins can overload this method in order to specify
	 * the length of such notes (in frames). A sampler for example would return
	 * the length of the loaded sample at the according pitch.
	 * Per default this method returns 0 which means the InstrumentTrack will
	 * look for the longest active envelope and use that value. Otherwise the
	 * note will not be played.
	 *
	 * \param notePlayHandle A NotePlayHandle describing the concerned note.
	 */
	virtual f_cnt_t beatLen( notePlayHandle * n ) const;


	/*! \brief Returns number of desired release frames for this instrument.
	 *
	 * Some instruments need a certain number of release frames even
	 * if no envelope is active - such instruments can re-implement this
	 * method for returning how many frames they at least like to have for
	 * release.
	 */
	virtual f_cnt_t desiredReleaseFrames() const
	{
		return 0;
	}

	/*! \brief Returns whether instrument is bendable.
	 *
	 * This is particularly important for instruments that do not supporting
	 * pitch bending. If the overloaded function returns false, the pitch bend
	 * knob will be hidden in InstrumentTrackWindow.
	 */
	inline virtual bool isBendable() const
	{
		return true;
	}

	/*! \brief Returns true if instrument if instrument is MIDI based.
	 *
	 * Instruments should return true here if they react to MIDI events passed
	 * to handleMidiEvent() rather than playNote() & Co.
	 */
	inline virtual bool isMidiBased() const
	{
		return false;
	}

	/*! \brief Allows to handle given MidiEvent.
	 *
	 * Subclasses can re-implement this for receiving all incoming MIDI events.
	 *
	 * \param midiEvent The MIDI event just received
	 * \param midiTime An optional offset for the MIDI event within current
	 * mixer period (e.g. NoteOn at frame X).
	 */
	inline virtual bool handleMidiEvent( const midiEvent &, const midiTime & )
	{
		return false;
	}

	virtual QString fullDisplayName() const;

	/*! \brief Instantiates instrument plugin with given name.
	 *
	 * Tries to instantiate instrument plugin with given name from available
	 * plugin files.
	 *
	 * \param pluginName The internal identifier for the plugin
	 * \param instrumentTrack The InstrumentTrack the new instrument should
	 * belong to.
	 *
	 * \return Pointer to instantiated instrument plugin or NULL on failure.
	 */
	static Instrument * instantiate( const QString & pluginName,
									InstrumentTrack * instrumentTrack );

	/*! \brief Returns whether this instance belongs to given track. */
	virtual bool isFromTrack( const track * _track ) const;

	/*! \brief Returns whether the InstrumentTrack this instrument is attached
	 * to is muted. */
	bool isMuted() const;


protected:
	inline InstrumentTrack * instrumentTrack() const
	{
		return m_instrumentTrack;
	}

	/*! \brief Internal helper method to apply a release on given buffer.
	 *
	 * Instrument plugins may use this to apply a soft fade-out at the end of
	 * a note. Please note that this is only done if the number of frames
	 * returned by NotePlayHandle::framesLeft() is equal or below the number
	 * of frames returned by Instrument::desiredReleaseFrames().
	 */
	void applyRelease( sampleFrame * buf, const notePlayHandle * n );


private:
	InstrumentTrack * m_instrumentTrack;

} ;

#endif
