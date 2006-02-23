#ifndef SINGLE_SOURCE_COMPILE

/*
 * note_play_handle.cpp - implementation of class notePlayHandle, part of
 *                        play-engine
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "note_play_handle.h"
#include "channel_track.h"
#include "envelope_tab_widget.h"
#include "midi.h"
#include "midi_port.h"
#include "song_editor.h"
#include "piano_widget.h"
#include "config_mgr.h"



notePlayHandle::notePlayHandle( channelTrack * _chnl_trk,
						const f_cnt_t _frames_ahead,
						const f_cnt_t _frames,
						note * _n,
						const bool _arp_note ) :
	playHandle( NOTE_PLAY_HANDLE, _chnl_trk->eng() ),
	note( *_n ),
	m_pluginData( NULL ),
	m_filter( NULL ),
	m_channelTrack( _chnl_trk ),
	m_frames( 0 ),
	m_framesAhead( _frames_ahead ),
	m_totalFramesPlayed( 0 ), 
	m_framesBeforeRelease( 0 ),
	m_releaseFramesToDo( 0 ),  
	m_releaseFramesDone( 0 ),
	m_released( FALSE ),
	m_baseNote( TRUE  ),
	m_arpNote( _arp_note ),
	m_muted( FALSE )
{
	setFrames( _frames );
	if( !configManager::inst()->value( "ui",
						"manualchannelpiano" ).toInt() )
	{
		m_channelTrack->m_pianoWidget->setKeyState( key(), TRUE );
	}
	// send MIDI-note-on-event
	m_channelTrack->processOutEvent( midiEvent( NOTE_ON,
				m_channelTrack->m_midiPort->outputChannel(),
					key(),
				tLimit<Uint16>(
				(Uint16) ( ( getVolume() / 100.0f ) *
				( m_channelTrack->getVolume() / 100.0f ) *
							127 ), 0, 127 ) ),
			midiTime::fromFrames( m_framesAhead,
				eng()->getSongEditor()->framesPerTact() ) );
}




notePlayHandle::~notePlayHandle()
{
	if( m_released == FALSE )
	{
		noteOff( 0 );
	}

	if( m_channelTrack != NULL )
	{
		m_channelTrack->deleteNotePluginData( this );
	}

	for( notePlayHandleVector::iterator it = m_subNotes.begin();
						it != m_subNotes.end(); ++it )
	{
		delete *it;
	}
	m_subNotes.clear();

	delete m_filter;
}




void notePlayHandle::play( void )
{
	if( m_muted == TRUE || m_channelTrack == NULL )
	{
		return;
	}

	if( m_released == FALSE &&
		m_totalFramesPlayed +
			eng()->getMixer()->framesPerAudioBuffer() >= m_frames )
	{
		noteOff( m_frames - m_totalFramesPlayed );
	} 

	// play note!
	m_channelTrack->playNote( this );

	if( m_released == TRUE )
	{
		f_cnt_t todo = eng()->getMixer()->framesPerAudioBuffer();
		// if this note is base-note for arpeggio, always set
		// m_releaseFramesToDo to bigger value than m_releaseFramesDone
		// because we do not allow notePlayHandle::done() to be true
		// until all sub-notes are completely played and no new ones
		// are inserted by arpAndChordsTabWidget::processNote()
		if( arpBaseNote() == TRUE )
		{
			m_releaseFramesToDo = m_releaseFramesDone + 2 *
				eng()->getMixer()->framesPerAudioBuffer();
		}
		// look whether we have frames left to be done before release
		if( m_framesBeforeRelease )
		{
			// yes, then look whether these samples can be played
			// within one audio-buffer
			if( m_framesBeforeRelease <=
				eng()->getMixer()->framesPerAudioBuffer() )
			{
				// yes, then we did less releaseFramesDone
				todo -= m_framesBeforeRelease;
				m_framesBeforeRelease = 0;
			}
			else
			{
				// no, then just decrese framesBeforeRelease
				// and wait for next loop... (we're not in
				// release-phase yet)
				todo = 0;
				m_framesBeforeRelease -=
			eng()->getMixer()->framesPerAudioBuffer();
			}
		}
		// look whether we're in release-phase
		if( todo && m_releaseFramesDone < m_releaseFramesToDo )
		{
			// check whether we have to do more frames in current
			// loop than left in current loop
			if( m_releaseFramesToDo - m_releaseFramesDone >= todo )
			{
				// yes, then increase number of release-frames
				// done
				m_releaseFramesDone += todo;
			}
			else
			{
				// no, we did all in this loop!
				m_releaseFramesDone = m_releaseFramesToDo;
			}
		}
	}

	// play sub-notes (e.g. chords)
	for( notePlayHandleVector::iterator it = m_subNotes.begin();
						it != m_subNotes.end(); )
	{
		( *it )->play();
		if( ( *it )->done() )
		{
			delete *it;
			m_subNotes.erase( it );
		}
		else
		{
			++it;
		}
	}

	// if this note is a base-note and there're no more sub-notes left we
	// can set m_releaseFramesDone to m_releaseFramesToDo so that
	// notePlayHandle::done() returns true and also this base-note is
	// removed from mixer's active note vector
	if( arpBaseNote() == TRUE && m_subNotes.size() == 0 )
	{
		m_releaseFramesDone = m_releaseFramesToDo;
	}

	// update internal data
	m_totalFramesPlayed += eng()->getMixer()->framesPerAudioBuffer();
}




void notePlayHandle::checkValidity( void )
{
	if( m_channelTrack != NULL &&
				m_channelTrack->type() == track::NULL_TRACK )
	{
		// track-type being track::NULL_TRACK indicates a track whose
		// removal is in progress, so we have to invalidate ourself
		if( m_released == FALSE )
		{
			noteOff( 0 );
		}
		m_channelTrack->deleteNotePluginData( this );
		m_channelTrack = NULL;
	}
	// sub-notes might not be registered at mixer (for example arpeggio-
	// notes), so they wouldn't invalidate them-selves
	for( notePlayHandleVector::iterator it = m_subNotes.begin();
						it != m_subNotes.end(); ++it )
	{
		( *it )->checkValidity();
	}
}




void notePlayHandle::noteOff( const f_cnt_t _s )
{
	// first note-off all sub-notes
	for( notePlayHandleVector::iterator it = m_subNotes.begin();
						it != m_subNotes.end(); ++it )
	{
		( *it )->noteOff( _s );
	}

	// then set some variables indicating release-state
	m_framesBeforeRelease = _s;
	if( m_channelTrack != NULL )
	{
		m_releaseFramesToDo =
				m_channelTrack->m_envWidget->releaseFrames();
		if( !configManager::inst()->value( "ui",
						"manualchannelpiano" ).toInt() )
		{
			m_channelTrack->m_pianoWidget->setKeyState( key(),
									FALSE );
		}
		// send MIDI-note-off-event
		m_channelTrack->processOutEvent( midiEvent( NOTE_OFF,
				m_channelTrack->m_midiPort->outputChannel(),
								key(), 0 ),
						midiTime::fromFrames(
							m_framesBeforeRelease,
				eng()->getSongEditor()->framesPerTact() ) );
	}
	else
	{
		m_releaseFramesToDo = 0;
	}

	m_released = TRUE;
}




f_cnt_t notePlayHandle::actualReleaseFramesToDo( void ) const
{
	return( ( m_channelTrack != NULL ) ?
			m_channelTrack->m_envWidget->releaseFrames() : 0 );
}




void notePlayHandle::setFrames( const f_cnt_t _frames )
{
	m_frames = _frames;
	if( m_frames == 0 && m_channelTrack != NULL )
	{
		m_frames = m_channelTrack->beatLen( this );
	}
}




float notePlayHandle::volumeLevel( const f_cnt_t _frame )
{
	return( ( m_channelTrack != NULL ) ?
		m_channelTrack->m_envWidget->volumeLevel( this, _frame ) : 0 );
}




void notePlayHandle::mute( void )
{
	// mute all sub-notes
	for( notePlayHandleVector::iterator it = m_subNotes.begin();
						it != m_subNotes.end(); ++it )
	{
		( *it )->mute();
	}
	m_muted = TRUE;
}




int notePlayHandle::index( void ) const
{
	const playHandleVector & phv = eng()->getMixer()->playHandles();
	int idx = 0;
	for( constPlayHandleVector::const_iterator it = phv.begin();
							it != phv.end(); ++it )
	{
		const notePlayHandle * nph =
				dynamic_cast<const notePlayHandle *>( *it );
		if( nph == NULL || nph->m_channelTrack != m_channelTrack ||
						nph->released() == TRUE )
		{
			continue;
		}
		if( nph == this )
		{
			break;
		}
		++idx;
	}
	return( idx );
}




constNotePlayHandleVector notePlayHandle::nphsOfChannelTrack(
						const channelTrack * _ct )
{
	const playHandleVector & phv = _ct->eng()->getMixer()->playHandles();
	constNotePlayHandleVector cnphv;

	for( constPlayHandleVector::const_iterator it = phv.begin();
							it != phv.end(); ++it )
	{
		const notePlayHandle * nph =
				dynamic_cast<const notePlayHandle *>( *it );
		if( nph != NULL && nph->m_channelTrack == _ct &&
						nph->released() == FALSE )
		{
			cnphv.push_back( nph );
		}
	}
	return( cnphv );
}



bool notePlayHandle::operator==( const notePlayHandle & _nph ) const
{
	return( length() == _nph.length() &&
			pos() == _nph.pos() &&
			key() == _nph.key() &&
			getVolume() == _nph.getVolume() &&
			getPanning() == _nph.getPanning() &&
			m_channelTrack == _nph.m_channelTrack &&
			m_frames == _nph.m_frames &&
			m_framesAhead == _nph.m_framesAhead &&
			m_totalFramesPlayed == _nph.m_totalFramesPlayed &&
			m_released == _nph.m_released &&
			m_baseNote == _nph.m_baseNote &&
			m_arpNote == _nph.m_arpNote &&
			m_muted == _nph.m_muted );
}



#endif
