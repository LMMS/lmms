/*
 * NotePlayHandle.cpp - implementation of class NotePlayHandle which manages
 *                      playback of a single note by an instrument
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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

#include "NotePlayHandle.h"
#include "BasicFilters.h"
#include "DetuningHelper.h"
#include "InstrumentSoundShaping.h"
#include "InstrumentTrack.h"
#include "Instrument.h"
#include "Mixer.h"
#include "Song.h"


NotePlayHandle::BaseDetuning::BaseDetuning( DetuningHelper *detuning ) :
	m_value( detuning ? detuning->automationPattern()->valueAt( 0 ) : 0 )
{
}






NotePlayHandle::NotePlayHandle( InstrumentTrack* instrumentTrack,
								const f_cnt_t _offset,
								const f_cnt_t _frames,
								const Note& n,
								NotePlayHandle *parent,
								int midiEventChannel,
								Origin origin ) :
	PlayHandle( TypeNotePlayHandle, _offset ),
	Note( n.length(), n.pos(), n.key(), n.getVolume(), n.getPanning(), n.detuning() ),
	m_pluginData( NULL ),
	m_instrumentTrack( instrumentTrack ),
	m_frames( 0 ),
	m_totalFramesPlayed( 0 ),
	m_framesBeforeRelease( 0 ),
	m_releaseFramesToDo( 0 ),
	m_releaseFramesDone( 0 ),
	m_subNotes(),
	m_released( false ),
	m_releaseStarted( false ),
	m_hasMidiNote( false ),
	m_hasParent( parent != NULL  ),
	m_parent( parent ),
	m_hadChildren( false ),
	m_muted( false ),
	m_bbTrack( NULL ),
	m_origTempo( Engine::getSong()->getTempo() ),
	m_origBaseNote( instrumentTrack->baseNote() ),
	m_frequency( 0 ),
	m_unpitchedFrequency( 0 ),
	m_baseDetuning( NULL ),
	m_songGlobalParentOffset( 0 ),
	m_midiChannel( midiEventChannel >= 0 ? midiEventChannel : instrumentTrack->midiPort()->realOutputChannel() ),
	m_origin( origin ),
	m_frequencyNeedsUpdate( false )
{
	lock();
	if( hasParent() == false )
	{
		m_baseDetuning = new BaseDetuning( detuning() );
		m_instrumentTrack->m_processHandles.push_back( this );
	}
	else
	{
		m_baseDetuning = parent->m_baseDetuning;

		parent->m_subNotes.push_back( this );
		parent->m_hadChildren = true;

		m_bbTrack = parent->m_bbTrack;

		parent->setUsesBuffer( false );
	}

	updateFrequency();

	setFrames( _frames );

	// inform attached components about new MIDI note (used for recording in Piano Roll)
	if( m_origin == OriginMidiInput )
	{
		m_instrumentTrack->midiNoteOn( *this );
	}

	if( m_instrumentTrack->instrument()->flags() & Instrument::IsSingleStreamed )
	{
		setUsesBuffer( false );
	}

	setAudioPort( instrumentTrack->audioPort() );

	unlock();
}


NotePlayHandle::~NotePlayHandle()
{
	lock();
	noteOff( 0 );

	if( hasParent() == false )
	{
		delete m_baseDetuning;
		m_instrumentTrack->m_processHandles.removeAll( this );
	}
	else
	{
		m_parent->m_subNotes.removeOne( this );
	}

	if( m_pluginData != NULL )
	{
		m_instrumentTrack->deleteNotePluginData( this );
	}

	if( m_instrumentTrack->m_notes[key()] == this )
	{
		m_instrumentTrack->m_notes[key()] = NULL;
	}

	m_subNotes.clear();

	if( buffer() ) releaseBuffer();

	unlock();
}




void NotePlayHandle::setVolume( volume_t _volume )
{
	Note::setVolume( _volume );

	const int baseVelocity = m_instrumentTrack->midiPort()->baseVelocity();

	m_instrumentTrack->processOutEvent( MidiEvent( MidiKeyPressure, midiChannel(), midiKey(), midiVelocity( baseVelocity ) ) );
}




void NotePlayHandle::setPanning( panning_t panning )
{
	Note::setPanning( panning );
}




int NotePlayHandle::midiKey() const
{
	return key() - m_origBaseNote + instrumentTrack()->baseNote();
}




void NotePlayHandle::play( sampleFrame * _working_buffer )
{
	if( m_muted )
	{
		return;
	}

	// if the note offset falls over to next period, then don't start playback yet
	if( offset() >= Engine::mixer()->framesPerPeriod() )
	{
		setOffset( offset() - Engine::mixer()->framesPerPeriod() );
		return;
	}

	lock();

	/* It is possible for NotePlayHandle::noteOff to be called before NotePlayHandle::play,
	 * which results in a note-on message being sent without a subsequent note-off message.
	 * Therefore, we check here whether the note has already been released before sending
	 * the note-on message. */
	if( !m_released
		&& m_totalFramesPlayed == 0 && !m_hasMidiNote
		&& ( hasParent() || ! m_instrumentTrack->isArpeggioEnabled() ) )
	{
		m_hasMidiNote = true;

		const int baseVelocity = m_instrumentTrack->midiPort()->baseVelocity();

		// send MidiNoteOn event
		m_instrumentTrack->processOutEvent(
			MidiEvent( MidiNoteOn, midiChannel(), midiKey(), midiVelocity( baseVelocity ) ),
			MidiTime::fromFrames( offset(), Engine::framesPerTick() ),
			offset() );
	}

	if( m_frequencyNeedsUpdate )
	{
		updateFrequency();
	}

	// number of frames that can be played this period
	f_cnt_t framesThisPeriod = m_totalFramesPlayed == 0
		? Engine::mixer()->framesPerPeriod() - offset()
		: Engine::mixer()->framesPerPeriod();

	// check if we start release during this period
	if( m_released == false &&
		instrumentTrack()->isSustainPedalPressed() == false &&
		m_totalFramesPlayed + framesThisPeriod > m_frames )
	{
		noteOff( m_totalFramesPlayed == 0
			? ( m_frames + offset() ) // if we have noteon and noteoff during the same period, take offset in account for release frame
			: ( m_frames - m_totalFramesPlayed ) ); // otherwise, the offset is already negated and can be ignored
	}

	// under some circumstances we're called even if there's nothing to play
	// therefore do an additional check which fixes crash e.g. when
	// decreasing release of an instrument-track while the note is active
	if( framesLeft() > 0 )
	{
		// play note!
		m_instrumentTrack->playNote( this, _working_buffer );
	}

	if( m_released && (!instrumentTrack()->isSustainPedalPressed() ||
		m_releaseStarted) )
	{
		m_releaseStarted = true;

		f_cnt_t todo = framesThisPeriod;

		// if this note is base-note for arpeggio, always set
		// m_releaseFramesToDo to bigger value than m_releaseFramesDone
		// because we do not allow NotePlayHandle::isFinished() to be true
		// until all sub-notes are completely played and no new ones
		// are inserted by arpAndChordsTabWidget::processNote()
		if( ! m_subNotes.isEmpty() )
		{
			m_releaseFramesToDo = m_releaseFramesDone + 2 * Engine::mixer()->framesPerPeriod();
		}
		// look whether we have frames left to be done before release
		if( m_framesBeforeRelease )
		{
			// yes, then look whether these samples can be played
			// within one audio-buffer
			if( m_framesBeforeRelease <= framesThisPeriod )
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
				m_framesBeforeRelease -= framesThisPeriod;
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

	// update internal data
	m_totalFramesPlayed += framesThisPeriod;
	unlock();
}




f_cnt_t NotePlayHandle::framesLeft() const
{
	if( instrumentTrack()->isSustainPedalPressed() )
	{
		return 4*Engine::mixer()->framesPerPeriod();
	}
	else if( m_released && actualReleaseFramesToDo() == 0 )
	{
		return m_framesBeforeRelease;
	}
	else if( m_released )
	{
		return m_framesBeforeRelease + m_releaseFramesToDo - m_releaseFramesDone;
	}
	return m_frames+actualReleaseFramesToDo()-m_totalFramesPlayed;
}




fpp_t NotePlayHandle::framesLeftForCurrentPeriod() const
{
	if( m_totalFramesPlayed == 0 )
	{
		return (fpp_t) qMin<f_cnt_t>( framesLeft(), Engine::mixer()->framesPerPeriod() - offset() );
	}
	return (fpp_t) qMin<f_cnt_t>( framesLeft(), Engine::mixer()->framesPerPeriod() );
}




bool NotePlayHandle::isFromTrack( const Track * _track ) const
{
	return m_instrumentTrack == _track || m_bbTrack == _track;
}




void NotePlayHandle::noteOff( const f_cnt_t _s )
{
	if( m_released )
	{
		return;
	}
	m_released = true;

	// first note-off all sub-notes
	for( NotePlayHandle * n : m_subNotes )
	{
		n->lock();
		n->noteOff( _s );
		n->unlock();
	}

	// then set some variables indicating release-state
	m_framesBeforeRelease = _s;
	m_releaseFramesToDo = qMax<f_cnt_t>( 0, actualReleaseFramesToDo() );

	if( m_hasMidiNote )
	{
		m_hasMidiNote = false;

		// send MidiNoteOff event
		m_instrumentTrack->processOutEvent(
				MidiEvent( MidiNoteOff, midiChannel(), midiKey(), 0 ),
				MidiTime::fromFrames( _s, Engine::framesPerTick() ),
				_s );
	}

	// inform attached components about MIDI finished (used for recording in Piano Roll)
	if (!instrumentTrack()->isSustainPedalPressed())
	{
		if( m_origin == OriginMidiInput )
		{
			setLength( MidiTime( static_cast<f_cnt_t>( totalFramesPlayed() / Engine::framesPerTick() ) ) );
			m_instrumentTrack->midiNoteOff( *this );
		}
	}
}




f_cnt_t NotePlayHandle::actualReleaseFramesToDo() const
{
	return m_instrumentTrack->m_soundShaping.releaseFrames();
}




void NotePlayHandle::setFrames( const f_cnt_t _frames )
{
	m_frames = _frames;
	if( m_frames == 0 )
	{
		m_frames = m_instrumentTrack->beatLen( this );
	}
	m_origFrames = m_frames;
}




float NotePlayHandle::volumeLevel( const f_cnt_t _frame )
{
	return m_instrumentTrack->m_soundShaping.volumeLevel( this, _frame );
}




void NotePlayHandle::mute()
{
	// mute all sub-notes
	for( NotePlayHandleList::Iterator it = m_subNotes.begin(); it != m_subNotes.end(); ++it )
	{
		( *it )->mute();
	}
	m_muted = true;
}




int NotePlayHandle::index() const
{
	const PlayHandleList & playHandles = Engine::mixer()->playHandles();
	int idx = 0;
	for( PlayHandleList::ConstIterator it = playHandles.begin(); it != playHandles.end(); ++it )
	{
		const NotePlayHandle * nph = dynamic_cast<const NotePlayHandle *>( *it );
		if( nph == NULL || nph->m_instrumentTrack != m_instrumentTrack || nph->isReleased() || nph->hasParent() )
		{
			continue;
		}
		if( nph == this )
		{
			return idx;
		}
		++idx;
	}
	return -1;
}




ConstNotePlayHandleList NotePlayHandle::nphsOfInstrumentTrack( const InstrumentTrack * _it, bool _all_ph )
{
	const PlayHandleList & playHandles = Engine::mixer()->playHandles();
	ConstNotePlayHandleList cnphv;

	for( PlayHandleList::ConstIterator it = playHandles.begin(); it != playHandles.end(); ++it )
	{
		const NotePlayHandle * nph = dynamic_cast<const NotePlayHandle *>( *it );
		if( nph != NULL && nph->m_instrumentTrack == _it && ( ( nph->isReleased() == false && nph->hasParent() == false ) || _all_ph == true ) )
		{
			cnphv.push_back( nph );
		}
	}
	return cnphv;
}




bool NotePlayHandle::operator==( const NotePlayHandle & _nph ) const
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
			m_hasParent == _nph.m_hasParent &&
			m_origBaseNote == _nph.m_origBaseNote &&
			m_muted == _nph.m_muted &&
			m_midiChannel == _nph.m_midiChannel &&
			m_origin == _nph.m_origin;
}




