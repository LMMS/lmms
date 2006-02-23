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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "qt3support.h"

#ifdef QT4

#include <QMutex>
#include <QMutexLocker>

#else

#include <qmutex.h>

#endif



#include "preset_preview_play_handle.h"
#include "note_play_handle.h"
#include "channel_track.h"
#include "track_container.h"
#include "mmp.h"
#include "debug.h"
#include "midi_port.h"



// invisible track-container which is needed as parent for preview-channels
class previewTrackContainer : public trackContainer
{
public:
	previewTrackContainer( engine * _engine ) :
		trackContainer( _engine ),
		m_previewChannelTrack( dynamic_cast<channelTrack *>(
					track::create( track::CHANNEL_TRACK,
								this ) )),
		m_previewNote( NULL ),
		m_dataMutex()
	{
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

	channelTrack * previewChannelTrack( void )
	{
		return( m_previewChannelTrack );
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
	channelTrack * m_previewChannelTrack;
	notePlayHandle * m_previewNote;
	QMutex m_dataMutex;

} ;


QMap<const engine *, previewTrackContainer *>
					presetPreviewPlayHandle::s_previewTCs;



presetPreviewPlayHandle::presetPreviewPlayHandle(
						const QString & _preset_file,
						engine * _engine ) :
	playHandle( PRESET_PREVIEW_PLAY_HANDLE, _engine ),
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


	multimediaProject mmp( _preset_file );
	previewTC()->previewChannelTrack()->loadTrackSpecificSettings(
				mmp.content().firstChild().toElement() );

	// make sure, our preset-preview-track does not appear in any MIDI-
	// devices list, so just disable receiving/sending MIDI-events at all
	previewTC()->previewChannelTrack()->m_midiPort->setMode(
							midiPort::DUMMY );

	// create temporary note
	note n( 0, 0, static_cast<tones>( A ),
				static_cast<octaves>( DEFAULT_OCTAVE-1 ), 100 );
	// create note-play-handle for it
	m_previewNote = new notePlayHandle( previewTC()->previewChannelTrack(),
								0, ~0, &n );


	previewTC()->setPreviewNote( m_previewNote );

	previewTC()->unlockData();
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




void presetPreviewPlayHandle::play( void )
{
	m_previewNote->play();
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




constNotePlayHandleVector presetPreviewPlayHandle::nphsOfChannelTrack(
						const channelTrack * _ct )
{
	constNotePlayHandleVector cnphv;
	if( s_previewTCs.contains( _ct->eng() ) == TRUE )
	{
		previewTrackContainer * tc = s_previewTCs[_ct->eng()];
		tc->lockData();
		if( tc->previewNote() != NULL &&
			tc->previewNote()->getChannelTrack() == _ct )
		{
			cnphv.push_back( tc->previewNote() );
		}
		tc->unlockData();
	}
	return( cnphv );
}





#endif
