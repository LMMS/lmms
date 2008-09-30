/*
 * note_play_handle.h - declaration of class notePlayHandle which is needed
 *                      by LMMS-Play-Engine
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


#ifndef _NOTE_PLAY_HANDLE_H
#define _NOTE_PLAY_HANDLE_H

#include "lmmsconfig.h"
#include "mixer.h"
#include "note.h"
#include "instrument.h"
#include "instrument_track.h"
#include "engine.h"


class notePlayHandle;

template<ch_cnt_t=DEFAULT_CHANNELS> class basicFilters;
typedef QVector<notePlayHandle *> notePlayHandleVector;
typedef QVector<const notePlayHandle *> constNotePlayHandleVector;


class EXPORT notePlayHandle : public playHandle, public note
{
public:
	void * m_pluginData;
	basicFilters<> * m_filter;

	notePlayHandle( instrumentTrack * _instrument_track,
					const f_cnt_t _offset,
					const f_cnt_t _frames, const note & _n,
					notePlayHandle * _parent = NULL,
					const bool _part_of_arp = false );
	virtual ~notePlayHandle();

	virtual void setVolume( const volume _volume = DefaultVolume );

	int getMidiVelocity( void ) const;

	const float & frequency( void ) const
	{
		return( m_frequency );
	}

	void updateFrequency( void );

	// returns frequency without pitch-wheel influence
	float unpitchedFrequency( void ) const
	{
		return( m_unpitchedFrequency );
	}

	virtual void play( sampleFrame * _working_buffer );

	virtual inline bool done( void ) const
	{
		return( m_released && framesLeft() <= 0 );
	}

	bool willFinishThisPeriod( void ) const
	{
		f_cnt_t rftd = m_releaseFramesToDo;
		f_cnt_t fbr = m_framesBeforeRelease;
		f_cnt_t rfd = m_releaseFramesDone;
		if( m_released == TRUE )
		{
			f_cnt_t todo = engine::getMixer()->framesPerPeriod();
			if( isArpeggioBaseNote() )
			{
				rftd = rfd + 2 *
					engine::getMixer()->framesPerPeriod();
			}
			if( fbr )
			{
				if( fbr <=
					engine::getMixer()->framesPerPeriod() )
				{
					todo -= fbr;
					fbr = 0;
				}
				else
				{
					todo = 0;
					fbr -=
					engine::getMixer()->framesPerPeriod();
				}
			}
			if( todo && rfd < rftd )
			{
				if( rftd - rfd >= todo )
				{
					rfd += todo;
				}
				else
				{
					rfd = rftd;
				}
			}
		}

		if( isArpeggioBaseNote() && m_subNotes.size() == 0 )
		{
			rfd = rftd;
		}

		return( ( m_released && fbr == 0 && rfd >= rftd ) );
	}

	f_cnt_t framesLeft( void ) const;

	inline f_cnt_t framesLeftForCurrentPeriod( void ) const
	{
		return( tMin<f_cnt_t>( framesLeft(),
				engine::getMixer()->framesPerPeriod() ) );
	}


	virtual bool isFromTrack( const track * _track ) const;


	void noteOff( const f_cnt_t _s = 0 );

	inline f_cnt_t framesBeforeRelease( void ) const
	{
		return( m_framesBeforeRelease );
	}

	inline f_cnt_t releaseFramesDone( void ) const
	{
		return( m_releaseFramesDone );
	}

	f_cnt_t actualReleaseFramesToDo( void ) const;


	// returns total numbers of frames to play
	inline f_cnt_t frames( void ) const
	{
		return( m_frames );
	}

	void setFrames( const f_cnt_t _frames );

	// returns whether note was released
	inline bool released( void ) const
	{
		return( m_released );
	}

	// returns total numbers of played frames
	inline f_cnt_t totalFramesPlayed( void ) const
	{
		return( m_totalFramesPlayed );
	}

	// returns volume-level at frame _frame (envelope/LFO)
	float volumeLevel( const f_cnt_t _frame );

	// returns instrument-track this note-play-handle plays
	inline instrumentTrack * getInstrumentTrack( void )
	{
		return( m_instrumentTrack );
	}

	// returns whether note is a base-note, e.g. is not part of an arpeggio
	// or a chord
	inline bool isBaseNote( void ) const
	{
		return( m_baseNote );
	}

	inline bool isPartOfArpeggio( void ) const
	{
		return( m_partOfArpeggio );
	}

	inline void setPartOfArpeggio( const bool _on )
	{
		m_partOfArpeggio = _on;
	}

	// returns whether note is base-note for arpeggio
	inline bool isArpeggioBaseNote( void ) const
	{
		return( isBaseNote() && ( m_partOfArpeggio || 
				m_instrumentTrack->arpeggiatorEnabled() ) );
	}

	inline bool isMuted( void ) const
	{
		return( m_muted );
	}

	void mute( void );

	// returns index of note-play-handle in vector of note-play-handles 
	// belonging to this instrument-track - used by arpeggiator
	int index( void ) const;

	// note-play-handles belonging to given channel, if _all_ph = TRUE,
	// also released note-play-handles are returned
	static constNotePlayHandleVector nphsOfInstrumentTrack(
			const instrumentTrack * _ct, bool _all_ph = FALSE );

	// return whether given note-play-handle is equal to *this
	bool operator==( const notePlayHandle & _nph ) const;

	bool bbTrackMuted( void )
	{
		return( m_bbTrack && m_bbTrack->isMuted() );
	}
	void setBBTrack( track * _bb_track )
	{
		m_bbTrack = _bb_track;
	}

	void processMidiTime( const midiTime & _time );
	void resize( const bpm_t _new_tempo );

#if LMMS_SINGERBOT_SUPPORT
	int patternIndex( void )
	{
		return( m_patternIndex );
	}

	void setPatternIndex( int _i )
	{
		m_patternIndex = _i;
	}
#endif


private:
	class baseDetuning
	{
	public:
		baseDetuning( detuningHelper * _detuning );

		void setValue( float _val )
		{
			m_value = _val;
		}

		float value( void ) const
		{
			return( m_value );
		}


	private:
		detuningHelper * m_detuning;
		float m_value;

	} ;


	instrumentTrack * m_instrumentTrack;	// needed for calling
					// instrumentTrack::playNote
	f_cnt_t m_frames;		// total frames to play
	f_cnt_t m_totalFramesPlayed;	// total frame-counter - used for
					// figuring out whether a whole note
					// has been played
	f_cnt_t m_framesBeforeRelease;	// number of frames after which note
					// is released
	f_cnt_t m_releaseFramesToDo;	// total numbers of frames to be
					// played after release
	f_cnt_t m_releaseFramesDone;	// number of frames done after
					// release of note
	notePlayHandleVector m_subNotes;// used for chords and arpeggios
	volatile bool m_released;	// indicates whether note is released
	bool m_baseNote;		// indicates whether note is a
					// base-note (i.e. no sub-note)
	bool m_partOfArpeggio;		// indicates whether note is part of
					// an arpeggio (either base-note or
					// sub-note)
	bool m_muted;			// indicates whether note is muted
	track * m_bbTrack;		// related BB track
#if LMMS_SINGERBOT_SUPPORT
	int m_patternIndex;		// position among relevant notes
#endif

	// tempo reaction
	bpm_t m_origTempo;		// original tempo
	f_cnt_t m_origFrames;		// original m_frames

	float m_frequency;
	float m_unpitchedFrequency;

	baseDetuning * m_baseDetuning;

} ;

#endif
