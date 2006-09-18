#ifndef SINGLE_SOURCE_COMPILE

/*
 * preset_preview_play_handle.cpp - implementation of class
 *                                  presetPreviewPlayHandle
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

#include "qt3support.h"

#ifdef QT4

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>

#else

#include <qmutex.h>

#endif



#include "preset_preview_play_handle.h"
#include "note_play_handle.h"
#include "instrument_track.h"
#include "track_container.h"
#include "mmp.h"
#include "debug.h"
#include "midi_port.h"
#include "project_journal.h"



// invisible track-container which is needed as parent for preview-channels
class previewTrackContainer : public trackContainer
{
public:
	previewTrackContainer( engine * _engine ) :
		trackContainer( _engine ),
		m_previewInstrumentTrack( NULL ),
		m_previewNote( NULL ),
		m_dataMutex()
	{
		setJournalling( FALSE );
		m_previewInstrumentTrack =  dynamic_cast<instrumentTrack *>(
					track::create( track::CHANNEL_TRACK,
								this ) );
		m_previewInstrumentTrack->setJournalling( FALSE );
		hide();
	}

	virtual ~previewTrackContainer()
	{
	}


	// implement pure-virtual functions...
	virtual inline bool fixedTCOs( void ) const
	{
		return( TRUE );
	}

	virtual inline QString nodeName( void ) const
	{
		return( "previewtc" );
	}

	instrumentTrack * previewInstrumentTrack( void )
	{
		return( m_previewInstrumentTrack );
	}

	notePlayHandle * previewNote( void )
	{
		return( m_previewNote );
	}

	void setPreviewNote( notePlayHandle * _note )
	{
		m_previewNote = _note;
	}

	void lockData( void )
	{
		m_dataMutex.lock();
	}

	void unlockData( void )
	{
		m_dataMutex.unlock();
	}


private:
	instrumentTrack * m_previewInstrumentTrack;
	notePlayHandle * m_previewNote;
	QMutex m_dataMutex;

} ;


QMap<const engine *, previewTrackContainer *>
					presetPreviewPlayHandle::s_previewTCs;



presetPreviewPlayHandle::presetPreviewPlayHandle(
						const QString & _preset_file,
						engine * _engine ) :
	playHandle( PresetPreviewHandle ),
	engineObject( _engine ),
	m_previewNote( NULL )
{
	if( s_previewTCs.contains( _engine ) == FALSE )
	{
		s_previewTCs[_engine] = new previewTrackContainer( eng() );
	}

	previewTC()->lockData();

	if( previewTC()->previewNote() != NULL )
	{
		previewTC()->previewNote()->mute();
	}


	const bool j = eng()->getProjectJournal()->isJournalling();
	eng()->getProjectJournal()->setJournalling( FALSE );

	multimediaProject mmp( _preset_file );
	previewTC()->previewInstrumentTrack()->loadTrackSpecificSettings(
				mmp.content().firstChild().toElement() );
	// preset also contains information about window-states etc. that's why
	// here we have to make sure that the instrument-track-window is hidden
	previewTC()->previewInstrumentTrack()->hide();

	// make sure, our preset-preview-track does not appear in any MIDI-
	// devices list, so just disable receiving/sending MIDI-events at all
	previewTC()->previewInstrumentTrack()->m_midiPort->setMode(
							midiPort::DUMMY );

	// create note-play-handle for it
	m_previewNote = new notePlayHandle(
			previewTC()->previewInstrumentTrack(), 0,
			valueRanges<f_cnt_t>::max,
		note( NULL, 0, 0, static_cast<tones>( A ),
			static_cast<octaves>( DEFAULT_OCTAVE - 1 ), 100 ) );


	previewTC()->setPreviewNote( m_previewNote );

	previewTC()->unlockData();
	eng()->getProjectJournal()->setJournalling( j );
}




presetPreviewPlayHandle::~presetPreviewPlayHandle()
{
	previewTC()->lockData();
	// not muted by other preset-preview-handle?
	if( m_previewNote->muted() == FALSE )
	{
		// then set according state
		previewTC()->setPreviewNote( NULL );
	}
	delete m_previewNote;
	previewTC()->unlockData();
}




void presetPreviewPlayHandle::play( bool _try_parallelizing )
{
	m_previewNote->play( _try_parallelizing );
}




bool presetPreviewPlayHandle::done( void ) const
{
	return( m_previewNote->muted() );
}




void presetPreviewPlayHandle::cleanUp( engine * _engine )
{
	if( s_previewTCs.contains( _engine ) == TRUE )
	{
		delete s_previewTCs[_engine];
		s_previewTCs.remove( _engine );
	}
}




constNotePlayHandleVector presetPreviewPlayHandle::nphsOfInstrumentTrack(
						const instrumentTrack * _it )
{
	constNotePlayHandleVector cnphv;
	if( s_previewTCs.contains( _it->eng() ) == TRUE )
	{
		previewTrackContainer * tc = s_previewTCs[_it->eng()];
		tc->lockData();
		if( tc->previewNote() != NULL &&
			tc->previewNote()->getInstrumentTrack() == _it )
		{
			cnphv.push_back( tc->previewNote() );
		}
		tc->unlockData();
	}
	return( cnphv );
}





#endif
