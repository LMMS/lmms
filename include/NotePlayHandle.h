/*
 * NotePlayHandle.h - declaration of class NotePlayHandle which manages
 *                    playback of a single note by an instrument
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

#ifndef LMMS_NOTE_PLAY_HANDLE_H
#define LMMS_NOTE_PLAY_HANDLE_H

#include <memory>

#include "BasicFilters.h"
#include "Note.h"
#include "PlayHandle.h"
#include "Track.h"

class QReadWriteLock;

namespace lmms
{

class InstrumentTrack;
class NotePlayHandle;

using NotePlayHandleList = QList<NotePlayHandle*>;
using ConstNotePlayHandleList = QList<const NotePlayHandle*>;

class LMMS_EXPORT NotePlayHandle : public PlayHandle, public Note
{
public:
	void * m_pluginData;
	std::unique_ptr<BasicFilters<>> m_filter;

	// length of the declicking fade in
	fpp_t m_fadeInLength;

	// specifies origin of NotePlayHandle
	enum class Origin
	{
		MidiClip,		/*! playback of a note from a MIDI clip */
		MidiInput,	/*! playback of a MIDI note input event */
		NoteStacking,	/*! created by note stacking instrument function */
		Arpeggio,		/*! created by arpeggio instrument function */
	};

	NotePlayHandle( InstrumentTrack* instrumentTrack,
					const f_cnt_t offset,
					const f_cnt_t frames,
					const Note& noteToPlay,
					NotePlayHandle* parent = nullptr,
					int midiEventChannel = -1,
					Origin origin = Origin::MidiClip );
	~NotePlayHandle() override;

	void * operator new ( size_t size, void * p )
	{
		return p;
	}

	void setVolume( volume_t volume ) override;
	void setPanning( panning_t panning ) override;

	int midiKey() const;
	int midiChannel() const
	{
		return m_midiChannel;
	}

	/*! convenience function that returns offset for the first period and zero otherwise,
		used by instruments to handle the offset: instruments have to check this property and
		add the correct number of empty frames in the beginning of the period */
	f_cnt_t noteOffset() const
	{
		return m_totalFramesPlayed == 0
			? offset()
			: 0;
	}

	const float& frequency() const
	{
		return m_frequency;
	}

	/*! Returns frequency without pitch wheel influence */
	float unpitchedFrequency() const
	{
		return m_unpitchedFrequency;
	}

	//! Get the current per-note detuning for this note
	float currentDetuning() const { return m_baseDetuning->value(); }

	/*! Renders one chunk using the attached instrument into the buffer */
	void play(CoreAudioDataMut buffer) override;

	/*! Returns whether playback of note is finished and thus handle can be deleted */
	bool isFinished() const override
	{
		return m_released && framesLeft() <= 0;
	}

	/*! Returns number of frames left for playback */
	f_cnt_t framesLeft() const;

	/*! Returns how many frames have to be rendered in current period */
	fpp_t framesLeftForCurrentPeriod() const;

	/*! Returns whether the play handle plays on a certain track */
	bool isFromTrack( const Track* _track ) const override;

	/*! Releases the note (and plays release frames) */
	void noteOff( const f_cnt_t offset = 0 );

	/*! Returns number of frames to be played until the note is going to be released */
	f_cnt_t framesBeforeRelease() const
	{
		return m_framesBeforeRelease;
	}

	/*! Returns how many frames were played since release */
	f_cnt_t releaseFramesDone() const
	{
		return m_releaseFramesDone;
	}

	/*! Returns the number of frames to be played after release according to
	    the release times in the envelopes */
	f_cnt_t actualReleaseFramesToDo() const;

	/*! Returns total numbers of frames to play (including release frames) */
	f_cnt_t frames() const
	{
		return m_frames;
	}

	/*! Sets the total number of frames to play (including release frames) */
	void setFrames( const f_cnt_t _frames );

	/*! Returns whether note was released */
	bool isReleased() const
	{
		return m_released;
	}

	bool isReleaseStarted() const
	{
		return m_releaseStarted;
	}

	/*! Returns total numbers of frames played so far */
	f_cnt_t totalFramesPlayed() const
	{
		return m_totalFramesPlayed;
	}

	/*! Returns volume level at given frame (envelope/LFO) */
	float volumeLevel( const f_cnt_t frame );

	/*! Returns instrument track which is being played by this handle (const version) */
	const InstrumentTrack* instrumentTrack() const
	{
		return m_instrumentTrack;
	}

	/*! Returns instrument track which is being played by this handle */
	InstrumentTrack* instrumentTrack()
	{
		return m_instrumentTrack;
	}

	/*! Returns whether note has a parent, e.g. is not part of an arpeggio or a chord */
	bool hasParent() const
	{
		return m_hasParent;
	}

	/*! Returns origin of note */
	Origin origin() const
	{
		return m_origin;
	}

	/*! Returns whether note has children */
	bool isMasterNote() const
	{
		return m_subNotes.size() > 0 || m_hadChildren;
	}

	void setMasterNote()
	{
		m_hadChildren = true;
		setUsesBuffer( false );
	}

	/*! Returns whether note is muted */
	bool isMuted() const
	{
		return m_muted;
	}

	/*! Mutes playback of note */
	void mute();

	/*! Returns index of NotePlayHandle in vector of note-play-handles
	    belonging to this instrument track - used by arpeggiator.
	    Ignores child note-play-handles, returns -1 when called on one */
	int index() const;

	/*! Returns list of note-play-handles belonging to given instrument track.
	    If allPlayHandles = true, also released note-play-handles and children
	    are returned */
	static ConstNotePlayHandleList nphsOfInstrumentTrack( const InstrumentTrack* Track, bool allPlayHandles = false );

	/*! Returns whether given NotePlayHandle instance is equal to *this */
	bool operator==( const NotePlayHandle & _nph ) const;

	/*! Returns whether NotePlayHandle belongs to pattern track and pattern track is muted */
	bool isPatternTrackMuted()
	{
		return m_patternTrack && m_patternTrack->isMuted();
	}

	/*! Sets attached pattern track */
	void setPatternTrack(Track* t)
	{
		m_patternTrack = t;
	}

	/*! Process note detuning automation */
	void processTimePos(const TimePos& time, float pitchValue, bool isRecording);

	/*! Updates total length (m_frames) depending on a new tempo */
	void resize( const bpm_t newTempo );

	/*! Set song-global offset (relative to containing MIDI clip) in order to properly perform the note detuning */
	void setSongGlobalParentOffset( const TimePos& offset )
	{
		m_songGlobalParentOffset = offset;
	}

	/*! Returns song-global offset */
	const TimePos& songGlobalParentOffset() const
	{
		return m_songGlobalParentOffset;
	}

	void setFrequencyUpdate()
	{
		m_frequencyNeedsUpdate = true;
	}

