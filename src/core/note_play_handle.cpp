/*
 * note_play_handle.cpp - implementation of class notePlayHandle, part of
 *                        rendering engine
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include "basic_filters.h"
#include "config_mgr.h"
#include "detuning_helper.h"
#include "InstrumentSoundShaping.h"
#include "InstrumentTrack.h"
#include "MidiPort.h"
#include "song.h"


inline notePlayHandle::baseDetuning::baseDetuning(
						detuningHelper * _detuning ) :
	m_detuning( _detuning ),
	m_value( m_detuning->getAutomationPattern()->valueAt( 0 ) )
{
}






notePlayHandle::notePlayHandle( InstrumentTrack * _it,
						const f_cnt_t _offset,
						const f_cnt_t _frames,
						const note & _n,
						notePlayHandle * _parent,
						const bool _part_of_arp ) :
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
	m_released( false ),
	m_baseNote( _parent == NULL  ),
	m_partOfArpeggio( _part_of_arp ),
	m_muted( false ),
	m_bbTrack( NULL ),
#ifdef LMMS_SINGERBOT_SUPPORT
	m_patternIndex( 0 ),
#endif
	m_origTempo( engine::getSong()->getTempo() )
{
	if( m_baseNote )
	{
		m_baseDetuning = new baseDetuning( detuning() );
		m_instrumentTrack->m_processHandles.push_back( this );
	}
	else
	{
		m_baseDetuning = _parent->m_baseDetuning;

		_parent->m_subNotes.push_back( this );
		// if there was an arp-note added and parent is a base-note
		// we set arp-note-flag for indicating that parent is an
		// arpeggio-base-note
		_parent->m_partOfArpeggio = isPartOfArpeggio() &&
							_parent->isBaseNote();

		m_bbTrack = _parent->m_bbTrack;
#ifdef LMMS_SINGERBOT_SUPPORT
		m_patternIndex = _parent->m_patternIndex;
#endif
	}

	updateFrequency();

	setFrames( _frames );


	if( !isBaseNote() || !instrumentTrack()->isArpeggiatorEnabled() )
	{
		// send MIDI-note-on-event
		m_instrumentTrack->processOutEvent( midiEvent( MidiNoteOn,
			m_instrumentTrack->midiPort()->realOutputChannel(),
			key(), getMidiVelocity() ),
				midiTime::fromFrames( offset(),
						engine::framesPerTick() ) );
	}
}




notePlayHandle::~notePlayHandle()
{
	noteOff( 0 );

	if( m_baseNote )
	{
		delete m_baseDetuning;
		m_instrumentTrack->m_processHandles.removeAll( this );
	}

	if( m_pluginData != NULL )
	{
		m_instrumentTrack->deleteNotePluginData( this );
	}

	if( m_instrumentTrack->m_notes[key()] == this )
	{
		m_instrumentTrack->m_notes[key()] = NULL;
	}

	for( NotePlayHandleList::Iterator it = m_subNotes.begin();
						it != m_subNotes.end(); ++it )
	{
		delete *it;
	}
	m_subNotes.clear();

	delete m_filter;
}




void notePlayHandle::setVolume( const volume_t _volume )
{
	note::setVolume( _volume );
	m_instrumentTrack->processOutEvent( midiEvent( MidiKeyPressure,
			m_instrumentTrack->midiPort()->realOutputChannel(),
						key(), getMidiVelocity() ), 0 );
	
}




int notePlayHandle::getMidiVelocity() const
{
	int vel = getVolume();
	if( m_instrumentTrack->getVolume() < DefaultVolume )
	{
		vel = ( vel * m_instrumentTrack->getVolume() ) / DefaultVolume;
	}
	return qMin( MidiMaxVelocity, vel * MidiMaxVelocity / DefaultVolume );
}




void notePlayHandle::play( sampleFrame * _working_buffer )
{
	if( m_muted )
	{
		return;
	}

	if( m_released == false &&
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
		m_instrumentTrack->playNote( this, _working_buffer );
	}

	if( m_released )
	{
		f_cnt_t todo = engine::getMixer()->framesPerPeriod();
		// if this note is base-note for arpeggio, always set
		// m_releaseFramesToDo to bigger value than m_releaseFramesDone
		// because we do not allow notePlayHandle::done() to be true
		// until all sub-notes are completely played and no new ones
		// are inserted by arpAndChordsTabWidget::processNote()
		if( isArpeggioBaseNote() )
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
	for( NotePlayHandleList::Iterator it = m_subNotes.begin();
						it != m_subNotes.end(); )
	{
		( *it )->play( _working_buffer );
		if( ( *it )->done() )
		{
			delete *it;
			it = m_subNotes.erase( it );
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
	if( isArpeggioBaseNote() && m_subNotes.size() == 0 )
	{
		m_releaseFramesDone = m_releaseFramesToDo;
	}

	// update internal data
	m_totalFramesPlayed += engine::getMixer()->framesPerPeriod();
}




f_cnt_t notePlayHandle::framesLeft() const
{
	if( m_released && actualReleaseFramesToDo() == 0 )
	{
		return m_framesBeforeRelease;
	}
	else if( m_released && actualReleaseFramesToDo() >= m_releaseFramesDone )
	{
		return m_framesBeforeRelease + actualReleaseFramesToDo() -
							m_releaseFramesDone;
	}
	return m_frames+actualReleaseFramesToDo()-m_totalFramesPlayed;
}




bool notePlayHandle::isFromTrack( const track * _track ) const
{
	return m_instrumentTrack == _track || m_bbTrack == _track;
}




void notePlayHandle::noteOff( const f_cnt_t _s )
{
	if( m_released )
	{
		return;
	}

	// first note-off all sub-notes
	for( NotePlayHandleList::Iterator it = m_subNotes.begin();
						it != m_subNotes.end(); ++it )
	{
		( *it )->noteOff( _s );
	}

	// then set some variables indicating release-state
	m_framesBeforeRelease = _s;
	m_releaseFramesToDo = qMax<f_cnt_t>( 0, // 10,
			m_instrumentTrack->m_soundShaping.releaseFrames() );

	if( !isBaseNote() || !instrumentTrack()->isArpeggiatorEnabled() )
	{
		// send MIDI-note-off-event
		m_instrumentTrack->processOutEvent( midiEvent( MidiNoteOff,
			m_instrumentTrack->midiPort()->realOutputChannel(),
								key(), 0 ),
			midiTime::fromFrames( m_framesBeforeRelease,
						engine::framesPerTick() ) );
	}

	m_released = true;
}




f_cnt_t notePlayHandle::actualReleaseFramesToDo() const
{
	return m_instrumentTrack->m_soundShaping.releaseFrames(/*
							isArpeggioBaseNote()*/ );
}




