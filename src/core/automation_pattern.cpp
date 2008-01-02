#ifndef SINGLE_SOURCE_COMPILE

/*
 * automation_pattern.cpp - implementation of class automationPattern which
 *                          holds dynamic values
 *
 * Copyright (c) 2006-2007 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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


#include <Qt/QtXml>


#include "automation_pattern.h"
#include "automation_editor.h"
#include "engine.h"
#include "level_object.h"
#include "note.h"
#include "templates.h"
#include "track.h"




automationPattern::automationPattern( track * _track, levelObject * _object ) :
	m_track( _track ),
	m_object( _object ),
	m_update_first( TRUE ),
	m_dynamic( FALSE )
{
}




automationPattern::automationPattern( const automationPattern & _pat_to_copy ) :
	QObject(),
	journallingObject(),
	m_track( _pat_to_copy.m_track ),
	m_object( _pat_to_copy.m_object ),
	m_update_first( _pat_to_copy.m_update_first ),
	m_dynamic( _pat_to_copy.m_dynamic )
{
	for( timeMap::const_iterator it = _pat_to_copy.m_time_map.begin();
				it != _pat_to_copy.m_time_map.end(); ++it )
	{
		m_time_map[it.key()] = it.value();
	}

	if( m_dynamic && m_track )
	{
		m_track->addAutomationPattern( this );
	}
}




automationPattern::automationPattern( const automationPattern & _pat_to_copy,
						levelObject * _object ) :
	m_track( _pat_to_copy.m_track ),
	m_object( _object ),
	m_update_first( _pat_to_copy.m_update_first ),
	m_dynamic( _pat_to_copy.m_dynamic )
{
	for( timeMap::const_iterator it = _pat_to_copy.m_time_map.begin();
				it != _pat_to_copy.m_time_map.end(); ++it )
	{
		m_time_map[it.key()] = it.value();
	}

	if( m_dynamic && m_track )
	{
		m_track->addAutomationPattern( this );
	}
}




automationPattern::~automationPattern()
{
	if( m_dynamic && m_track )
	{
		m_track->removeAutomationPattern( this );
	}

	if( engine::getAutomationEditor()
		&& engine::getAutomationEditor()->currentPattern() == this )
	{
		engine::getAutomationEditor()->setCurrentPattern( NULL );
	}
}




//TODO: Improve this
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
			engine::getAutomationEditor()->quantization() ) :
		_time;

	m_time_map[-new_time] = _value;

	if( !m_dynamic && new_time != 0 )
	{
		m_dynamic = TRUE;
		if( m_track )
		{
			m_track->addAutomationPattern( this );
		}
	}

	return( new_time );
}




void automationPattern::removeValue( const midiTime & _time )
{
	if( _time != 0 )
	{
		m_time_map.remove( -_time );

		if( m_time_map.size() == 1 )
		{
			m_dynamic = FALSE;
			m_object->setLevel( m_time_map[0] );
			if( m_track )
			{
				m_track->removeAutomationPattern( this );
			}
		}
	}
}




void automationPattern::clear( void )
{
	m_time_map.clear();
	if( engine::getAutomationEditor()->currentPattern() == this )
	{
		engine::getAutomationEditor()->update();
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

	if( !m_dynamic )
	{
		m_dynamic = TRUE;
		if( m_track )
		{
			m_track->addAutomationPattern( this );
		}
	}
}




void automationPattern::openInAutomationEditor( void )
{
	engine::getAutomationEditor()->setCurrentPattern( this );
	engine::getAutomationEditor()->parentWidget()->show();
	engine::getAutomationEditor()->setFocus();
}




const QString automationPattern::name( void )
{
	if( m_track )
	{
		QString widget_name = m_object->displayName();
/* dynamic_cast<QWidget *>( m_object )->accessibleName();*/
		return( m_track->name() + " - " + widget_name );
	}
	else
	{
		return( m_object->displayName() );
	}
}




void automationPattern::processMidiTime( const midiTime & _time )
{
	if( _time >= 0 )
	{
		m_object->setLevel( m_time_map.lowerBound( -_time ).value() );
	}
}




#include "automation_pattern.moc"


#endif
