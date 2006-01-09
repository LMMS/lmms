/*
 * note_play_handle.h - declaration of class notePlayHandle which is needed
 *                      by LMMS-Play-Engine
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


#ifndef _NOTE_PLAY_HANDLE_H
#define _NOTE_PLAY_HANDLE_H

#include <qpair.h>

#include "play_handle.h"
#include "basic_filters.h"
#include "note.h"


class channelTrack;
class notePlayHandle;

typedef vvector<notePlayHandle *> notePlayHandleVector;
typedef vvector<const notePlayHandle *> constNotePlayHandleVector;


class notePlayHandle : public playHandle, public note
{
public:
	void * m_pluginData;
	basicFilters<> * m_filter;

	notePlayHandle( channelTrack * _chnl_trk, const Uint32 _frames_ahead,
					const Uint32 _frames, note * _n,
					const bool _arp_note = FALSE ) FASTCALL;
	~notePlayHandle();


	virtual void play( void );

	virtual inline bool done( void ) const
	{
		return( ( m_released && m_framesBeforeRelease == 0 &&
			m_releaseFramesDone >= m_releaseFramesToDo  ) ||
						m_channelTrack == NULL );
	}

	virtual void checkValidity( void );


	void FASTCALL noteOff( Uint32 _s = 0 );

	inline Uint32 framesBeforeRelease( void ) const
	{
		return( m_framesBeforeRelease );
	}

	inline Uint32 releaseFramesDone( void ) const
	{
		return( m_releaseFramesDone );
	}

	Uint32 actualReleaseFramesToDo( void ) const;

	// returns how many samples this note is aligned ahead, i.e.
	// at which position it is inserted in the according buffer
	inline Uint32 framesAhead( void ) const
	{
		return ( m_framesAhead );
	}

	// returns total numbers of frames to play
	inline Uint32 frames( void ) const
	{
		return( m_frames );
	}

	void setFrames( Uint32 _frames );

	// returns whether note was released
	inline bool released( void ) const
	{
		return( m_released );
	}

	// returns total numbers of played frames
	inline Uint32 totalFramesPlayed( void ) const
	{
		return( m_totalFramesPlayed );
	}

	// returns volume-level at frame _frame (envelope/LFO)
	float FASTCALL volumeLevel( Uint32 _frame );

	// adds note-play-handle _n as subnote
	inline void addSubNote( notePlayHandle * _n )
	{
		m_subNotes.push_back( _n );
		_n->m_baseNote = FALSE;
		// if there was an arp-note added and this note is a base-note
		// we set arp-note-flag for indicating that this note is an
		// arpeggio-base-note
		m_arpNote = _n->arpNote() && baseNote();
	}

	// returns channel-track this note-play-handle plays
	inline channelTrack * getChannelTrack( void )
	{
		return( m_channelTrack );
	}

	// returns whether note is a base-note, e.g. is not part of an arpeggio
	// or a chord
	inline bool baseNote( void ) const
	{
		return( m_baseNote );
	}

	// returns whether note is part of an arpeggio
	inline bool arpNote( void ) const
	{
		return( m_arpNote );
	}

	inline void setArpNote( bool _on )
	{
		m_arpNote = _on;
	}

	// returns whether note is base-note for arpeggio
	inline bool arpBaseNote( void ) const
	{
		return( baseNote() && arpNote() );
	}

	inline bool muted( void ) const
	{
		return( m_muted );
	}

	void mute( void );

	// returns index of note-play-handle in vector of note-play-handles 
	// belonging to this channel
	int index( void ) const;

	// note-play-handles belonging to given channel
	static constNotePlayHandleVector nphsOfChannelTrack(
						const channelTrack * _ct );

	// return whether given note-play-handle is equal to *this
	bool operator==( const notePlayHandle & _nph ) const;


private:
	channelTrack * m_channelTrack;	// needed for calling
					// channelTrack::playNote
	Uint32 m_frames;		// total frames to play
	Uint32 m_framesAhead;		// numbers of frames ahead in buffer
					// to mix in
	Uint32 m_totalFramesPlayed;	// total frame-counter - used for
					// figuring out whether a whole note
					// has been played
	Uint32 m_framesBeforeRelease;	// number of frames after which note
					// is released
	Uint32 m_releaseFramesToDo;	// total numbers of frames to be
					// played after release
	Uint32 m_releaseFramesDone;	// number of frames done after
					// release of note
	notePlayHandleVector m_subNotes;// used for chords and arpeggios
	bool m_released;		// indicates whether note is released
	bool m_baseNote;		// indicates whether note is a
					// base-note (i.e. no sub-note)
	bool m_arpNote;			// indicates whether note is part of
					// an arpeggio (either base-note or
					// sub-note)
	bool m_muted;			// indicates whether note is muted

} ;

#endif
