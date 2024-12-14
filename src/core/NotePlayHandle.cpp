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

#include "AudioEngine.h"
#include "BasicFilters.h"
#include "DetuningHelper.h"
#include "InstrumentSoundShaping.h"
#include "InstrumentTrack.h"
#include "Instrument.h"
#include "Song.h"

namespace lmms
{

NotePlayHandle::BaseDetuning::BaseDetuning( DetuningHelper *detuning ) :
	m_value( detuning ? detuning->automationClip()->valueAt( 0 ) : 0 )
{
}






NotePlayHandle::NotePlayHandle( InstrumentTrack* instrumentTrack,
								const f_cnt_t _offset,
								const f_cnt_t _frames,
								const Note& n,
								NotePlayHandle *parent,
								int midiEventChannel,
								Origin origin ) :
	PlayHandle( PlayHandle::Type::NotePlayHandle, _offset ),
	Note( n.length(), n.pos(), n.key(), n.getVolume(), n.getPanning(), n.detuning() ),
	m_pluginData( nullptr ),
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
	m_hasParent( parent != nullptr  ),
	m_parent( parent ),
	m_hadChildren( false ),
	m_muted( false ),
	m_patternTrack( nullptr ),
	m_origTempo( Engine::getSong()->getTempo() ),
	m_origBaseNote( instrumentTrack->baseNote() ),
	m_frequency( 0 ),
	m_unpitchedFrequency( 0 ),
	m_baseDetuning( nullptr ),
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

		m_patternTrack = parent->m_patternTrack;

		parent->setUsesBuffer( false );
	}

	updateFrequency();

	setFrames( _frames );

	// inform attached components about new MIDI note (used for recording in Piano Roll)
	if( m_origin == Origin::MidiInput )
	{
		m_instrumentTrack->midiNoteOn( *this );
	}

	if (m_instrumentTrack->instrument() && m_instrumentTrack->instrument()->isSingleStreamed())
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

	if( m_pluginData != nullptr )
	{
		// TODO:
		m_instrumentTrack->deleteNotePluginData( this );
	}

	if( m_instrumentTrack->m_notes[key()] == this )
	{
		m_instrumentTrack->m_notes[key()] = nullptr;
	}

	m_subNotes.clear();

	if (buffer().data() != nullptr)
	{
		releaseBuffer();
	}

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




void NotePlayHandle::play(CoreAudioDataMut buffer)
{
	if (m_muted)
	{
		return;
	}

	// if the note offset falls over to next period, then don't start playback yet
	if( offset() >= Engine::audioEngine()->framesPerPeriod() )
	{
		setOffset( offset() - Engine::audioEngine()->framesPerPeriod() );
		return;
	}

	lock();

	// Don't play the note if it falls outside of the user defined key range
	// TODO: handle the range check by Microtuner, and if the key becomes "not mapped", save the current frequency
	// so that the note release can finish playing using a valid frequency instead of a 1 Hz placeholder
	if (key() < m_instrumentTrack->m_firstKeyModel.value() ||
		key() > m_instrumentTrack->m_lastKeyModel.value())
	{
		// Release the note in case it started playing before going out of range
		noteOff(0);
		// Exit if the note did not start playing before going out of range (i.e. there is no need to play release)
		if (m_totalFramesPlayed == 0)
		{
			unlock();
			return;
		}
	}

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
			TimePos::fromFrames( offset(), Engine::framesPerTick() ),
			offset() );
	}

	if( m_frequencyNeedsUpdate )
	{
		updateFrequency();
	}

	// number of frames that can be played this period
	f_cnt_t framesThisPeriod = m_totalFramesPlayed == 0
		? Engine::audioEngine()->framesPerPeriod() - offset()
		: Engine::audioEngine()->framesPerPeriod();

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
		m_instrumentTrack->playNote(this, buffer);
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
			m_releaseFramesToDo = m_releaseFramesDone + 2 * Engine::audioEngine()->framesPerPeriod();
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
		return 4 * Engine::audioEngine()->framesPerPeriod();
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
		return static_cast<fpp_t>(std::min<f_cnt_t>(framesLeft(), Engine::audioEngine()->framesPerPeriod() - offset()));
	}
	return static_cast<fpp_t>(std::min<f_cnt_t>(framesLeft(), Engine::audioEngine()->framesPerPeriod()));
}




