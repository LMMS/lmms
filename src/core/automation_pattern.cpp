/*
 * automation_pattern.cpp - implementation of class automationPattern which
 *                          holds dynamic values
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2006-2008 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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


#include <QtXml/QDomElement>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>

#include "automation_pattern.h"
#include "automation_pattern_view.h"
#include "automation_editor.h"
#include "automation_track.h"
#include "ProjectJournal.h"
#include "bb_track_container.h"
#include "song.h"



automationPattern::automationPattern( automationTrack * _auto_track ) :
	trackContentObject( _auto_track ),
	m_autoTrack( _auto_track ),
	m_objects(),
	m_hasAutomation( false )
{
	changeLength( midiTime( 1, 0 ) );
	m_timeMap[0] = 0;
}




automationPattern::automationPattern( const automationPattern & _pat_to_copy ) :
	trackContentObject( _pat_to_copy.m_autoTrack ),
	m_autoTrack( _pat_to_copy.m_autoTrack ),
	m_objects( _pat_to_copy.m_objects ),
	m_hasAutomation( _pat_to_copy.m_hasAutomation )
{
	for( timeMap::const_iterator it = _pat_to_copy.m_timeMap.begin();
				it != _pat_to_copy.m_timeMap.end(); ++it )
	{
		m_timeMap[it.key()] = it.value();
	}
}




automationPattern::~automationPattern()
{
	if( engine::getAutomationEditor() &&
		engine::getAutomationEditor()->currentPattern() == this )
	{
		engine::getAutomationEditor()->setCurrentPattern( NULL );
	}
}




void automationPattern::addObject( AutomatableModel * _obj, bool _search_dup )
{
	bool addIt = true;

	if( _search_dup )
	{
		for( objectVector::iterator it = m_objects.begin();
					it != m_objects.end(); ++it )
		{
			if( *it == _obj )
			{
				// Already exists
				// TODO: Maybe let the user know in some non-annoying way
				addIt = false;
				break;
			}
		}
	}

	if( addIt )
	{
		m_objects += _obj;
		// been empty before?
		if( m_objects.size() == 1 && !hasAutomation() )
		{
			// then initialize default-value
			putValue( 0, _obj->value<float>(), false );
		}
		connect( _obj, SIGNAL( destroyed( jo_id_t ) ),
				this, SLOT( objectDestroyed( jo_id_t ) ),
							Qt::DirectConnection );
	}
}




const AutomatableModel * automationPattern::firstObject() const
{
	AutomatableModel * m;
	if( !m_objects.isEmpty() && ( m = m_objects.first() ) != NULL )
	{
		return m;
	}

	static FloatModel _fm( 0, 0, 1, 0.001 );
	return &_fm;
}





//TODO: Improve this
midiTime automationPattern::length() const
{
	tick_t max_length = 0;

	for( timeMap::const_iterator it = m_timeMap.begin();
						it != m_timeMap.end(); ++it )
	{
		max_length = qMax<tick_t>( max_length, it.key() );
	}
	return midiTime( qMax( midiTime( max_length ).getTact() + 1, 1 ), 0 );
}




midiTime automationPattern::putValue( const midiTime & _time,
	const float _value, const bool _quant_pos )
{
	midiTime newTime = _quant_pos && engine::getAutomationEditor() ?
		note::quantized( _time, engine::getAutomationEditor()->quantization() )
		: _time;

	m_timeMap[newTime] = _value;

	// just one automation value?
	if( m_timeMap.size() == 1 )
	{
		m_hasAutomation = m_objects.isEmpty();	// usually false
		for( objectVector::iterator it = m_objects.begin();
						it != m_objects.end(); ++it )
		{
			// default value differs from current value?
			if( *it && _value != ( *it )->initValue<float>() )
			{
				// then enable automating this object
				m_hasAutomation = true;
			}
		}
	}
	else
	{
		// in all other cases assume we have automation
		m_hasAutomation = true;
	}

	// we need to maximize our length in case we're part of a hidden
	// automation track as the user can't resize this pattern
	if( getTrack() && getTrack()->type() == track::HiddenAutomationTrack )
	{
		changeLength( length() );
	}

	emit dataChanged();

	return newTime;
}




void automationPattern::removeValue( const midiTime & _time )
{
	if( _time != 0 )
	{
		m_timeMap.remove( _time );

		if( m_timeMap.size() == 1 )
		{
			const float val = m_timeMap[0];
			m_hasAutomation = false;
			for( objectVector::iterator it = m_objects.begin();
						it != m_objects.end(); )
			{
				if( *it )
				{
					( *it )->setValue( val );
					if( ( *it )->initValue<float>() != val )
					{
						m_hasAutomation = true;
					}
					++it;
				}
				else
				{
					it = m_objects.erase( it );
				}
			}
		}

		if( getTrack() &&
			getTrack()->type() == track::HiddenAutomationTrack )
		{
			changeLength( length() );
		}

		emit dataChanged();
	}
}




float automationPattern::valueAt( const midiTime & _time ) const
{
	if( m_timeMap.isEmpty() )
	{
		return 0;
	}
	timeMap::const_iterator v = m_timeMap.lowerBound( _time );
	// lowerBound returns next value with greater key, therefore we take
	// the previous element to get the current value
	return ( v != m_timeMap.begin() ) ? (v-1).value() : v.value();
}




void automationPattern::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "pos", startPosition() );
	_this.setAttribute( "len", trackContentObject::length() );
	_this.setAttribute( "name", name() );

	for( timeMap::const_iterator it = m_timeMap.begin();
						it != m_timeMap.end(); ++it )
	{
		QDomElement element = _doc.createElement( "time" );
		element.setAttribute( "pos", it.key() );
		element.setAttribute( "value", it.value() );
		_this.appendChild( element );
	}

	for( objectVector::const_iterator it = m_objects.begin();
						it != m_objects.end(); ++it )
	{
		if( *it )
		{
			QDomElement element = _doc.createElement( "object" );
			element.setAttribute( "id", ( *it )->id() );
			_this.appendChild( element );
		}
	}
}




void automationPattern::loadSettings( const QDomElement & _this )
{
	clear();

	movePosition( _this.attribute( "pos" ).toInt() );
	setName( _this.attribute( "name" ) );

	for( QDomNode node = _this.firstChild(); !node.isNull();
						node = node.nextSibling() )
	{
		QDomElement element = node.toElement();
		if( element.isNull()  )
		{
			continue;
		}
		if( element.tagName() == "time" )
		{
			m_timeMap[element.attribute( "pos" ).toInt()]
				= element.attribute( "value" ).toFloat();
		}
		else if( element.tagName() == "object" )
		{
			m_idsToResolve << element.attribute( "id" ).toInt();
		}
	}

	m_hasAutomation = m_timeMap.size() > 1;
	if( m_hasAutomation == false )
	{
		for( objectVector::iterator it = m_objects.begin();
						it != m_objects.end(); ++it )
		{
			if( *it )
			{
				( *it )->setValue( m_timeMap[0] );
			}
		}
	}
	int len = _this.attribute( "len" ).toInt();
	if( len <= 0 )
	{
		len = length();
	}
	changeLength( len );
}




const QString automationPattern::name() const
{
	if( !trackContentObject::name().isEmpty() )
	{
		return trackContentObject::name();
	}
	if( !m_objects.isEmpty() && m_objects.first() != NULL )
	{
		return m_objects.first()->fullDisplayName();
	}
	return tr( "Drag a control while pressing <Ctrl>" );
}




void automationPattern::processMidiTime( const midiTime & _time )
{
	if( _time >= 0 && m_hasAutomation )
	{
		const float val = valueAt( _time );
		for( objectVector::iterator it = m_objects.begin();
						it != m_objects.end(); ++it )
		{
			if( *it )
			{
				( *it )->setAutomatedValue( val );
			}

		}
	}
}





trackContentObjectView * automationPattern::createView( trackView * _tv )
{
	return new automationPatternView( this, _tv );
}





bool automationPattern::isAutomated( const AutomatableModel * _m )
{
	trackContainer::trackList l = engine::getSong()->tracks() +
				engine::getBBTrackContainer()->tracks();
	l += engine::getSong()->globalAutomationTrack();
	for( trackContainer::trackList::const_iterator it = l.begin();
							it != l.end(); ++it )
	{
		if( ( *it )->type() == track::AutomationTrack ||
			( *it )->type() == track::HiddenAutomationTrack )
		{
			const track::tcoVector & v = ( *it )->getTCOs();
			for( track::tcoVector::const_iterator j = v.begin();
							j != v.end(); ++j )
			{
				const automationPattern * a =
					dynamic_cast<const
						automationPattern *>( *j );
				if( a && a->m_hasAutomation )
				{
	for( objectVector::const_iterator k = a->m_objects.begin();
					k != a->m_objects.end(); ++k )
	{
		if( *k == _m )
		{
			return true;
		}
	}
				}
			}
		}
	}
	return false;
}





automationPattern * automationPattern::globalAutomationPattern(
							AutomatableModel * _m )
{
	automationTrack * t = engine::getSong()->globalAutomationTrack();
	track::tcoVector v = t->getTCOs();
	for( track::tcoVector::const_iterator j = v.begin(); j != v.end(); ++j )
	{
		automationPattern * a = dynamic_cast<automationPattern *>( *j );
		if( a )
		{
			for( objectVector::const_iterator k =
							a->m_objects.begin();
						k != a->m_objects.end(); ++k )
			{
				if( *k == _m )
				{
					return a;
				}
			}
		}
	}

	automationPattern * a = new automationPattern( t );
	a->addObject( _m, false );
	return a;
}




void automationPattern::resolveAllIDs()
{
	trackContainer::trackList l = engine::getSong()->tracks() +
				engine::getBBTrackContainer()->tracks();
	l += engine::getSong()->globalAutomationTrack();
	for( trackContainer::trackList::iterator it = l.begin();
							it != l.end(); ++it )
	{
		if( ( *it )->type() == track::AutomationTrack ||
			 ( *it )->type() == track::HiddenAutomationTrack )
		{
			track::tcoVector v = ( *it )->getTCOs();
			for( track::tcoVector::iterator j = v.begin();
							j != v.end(); ++j )
			{
				automationPattern * a =
					dynamic_cast<automationPattern *>( *j );
				if( a )
				{
	for( QVector<jo_id_t>::Iterator k = a->m_idsToResolve.begin();
					k != a->m_idsToResolve.end(); ++k )
	{
		JournallingObject * o = engine::projectJournal()->
										journallingObject( *k );
		if( o && dynamic_cast<AutomatableModel *>( o ) )
		{
			a->addObject( dynamic_cast<AutomatableModel *>( o ), false );
		}
	}
	a->m_idsToResolve.clear();
	a->dataChanged();
				}
			}
		}
	}
}




void automationPattern::clear()
{
	const float val = firstObject()->value<float>();
	m_timeMap.clear();
	putValue( 0, val );

	if( engine::getAutomationEditor() &&
		engine::getAutomationEditor()->currentPattern() == this )
	{
		engine::getAutomationEditor()->update();
	}
}




void automationPattern::openInAutomationEditor()
{
	engine::getAutomationEditor()->setCurrentPattern( this );
	engine::getAutomationEditor()->parentWidget()->show();
	engine::getAutomationEditor()->setFocus();
}




void automationPattern::objectDestroyed( jo_id_t _id )
{
	// TODO: distict between temporary removal (e.g. LADSPA controls
	// when switching samplerate) and real deletions because in the latter
	// case we had to remove ourselves if we're the global automation
	// pattern of the destroyed object
	m_idsToResolve += _id;
}




#include "moc_automation_pattern.cxx"

/* vim: set tw=0 noexpandtab: */
