#ifndef SINGLE_SOURCE_COMPILE

/*
 * note.cpp - implementation of class note
 *
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "qt3support.h"

#ifdef QT4

#include <Qt/QtXml>

#else

#include <qdom.h>

#endif

#include <math.h>

#include "note.h"
#include "automatable_object_templates.h"
#include "detuning_helper.h"
#include "templates.h"


const float note::MAX_DETUNING = 4 * 12.0f;




note::note( const midiTime & _length, const midiTime & _pos,
		tones _tone, octaves _octave, volume _volume,
				panning _panning, detuningHelper * _detuning ) :
	m_tone( C ),
	m_octave( DEFAULT_OCTAVE ),
	m_volume( DEFAULT_VOLUME ),
	m_panning( DEFAULT_PANNING ),
	m_length( _length ),
	m_pos( _pos )
{
	//saveJournallingState( FALSE );
	setJournalling( FALSE );

	setTone( _tone );
	setOctave( _octave );
	setVolume( _volume );
	setPanning( _panning );

	if( _detuning )
	{
		m_detuning = sharedObject::ref( _detuning );
	}
	else
	{
		createDetuning();
	}
	//restoreJournallingState();
}




note::note( const note & _note ) :
	journallingObject( _note ),
	m_tone( _note.m_tone ),
	m_octave( _note.m_octave ),
	m_volume( _note.m_volume ),
	m_panning( _note.m_panning ),
	m_length( _note.m_length ),
	m_pos( _note.m_pos )
{
	m_detuning = sharedObject::ref( _note.m_detuning );
}




note::~note()
{
	sharedObject::unref( m_detuning );
}




void note::setLength( const midiTime & _length )
{
	addJournalEntry( journalEntry( CHANGE_LENGTH, m_length - _length ) );
	m_length = _length;
}




void note::setPos( const midiTime & _pos )
{
	addJournalEntry( journalEntry( CHANGE_POSITION, m_pos - _pos ) );
	m_pos = _pos;
}




void note::setTone( const tones _tone )
{
	const tones t = tLimit( _tone, C, H );
	addJournalEntry( journalEntry( CHANGE_KEY, (int) m_tone - t ) );
	m_tone = t;
}




void note::setOctave( const octaves _octave )
{
	const octaves o = tLimit( _octave, MIN_OCTAVE, MAX_OCTAVE );
	addJournalEntry( journalEntry( CHANGE_KEY, NOTES_PER_OCTAVE *
						( (int) m_octave - o ) ) );
	m_octave = o;
}




void note::setKey( const int _key )
{
	const int k = key();
	saveJournallingState( FALSE );
	setTone( static_cast<tones>( _key % NOTES_PER_OCTAVE ) );
	setOctave( static_cast<octaves>( _key / NOTES_PER_OCTAVE ) );
	restoreJournallingState();
	addJournalEntry( journalEntry( CHANGE_KEY, k - key() ) );
}




void note::setVolume( const volume _volume )
{
	const volume v = tMin( _volume, MAX_VOLUME );
	addJournalEntry( journalEntry( CHANGE_VOLUME, (int) m_volume - v ) );
	m_volume = v;
}




void note::setPanning( const panning _panning )
{
	const panning p = tLimit( _panning, PANNING_LEFT, PANNING_RIGHT );
	addJournalEntry( journalEntry( CHANGE_PANNING, (int) m_panning - p ) );
	m_panning = p;
}




midiTime note::quantized( const midiTime & _m, const int _q_grid,
							const float _align )
{
	const float p = ( (float) _m / _q_grid );
	if( p - floorf( p ) < _align )
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
	_this.setAttribute( "tone", m_tone );
	_this.setAttribute( "oct", m_octave );
	_this.setAttribute( "vol", m_volume );
	_this.setAttribute( "pan", m_panning );
	_this.setAttribute( "len", m_length );
	_this.setAttribute( "pos", m_pos );
	if( m_length > 0 )
	{
		m_detuning->saveSettings( _doc, _this, "detuning" );
	}
}




void note::loadSettings( const QDomElement & _this )
{
	m_tone = static_cast<tones>( _this.attribute( "tone" ).toInt() );
	m_octave = static_cast<octaves>( _this.attribute( "oct" ).toInt() );
	m_volume = _this.attribute( "vol" ).toInt();
	m_panning = _this.attribute( "pan" ).toInt();
	m_length = _this.attribute( "len" ).toInt();
	m_pos = _this.attribute( "pos" ).toInt();
	m_detuning->loadSettings( _this, "detuning" );
}




void note::undoStep( journalEntry & _je )
{
	saveJournallingState( FALSE );
	switch( static_cast<actions>( _je.actionID() ) )
	{
		case CHANGE_KEY:
			setKey( key() - _je.data().toInt() );
			break;

		case CHANGE_VOLUME:
			setVolume( getVolume() - _je.data().toInt() );
			break;

		case CHANGE_PANNING:
			setVolume( getPanning() - _je.data().toInt() );
			break;

		case CHANGE_LENGTH:
			setLength( length() - _je.data().toInt() );
			break;

		case CHANGE_POSITION:
			setPos( pos() - _je.data().toInt() );
			break;
	}
	restoreJournallingState();
}




void note::redoStep( journalEntry & _je )
{
	journalEntry je( _je.actionID(), -_je.data().toInt() );
	undoStep( je );
}




void note::editDetuningPattern( void )
{
	m_detuning->getAutomationPattern()->openInAutomationEditor();
}




void note::createDetuning( void )
{
	m_detuning = new detuningHelper;
	m_detuning->initAutomationPattern();
	m_detuning->setRange( -MAX_DETUNING, MAX_DETUNING, 0.1f );
}




bool note::hasDetuningInfo( void )
{
	automationPattern::timeMap map =
			m_detuning->getAutomationPattern()->getTimeMap();
	return( map.size() > 1 || map[0] != 0 );
}




#endif
