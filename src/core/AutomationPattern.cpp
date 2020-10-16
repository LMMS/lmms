/*
 * AutomationPattern.cpp - implementation of class AutomationPattern which
 *                         holds dynamic values
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2006-2008 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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

#include "AutomationPattern.h"

#include "AutomationPatternView.h"
#include "AutomationTrack.h"
#include "LocaleHelper.h"
#include "Note.h"
#include "ProjectJournal.h"
#include "BBTrackContainer.h"
#include "Song.h"
#include "AutomationNode.h"

#include <cmath>

int AutomationPattern::s_quantization = 1;
const float AutomationPattern::DEFAULT_MIN_VALUE = 0;
const float AutomationPattern::DEFAULT_MAX_VALUE = 1;


AutomationPattern::AutomationPattern( AutomationTrack * _auto_track ) :
	TrackContentObject( _auto_track ),
	m_autoTrack( _auto_track ),
	m_objects(),
	m_tension( 1.0 ),
	m_progressionType( DiscreteProgression ),
	m_dragging( false ),
	m_isRecording( false ),
	m_lastRecordedValue( 0 )
{
	changeLength( MidiTime( 1, 0 ) );
	if( getTrack() )
	{
		switch( getTrack()->trackContainer()->type() )
		{
			case TrackContainer::BBContainer:
				setAutoResize( true );
				break;

			case TrackContainer::SongContainer:
				// move down
			default:
				setAutoResize( false );
				break;
		}
	}
}




AutomationPattern::AutomationPattern( const AutomationPattern & _pat_to_copy ) :
	TrackContentObject( _pat_to_copy.m_autoTrack ),
	m_autoTrack( _pat_to_copy.m_autoTrack ),
	m_objects( _pat_to_copy.m_objects ),
	m_tension( _pat_to_copy.m_tension ),
	m_progressionType( _pat_to_copy.m_progressionType )
{
	for( timeMap::const_iterator it = _pat_to_copy.m_timeMap.begin();
				it != _pat_to_copy.m_timeMap.end(); ++it )
	{
		// Copies the automation node (in/out values and in/out tangents)
		m_timeMap[ it.key() ] = it.value();
	}
	switch( getTrack()->trackContainer()->type() )
	{
		case TrackContainer::BBContainer:
			setAutoResize( true );
			break;

		case TrackContainer::SongContainer:
			// move down
		default:
			setAutoResize( false );
			break;
	}
}

bool AutomationPattern::addObject( AutomatableModel * _obj, bool _search_dup )
{
	if( _search_dup && m_objects.contains(_obj) )
	{
		return false;
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

	return true;
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
	float nt = LocaleHelper::toFloat(_new_tension, & ok);

	if( ok && nt > -0.01 && nt < 1.01 )
	{
		m_tension = nt;
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

const AutomationPattern::objectVector& AutomationPattern::objects() const
{
	return m_objects;
}




MidiTime AutomationPattern::timeMapLength() const
{
	MidiTime one_bar = MidiTime(1, 0);
	if (m_timeMap.isEmpty()) { return one_bar; }

	timeMap::const_iterator it = m_timeMap.end();
	tick_t last_tick = static_cast<tick_t>((it-1).key());
	// if last_tick is 0 (single item at tick 0)
	// return length as a whole bar to prevent disappearing TCO
	if (last_tick == 0) { return one_bar; }

	return MidiTime(last_tick);
}




void AutomationPattern::updateLength()
{
	// Do not resize down in case user manually extended up
	changeLength(qMax(length(), timeMapLength()));
}




// For now, when creating a node, the inValue and outValue will match. After the node is
// created, the user will then be able to drag the outValue to make discrete jumps
MidiTime AutomationPattern::putValue( const MidiTime & time,
					const float value,
					const bool quantPos,
					const bool ignoreSurroundingPoints )
{
	cleanObjects();

	MidiTime newTime = quantPos ?
				Note::quantized( time, quantization() ) :
				time;

	// Create a node or replace the existing one on newTime
	m_timeMap[ newTime ] = AutomationNode( this, value, newTime );

	timeMap::iterator it = m_timeMap.find( newTime );

	// Remove control points that are covered by the new points
	// quantization value. Control Key to override
	if( ! ignoreSurroundingPoints )
	{
		for( int i = newTime + 1; i < newTime + quantization(); ++i )
		{
			AutomationPattern::removeValue( i );
		}
	}
	if( it != m_timeMap.begin() )
	{
		--it;
	}
	generateTangents( it, 3 );

	updateLength();

	emit dataChanged();

	return newTime;
}




void AutomationPattern::removeValue( const MidiTime & time )
{
	cleanObjects();

	m_timeMap.remove( time );
	timeMap::iterator it = m_timeMap.lowerBound( time );
	if( it != m_timeMap.begin() )
	{
		--it;
	}
	generateTangents(it, 3);

	updateLength();

	emit dataChanged();
}



void AutomationPattern::recordValue(MidiTime time, float value)
{
	if( value != m_lastRecordedValue )
	{
		putValue( time, value, true );
		m_lastRecordedValue = value;
	}
	else if( valueAt( time ) != value )
	{
		removeValue( time );
	}
}




/**
 * @brief Set the position of the point that is being dragged.
 *        Calling this function will also automatically set m_dragging to true,
 *        which applyDragValue() have to be called to m_dragging.
 * @param the time(x position) of the point being dragged
 * @param the value(y position) of the point being dragged
 * @param true to snip x position
 * @return
 */
