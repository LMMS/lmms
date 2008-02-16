#ifndef SINGLE_SOURCE_COMPILE

/*
 * preset_preview_play_handle.cpp - implementation of class
 *                                  presetPreviewPlayHandle
 *
 * Copyright (c) 2005-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>

#include "preset_preview_play_handle.h"
#include "automatable_model_templates.h"
#include "debug.h"
#include "engine.h"
#include "instrument_track.h"
#include "midi_port.h"
#include "mmp.h"
#include "note_play_handle.h"
#include "project_journal.h"
#include "track_container.h"



// invisible track-container which is needed as parent for preview-channels
class previewTrackContainer : public trackContainer
{
public:
	previewTrackContainer( void ) :
		m_previewInstrumentTrack( NULL ),
		m_previewNote( NULL ),
		m_dataMutex()
	{
		setJournalling( FALSE );
		m_previewInstrumentTrack = dynamic_cast<instrumentTrack *>(
					track::create( track::InstrumentTrack,
								this ) );
		m_previewInstrumentTrack->setJournalling( FALSE );
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


previewTrackContainer * presetPreviewPlayHandle::s_previewTC;



presetPreviewPlayHandle::presetPreviewPlayHandle(
						const QString & _preset_file ) :
	playHandle( PresetPreviewHandle ),
	m_previewNote( NULL )
{
	s_previewTC->lockData();

	if( s_previewTC->previewNote() != NULL )
	{
		s_previewTC->previewNote()->mute();
	}


	const bool j = engine::getProjectJournal()->isJournalling();
	engine::getProjectJournal()->setJournalling( FALSE );

	multimediaProject mmp( _preset_file );
	s_previewTC->previewInstrumentTrack()->loadTrackSpecificSettings(
				mmp.content().firstChild().toElement() );
	// preset also contains information about window-states etc. that's why
	// here we have to make sure that the instrument-track-window is hidden
//	s_previewTC->previewInstrumentTrack()->hide();

	// make sure, our preset-preview-track does not appear in any MIDI-
	// devices list, so just disable receiving/sending MIDI-events at all
	s_previewTC->previewInstrumentTrack()->m_midiPort->setMode(
							midiPort::DUMMY );

	// create note-play-handle for it
	m_previewNote = new notePlayHandle(
			s_previewTC->previewInstrumentTrack(), 0,
			valueRanges<f_cnt_t>::max() / 2,
		note( 0, 0, static_cast<tones>( A ),
			static_cast<octaves>( DEFAULT_OCTAVE - 1 ), 100 ) );


	s_previewTC->setPreviewNote( m_previewNote );

	s_previewTC->unlockData();
	engine::getProjectJournal()->setJournalling( j );
}




presetPreviewPlayHandle::~presetPreviewPlayHandle()
{
	s_previewTC->lockData();
	// not muted by other preset-preview-handle?
	if( m_previewNote->muted() == FALSE )
	{
		// then set according state
		s_previewTC->setPreviewNote( NULL );
	}
	delete m_previewNote;
	s_previewTC->unlockData();
}




void presetPreviewPlayHandle::play( bool _try_parallelizing )
{
	m_previewNote->play( _try_parallelizing );
}




bool presetPreviewPlayHandle::done( void ) const
{
	return( m_previewNote->muted() );
}




bool presetPreviewPlayHandle::isFromTrack( const track * _track ) const
{
	return( s_previewTC->previewInstrumentTrack() == _track );
}




void presetPreviewPlayHandle::init( void )
{
	if( !s_previewTC )
	{
		s_previewTC = new previewTrackContainer;
	}
}




void presetPreviewPlayHandle::cleanUp( void )
{
	delete s_previewTC;
	s_previewTC = NULL;
}




constNotePlayHandleVector presetPreviewPlayHandle::nphsOfInstrumentTrack(
						const instrumentTrack * _it )
{
	constNotePlayHandleVector cnphv;
	s_previewTC->lockData();
	if( s_previewTC->previewNote() != NULL &&
		s_previewTC->previewNote()->getInstrumentTrack() == _it )
	{
		cnphv.push_back( s_previewTC->previewNote() );
	}
	s_previewTC->unlockData();
	return( cnphv );
}





#endif
