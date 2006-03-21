/*
 * note.h - declaration of class note which contains all informations about a
 *          note + definitions of several constants and enums
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


#ifndef _NOTE_H
#define _NOTE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "qt3support.h"

#ifdef QT4

#include <QVector>

#else

#include <qvaluevector.h>

#endif

#include "volume.h"
#include "panning.h"
#include "midi_time.h"
#include "journalling_object.h"

enum tones
{
	C = 0,
	CIS = 1, DES = 1,
	D = 2,
	DIS = 3, ES = 3,
	E = 4, FES = 4,
	F = 5,
	FIS = 6, GES = 6,
	G = 7,
	GIS = 8, AS = 8,
	A = 9,
	AIS = 10, B = 10,
	H = 11 
} ;


enum octaves
{
	OCTAVE_0,
	OCTAVE_1,
	OCTAVE_2,
	OCTAVE_3,
	OCTAVE_4,	// default
	OCTAVE_5,
	OCTAVE_6,
	OCTAVE_7,
	OCTAVE_8
} ;

const octaves DEFAULT_OCTAVE = OCTAVE_4;
const octaves MIN_OCTAVE = OCTAVE_0;
const octaves MAX_OCTAVE = OCTAVE_8;

const int WHITE_KEYS_PER_OCTAVE	= 7;
const int BLACK_KEYS_PER_OCTAVE	= 5;
const int NOTES_PER_OCTAVE = WHITE_KEYS_PER_OCTAVE + BLACK_KEYS_PER_OCTAVE;
const int OCTAVES = 9;



class note : public journallingObject
{
public:
	note( engine * _engine = NULL,
		const midiTime & _length = 0,
		const midiTime & _pos = 0,
		tones _tone = A,
		octaves _octave = DEFAULT_OCTAVE,
		volume _volume = DEFAULT_VOLUME,
		panning _panning = DEFAULT_PANNING ) FASTCALL;

	virtual ~note();

	void FASTCALL setLength( const midiTime & _length );
	void FASTCALL setPos( const midiTime & _pos );
	void FASTCALL setTone( const tones _tone = C );
	void FASTCALL setOctave( const octaves _octave = DEFAULT_OCTAVE );
	void FASTCALL setKey( const int _key );
	void FASTCALL setVolume( const volume _volume = DEFAULT_VOLUME );
	void FASTCALL setPanning( const panning _panning = DEFAULT_PANNING );
	void FASTCALL quantizeLength( const int _q_grid );
	void FASTCALL quantizePos( const int _q_grid );

	inline midiTime endPos( void ) const
	{
		return( m_pos + m_length);
	}

	inline const midiTime & length( void ) const
	{
		return( m_length );
	}

	inline const midiTime & pos( void ) const
	{
		return( m_pos );
	}

	inline midiTime pos( midiTime _base_pos ) const
	{
		return( m_pos - _base_pos );
	}

	inline tones tone( void ) const
	{
		return( m_tone );
	}

	inline octaves octave( void ) const
	{
		return( m_octave );
	}

	inline int key( void ) const
	{
		return( m_octave * NOTES_PER_OCTAVE + m_tone );
	}

	inline volume getVolume( void ) const
	{
		return( m_volume );
	}

	inline panning getPanning( void ) const
	{
		return( m_panning );
	}

	inline virtual QString nodeName( void ) const
	{
		return( "note" );
	}


protected:
	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );

	virtual void undoStep( journalEntry & _je );
	virtual void redoStep( journalEntry & _je );


private:
	midiTime FASTCALL quantized( const midiTime & _m, const int _q_grid );

	enum actions
	{
		CHANGE_KEY, CHANGE_VOLUME, CHANGE_PANNING,
		CHANGE_LENGTH, CHANGE_POSITION
	} ;


	tones m_tone;
	octaves m_octave;
	volume m_volume;
	panning m_panning;
	midiTime m_length;
	midiTime m_pos;

} ;


typedef vvector<note *> noteVector;


#endif