bool NotePlayHandle::isFromTrack( const Track * _track ) const
{
	return m_instrumentTrack == _track || m_patternTrack == _track;
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
	m_releaseFramesToDo = std::max<f_cnt_t>(0, actualReleaseFramesToDo());

	if( m_hasMidiNote )
	{
		m_hasMidiNote = false;

		// send MidiNoteOff event
		m_instrumentTrack->processOutEvent(
				MidiEvent( MidiNoteOff, midiChannel(), midiKey(), 0 ),
				TimePos::fromFrames( _s, Engine::framesPerTick() ),
				_s );
	}

	// inform attached components about MIDI finished (used for recording in Piano Roll)
	if (!instrumentTrack()->isSustainPedalPressed())
	{
		if( m_origin == Origin::MidiInput )
		{
			setLength( TimePos( static_cast<f_cnt_t>( totalFramesPlayed() / Engine::framesPerTick() ) ) );
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
	for (const auto& subNote : m_subNotes)
	{
		subNote->mute();
	}
	m_muted = true;
}




int NotePlayHandle::index() const
{
	const PlayHandleList & playHandles = Engine::audioEngine()->playHandles();
	int idx = 0;
	for (const auto& playHandle : playHandles)
	{
		const auto nph = dynamic_cast<const NotePlayHandle*>(playHandle);
		if( nph == nullptr || nph->m_instrumentTrack != m_instrumentTrack || nph->isReleased() || nph->hasParent() )
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
	const PlayHandleList & playHandles = Engine::audioEngine()->playHandles();
	ConstNotePlayHandleList cnphv;

	for (const auto& playHandle : playHandles)
	{
		const auto nph = dynamic_cast<const NotePlayHandle*>(playHandle);
		if( nph != nullptr && nph->m_instrumentTrack == _it && ( ( nph->isReleased() == false && nph->hasParent() == false ) || _all_ph == true ) )
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
	int masterPitch = m_instrumentTrack->m_useMasterPitchModel.value() ? Engine::getSong()->masterPitch() : 0;
	int baseNote = m_instrumentTrack->baseNoteModel()->value();
	float detune = m_baseDetuning->value();
	float instrumentPitch = m_instrumentTrack->pitchModel()->value();

	if (m_instrumentTrack->m_microtuner.enabled())
	{
		// custom key mapping and scale: get frequency from the microtuner
		const auto transposedKey = key() + masterPitch;

		if (m_instrumentTrack->isKeyMapped(transposedKey))
		{
			const auto frequency = m_instrumentTrack->m_microtuner.keyToFreq(transposedKey, baseNote);
			m_frequency = frequency * powf(2.f, (detune + instrumentPitch / 100) / 12.f);
			m_unpitchedFrequency = frequency * powf(2.f, detune / 12.f);
		}
		else
		{
			m_frequency = m_unpitchedFrequency = 0;
		}
	}
	else
	{
		// default key mapping and 12-TET frequency computation with default 440 Hz base note frequency
		const float pitch = (key() - baseNote + masterPitch + detune) / 12.0f;
		m_frequency = DefaultBaseFreq * powf(2.0f, pitch + instrumentPitch / (100 * 12.0f));
		m_unpitchedFrequency = DefaultBaseFreq * powf(2.0f, pitch);
	}

	for (auto it : m_subNotes)
	{
		it->updateFrequency();
	}
}




void NotePlayHandle::processTimePos(const TimePos& time, float pitchValue, bool isRecording)
{
	if (!detuning() || time < songGlobalParentOffset() + pos()) { return; }

	if (isRecording && m_origin == Origin::MidiInput)
	{
		detuning()->automationClip()->recordValue(time - songGlobalParentOffset() - pos(), pitchValue / 100);
	}
	else
	{
		const float v = detuning()->automationClip()->valueAt(time - songGlobalParentOffset() - pos());
		if (!approximatelyEqual(v, m_baseDetuning->value()))
		{
			m_baseDetuning->setValue(v);
			updateFrequency();
		}
	}
}




void NotePlayHandle::resize( const bpm_t _new_tempo )
{
	if (origin() == Origin::MidiInput ||
		(origin() == Origin::NoteStacking && m_parent->origin() == Origin::MidiInput))
	{
		// Don't resize notes from MIDI input - they should continue to play
		// until the key is released, and their large duration can cause
		// overflows in this method.
		return;
	}

	double completed = m_totalFramesPlayed / (double) m_frames;
	double new_frames = m_origFrames * m_origTempo / (double) _new_tempo;
	m_frames = (f_cnt_t)new_frames;
	m_totalFramesPlayed = (f_cnt_t)( completed * new_frames );

	for (const auto& subNote : m_subNotes)
	{
		subNote->resize(_new_tempo);
	}
}


NotePlayHandle ** NotePlayHandleManager::s_available;
QReadWriteLock NotePlayHandleManager::s_mutex;
std::atomic_int NotePlayHandleManager::s_availableIndex;
int NotePlayHandleManager::s_size;


void NotePlayHandleManager::init()
{
	s_available = new NotePlayHandle*[INITIAL_NPH_CACHE];

	auto n = static_cast<NotePlayHandle *>(std::malloc(sizeof(NotePlayHandle) * INITIAL_NPH_CACHE));

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
	auto tmp = new NotePlayHandle*[s_size];
	delete[] s_available;
	s_available = tmp;

	auto n = static_cast<NotePlayHandle *>(std::malloc(sizeof(NotePlayHandle) * c));

	for( int i=0; i < c; ++i )
	{
		s_available[++s_availableIndex] = n;
		++n;
	}
}

void NotePlayHandleManager::free()
{
	delete[] s_available;
}


} // namespace lmms
