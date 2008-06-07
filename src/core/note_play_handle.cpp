#ifndef SINGLE_SOURCE_COMPILE

/*
 * note_play_handle.cpp - implementation of class notePlayHandle, part of
 *                        play-engine
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "note_play_handle.h"
#include "config_mgr.h"
#include "detuning_helper.h"
#include "instrument_sound_shaping.h"
#include "instrument_track.h"
#include "midi_port.h"
#include "song.h"


inline notePlayHandle::baseDetuning::baseDetuning(
						detuningHelper * _detuning ) :
	m_detuning( _detuning ),
	m_value( m_detuning->getAutomationPattern()->valueAt( 0 ) )
{
}











notePlayHandle::notePlayHandle( instrumentTrack * _it,
						const f_cnt_t _offset,
						const f_cnt_t _frames,
						const note & _n,
						notePlayHandle * _parent,
						const bool _arp_note ) :
	playHandle( NotePlayHandle, _offset ),
	note( _n.length(), _n.pos(), _n.key(),
			_n.getVolume(), _n.getPanning(), _n.detuning() ),
	m_pluginData( NULL ),
	m_filter( NULL ),
	m_instrumentTrack( _it ),
	m_frames( 0 ),
	m_totalFramesPlayed( 0 ), 
	m_framesBeforeRelease( 0 ),
	m_releaseFramesToDo( 0 ),
	m_releaseFramesDone( 0 ),
	m_released( FALSE ),
	m_baseNote( _parent == NULL  ),
	m_arpNote( _arp_note ),
	m_muted( FALSE ),
	m_bbTrack( NULL ),
#if SINGERBOT_SUPPORT
	m_patternIndex( 0 ),
#endif
	m_orig_bpm( engine::getSong()->getTempo() )
{
	if( m_baseNote )
	{
		m_base_detuning = new baseDetuning( detuning() );
		m_instrumentTrack->m_processHandles.push_back( this );
	}
	else
	{
		m_base_detuning = _parent->m_base_detuning;

		_parent->m_subNotes.push_back( this );
		// if there was an arp-note added and parent is a base-note
		// we set arp-note-flag for indicating that parent is an
		// arpeggio-base-note
		_parent->m_arpNote = arpNote() && _parent->baseNote();

		m_bbTrack = _parent->m_bbTrack;
#if SINGERBOT_SUPPORT
		m_patternIndex = _parent->m_patternIndex;
#endif
	}

	updateFrequency();

	setFrames( _frames );
	// send MIDI-note-on-event
	m_instrumentTrack->processOutEvent( midiEvent( NOTE_ON,
				m_instrumentTrack->m_midiPort.outputChannel(),
					key(),
				tLimit<Uint16>(
				(Uint16) ( ( getVolume() / 100.0f ) *
				( m_instrumentTrack->getVolume() / 100.0f ) *
							127 ), 0, 127 ) ),
			midiTime::fromFrames( offset(),
						engine::framesPerTick() ) );
}




notePlayHandle::~notePlayHandle()
{
	if( m_released == FALSE )
	{
		noteOff( 0 );
	}

	if( m_baseNote )
	{
		delete m_base_detuning;
		m_instrumentTrack->m_processHandles.removeAll( this );
	}

	if( m_pluginData != NULL )
	{
		m_instrumentTrack->deleteNotePluginData( this );
	}

	for( notePlayHandleVector::iterator it = m_subNotes.begin();
						it != m_subNotes.end(); ++it )
	{
		delete *it;
	}
	m_subNotes.clear();

	delete m_filter;
}




void notePlayHandle::play( bool _try_parallelizing,
						sampleFrame * _working_buffer )
{
	if( m_muted == TRUE )
	{
		return;
	}

	if( m_released == FALSE &&
		m_totalFramesPlayed + engine::getMixer()->framesPerPeriod()
								>= m_frames )
	{
		noteOff( m_frames - m_totalFramesPlayed );
	}

	// under some circumstances we're called even if there's nothing to play
	// therefore do an additional check which fixes crash e.g. when
	// decreasing release of an instrument-track while the note is active
	if( framesLeft() > 0 )
	{
		// play note!
		m_instrumentTrack->playNote( this, _try_parallelizing,
							_working_buffer );
	}

	if( m_released == TRUE )
	{
		f_cnt_t todo = engine::getMixer()->framesPerPeriod();
		// if this note is base-note for arpeggio, always set
		// m_releaseFramesToDo to bigger value than m_releaseFramesDone
		// because we do not allow notePlayHandle::done() to be true
		// until all sub-notes are completely played and no new ones
		// are inserted by arpAndChordsTabWidget::processNote()
		if( arpBaseNote() == TRUE )
		{
			m_releaseFramesToDo = m_releaseFramesDone + 2 *
				engine::getMixer()->framesPerPeriod();
		}
		// look whether we have frames left to be done before release
		if( m_framesBeforeRelease )
		{
			// yes, then look whether these samples can be played
			// within one audio-buffer
			if( m_framesBeforeRelease <=
				engine::getMixer()->framesPerPeriod() )
			{
				// yes, then we did less releaseFramesDone
				todo -= m_framesBeforeRelease;
				m_framesBeforeRelease = 0;
			}
			else
			{
				// no, then just decrease framesBeforeRelease
				// and wait for next loop... (we're not in
				// release-phase yet)
				todo = 0;
				m_framesBeforeRelease -=
				engine::getMixer()->framesPerPeriod();
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
		( *it )->play( _try_parallelizing, _working_buffer );
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
	m_totalFramesPlayed += engine::getMixer()->framesPerPeriod();
}




f_cnt_t notePlayHandle::framesLeft( void ) const
{
/*	const instrument * i = ( m_instrumentTrack != NULL ) ?
				m_instrumentTrack->getInstrument() : NULL;
	f_cnt_t rftd  = ( i != NULL && i->isMonophonic() ) ?
						0 : actualReleaseFramesToDo();*/
	if( m_released && actualReleaseFramesToDo() == 0 )
	{
		return( m_framesBeforeRelease );
	}
	else if( m_released && actualReleaseFramesToDo() >= m_releaseFramesDone )
	{
		return( m_framesBeforeRelease + actualReleaseFramesToDo() - m_releaseFramesDone );
	}
	return( m_frames+actualReleaseFramesToDo()-m_totalFramesPlayed );
}




