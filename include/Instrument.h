/*
 * Instrument.h - declaration of class Instrument, which provides a
 *                standard interface for all instrument plugins
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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

#ifndef LMMS_INSTRUMENT_H
#define LMMS_INSTRUMENT_H

#include <QString>
#include "lmms_export.h"
#include "lmms_basics.h"
#include "MemoryManager.h"
#include "Plugin.h"
#include "TimePos.h"

namespace lmms
{

// forward-declarations
class InstrumentTrack;
class MidiEvent;
class NotePlayHandle;
class Track;


class LMMS_EXPORT Instrument : public Plugin
{
	MM_OPERATORS
public:
	enum Flag
	{
		NoFlags = 0x00,
		IsSingleStreamed = 0x01,	/*! Instrument provides a single audio stream for all notes */
		IsMidiBased = 0x02,			/*! Instrument is controlled by MIDI events rather than NotePlayHandles */
		IsNotBendable = 0x04,		/*! Instrument can't react to pitch bend changes */
	};

	Q_DECLARE_FLAGS(Flags, Flag);

	Instrument(InstrumentTrack * _instrument_track,
			const Descriptor * _descriptor,
			const Descriptor::SubPluginFeatures::Key * key = nullptr);
	~Instrument() override = default;

	// --------------------------------------------------------------------
	// functions that can/should be re-implemented:
	// --------------------------------------------------------------------

	virtual bool hasNoteInput() const { return true; }

	// if the plugin doesn't play each note, it can create an instrument-
	// play-handle and re-implement this method, so that it mixes its
	// output buffer only once per audio engine period
	virtual void play( sampleFrame * _working_buffer );

	// to be implemented by actual plugin
	virtual void playNote( NotePlayHandle * /* _note_to_play */,
					sampleFrame * /* _working_buf */ )
	{
	}

	// needed for deleting plugin-specific-data of a note - plugin has to
	// cast void-ptr so that the plugin-data is deleted properly
	// (call of dtor if it's a class etc.)
	virtual void deleteNotePluginData( NotePlayHandle * _note_to_play );

	// Get number of sample-frames that should be used when playing beat
	// (note with unspecified length)
	// Per default this function returns 0. In this case, channel is using
	// the length of the longest envelope (if one active).
	virtual f_cnt_t beatLen( NotePlayHandle * _n ) const;


	// some instruments need a certain number of release-frames even
	// if no envelope is active - such instruments can re-implement this
	// method for returning how many frames they at least like to have for
	// release
	virtual f_cnt_t desiredReleaseFrames() const
	{
		return 0;
	}

	virtual Flags flags() const
	{
		return NoFlags;
	}

	// sub-classes can re-implement this for receiving all incoming
	// MIDI-events
	inline virtual bool handleMidiEvent( const MidiEvent&, const TimePos& = TimePos(), f_cnt_t offset = 0 )
	{
		return true;
	}

	QString fullDisplayName() const override;

	// An indication if the plugin supports natively preset change.
	// If the function returns false, the appropriate control might
	// be hidden.
	inline virtual bool presetChangeSupported()
	{
		return false;
	}

	// An instruction to change the preset.
	// The preset number is always an integer between 0 and 127
	// The bank number can be equal to -1, meaning that bank selection
	// is ignored. The plugin should decide what to do in such situation.
	inline virtual void changePreset(int bank, unsigned int preset) {}

	// --------------------------------------------------------------------
	// provided functions:
	// --------------------------------------------------------------------

	//! instantiate instrument-plugin with given name or return NULL
	//! on failure
	static Instrument * instantiate(const QString & _plugin_name,
		InstrumentTrack * _instrument_track,
		const Plugin::Descriptor::SubPluginFeatures::Key* key,
		bool keyFromDnd = false);

	virtual bool isFromTrack( const Track * _track ) const;

	inline InstrumentTrack * instrumentTrack() const
	{
		return m_instrumentTrack;
	}


protected:
	// fade in to prevent clicks
	void applyFadeIn(sampleFrame * buf, NotePlayHandle * n);

	// instruments may use this to apply a soft fade out at the end of
	// notes - method does this only if really less or equal
	// desiredReleaseFrames() frames are left
	void applyRelease( sampleFrame * buf, const NotePlayHandle * _n );


private:
	InstrumentTrack * m_instrumentTrack;

} ;


Q_DECLARE_OPERATORS_FOR_FLAGS(Instrument::Flags)


} // namespace lmms

#endif // LMMS_INSTRUMENT_H
