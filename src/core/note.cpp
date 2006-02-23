#ifndef SINGLE_SOURCE_COMPILE

/*
 * note.cpp - implementation of class note
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


#include "qt3support.h"

#ifdef QT4

#include <Qt/QtXml>

#else

#include <qdom.h>

#endif

#include <math.h>

#include "debug.h"
#include "note.h"
#include "templates.h"



note::note( const midiTime & _length, const midiTime & _pos, tones _tone,
			octaves _octave, volume _volume, panning _panning ) :
	m_tone( C ),
	m_octave( DEFAULT_OCTAVE ),
	m_volume( DEFAULT_VOLUME ),
	m_panning( DEFAULT_PANNING ),
	m_length( _length ),
	m_pos( _pos )
{
	setTone( _tone );
	setOctave( _octave );
	setVolume( _volume );
	setPanning( _panning );
}




note::~note()
{
}




void note::setLength( const midiTime & _length )
{
	m_length = _length;
}




void note::setPos( const midiTime & _pos )
{
	m_pos = _pos;
}




void note::setTone( const tones _tone )
{
	if( _tone >= C && _tone <= H )
	{
		m_tone = _tone;
	}
	else
	{
		m_tone = tLimit( _tone, C, H );
#ifdef LMMS_DEBUG
		printf ( "Tone out of range (note::setTone)\n" );
#endif
	}
}




void note::setOctave( const octaves _octave )
{
	if( _octave >= MIN_OCTAVE && _octave <= MAX_OCTAVE )
	{
		m_octave = _octave;
	}
	else
	{
		m_octave = tLimit( _octave, MIN_OCTAVE, MAX_OCTAVE );
#ifdef LMMS_DEBUG
		printf( "Octave out of range (note::setOctave)\n" );
#endif
	}
}




void note::setKey( const int _key )
{
	setTone( static_cast<tones>( _key % NOTES_PER_OCTAVE ) );
	setOctave( static_cast<octaves>( _key / NOTES_PER_OCTAVE ) );
}




void note::setVolume( const volume _volume )
{
	if( _volume <= MAX_VOLUME )
	{
		m_volume = _volume;
	}
	else
	{
		m_volume = tMin( _volume, MAX_VOLUME );
#ifdef LMMS_DEBUG
		printf( "Volume out of range (note::setVolume)\n" );
#endif
	}
}




void note::setPanning( const panning _panning )
{
	if( _panning >= PANNING_LEFT && _panning <= PANNING_RIGHT )
	{
		m_panning = _panning;
	}
#ifdef LMMS_DEBUG
	else
	{
		printf( "Paning out of range (note::setPanning)\n" );
	}
#endif
}




inline midiTime note::quantized( const midiTime & _m, const int _q_grid )
{
	float p = ( (float) _m / _q_grid );
	if( p - floorf( p ) < 0.5f )
	{
		return( static_cast<int>( p ) * _q_grid );
	}
	return( static_cast<int>( p + 1 ) * _q_grid );
}




void note::quantizeLength( const int _q_grid )
{
	setLength( quantized( length(), _q_grid ) );
	if( length() == 0 )
	{
		setLength( _q_grid );
	}
}




void note::quantizePos( const int _q_grid )
{
	setPos( quantized( pos(), _q_grid ) );
}




void note::saveSettings( QDomDocument & _doc, QDomElement & _parent )
{
	QDomElement note_de = _doc.createElement( "note" );
	note_de.setAttribute( "tone", m_tone );
	note_de.setAttribute( "oct", m_octave );
	note_de.setAttribute( "vol", m_volume );
	note_de.setAttribute( "pan", m_panning );
	note_de.setAttribute( "len", m_length );
	note_de.setAttribute( "pos", m_pos );
	_parent.appendChild( note_de );
}




void note::loadSettings( const QDomElement & _this )
{
	m_tone = static_cast<tones>( _this.attribute( "tone" ).toInt() );
	m_octave = static_cast<octaves>( _this.attribute( "oct" ).toInt() );
	m_volume = _this.attribute( "vol" ).toInt();
	m_panning = _this.attribute( "pan" ).toInt();
	m_length = _this.attribute( "len" ).toInt();
	m_pos = _this.attribute( "pos" ).toInt();
}





#endif
