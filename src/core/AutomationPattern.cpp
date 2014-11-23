/*
 * AutomationPattern.cpp - implementation of class AutomationPattern which
 *                         holds dynamic values
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2006-2008 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>

#include "AutomationPattern.h"
#include "AutomationPatternView.h"
#include "AutomationEditor.h"
#include "AutomationTrack.h"
#include "ProjectJournal.h"
#include "bb_track_container.h"
#include "song.h"
#include "text_float.h"
#include "embed.h"


const float AutomationPattern::DEFAULT_MIN_VALUE = 0;
const float AutomationPattern::DEFAULT_MAX_VALUE = 1;


AutomationPattern::AutomationPattern( AutomationTrack * _auto_track ) :
	trackContentObject( _auto_track ),
	m_autoTrack( _auto_track ),
	m_objects(),
	m_tension( 1.0 ),
	m_progressionType( DiscreteProgression ),
	m_dragging( false ),
	m_isRecording( false ),
	m_lastRecordedValue( 0 )
{
	changeLength( MidiTime( 1, 0 ) );
}




AutomationPattern::AutomationPattern( const AutomationPattern & _pat_to_copy ) :
	trackContentObject( _pat_to_copy.m_autoTrack ),
	m_autoTrack( _pat_to_copy.m_autoTrack ),
	m_objects( _pat_to_copy.m_objects ),
	m_tension( _pat_to_copy.m_tension ),
	m_progressionType( _pat_to_copy.m_progressionType )
{
	for( timeMap::const_iterator it = _pat_to_copy.m_timeMap.begin();
				it != _pat_to_copy.m_timeMap.end(); ++it )
	{
		m_timeMap[it.key()] = it.value();
		m_tangents[it.key()] = _pat_to_copy.m_tangents[it.key()];
	}
}




AutomationPattern::~AutomationPattern()
{
	if( engine::automationEditor() &&
		engine::automationEditor()->currentPattern() == this )
	{
		engine::automationEditor()->setCurrentPattern( NULL );
	}
}




void AutomationPattern::addObject( AutomatableModel * _obj, bool _search_dup )
{
	if( _search_dup )
	{
		for( objectVector::iterator it = m_objects.begin();
					it != m_objects.end(); ++it )
		{
			if( *it == _obj )
			{
				textFloat::displayMessage( _obj->displayName(), tr( "Model is already connected "
												"to this pattern." ), embed::getIconPixmap( "automation" ), 2000 );
				return;
			}
		}
	}

	// the automation track is unconnected and there is nothing in the track
	if( m_objects.isEmpty() && hasAutomation() == false )
	{
		// then initialize first value
		putValue( MidiTime(0), _obj->inverseScaledValue( _obj->value<float>() ), false );
	}

	m_objects += _obj;

	connect( _obj, SIGNAL( destroyed( jo_id_t ) ),
			this, SLOT( objectDestroyed( jo_id_t ) ),
						Qt::DirectConnection );

	emit dataChanged();

}




void AutomationPattern::setProgressionType(
					ProgressionTypes _new_progression_type )
{
	if ( _new_progression_type == DiscreteProgression ||
		_new_progression_type == LinearProgression ||
		_new_progression_type == CubicHermiteProgression )
	{
		m_progressionType = _new_progression_type;
		emit dataChanged();
	}
}




void AutomationPattern::setTension( QString _new_tension )
{
	bool ok;
	float nt = _new_tension.toFloat( & ok );

	if( ok && nt > -0.01 && nt < 1.01 )
	{
		m_tension = _new_tension.toFloat();
	}
}




const AutomatableModel * AutomationPattern::firstObject() const
{
	AutomatableModel * m;
	if( !m_objects.isEmpty() && ( m = m_objects.first() ) != NULL )
	{
		return m;
	}

	static FloatModel _fm( 0, DEFAULT_MIN_VALUE, DEFAULT_MAX_VALUE, 0.001 );
	return &_fm;
}





MidiTime AutomationPattern::length() const
{
	if( m_timeMap.isEmpty() ) return 0;
	timeMap::const_iterator it = m_timeMap.end();	
	return MidiTime( qMax( MidiTime( (it-1).key() ).getTact() + 1, 1 ), 0 );
}




MidiTime AutomationPattern::putValue( const MidiTime & _time,
							const float _value,
							const bool _quant_pos )
{
	cleanObjects();

	MidiTime newTime = _quant_pos && engine::automationEditor() ?
		note::quantized( _time,
			engine::automationEditor()->quantization() ) :
		_time;

	m_timeMap[newTime] = _value;
	timeMap::const_iterator it = m_timeMap.find( newTime );
	if( it != m_timeMap.begin() )
	{
		it--;
	}
	generateTangents(it, 3);

	// we need to maximize our length in case we're part of a hidden
	// automation track as the user can't resize this pattern
	if( getTrack() && getTrack()->type() == track::HiddenAutomationTrack )
	{
		changeLength( length() );
	}

	emit dataChanged();

	return newTime;
}




void AutomationPattern::removeValue( const MidiTime & _time,
									 const bool _quant_pos )
{
	cleanObjects();

	MidiTime newTime = _quant_pos && engine::automationEditor() ?
		note::quantized( _time,
			engine::automationEditor()->quantization() ) :
		_time;

	m_timeMap.remove( newTime );
	m_tangents.remove( newTime );
	timeMap::const_iterator it = m_timeMap.lowerBound( newTime );
	if( it != m_timeMap.begin() )
	{
		it--;
	}
	generateTangents(it, 3);

	if( getTrack() &&
		getTrack()->type() == track::HiddenAutomationTrack )
	{
		changeLength( length() );
	}

	emit dataChanged();
}




/**
 * @brief Set the position of the point that is being draged.
 *        Calling this function will also automatically set m_dragging to true,
 *        which applyDragValue() have to be called to m_dragging.
 * @param the time(x position) of the point being dragged
 * @param the value(y position) of the point being dragged
 * @param true to snip x position
 * @return
 */
