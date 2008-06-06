/*
 * note.h - declaration of class note which contains all informations about a
 *          note + definitions of several constants and enums
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


#ifndef _NOTE_H
#define _NOTE_H

#include <QtCore/QVector>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "volume.h"
#include "panning.h"
#include "midi_time.h"
#include "serializing_object.h"

class detuningHelper;


enum Keys
{
	Key_C = 0,
	Key_CIS = 1, Key_DES = 1,
	Key_D = 2,
	Key_DIS = 3, Key_ES = 3,
	Key_E = 4, Key_FES = 4,
	Key_F = 5,
	Key_FIS = 6, Key_GES = 6,
	Key_G = 7,
	Key_GIS = 8, Key_AS = 8,
	Key_A = 9,
	Key_AIS = 10, Key_B = 10,
	Key_H = 11 
} ;


enum Octaves
{
	Octave_0,
	Octave_1,
	Octave_2,
	Octave_3,
	Octave_4, DefaultOctave = Octave_4,
	Octave_5,
	Octave_6,
	Octave_7,
	Octave_8,
	NumOctaves
} ;


const int WhiteKeysPerOctave = 7;
const int BlackKeysPerOctave = 5;
const int KeysPerOctave = WhiteKeysPerOctave + BlackKeysPerOctave;
const int NumKeys = NumOctaves * KeysPerOctave;
const int DefaultKey = DefaultOctave*KeysPerOctave + Key_A;

const float MaxDetuning = 4 * 12.0f;



class EXPORT note : public serializingObject
{
public:
	note( const midiTime & _length = 0,
		const midiTime & _pos = 0,
		int key = DefaultKey,
		volume _volume = DefaultVolume,
		panning _panning = DefaultPanning,
		detuningHelper * _detuning = NULL );
	note( const note & _note );
	virtual ~note();

	void setLength( const midiTime & _length );
	void setPos( const midiTime & _pos );
	void setKey( const int _key );
	void setVolume( const volume _volume = DefaultVolume );
	void setPanning( const panning _panning = DefaultPanning );
	void quantizeLength( const int _q_grid );
	void quantizePos( const int _q_grid );

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

	inline int key( void ) const
	{
		return( m_key );
	}

	inline volume getVolume( void ) const
	{
		return( m_volume );
	}

	inline panning getPanning( void ) const
	{
		return( m_panning );
	}

	static QString classNodeName( void )
	{
		return( "note" );
	}

	inline virtual QString nodeName( void ) const
	{
		return( classNodeName() );
	}

	static midiTime quantized( const midiTime & _m, const int _q_grid );

	detuningHelper * detuning( void ) const
	{
		return( m_detuning );
	}

	void editDetuningPattern( void );

	bool hasDetuningInfo( void );


protected:
	virtual void saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

/*	virtual void undoStep( journalEntry & _je );
	virtual void redoStep( journalEntry & _je );*/


private:
/*	enum Actions
	{
		ChangeKey,
		ChangeVolume,
		ChangePanning,
		ChangeLength,
		ChangePosition
	} ;*/


	int m_key;
	volume m_volume;
	panning m_panning;
	midiTime m_length;
	midiTime m_pos;
	detuningHelper * m_detuning;

	void createDetuning( void );

} ;


typedef QVector<note *> noteVector;


#endif

