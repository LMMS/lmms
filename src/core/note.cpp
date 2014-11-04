/*
 * note.cpp - implementation of class note
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


#include <QtXml/QDomElement>

#include <math.h>

#include "note.h"
#include "DetuningHelper.h"
#include "templates.h"





note::note( const MidiTime & _length, const MidiTime & _pos,
		int _key, volume_t _volume, panning_t _panning,
						DetuningHelper * _detuning ) :
	m_selected( false ),
	m_oldKey( qBound( 0, _key, NumKeys ) ),
	m_oldPos( _pos ),
	m_oldLength( _length ),
	m_isPlaying( false ),
	m_key( qBound( 0, _key, NumKeys ) ),
	m_volume( qBound( MinVolume, _volume, MaxVolume ) ),
	m_panning( qBound( PanningLeft, _panning, PanningRight ) ),
	m_length( _length ),
	m_pos( _pos ),
	m_detuning( NULL )
{
	if( _detuning )
	{
		m_detuning = sharedObject::ref( _detuning );
	}
	else
	{
		createDetuning();
	}
}




note::note( const note & _note ) :
	SerializingObject( _note ),
	m_selected( _note.m_selected ),
	m_oldKey( _note.m_oldKey ),
	m_oldPos( _note.m_oldPos ),
	m_oldLength( _note.m_oldLength ),	
	m_isPlaying( _note.m_isPlaying ),
	m_key( _note.m_key),
	m_volume( _note.m_volume ),
	m_panning( _note.m_panning ),
	m_length( _note.m_length ),
	m_pos( _note.m_pos ),
	m_detuning( NULL )
{
	if( _note.m_detuning )
	{
		m_detuning = sharedObject::ref( _note.m_detuning );
	}
}




note::~note()
{
	if( m_detuning )
	{
		sharedObject::unref( m_detuning );
	}
}




void note::setLength( const MidiTime & _length )
{
	m_length = _length;
}




void note::setPos( const MidiTime & _pos )
{
	m_pos = _pos;
}




void note::setKey( const int _key )
{
	const int k = qBound( 0, _key, NumKeys );
	m_key = k;
}




void note::setVolume( volume_t _volume )
{
	const volume_t v = qBound( MinVolume, _volume, MaxVolume );
	m_volume = v;
}




void note::setPanning( panning_t _panning )
{
	const panning_t p = qBound( PanningLeft, _panning, PanningRight );
	m_panning = p;
}




MidiTime note::quantized( const MidiTime & _m, const int _q_grid )
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




void note::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "key", m_key );
	_this.setAttribute( "vol", m_volume );
	_this.setAttribute( "pan", m_panning );
	_this.setAttribute( "len", m_length );
	_this.setAttribute( "pos", m_pos );

	if( m_detuning && m_length )
	{
		m_detuning->saveSettings( _doc, _this );
	}
}




void note::loadSettings( const QDomElement & _this )
{
	const int oldKey = _this.attribute( "tone" ).toInt() + _this.attribute( "oct" ).toInt() * KeysPerOctave;
	m_key = qMax( oldKey, _this.attribute( "key" ).toInt() );
	m_volume = _this.attribute( "vol" ).toInt();
	m_panning = _this.attribute( "pan" ).toInt();
	m_length = _this.attribute( "len" ).toInt();
	m_pos = _this.attribute( "pos" ).toInt();

	if( _this.hasChildNodes() )
	{
		createDetuning();
		m_detuning->loadSettings( _this );
	}
}




void note::editDetuningPattern()
{
	createDetuning();
	m_detuning->automationPattern()->openInAutomationEditor();
}




void note::createDetuning()
{
	if( m_detuning == NULL )
	{
		m_detuning = new DetuningHelper;
		(void) m_detuning->automationPattern();
		m_detuning->setRange( -MaxDetuning, MaxDetuning, 0.5f );
		m_detuning->automationPattern()->setProgressionType( AutomationPattern::LinearProgression );
	}
}




bool note::hasDetuningInfo() const
{
	return m_detuning && m_detuning->hasAutomation();
}