void NotePlayHandle::updateFrequency()
{
	int mp = m_instrumentTrack->m_useMasterPitchModel.value() ? Engine::getSong()->masterPitch() : 0;
	const float pitch =
		( key() -
				m_instrumentTrack->baseNoteModel()->value() +
				mp +
				m_baseDetuning->value() )
												 / 12.0f;
	m_frequency = BaseFreq * powf( 2.0f, pitch + m_instrumentTrack->pitchModel()->value() / ( 100 * 12.0f ) );
	m_unpitchedFrequency = BaseFreq * powf( 2.0f, pitch );

	for( NotePlayHandleList::Iterator it = m_subNotes.begin(); it != m_subNotes.end(); ++it )
	{
		( *it )->updateFrequency();
	}
}




void NotePlayHandle::processMidiTime( const MidiTime& time )
{
	if( detuning() && time >= songGlobalParentOffset()+pos() )
	{
		const float v = detuning()->automationPattern()->valueAt( time - songGlobalParentOffset() - pos() );
		if( !typeInfo<float>::isEqual( v, m_baseDetuning->value() ) )
		{
			m_baseDetuning->setValue( v );
			updateFrequency();
		}
	}
}




void NotePlayHandle::resize( const bpm_t _new_tempo )
{
	double completed = m_totalFramesPlayed / (double) m_frames;
	double new_frames = m_origFrames * m_origTempo / (double) _new_tempo;
	m_frames = (f_cnt_t)new_frames;
	m_totalFramesPlayed = (f_cnt_t)( completed * new_frames );

	for( NotePlayHandleList::Iterator it = m_subNotes.begin(); it != m_subNotes.end(); ++it )
	{
		( *it )->resize( _new_tempo );
	}
}


