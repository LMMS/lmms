/*
 * NotePlayHandle.h - declaration of class NotePlayHandle which manages
 *                    playback of a single note by an instrument
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#ifndef NOTE_PLAY_HANDLE_H
#define NOTE_PLAY_HANDLE_H

#include "lmmsconfig.h"
#include "note.h"
#include "PlayHandle.h"
#include "track.h"


class InstrumentTrack;
class NotePlayHandle;

template<ch_cnt_t=DEFAULT_CHANNELS> class basicFilters;
typedef QList<NotePlayHandle *> NotePlayHandleList;
typedef QList<const NotePlayHandle *> ConstNotePlayHandleList;


class EXPORT NotePlayHandle : public PlayHandle, public note
{
public:
	void * m_pluginData;
	basicFilters<> * m_filter;

	// specifies origin of NotePlayHandle
	enum Origins
	{
		OriginPattern,		/*! playback of a note from a pattern */
		OriginMidiInput,	/*! playback of a MIDI note input event */
		OriginNoteStacking,	/*! created by note stacking instrument function */
		OriginArpeggio,		/*! created by arpeggio instrument function */
		OriginCount
	};
	typedef Origins Origin;

	NotePlayHandle( InstrumentTrack* instrumentTrack,
					const f_cnt_t offset,
					const f_cnt_t frames,
					const note& noteToPlay,
					NotePlayHandle* parent = NULL,
					int midiEventChannel = -1,
					Origin origin = OriginPattern );
	virtual ~NotePlayHandle();

	virtual void setVolume( volume_t volume );
	virtual void setPanning( panning_t panning );

	int midiKey() const;
	int midiChannel() const
	{
		return m_midiChannel;
	}

	const float& frequency() const
	{
		return m_frequency;
	}

	void updateFrequency();

	/*! Returns frequency without pitch wheel influence */
	float unpitchedFrequency() const
	{
		return m_unpitchedFrequency;
	}

	/*! Renders one chunk using the attached instrument into the buffer */
	virtual void play( sampleFrame* buffer );

	/*! Returns whether playback of note is finished and thus handle can be deleted */
	virtual bool isFinished() const
	{
		return m_released && framesLeft() <= 0 && m_scheduledNoteOff < 0;
	}

	/*! Returns number of frames left for playback */
	f_cnt_t framesLeft() const;

	/*! Returns how many frames have to be rendered in current period */
	fpp_t framesLeftForCurrentPeriod() const;

	/*! Returns whether the play handle plays on a certain track */
	virtual bool isFromTrack( const track* _track ) const;

	/*! Releases the note (and plays release frames */
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

	/*! Returns whether note is muted */
	bool isMuted() const
	{
		return m_muted;
	}

	/*! Mutes playback of note */
	void mute();

	/*! Returns index of NotePlayHandle in vector of note-play-handles
        belonging to this instrument track - used by arpeggiator */
	int index() const;

	/*! returns list of note-play-handles belonging to given instrument track,
	    if allPlayHandles = true, also released note-play-handles are returned */
	static ConstNotePlayHandleList nphsOfInstrumentTrack( const InstrumentTrack* track, bool allPlayHandles = false );

	/*! Returns whether given NotePlayHandle instance is equal to *this */
	bool operator==( const NotePlayHandle & _nph ) const;

	/*! Returns whether NotePlayHandle belongs to BB track and BB track is muted */
	bool isBbTrackMuted()
	{
		return m_bbTrack && m_bbTrack->isMuted();
	}

	/*! Sets attached BB track */
	void setBBTrack( track* t )
	{
		m_bbTrack = t;
	}

	/*! Process note detuning automation */
	void processMidiTime( const MidiTime& time );

	/*! Updates total length (m_frames) depending on a new tempo */
	void resize( const bpm_t newTempo );

	/*! Set song-global offset (relative to containing pattern) in order to properly perform the note detuning */
	void setSongGlobalParentOffset( const MidiTime& offset )
	{
		m_songGlobalParentOffset = offset;
	}

	/*! Returns song-global offset */
	const MidiTime& songGlobalParentOffset() const
	{
		return m_songGlobalParentOffset;
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
	f_cnt_t m_scheduledNoteOff;			// variable for scheduling noteoff at next period
	NotePlayHandleList m_subNotes;			// used for chords and arpeggios
	volatile bool m_released;				// indicates whether note is released
	bool m_hasParent;
	bool m_hadChildren;
	bool m_muted;							// indicates whether note is muted
	track* m_bbTrack;						// related BB track

	// tempo reaction
	bpm_t m_origTempo;						// original tempo
	f_cnt_t m_origFrames;					// original m_frames

	const int m_origBaseNote;

	float m_frequency;
	float m_unpitchedFrequency;

	BaseDetuning* m_baseDetuning;
	MidiTime m_songGlobalParentOffset;

	const int m_midiChannel;
	const Origin m_origin;

} ;

#endif