MidiTime AutomationPattern::setDragValue( const MidiTime & _time, const float _value,
					   const bool _quant_pos )
{
	if( m_dragging == false )
	{
		MidiTime newTime = _quant_pos && engine::automationEditor() ?
			note::quantized( _time,
				engine::automationEditor()->quantization() ) :
			_time;
		this->removeValue( newTime );
		m_oldTimeMap = m_timeMap;
		m_dragging = true;
	}

	//Restore to the state before it the point were being dragged
	m_timeMap = m_oldTimeMap;

	for( timeMap::const_iterator it = m_timeMap.begin(); it != m_timeMap.end(); it++ )
	{
		generateTangents(it, 3);
	}

	return this->putValue( _time, _value, _quant_pos );

}




/**
 * @brief After the point is dragged, this function is called to apply the change.
 */
void AutomationPattern::applyDragValue()
{
	m_dragging = false;
}




float AutomationPattern::valueAt( const MidiTime & _time ) const
{
	if( m_timeMap.isEmpty() )
	{
		return 0;
	}

	if( m_timeMap.contains( _time ) )
	{
		return m_timeMap[_time];
	}

	// lowerBound returns next value with greater key, therefore we take
	// the previous element to get the current value
	timeMap::ConstIterator v = m_timeMap.lowerBound( _time );

	if( v == m_timeMap.begin() )
	{
		return 0;
	}
	if( v == m_timeMap.end() )
	{
		return (v-1).value();
	}

	return valueAt( v-1, _time - (v-1).key() );
}




float AutomationPattern::valueAt( timeMap::const_iterator v, int offset ) const
{
	if( m_progressionType == DiscreteProgression || v == m_timeMap.end() )
	{
		return v.value();
	}
	else if( m_progressionType == LinearProgression )
	{
		float slope = ((v+1).value() - v.value()) /
							((v+1).key() - v.key());
		return v.value() + offset * slope;
	}
	else /* CubicHermiteProgression */
	{
		// Implements a Cubic Hermite spline as explained at:
		// http://en.wikipedia.org/wiki/Cubic_Hermite_spline#Unit_interval_.280.2C_1.29
		//
		// Note that we are not interpolating a 2 dimensional point over
		// time as the article describes.  We are interpolating a single
		// value: y.  To make this work we map the values of x that this
		// segment spans to values of t for t = 0.0 -> 1.0 and scale the
		// tangents _m1 and _m2
		int numValues = ((v+1).key() - v.key());
		float t = (float) offset / (float) numValues;
		float m1 = (m_tangents[v.key()]) * numValues * m_tension;
		float m2 = (m_tangents[(v+1).key()]) * numValues * m_tension;

		return ( 2*pow(t,3) - 3*pow(t,2) + 1 ) * v.value()
				+ ( pow(t,3) - 2*pow(t,2) + t) * m1
				+ ( -2*pow(t,3) + 3*pow(t,2) ) * (v+1).value()
				+ ( pow(t,3) - pow(t,2) ) * m2;
	}
}




