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



// invisible track-container which is needed as parents for preview-channels
class blindTrackContainer : public trackContainer
{
public:
	static inline blindTrackContainer * inst( void )
	{
		if( s_instanceOfMe == NULL )
		{
			s_instanceOfMe = new blindTrackContainer();
		}
		return( s_instanceOfMe );
	}

	// implement pure-virtual functions...
	virtual inline bool fixedTCOs( void ) const
	{
		return( TRUE );
	}

	virtual inline QString nodeName( void ) const
	{
		return( "blindtc" );
	}


private:
	blindTrackContainer( void ) :
		trackContainer()
	{
		hide();
	}

	virtual ~blindTrackContainer()
	{
	}


	static blindTrackContainer * s_instanceOfMe;

	friend void presetPreviewPlayHandle::cleanUp( void );

} ;



blindTrackContainer * blindTrackContainer::s_instanceOfMe = NULL;

channelTrack * presetPreviewPlayHandle::s_globalChannelTrack = NULL;
notePlayHandle * presetPreviewPlayHandle::s_globalPreviewNote = NULL;
QMutex presetPreviewPlayHandle::s_globalDataMutex;


presetPreviewPlayHandle::presetPreviewPlayHandle(
						const QString & _preset_file ) :
	playHandle( PRESET_PREVIEW_PLAY_HANDLE ),
	m_previewNote( NULL )
{
/*	if( s_globalDataMutex == NULL )
	{
		s_globalDataMutex = new QMutex;
	}

	s_globalDataMutex->lock();*/
	s_globalDataMutex.lock();

	if( s_globalPreviewNote != NULL )
	{
		s_globalPreviewNote->mute();
	}


	multimediaProject mmp( _preset_file );
	if( s_globalChannelTrack == NULL )
	{
		track * t = track::create( track::CHANNEL_TRACK,
						blindTrackContainer::inst() );
		s_globalChannelTrack = dynamic_cast<channelTrack *>( t );
#ifdef LMMS_DEBUG
		assert( s_globalChannelTrack != NULL );
#endif
	}
	s_globalChannelTrack->loadTrackSpecificSettings( mmp.content().
								firstChild().
								toElement() );
	// make sure, our preset-preview-track does not appear in any MIDI-
	// devices list, so just disable receiving/sending MIDI-events at all
	s_globalChannelTrack->m_midiPort->setMode( midiPort::DUMMY );

	// create temporary note
	note n( 0, 0, static_cast<tones>( A ),
				static_cast<octaves>( DEFAULT_OCTAVE-1 ), 100 );
	// create note-play-handle for it
	m_previewNote = new notePlayHandle( s_globalChannelTrack, 0, ~0, &n );


	s_globalPreviewNote = m_previewNote;

	s_globalDataMutex.unlock();
}




presetPreviewPlayHandle::~presetPreviewPlayHandle()
{
	s_globalDataMutex.lock();
	if( m_previewNote->muted() == FALSE )
	{
		s_globalPreviewNote = NULL;
	}
	delete m_previewNote;
	s_globalDataMutex.unlock();
}




void presetPreviewPlayHandle::play( void )
{
	m_previewNote->play();
}




bool presetPreviewPlayHandle::done( void ) const
{
	return( m_previewNote->muted() );
}




void presetPreviewPlayHandle::cleanUp( void )
{
	delete blindTrackContainer::inst();
}




constNotePlayHandleVector presetPreviewPlayHandle::nphsOfChannelTrack(
						const channelTrack * _ct )
{
	constNotePlayHandleVector cnphv;
	s_globalDataMutex.lock();
	if( s_globalPreviewNote != NULL &&
			s_globalPreviewNote->getChannelTrack() == _ct )
	{
		cnphv.push_back( s_globalPreviewNote );
	}
	s_globalDataMutex.unlock();
	return( cnphv );
}