bool notePlayHandle::isFromTrack( const track * _track ) const
{
	return( m_instrumentTrack == _track || m_bbTrack == _track );
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
	m_releaseFramesToDo = tMax<f_cnt_t>( 0, // 10,
			m_instrumentTrack->m_soundShaping.releaseFrames() );
	// send MIDI-note-off-event
	m_instrumentTrack->processOutEvent( midiEvent( NOTE_OFF,
				m_instrumentTrack->m_midiPort.outputChannel(),
								key(), 0 ),
			midiTime::fromFrames( m_framesBeforeRelease,
						engine::framesPerTick() ) );

	m_released = TRUE;
}




f_cnt_t notePlayHandle::actualReleaseFramesToDo( void ) const
{
	return( m_instrumentTrack->m_soundShaping.releaseFrames(
							arpBaseNote() ) );
}




void notePlayHandle::setFrames( const f_cnt_t _frames )
{
	m_frames = _frames;
	if( m_frames == 0 )
	{
		m_frames = m_instrumentTrack->beatLen( this );
	}
	m_orig_frames = m_frames;
}




float notePlayHandle::volumeLevel( const f_cnt_t _frame )
{
	return( m_instrumentTrack->m_soundShaping.volumeLevel( this, _frame ) );
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
	const playHandleVector & phv = engine::getMixer()->playHandles();
	int idx = 0;
	for( constPlayHandleVector::const_iterator it = phv.begin();
							it != phv.end(); ++it )
	{
		const notePlayHandle * nph =
				dynamic_cast<const notePlayHandle *>( *it );
		if( nph == NULL ||
			nph->m_instrumentTrack != m_instrumentTrack ||
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




constNotePlayHandleVector notePlayHandle::nphsOfInstrumentTrack(
				const instrumentTrack * _it, bool _all_ph )
{
	const playHandleVector & phv = engine::getMixer()->playHandles();
	constNotePlayHandleVector cnphv;

	for( constPlayHandleVector::const_iterator it = phv.begin();
							it != phv.end(); ++it )
	{
		const notePlayHandle * nph =
				dynamic_cast<const notePlayHandle *>( *it );
		if( nph != NULL && nph->m_instrumentTrack == _it &&
			( nph->released() == FALSE || _all_ph == TRUE ) )
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
			m_instrumentTrack == _nph.m_instrumentTrack &&
			m_frames == _nph.m_frames &&
			offset() == _nph.offset() &&
			m_totalFramesPlayed == _nph.m_totalFramesPlayed &&
			m_released == _nph.m_released &&
			m_baseNote == _nph.m_baseNote &&
			m_arpNote == _nph.m_arpNote &&
			m_muted == _nph.m_muted );
}




void notePlayHandle::updateFrequency( void )
{
	const int base_tone = m_instrumentTrack->baseNoteModel()->value() %
								KeysPerOctave;
	const int base_octave = m_instrumentTrack->baseNoteModel()->value() /
								KeysPerOctave;
	const float pitch = (float)( key() % KeysPerOctave - base_tone +
			engine::getSong()->masterPitch() ) / 12.0f +
			(float)( key() / KeysPerOctave - base_octave ) +
					 m_base_detuning->value() / 12.0f;
	m_frequency = BaseFreq * powf( 2.0f, pitch );

	for( notePlayHandleVector::iterator it = m_subNotes.begin();
						it != m_subNotes.end(); ++it )
	{
		( *it )->updateFrequency();
	}
}




void notePlayHandle::processMidiTime( const midiTime & _time )
{
	if( _time >= pos() )
	{
		float v = detuning()->getAutomationPattern()->valueAt( _time -
									pos() );
		if( v != m_base_detuning->value() )
		{
			m_base_detuning->setValue( v );
			updateFrequency();
		}
	}
}




void notePlayHandle::resize( const bpm_t _new_bpm )
{
	double completed = m_totalFramesPlayed / (double)m_frames;
	double new_frames = m_orig_frames * m_orig_bpm / (double)_new_bpm;
	m_frames = (f_cnt_t)new_frames;
	m_totalFramesPlayed = (f_cnt_t)( completed * new_frames );

	for( notePlayHandleVector::iterator it = m_subNotes.begin();
						it != m_subNotes.end(); ++it )
	{
		( *it )->resize( _new_bpm );
	}
}




#endif