float *AutomationPattern::valuesAfter( const MidiTime & _time ) const
{
	timeMap::ConstIterator v = m_timeMap.lowerBound( _time );
	if( v == m_timeMap.end() || (v+1) == m_timeMap.end() )
	{
		return NULL;
	}

	int numValues = (v+1).key() - v.key();
	float *ret = new float[numValues];

	for( int i = 0; i < numValues; i++ )
	{
		ret[i] = valueAt( v, i );
	}

	return ret;
}




void AutomationPattern::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "pos", startPosition() );
	_this.setAttribute( "len", trackContentObject::length() );
	_this.setAttribute( "name", name() );
	_this.setAttribute( "prog", QString::number( progressionType() ) );
	_this.setAttribute( "tens", QString::number( getTension() ) );

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




void AutomationPattern::loadSettings( const QDomElement & _this )
{
	clear();

	movePosition( _this.attribute( "pos" ).toInt() );
	setName( _this.attribute( "name" ) );
	setProgressionType( static_cast<ProgressionTypes>( _this.attribute(
							"prog" ).toInt() ) );
	setTension( _this.attribute( "tens" ) );

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

	int len = _this.attribute( "len" ).toInt();
	if( len <= 0 )
	{
		len = length();
	}
	changeLength( len );
	generateTangents();
}




const QString AutomationPattern::name() const
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