NotePlayHandle ** NotePlayHandleManager::s_available;
QReadWriteLock NotePlayHandleManager::s_mutex;
std::atomic_int NotePlayHandleManager::s_availableIndex;
int NotePlayHandleManager::s_size;


void NotePlayHandleManager::init()
{
	s_available = MM_ALLOC( NotePlayHandle*, INITIAL_NPH_CACHE );

	NotePlayHandle * n = MM_ALLOC( NotePlayHandle, INITIAL_NPH_CACHE );

	for( int i=0; i < INITIAL_NPH_CACHE; ++i )
	{
		s_available[ i ] = n;
		++n;
	}
	s_availableIndex = INITIAL_NPH_CACHE - 1;
	s_size = INITIAL_NPH_CACHE;
}


NotePlayHandle * NotePlayHandleManager::acquire( InstrumentTrack* instrumentTrack,
				const f_cnt_t offset,
				const f_cnt_t frames,
				const Note& noteToPlay,
				NotePlayHandle* parent,
				int midiEventChannel,
				NotePlayHandle::Origin origin )
{
	// TODO: use some lockless data structures
	s_mutex.lockForWrite();
	if (s_availableIndex < 0) { extend(NPH_CACHE_INCREMENT); }
	NotePlayHandle * nph = s_available[s_availableIndex--];
	s_mutex.unlock();

	new( (void*)nph ) NotePlayHandle( instrumentTrack, offset, frames, noteToPlay, parent, midiEventChannel, origin );
	return nph;
}


void NotePlayHandleManager::release( NotePlayHandle * nph )
{
	nph->NotePlayHandle::~NotePlayHandle();
	s_mutex.lockForRead();
	s_available[++s_availableIndex] = nph;
	s_mutex.unlock();
}


void NotePlayHandleManager::extend( int c )
{
	s_size += c;
	NotePlayHandle ** tmp = MM_ALLOC( NotePlayHandle*, s_size );
	MM_FREE( s_available );
	s_available = tmp;

	NotePlayHandle * n = MM_ALLOC( NotePlayHandle, c );

	for( int i=0; i < c; ++i )
	{
		s_available[++s_availableIndex] = n;
		++n;
	}
}
