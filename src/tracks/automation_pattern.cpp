#ifndef SINGLE_SOURCE_COMPILE

/*
 * automation_pattern.cpp - implementation of class automationPattern which
 *                          holds dynamic values
 *
 * Copyright (c) 2006 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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


#include "automation_pattern.h"
#include "templates.h"
#include "automation_editor.h"




automationPattern::automationPattern ( track * _track, levelObject * _object ) :
	journallingObject( _track->eng() ),
	m_track( _track ),
	m_object( _object ),
	m_update_first( TRUE )
{
	init();
}




automationPattern::automationPattern( const automationPattern & _pat_to_copy ) :
	journallingObject( _pat_to_copy.m_track->eng() ),
	m_track( _pat_to_copy.m_track ),
	m_object( _pat_to_copy.m_object ),
	m_update_first( _pat_to_copy.m_update_first )
{
	for( timeMap::const_iterator it = _pat_to_copy.m_time_map.begin();
				it != _pat_to_copy.m_time_map.end(); ++it )
	{
		m_time_map[it.key()] = it.data();
	}

	init();
}




automationPattern::~automationPattern()
{
	m_track->removeAutomationPattern( this );

	if( eng()->getAutomationEditor()->currentPattern() == this )
	{
		eng()->getAutomationEditor()->setCurrentPattern( NULL );
	}

	m_time_map.clear();
}




void automationPattern::init( void )
{
	m_track->addAutomationPattern( this );
}




midiTime automationPattern::length( void ) const
{
	Sint32 max_length = 0;

	for( timeMap::const_iterator it = m_time_map.begin();
							it != m_time_map.end();
									++it )
	{
		max_length = tMax<Sint32>( max_length, it.key() );
	}
	if( max_length % 64 == 0 )
	{
		return( midiTime( tMax<Sint32>( max_length, 64 ) ) );
	}
	return( midiTime( tMax( midiTime( max_length ).getTact() + 1, 1 ),
									0 ) );
}




midiTime automationPattern::putValue( const midiTime & _time, const int _value,
							const bool _quant_pos )
{
	midiTime new_time = _quant_pos ?
		note::quantized( _time,
				eng()->getAutomationEditor()->quantization() ) :
		_time;

	m_time_map[new_time] = _value;

	return( new_time );
}




void automationPattern::removeValue( const midiTime & _time )
{
	m_time_map.remove( _time );
}




void automationPattern::clearValues( void )
{
	m_time_map.clear();
	if( eng()->getAutomationEditor()->currentPattern() == this )
	{
		eng()->getAutomationEditor()->update();
	}
}




int automationPattern::valueAt( const midiTime & _time )
{
	if( m_time_map.contains( _time ) )
	{
		return( m_time_map[_time] );
	}
	//TODO: Return a better value!!
	return( 0 );
}




void automationPattern::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	for( timeMap::iterator it = m_time_map.begin(); it != m_time_map.end();
									++it )
	{
		QDomElement element = _doc.createElement( "time" );
		element.setAttribute( "pos", static_cast<Sint32>( it.key() ) );
		element.setAttribute( "value", m_object->levelToLabel(
								it.data() ) );
		_this.appendChild( element );
	}
}




void automationPattern::loadSettings( const QDomElement & _this )
{
	clearValues();

	for( QDomNode node = _this.firstChild(); !node.isNull();
						node = node.nextSibling() )
	{
		QDomElement element = node.toElement();
		if( element.isNull() || element.tagName() != "time" )
		{
			continue;
		}
		m_time_map[midiTime( element.attribute( "pos" ).toInt() )]
				= m_object->labelToLevel(
						element.attribute( "value" ) );
	}
}




void automationPattern::openInAutomationEditor( void )
{
	eng()->getAutomationEditor()->setCurrentPattern( this );
	eng()->getAutomationEditor()->show();
	eng()->getAutomationEditor()->setFocus();
}




void automationPattern::clear( void )
{
	clearValues();
}




const QString automationPattern::name( void )
{
	return( m_track->name() + " - " + dynamic_cast<QWidget *>( m_object )
							->accessibleName() );
}




void automationPattern::processMidiTime( const midiTime & _time )
{
	timeMap::iterator it = m_time_map.find( _time );
	if( it != m_time_map.end() )
	{
		m_object->setLevel( it.data() );
	}
}




#include "automation_pattern.moc"


#endif