void AutomationPattern::processMidiTime( const MidiTime & time )
{
	if( ! isRecording() )
	{
		if( time >= 0 && hasAutomation() )
		{
			const float val = valueAt( time );
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
	else
	{
		if( time >= 0 && ! m_objects.isEmpty() )
		{
			const float value = static_cast<float>( firstObject()->value<float>() );
			if( value != m_lastRecordedValue ) 
			{
				putValue( time, value, true );
				m_lastRecordedValue = value;
			}
			else if( valueAt( time ) != value )
			{
				removeValue( time, false );
			}
		}
	}
}



trackContentObjectView * AutomationPattern::createView( trackView * _tv )
{
	return new AutomationPatternView( this, _tv );
}





bool AutomationPattern::isAutomated( const AutomatableModel * _m )
{
	TrackContainer::TrackList l;
	l += engine::getSong()->tracks();
	l += engine::getBBTrackContainer()->tracks();
	l += engine::getSong()->globalAutomationTrack();

	for( TrackContainer::TrackList::ConstIterator it = l.begin(); it != l.end(); ++it )
	{
		if( ( *it )->type() == track::AutomationTrack ||
			( *it )->type() == track::HiddenAutomationTrack )
		{
			const track::tcoVector & v = ( *it )->getTCOs();
			for( track::tcoVector::ConstIterator j = v.begin(); j != v.end(); ++j )
			{
				const AutomationPattern * a = dynamic_cast<const AutomationPattern *>( *j );
				if( a && a->hasAutomation() )
				{
					for( objectVector::const_iterator k = a->m_objects.begin(); k != a->m_objects.end(); ++k )
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


/*! \brief returns a list of all the automation patterns everywhere that are connected to a specific model
 *  \param _m the model we want to look for
 */
QVector<AutomationPattern *> AutomationPattern::patternsForModel( const AutomatableModel * _m )
{
	QVector<AutomationPattern *> patterns;
	TrackContainer::TrackList l;
	l += engine::getSong()->tracks();
	l += engine::getBBTrackContainer()->tracks();
	l += engine::getSong()->globalAutomationTrack();
	
	// go through all tracks...
	for( TrackContainer::TrackList::ConstIterator it = l.begin(); it != l.end(); ++it )
	{
		// we want only automation tracks...
		if( ( *it )->type() == track::AutomationTrack ||
			( *it )->type() == track::HiddenAutomationTrack )
		{
			// get patterns in those tracks....
			const track::tcoVector & v = ( *it )->getTCOs();
			// go through all the patterns...
			for( track::tcoVector::ConstIterator j = v.begin(); j != v.end(); ++j )
			{
				AutomationPattern * a = dynamic_cast<AutomationPattern *>( *j );
				// check that the pattern has automation
				if( a && a->hasAutomation() )
				{
					// now check is the pattern is connected to the model we want by going through all the connections
					// of the pattern
					bool has_object = false;
					for( objectVector::const_iterator k = a->m_objects.begin(); k != a->m_objects.end(); ++k )
					{
						if( *k == _m )
						{
							has_object = true;
						}
					}
					// if the patterns is connected to the model, add it to the list
					if( has_object ) { patterns += a; }
				}
			}
		}
	}
	return patterns;
}



AutomationPattern * AutomationPattern::globalAutomationPattern(
							AutomatableModel * _m )
{
	AutomationTrack * t = engine::getSong()->globalAutomationTrack();
	track::tcoVector v = t->getTCOs();
	for( track::tcoVector::const_iterator j = v.begin(); j != v.end(); ++j )
	{
		AutomationPattern * a = dynamic_cast<AutomationPattern *>( *j );
		if( a )
		{
			for( objectVector::const_iterator k = a->m_objects.begin();
												k != a->m_objects.end(); ++k )
			{
				if( *k == _m )
				{
					return a;
				}
			}
		}
	}

	AutomationPattern * a = new AutomationPattern( t );
	a->addObject( _m, false );
	return a;
}




void AutomationPattern::resolveAllIDs()
{
	TrackContainer::TrackList l = engine::getSong()->tracks() +
				engine::getBBTrackContainer()->tracks();
	l += engine::getSong()->globalAutomationTrack();
	for( TrackContainer::TrackList::iterator it = l.begin();
							it != l.end(); ++it )
	{
		if( ( *it )->type() == track::AutomationTrack ||
			 ( *it )->type() == track::HiddenAutomationTrack )
		{
			track::tcoVector v = ( *it )->getTCOs();
			for( track::tcoVector::iterator j = v.begin();
							j != v.end(); ++j )
			{
				AutomationPattern * a = dynamic_cast<AutomationPattern *>( *j );
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




void AutomationPattern::clear()
{
	m_timeMap.clear();
	m_tangents.clear();

	emit dataChanged();

	if( engine::automationEditor() &&
		engine::automationEditor()->currentPattern() == this )
	{
		engine::automationEditor()->update();
	}
}




void AutomationPattern::openInAutomationEditor()
{
	engine::automationEditor()->setCurrentPattern( this );
	engine::automationEditor()->parentWidget()->show();
	engine::automationEditor()->setFocus();
}




void AutomationPattern::objectDestroyed( jo_id_t _id )
{
	// TODO: distict between temporary removal (e.g. LADSPA controls
	// when switching samplerate) and real deletions because in the latter
	// case we had to remove ourselves if we're the global automation
	// pattern of the destroyed object
	m_idsToResolve += _id;

	for( objectVector::Iterator objIt = m_objects.begin();
		objIt != m_objects.end(); objIt++ )
	{
		Q_ASSERT( !(*objIt).isNull() );
		if( (*objIt)->id() == _id )
		{
			//Assign to objIt so that this loop work even break; is removed.
			objIt = m_objects.erase( objIt );
			break;
		}
	}

	emit dataChanged();
}




void AutomationPattern::cleanObjects()
{
	for( objectVector::iterator it = m_objects.begin(); it != m_objects.end(); )
	{
		if( *it )
		{
			++it;
		}
		else
		{
			it = m_objects.erase( it );
		}
	}
}




void AutomationPattern::generateTangents()
{
	generateTangents(m_timeMap.begin(), m_timeMap.size());
}




void AutomationPattern::generateTangents( timeMap::const_iterator it,
							int numToGenerate )
{
	if( m_timeMap.size() < 2 )
	{
		m_tangents[it.key()] = 0;
		return;
	}

	for( int i = 0; i < numToGenerate; i++ )
	{
		if( it == m_timeMap.begin() )
		{
			m_tangents[it.key()] =
					( (it+1).value() - (it).value() ) /
						( (it+1).key() - (it).key() );
		}
		else if( it+1 == m_timeMap.end() )
		{
			m_tangents[it.key()] = 0;
			return;
		}
		else
		{
			m_tangents[it.key()] =
					( (it+1).value() - (it-1).value() ) /
						( (it+1).key() - (it-1).key() );
		}
		it++;
	}
}




#include "moc_AutomationPattern.cxx"