MidiTime AutomationPattern::setDragValue( const MidiTime & time,
						const float value,
						const bool quantPos,
						const bool controlKey )
{
	if( m_dragging == false )
	{
		MidiTime newTime = quantPos  ?
				Note::quantized( time, quantization() ) :
							time;
		this->removeValue( newTime );
		m_oldTimeMap = m_timeMap;
		m_dragging = true;
	}

	//Restore to the state before it the point were being dragged
	m_timeMap = m_oldTimeMap;

	generateTangents();

	return this->putValue( time, value, quantPos, controlKey );

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

	// If we have a node at that time, just return its value
	if( m_timeMap.contains( _time ) )
	{
		// When the time is exactly the node's time, we want the inValue
		return m_timeMap[_time].getInValue();
	}

	// lowerBound returns next value with greater key, therefore we take
	// the previous element to get the current value
	timeMap::const_iterator v = m_timeMap.lowerBound( _time );

	if( v == m_timeMap.begin() )
	{
		return 0;
	}
	if( v == m_timeMap.end() )
	{
		// When the time is after the last node, we want the outValue of it
		return (v-1).value().getOutValue();
	}

	return valueAt( v-1, _time - (v-1).key() );
}




// This method will get the value at a offset from a node, so we use the outValue of
// that node and the inValue of the next node to the calculations. This assumes that offset
// will not be zero, because when the midi time given to AutomationPattern::valueAt(MidiTime)
// matches a node's position, that node's value will be returned and this method won't be even
// called.
// TODO: If in the future we want to be able to call this method manually with offset = 0, we
// need to account for that with a simple conditional at the beginning that just returns the
// node's inValue.
float AutomationPattern::valueAt( timeMap::const_iterator v, int offset ) const
{
	if( m_progressionType == DiscreteProgression || v == m_timeMap.end() )
	{
		return v.value().getOutValue();
	}
	else if( m_progressionType == LinearProgression )
	{
		float slope = ((v+1).value().getInValue() - v.value().getOutValue()) /
							((v+1).key() - v.key());
		return v.value().getOutValue() + offset * slope;
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
		float m1 = v.value().getOutTangent() * numValues * m_tension;
		float m2 = (v+1).value().getInTangent() * numValues * m_tension;

		return ( 2*pow(t,3) - 3*pow(t,2) + 1 ) * v.value().getOutValue()
				+ ( pow(t,3) - 2*pow(t,2) + t) * m1
				+ ( -2*pow(t,3) + 3*pow(t,2) ) * (v+1).value().getInValue()
				+ ( pow(t,3) - pow(t,2) ) * m2;
	}
}




