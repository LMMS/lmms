/*
 * note_play_handle.cpp - implementation of class notePlayHandle, part of
 *                        play-engine
 *
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <qmap.h>

#include "note_play_handle.h"
#include "channel_track.h"
#include "envelope_tab_widget.h"
#include "midi.h"
#include "midi_port.h"
#include "song_editor.h"


notePlayHandle::notePlayHandle( channelTrack * _chnl_trk, Uint32 _frames_ahead,
				Uint32 _frames, note * n, bool _arp_note ) :
	playHandle(),
	note( *n ),
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
	// send MIDI-note-on-event
	m_channelTrack->processOutEvent( midiEvent( NOTE_ON,
				m_channelTrack->m_midiPort->outputChannel(),
					key(),
				(Uint16) ( ( getVolume() / 100.0f ) *
				( m_channelTrack->getVolume() / 100.0f ) *
								127 ) ),
				midiTime::fromFrames( m_framesAhead,
					songEditor::inst()->framesPerTact() ) );
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
		m_totalFramesPlayed + mixer::inst()->framesPerAudioBuffer() >=
								m_frames )
	{
		noteOff( m_frames - m_totalFramesPlayed );
	} 

	// play note!
	m_channelTrack->playNote( this );

	if( m_released == TRUE )
	{
		Uint32 todo = mixer::inst()->framesPerAudioBuffer();
		// if this note is base-note for arpeggio, always set
		// m_releaseFramesToDo to bigger value than m_releaseFramesDone
		// because we do not allow notePlayHandle::done() to be true
		// until all sub-notes are completely played and no new ones
		// are inserted by arpAndChordsTabWidget::processNote()
		if( arpBaseNote() == TRUE )
		{
			m_releaseFramesToDo = m_releaseFramesDone + 2 *
					mixer::inst()->framesPerAudioBuffer();
		}
		// look whether we have frames left to be done before release
		if( m_framesBeforeRelease )
		{
			// yes, then look whether these samples can be played
			// within one audio-buffer
			if( m_framesBeforeRelease <=
					mixer::inst()->framesPerAudioBuffer() )
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
					mixer::inst()->framesPerAudioBuffer();
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
	m_totalFramesPlayed += mixer::inst()->framesPerAudioBuffer();
}




void notePlayHandle::checkValidity( void )
{
	if( m_channelTrack != NULL &&
			m_channelTrack->trackType() == track::NULL_TRACK )
	{
		m_channelTrack = NULL;
	}
}




void notePlayHandle::noteOff( Uint32 _s )
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
		// send MIDI-note-off-event
		m_channelTrack->processOutEvent( midiEvent( NOTE_OFF,
				m_channelTrack->m_midiPort->outputChannel(),
								key(), 0 ),
						midiTime::fromFrames(
							m_framesBeforeRelease,
					songEditor::inst()->framesPerTact() ) );
	}
	else
	{
		m_releaseFramesToDo = 0;
	}

	m_released = TRUE;
}




Uint32 notePlayHandle::actualReleaseFramesToDo( void ) const
{
	return( ( m_channelTrack != NULL ) ?
			m_channelTrack->m_envWidget->releaseFrames() : 0 );
}




void notePlayHandle::setFrames( Uint32 _frames )
{
	m_frames = _frames;
	if( m_frames == 0 && m_channelTrack != NULL )
	{
		m_frames = m_channelTrack->beatLen( this );
	}
}




float notePlayHandle::volumeLevel( Uint32 _frame )
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

