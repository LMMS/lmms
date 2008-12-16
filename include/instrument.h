/*
 * instrument.h - declaration of class instrument, which provides a
 *                standard interface for all instrument plugins
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "plugin.h"
#include "mixer.h"


// forward-declarations
class instrumentTrack;
class instrumentView;
class midiEvent;
class midiTime;
class notePlayHandle;
class track;


class EXPORT instrument : public plugin
{
public:
	instrument( instrumentTrack * _instrument_track,
					const descriptor * _descriptor );
	virtual ~instrument();

	// --------------------------------------------------------------------
	// functions that can/should be re-implemented:
	// --------------------------------------------------------------------

	// if the plugin doesn't play each note, it can create an instrument-
	// play-handle and re-implement this method, so that it mixes its
	// output buffer only once per mixer-period
	virtual void play( sampleFrame * _working_buffer );

	// to be implemented by actual plugin
	virtual void playNote( notePlayHandle * /* _note_to_play */,
					sampleFrame * /* _working_buf */ )
	{
	}
		
	// needed for deleting plugin-specific-data of a note - plugin has to
	// cast void-ptr so that the plugin-data is deleted properly
	// (call of dtor if it's a class etc.)
	virtual void deleteNotePluginData( notePlayHandle * _note_to_play );

	// Get number of sample-frames that should be used when playing beat
	// (note with unspecified length)
	// Per default this function returns 0. In this case, channel is using
	// the length of the longest envelope (if one active).
	virtual f_cnt_t beatLen( notePlayHandle * _n ) const;


	// some instruments need a certain number of release-frames even
	// if no envelope is active - such instruments can re-implement this
	// method for returning how many frames they at least like to have for
	// release
	virtual f_cnt_t desiredReleaseFrames( void ) const
	{
		return 0;
	}

	// return false if instrument is not bendable
	inline virtual bool isBendable( void ) const
	{
		return true;
	}

	// return true if instruments reacts to MIDI events passed to
	// handleMidiEvent() rather than playNote() & Co
	inline virtual bool isMidiBased( void ) const
	{
		return false;
	}

	// sub-classes can re-implement this for receiving all incoming
	// MIDI-events
	inline virtual bool handleMidiEvent( const midiEvent &,
						const midiTime & )
	{
		return false;
	}

	virtual QString fullDisplayName( void ) const;

	// --------------------------------------------------------------------
	// provided functions:
	// --------------------------------------------------------------------

	// instantiate instrument-plugin with given name or return NULL
	// on failure
	static instrument * instantiate( const QString & _plugin_name,
					instrumentTrack * _instrument_track );

	virtual bool isFromTrack( const track * _track ) const;


protected:
	inline instrumentTrack * getInstrumentTrack( void ) const
	{
		return m_instrumentTrack;
	}

	// instruments may use this to apply a soft fade out at the end of
	// notes - method does this only if really less or equal
	// desiredReleaseFrames() frames are left
	void applyRelease( sampleFrame * buf, const notePlayHandle * _n );


private:
	instrumentTrack * m_instrumentTrack;

} ;


#endif
