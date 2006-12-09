/*
 * instrument.h - declaration of class instrument, which provides a
 *                standard interface for all instrument plugins
 *
 * Copyright (c) 2005-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "qt3support.h"

#ifdef QT4

#include <QtGui/QWidget>
#include <QtCore/QVector>

#else

#include <qwidget.h>
#include <qvaluevector.h>

#endif


#include "plugin.h"
#include "mixer.h"


// forward-declarations
class instrumentTrack;
class notePlayHandle;
class midiEvent;


class instrument : public QWidget, public plugin
{
public:
	instrument( instrumentTrack * _channel_track,
					const descriptor * _descriptor );
	virtual ~instrument();

	// if the plugin doesn't play each note, it can create an instrument-
	// play-handle and re-implement this method, so that it mixes it's
	// output buffer only once per mixer-period
	virtual void play( bool _try_parallelizing = FALSE );

	// to be overloaded by actual plugin
	virtual void FASTCALL playNote( notePlayHandle * note_to_play,
						bool _try_parallelizing );

	// needed for deleting plugin-specific-data of a note - plugin has to
	// cast void-ptr so that the plugin-data is deleted properly
	// (call of dtor if it's a class etc.)
	virtual void FASTCALL deleteNotePluginData( notePlayHandle *
							_note_to_play );

	// Get number of sample-frames that should be used when playing beat
	// (note with unspecified length)
	// Per default this function returns 0. In this case, channel is using
	// the length of the longest envelope (if one active).
	virtual f_cnt_t FASTCALL beatLen( notePlayHandle * _n ) const;


	// some instruments need a certain number of release-frames even
	// if no envelope is active - such instruments can re-implement this
	// method for returning how many frames they at least like to have for
	// release
	virtual f_cnt_t desiredReleaseFrames( void ) const
	{
		return( 0 );
	}

	// monophonic instruments can re-implement this indicate that they do
	// not allow more then one note being played at the same time
	virtual bool isMonophonic( void ) const
	{
		return( FALSE );
	}

	// instrument-play-handles use this for checking whether they can mark
	// themselves as done, so that mixer trashes them
	inline virtual bool valid( void ) const
	{
		return( m_valid );
	}

	inline virtual bool notePlayHandleBased( void ) const
	{
		return( TRUE );
	}

	// sub-classes can re-implement this for receiving all incoming
	// MIDI-events except NoteOn and NoteOff
	inline virtual bool handleMidiEvent( const midiEvent & _me,
						const midiTime & _time )
	{
		return( FALSE );
	}

	// instantiate instrument-plugin with given name or return NULL
	// on failure
	static instrument * FASTCALL instantiate( const QString & _plugin_name,
					instrumentTrack * _channel_track );

protected:
	inline instrumentTrack * getInstrumentTrack( void ) const
	{
		return( m_instrumentTrack );
	}

	// instruments can use this for invalidating themselves, which is for
	// example neccessary when being destroyed and having instrument-play-
	// handles running
	inline void invalidate( void )
	{
		m_valid = FALSE;
		eng()->getMixer()->checkValidityOfPlayHandles();
	}


private:
	instrumentTrack * m_instrumentTrack;
	bool m_valid;

} ;


#endif