float *AutomationPattern::valuesAfter( const MidiTime & _time ) const
{
	timeMap::const_iterator v = m_timeMap.lowerBound( _time );
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




// TODO: Because putValue sets both the inValue and outValue to the same value, flipping
// the pattern will ignore outValues that are different (meaning discrete jumps will be lost).
// If we want them to be kept we need to update the logic so they are accounted for. For now
// this behavior is acceptable (discrete jumps will likely lose their meaning on a flipped
// pattern anyways)
void AutomationPattern::flipY( int min, int max )
{
	timeMap tempMap = m_timeMap;
	timeMap::const_iterator iterate = m_timeMap.lowerBound(0);
	float tempValue = 0;

	int numPoints = 0;

	// TODO: This loop looks really odd. Is there a particular case where iterate+i+1 != m_timeMap.end()
	// will be true but iterate + 1 != m_timeMap.end() will be false?
	for( int i = 0; ( iterate + i + 1 ) != m_timeMap.end() && ( iterate + i ) != m_timeMap.end() ; i++)
	{
		numPoints++;
	}

	for( int i = 0; i <= numPoints; i++ )
	{

		if ( min < 0 )
		{
			tempValue = valueAt( ( iterate + i ).key() ) * -1;
			putValue( MidiTime( (iterate + i).key() ) , tempValue, false);
		}
		else
		{
			tempValue = max - valueAt( ( iterate + i ).key() );
			putValue( MidiTime( (iterate + i).key() ) , tempValue, false);
		}
	}

	generateTangents();
	emit dataChanged();
}




void AutomationPattern::flipY()
{
	flipY(getMin(), getMax());
}




// TODO: This method sets both the inValue and outValue of the new timeMap to the same value,
// so flipping the pattern will ignore outValues that are different (meaning discrete jumps will be lost).
// If we want them to be kept we need to update the logic so they are accounted for. For now
// this behavior is acceptable (discrete jumps will likely lose their meaning on a flipped
// pattern anyways)
void AutomationPattern::flipX( int length )
{
	timeMap tempMap;

	timeMap::const_iterator iterate = m_timeMap.lowerBound(0);
	float tempValue = 0;
	int numPoints = 0;

	// TODO: Same as the comment on AutomationPattern::flipY
	for( int i = 0; ( iterate + i + 1 ) != m_timeMap.end() && ( iterate + i ) != m_timeMap.end() ; i++)
	{
		numPoints++;
	}

	float realLength = ( iterate + numPoints ).key();

	if ( length != -1 && length != realLength)
	{
		if ( realLength < length )
		{
			tempValue = valueAt( ( iterate + numPoints ).key() );
			putValue( MidiTime( length ) , tempValue, false);
			numPoints++;
			for( int i = 0; i <= numPoints; i++ )
			{
				tempValue = valueAt( ( iterate + i ).key() );
				MidiTime newTime = MidiTime( length - ( iterate + i ).key() );
				tempMap[newTime] = AutomationNode( this, tempValue, newTime );
			}
		}
		else
		{
			for( int i = 0; i <= numPoints; i++ )
			{
				tempValue = valueAt( ( iterate + i ).key() );
				MidiTime newTime;

				if ( ( iterate + i ).key() <= length )
				{
					newTime = MidiTime( length - ( iterate + i ).key() );
				}
				else
				{
					newTime = MidiTime( ( iterate + i ).key() );
				}
				tempMap[newTime] = AutomationNode( this, tempValue, newTime );
			}
		}
	}
	else
	{
		for( int i = 0; i <= numPoints; i++ )
		{
			tempValue = valueAt( ( iterate + i ).key() );
			cleanObjects();
			MidiTime newTime = MidiTime( realLength - ( iterate + i ).key() );
			tempMap[newTime] = AutomationNode( this, tempValue, newTime );
		}
	}

	m_timeMap.clear();

	m_timeMap = tempMap;

	generateTangents();
	emit dataChanged();
}




void AutomationPattern::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "pos", startPosition() );
	_this.setAttribute( "len", length() );
	_this.setAttribute( "name", name() );
	_this.setAttribute( "prog", QString::number( progressionType() ) );
	_this.setAttribute( "tens", QString::number( getTension() ) );
	_this.setAttribute( "mute", QString::number( isMuted() ) );

	for( timeMap::const_iterator it = m_timeMap.begin();
						it != m_timeMap.end(); ++it )
	{
		QDomElement element = _doc.createElement( "time" );
		element.setAttribute( "pos", it.key() );
		// TODO: Requires project file upgrade!!!
		element.setAttribute( "inValue", it.value().getInValue() );
		element.setAttribute( "outValue", it.value().getOutValue() );
		_this.appendChild( element );
	}

	for( objectVector::const_iterator it = m_objects.begin();
						it != m_objects.end(); ++it )
	{
		if( *it )
		{
			QDomElement element = _doc.createElement( "object" );
			element.setAttribute( "id",
				ProjectJournal::idToSave( ( *it )->id() ) );
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
	setMuted(_this.attribute( "mute", QString::number( false ) ).toInt() );

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
			int timeMapPos = element.attribute("pos").toInt();
			// TODO: Requires project file upgrade!!!
			float timeMapInValue = LocaleHelper::toFloat(element.attribute("inValue"));
			float timeMapOutValue = LocaleHelper::toFloat(element.attribute("outValue"));

			m_timeMap[timeMapPos] = AutomationNode( this, timeMapInValue, timeMapOutValue, timeMapPos );
		}
		else if( element.tagName() == "object" )
		{
			m_idsToResolve << element.attribute( "id" ).toInt();
		}
	}

	int len = _this.attribute( "len" ).toInt();
	if( len <= 0 )
	{
		// TODO: Handle with an upgrade method
		updateLength();
	}
	else
	{
		changeLength( len );
	}
	generateTangents();
}




