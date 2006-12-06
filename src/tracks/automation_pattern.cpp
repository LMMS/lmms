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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#include "qt3support.h"

#ifdef QT4

#include <Qt/QtXml>

#else

#include <qdom.h>
#define value data

#endif


#include "automation_pattern.h"
#include "templates.h"
#include "automation_editor.h"




automationPattern::automationPattern( track * _track, levelObject * _object ) :
	journallingObject( _track->eng() ),
	m_track( _track ),
	m_object( _object ),
	m_update_first( TRUE )
{
	init();
}




automationPattern::automationPattern( engine * _engine,
						levelObject * _object ) :
	journallingObject( _engine ),
	m_track( NULL ),
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
		m_time_map[it.key()] = it.value();
	}

	init();
}




automationPattern::automationPattern( const automationPattern & _pat_to_copy,
						levelObject * _object ) :
	journallingObject( _pat_to_copy.m_track->eng() ),
	m_track( _pat_to_copy.m_track ),
	m_object( _object ),
	m_update_first( _pat_to_copy.m_update_first )
{
	for( timeMap::const_iterator it = _pat_to_copy.m_time_map.begin();
				it != _pat_to_copy.m_time_map.end(); ++it )
	{
		m_time_map[it.key()] = it.value();
	}

	init();
}




automationPattern::~automationPattern()
{
	if( m_track )
	{
		m_track->removeAutomationPattern( this );
	}

	if( eng()->getAutomationEditor()
		&& eng()->getAutomationEditor()->currentPattern() == this )
	{
		eng()->getAutomationEditor()->setCurrentPattern( NULL );
	}
}




void automationPattern::init( void )
{
	if( m_track )
	{
		m_track->addAutomationPattern( this );
	}
}




midiTime automationPattern::length( void ) const
{
	Sint32 max_length = 0;

	for( timeMap::const_iterator it = m_time_map.begin();
							it != m_time_map.end();
									++it )
	{
		max_length = tMax<Sint32>( max_length, -it.key() );
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

	m_time_map[-new_time] = _value;

	return( new_time );
}




void automationPattern::removeValue( const midiTime & _time )
{
	if( _time != 0 )
	{
		m_time_map.remove( -_time );
	}
}




void automationPattern::clear( void )
{
	m_time_map.clear();
	if( eng()->getAutomationEditor()->currentPattern() == this )
	{
		eng()->getAutomationEditor()->update();
	}
}




int automationPattern::valueAt( const midiTime & _time )
{
	return( m_time_map.lowerBound( -_time ).value() );
}




void automationPattern::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	for( timeMap::iterator it = m_time_map.begin(); it != m_time_map.end();
									++it )
	{
		QDomElement element = _doc.createElement( "time" );
		element.setAttribute( "pos", -it.key() );
		element.setAttribute( "value", m_object->levelToLabel(
								it.value() ) );
		_this.appendChild( element );
	}
}




void automationPattern::loadSettings( const QDomElement & _this )
{
	clear();

	for( QDomNode node = _this.firstChild(); !node.isNull();
						node = node.nextSibling() )
	{
		QDomElement element = node.toElement();
		if( element.isNull() || element.tagName() != "time" )
		{
			continue;
		}
		m_time_map[-element.attribute( "pos" ).toInt()]
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




const QString automationPattern::name( void )
{
	QString widget_name = dynamic_cast<QWidget *>( m_object )
							->accessibleName();
	if( m_track )
	{
		return( m_track->name() + " - " + widget_name );
	}
	else
	{
		return( widget_name );
	}
}




void automationPattern::processMidiTime( const midiTime & _time )
{
	if( _time >= 0 )
	{
		timeMap::iterator it = m_time_map.lowerBound( -_time );
		m_object->setLevel( it.value() );
	}
}


#undef value

#include "automation_pattern.moc"


#endif