void notePlayHandle::setFrames( const f_cnt_t _frames )
{
	m_frames = _frames;
	if( m_frames == 0 )
	{
		m_frames = m_instrumentTrack->beatLen( this );
	}
	m_origFrames = m_frames;
}




float notePlayHandle::volumeLevel( const f_cnt_t _frame )
{
	return m_instrumentTrack->m_soundShaping.volumeLevel( this, _frame );
}




bool notePlayHandle::isArpeggioBaseNote() const
{
	return isBaseNote() && ( m_partOfArpeggio ||
			m_instrumentTrack->isArpeggiatorEnabled() );
}




void notePlayHandle::mute()
{
	// mute all sub-notes
	for( NotePlayHandleList::Iterator it = m_subNotes.begin();
						it != m_subNotes.end(); ++it )
	{
		( *it )->mute();
	}
	m_muted = true;
}




int notePlayHandle::index() const
{
	const PlayHandleList & playHandles =
					engine::getMixer()->playHandles();
	int idx = 0;
	for( PlayHandleList::ConstIterator it = playHandles.begin();
						it != playHandles.end(); ++it )
	{
		const notePlayHandle * nph =
				dynamic_cast<const notePlayHandle *>( *it );
		if( nph == NULL ||
			nph->m_instrumentTrack != m_instrumentTrack ||
						nph->released() == true )
		{
			continue;
		}
		if( nph == this )
		{
			break;
		}
		++idx;
	}
	return idx;
}




ConstNotePlayHandleList notePlayHandle::nphsOfInstrumentTrack(
				const InstrumentTrack * _it, bool _all_ph )
{
	const PlayHandleList & playHandles = engine::getMixer()->playHandles();
	ConstNotePlayHandleList cnphv;

	for( PlayHandleList::ConstIterator it = playHandles.begin();
						it != playHandles.end(); ++it )
	{
		const notePlayHandle * nph =
				dynamic_cast<const notePlayHandle *>( *it );
		if( nph != NULL && nph->m_instrumentTrack == _it &&
			( nph->released() == false || _all_ph == true ) )
		{
			cnphv.push_back( nph );
		}
	}
	return cnphv;
}




bool notePlayHandle::operator==( const notePlayHandle & _nph ) const
{
	return length() == _nph.length() &&
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
			m_partOfArpeggio == _nph.m_partOfArpeggio &&
			m_muted == _nph.m_muted;
}




void notePlayHandle::updateFrequency()
{
	const float pitch =
		( key() - m_instrumentTrack->baseNoteModel()->value() +
				engine::getSong()->masterPitch() ) / 12.0f;
	m_frequency = BaseFreq * powf( 2.0f, pitch +
		m_instrumentTrack->pitchModel()->value() / ( 100 * 12.0f ) );
	m_unpitchedFrequency = BaseFreq * powf( 2.0f, pitch );

	for( NotePlayHandleList::Iterator it = m_subNotes.begin();
						it != m_subNotes.end(); ++it )
	{
		( *it )->updateFrequency();
	}
}




void notePlayHandle::processMidiTime( const midiTime & _time )
{
	if( _time >= pos() )
	{
		const float v = detuning()->getAutomationPattern()->
						valueAt( _time - pos() );
		if( !typeInfo<float>::isEqual( v, m_baseDetuning->value() ) )
		{
			m_baseDetuning->setValue( v );
			updateFrequency();
		}
	}
}




void notePlayHandle::resize( const bpm_t _new_tempo )
{
	double completed = m_totalFramesPlayed / (double) m_frames;
	double new_frames = m_origFrames * m_origTempo / (double) _new_tempo;
	m_frames = (f_cnt_t)new_frames;
	m_totalFramesPlayed = (f_cnt_t)( completed * new_frames );

	for( NotePlayHandleList::Iterator it = m_subNotes.begin();
						it != m_subNotes.end(); ++it )
	{
		( *it )->resize( _new_tempo );
	}
}