const QString AutomationPattern::name() const
{
	if( !TrackContentObject::name().isEmpty() )
	{
		return TrackContentObject::name();
	}
	if( !m_objects.isEmpty() && m_objects.first() != NULL )
	{
		return m_objects.first()->fullDisplayName();
	}
	return tr( "Drag a control while pressing <%1>" ).arg(UI_CTRL_KEY);
}




TrackContentObjectView * AutomationPattern::createView( TrackView * _tv )
{
	return new AutomationPatternView( this, _tv );
}





bool AutomationPattern::isAutomated( const AutomatableModel * _m )
{
	TrackContainer::TrackList l;
	l += Engine::getSong()->tracks();
	l += Engine::getBBTrackContainer()->tracks();
	l += Engine::getSong()->globalAutomationTrack();

	for( TrackContainer::TrackList::ConstIterator it = l.begin(); it != l.end(); ++it )
	{
		if( ( *it )->type() == Track::AutomationTrack ||
			( *it )->type() == Track::HiddenAutomationTrack )
		{
			const Track::tcoVector & v = ( *it )->getTCOs();
			for( Track::tcoVector::ConstIterator j = v.begin(); j != v.end(); ++j )
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
	l += Engine::getSong()->tracks();
	l += Engine::getBBTrackContainer()->tracks();
	l += Engine::getSong()->globalAutomationTrack();

	// go through all tracks...
	for( TrackContainer::TrackList::ConstIterator it = l.begin(); it != l.end(); ++it )
	{
		// we want only automation tracks...
		if( ( *it )->type() == Track::AutomationTrack ||
			( *it )->type() == Track::HiddenAutomationTrack )
		{
			// get patterns in those tracks....
			const Track::tcoVector & v = ( *it )->getTCOs();
			// go through all the patterns...
			for( Track::tcoVector::ConstIterator j = v.begin(); j != v.end(); ++j )
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
	AutomationTrack * t = Engine::getSong()->globalAutomationTrack();
	Track::tcoVector v = t->getTCOs();
	for( Track::tcoVector::const_iterator j = v.begin(); j != v.end(); ++j )
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
	TrackContainer::TrackList l = Engine::getSong()->tracks() +
				Engine::getBBTrackContainer()->tracks();
	l += Engine::getSong()->globalAutomationTrack();
	for( TrackContainer::TrackList::iterator it = l.begin();
							it != l.end(); ++it )
	{
		if( ( *it )->type() == Track::AutomationTrack ||
			 ( *it )->type() == Track::HiddenAutomationTrack )
		{
			Track::tcoVector v = ( *it )->getTCOs();
			for( Track::tcoVector::iterator j = v.begin();
							j != v.end(); ++j )
			{
				AutomationPattern * a = dynamic_cast<AutomationPattern *>( *j );
				if( a )
				{
					for( QVector<jo_id_t>::Iterator k = a->m_idsToResolve.begin();
									k != a->m_idsToResolve.end(); ++k )
					{
						JournallingObject * o = Engine::projectJournal()->
														journallingObject( *k );
						if( o && dynamic_cast<AutomatableModel *>( o ) )
						{
							a->addObject( dynamic_cast<AutomatableModel *>( o ), false );
						}
						else
						{
							// FIXME: Remove this block once the automation system gets fixed
							// This is a temporary fix for https://github.com/LMMS/lmms/issues/3781
							o = Engine::projectJournal()->journallingObject(ProjectJournal::idFromSave(*k));
							if( o && dynamic_cast<AutomatableModel *>( o ) )
							{
								a->addObject( dynamic_cast<AutomatableModel *>( o ), false );
							}
							else
							{
								// FIXME: Remove this block once the automation system gets fixed
								// This is a temporary fix for https://github.com/LMMS/lmms/issues/4781
								o = Engine::projectJournal()->journallingObject(ProjectJournal::idToSave(*k));
								if( o && dynamic_cast<AutomatableModel *>( o ) )
								{
									a->addObject( dynamic_cast<AutomatableModel *>( o ), false );
								}
							}
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

	emit dataChanged();
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




// We have two tangents, one for the left side of the node and one for the right side
// of the node (in case we have discrete value jumps in the middle of a curve).
// If the inValue and outValue of a node are the same, consequently the inTangent and
// outTangent values of the node will be the same too.
void AutomationPattern::generateTangents( timeMap::iterator it,
							int numToGenerate )
{
	if( m_timeMap.size() < 2 && numToGenerate > 0 )
	{
		it.value().setInTangent( 0 );
		it.value().setOutTangent( 0 );
		return;
	}

	for( int i = 0; i < numToGenerate; i++ )
	{
		if( it == m_timeMap.begin() )
		{
			// On the first node there's no curve behind it, so we will only calculate the outTangent
			// and inTangent will be set to 0.
			float tangent = ( (it+1).value().getInValue() - (it).value().getOutValue() ) /
				( (it+1).key() - (it).key() );
			it.value().setInTangent( 0 );
			it.value().setOutTangent( tangent );
		}
		else if( it+1 == m_timeMap.end() )
		{
			// Previously, the last value's tangent was always set to 0. That logic was kept for both tangents
			// of the last node
			it.value().setInTangent( 0 );
			it.value().setOutTangent( 0 );
			return;
		}
		else
		{
			// When we are in a node that is in the middle of two other nodes, we need to check if we
			// have a discrete jump at this node. If we do not, then we can calculate the tangents normally.
			// If we do have a discrete jump, then we have to calculate the tangents differently for each side
			// of the curve.
			float inTangent;
			float outTangent;
			if( it.value().getInValue() == it.value().getOutValue() )
			{
				inTangent = ( (it+1).value().getInValue() - (it-1).value().getOutValue() ) /
					( (it+1).key() - (it-1).key() );
				it.value().setInTangent( inTangent );
				// inTangent == outTangent in this case
				it.value().setOutTangent( inTangent );
			}
			else
			{
				// Calculate the left side of the curve
				inTangent = ( (it).value().getInValue() - (it-1).value().getOutValue() ) /
					( (it).key() - (it-1).key() );
				// Calculate the right side of the curve
				outTangent = ( (it+1).value().getInValue() - (it).value().getOutValue() ) /
					( (it+1).key() - (it).key() );
				it.value().setInTangent( inTangent );
				it.value().setOutTangent( outTangent );
			}
		}
		it++;
	}
}