private:
	class BaseDetuning
	{
	public:
		BaseDetuning( DetuningHelper* detuning );

		void setValue( float val )
		{
			m_value = val;
		}

		float value() const
		{
			return m_value;
		}


	private:
		float m_value;

	} ;

	void updateFrequency();

	InstrumentTrack* m_instrumentTrack;		// needed for calling
											// InstrumentTrack::playNote
	f_cnt_t m_frames;						// total frames to play
	f_cnt_t m_totalFramesPlayed;			// total frame-counter - used for
											// figuring out whether a whole note
											// has been played
	f_cnt_t m_framesBeforeRelease;			// number of frames after which note
											// is released
	f_cnt_t m_releaseFramesToDo;			// total numbers of frames to be
											// played after release
	f_cnt_t m_releaseFramesDone;			// number of frames done after
											// release of note
	NotePlayHandleList m_subNotes;			// used for chords and arpeggios
	volatile bool m_released;				// indicates whether note is released
	bool m_releaseStarted;
	bool m_hasMidiNote;
	bool m_hasParent;						// indicates whether note has parent
	NotePlayHandle * m_parent;			// parent note
	bool m_hadChildren;
	bool m_muted;							// indicates whether note is muted
	Track* m_patternTrack;						// related pattern track

	// tempo reaction
	bpm_t m_origTempo;						// original tempo
	f_cnt_t m_origFrames;					// original m_frames

	int m_origBaseNote;

	float m_frequency;
	float m_unpitchedFrequency;

	BaseDetuning* m_baseDetuning;
	TimePos m_songGlobalParentOffset;

	int m_midiChannel;
	Origin m_origin;

	bool m_frequencyNeedsUpdate;				// used to update pitch
} ;


const int INITIAL_NPH_CACHE = 256;
const int NPH_CACHE_INCREMENT = 16;

class NotePlayHandleManager
{
public:
	static void init();
	static NotePlayHandle * acquire( InstrumentTrack* instrumentTrack,
					const f_cnt_t offset,
					const f_cnt_t frames,
					const Note& noteToPlay,
					NotePlayHandle* parent = nullptr,
					int midiEventChannel = -1,
					NotePlayHandle::Origin origin = NotePlayHandle::Origin::MidiClip );
	static void release( NotePlayHandle * nph );
	static void extend( int i );
	static void free();

private:
	static NotePlayHandle ** s_available;
	static QReadWriteLock s_mutex;
	static std::atomic_int s_availableIndex;
	static int s_size;
};


} // namespace lmms

#endif // LMMS_NOTE_PLAY_HANDLE_H
