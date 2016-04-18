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

#include "AutomationPattern.h"

#include "AutomationPatternView.h"
#include "AutomationTrack.h"
#include "ProjectJournal.h"
#include "BBTrackContainer.h"
#include "Song.h"
#include "embed.h"

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
		m_timeMap[it.key()] = it.value();
		m_tangents[it.key()] = _pat_to_copy.m_tangents[it.key()];
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




AutomationPattern::~AutomationPattern()
{
}




bool AutomationPattern::addObject( AutomatableModel * _obj, bool _search_dup )
{
	if( _search_dup )
	{
		for( objectVector::iterator it = m_objects.begin();
					it != m_objects.end(); ++it )
		{
			if( *it == _obj )
			{				
				return false;
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

	return true;
}




void AutomationPattern::setProgressionType(
					ProgressionTypes _new_progression_type )
{
	if ( _new_progression_type == DiscreteProgression ||
		_new_progression_type == LinearProgression ||
		_new_progression_type == CubicHermiteProgression ||
		_new_progression_type == BezierProgression )
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





MidiTime AutomationPattern::putControlPoint( timeMap::const_iterator it, 
					const int time, const float _value, const bool flip )
{
	if (flip)
	{
		putControlPoint( it, 2 * it.key() - time, 2 * it.value() - _value );
	}
	else
	{
		putControlPoint( it, time, _value );
	}
	return it.key();
}




/* If we are only given the value and automation point
	then figure out where to put the control point */
MidiTime AutomationPattern::putControlPoint(timeMap::const_iterator it, 
							const float _value)
{
	// Insert control point at the automation point
	return putControlPoint( it, (float)it.key() + 50, _value );
}




MidiTime AutomationPattern::putControlPoint(timeMap::const_iterator it, 
						const int time, const float _value)
{
	m_controlPoints.remove( it.key() );
	m_controlPoints[it.key()].insert( 0, time );
	m_controlPoints[it.key()].insert( 1, _value );
	clampControlPoints();
	return it.key();
}




MidiTime AutomationPattern::putValue( const MidiTime & _time,
							const float _value,
							const bool _quant_pos )
{
	cleanObjects();

	MidiTime newTime = _quant_pos ?
				Note::quantized( _time, quantization() ) :
				_time;

	m_timeMap[newTime] = _value;
	timeMap::const_iterator it = m_timeMap.find( newTime );

	putControlPoint(it, _value);
	clampControlPoints();

	if( it != m_timeMap.begin() )
	{
		--it;
	}
	generateTangents(it, 3);

	// we need to maximize our length in case we're part of a hidden
	// automation track as the user can't resize this pattern
	if( getTrack() && getTrack()->type() == Track::HiddenAutomationTrack )
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

	MidiTime newTime = _quant_pos ?
				Note::quantized( _time, quantization() ) :
				_time;

	m_timeMap.remove( newTime );
	m_tangents.remove( newTime );
	m_controlPoints.remove( newTime );
	timeMap::const_iterator it = m_timeMap.lowerBound( newTime );
	if( it != m_timeMap.begin() )
	{
		--it;
	}

	generateTangents(it, 3);

	if( getTrack() &&
		getTrack()->type() == Track::HiddenAutomationTrack )
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
		MidiTime newTime = _quant_pos  ?
					Note::quantized( _time, quantization() ) :
					_time;

		if ( m_timeMap.contains( newTime ) )
		{
			// Set the offset for the control point, so it gets dragged around with the automation point
			m_controlPointDragOffset[0] = (float)m_controlPoints[newTime][0] - (float)newTime;
			m_controlPointDragOffset[1] = m_controlPoints[newTime][1] - m_timeMap[newTime];
		}
		else
		{
			m_controlPointDragOffset[0] = 50;
			m_controlPointDragOffset[1] = 0;
		}

		this->removeValue( newTime );
		m_oldTimeMap = m_timeMap;
		m_oldControlPoints = m_controlPoints;
		m_dragging = true;
	}

	//Restore to the state before it the point were being dragged
	m_timeMap = m_oldTimeMap;
	m_controlPoints = m_oldControlPoints;

	for( timeMap::const_iterator it = m_timeMap.begin(); it != m_timeMap.end(); ++it )
	{
		generateTangents(it, 3);
	}

	// Put the new automation point down
	MidiTime returnValue = this->putValue( _time, _value, _quant_pos );
	// Put a new control point down at an offset
	m_controlPoints.remove( returnValue );
	putControlPoint(m_timeMap.find( returnValue ), (float)returnValue + m_controlPointDragOffset[0], 
			_value + m_controlPointDragOffset[1]);
	clampControlPoints();

	return returnValue;
}





MidiTime AutomationPattern::setControlPointDragValue( const MidiTime & _time, const float _value, const int _x,
					   const bool _quant_pos)
{
	if( m_dragging == false )
	{
		MidiTime newTime = _quant_pos  ?
					Note::quantized( _time, quantization() ) :
					_time;
		m_controlPoints.remove( newTime );
		m_oldControlPointNode = m_timeMap.find( newTime );
		m_dragging = true;
	}

	return this->putControlPoint(m_oldControlPointNode, _x, _value, m_controlFlip);
}




/**
 * @breif If the control point grabbed is on the left of the automation point,
 *        be flipped in order to get the control points actual location.
 * @param should the value be flipped or not
 */
void AutomationPattern::flipControlPoint(bool flip)
{
	m_controlFlip = flip;
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
	else if( m_progressionType == CubicHermiteProgression )
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
	else /* BezierProgression */
	{

		/* Formula goes as such:
			Automation points: 		(x0, y0), (x3, y3)
			Relative control points: 	(x1, y1), (x2, y2)
			Where the control points are BETWEEN the automation points. 
				(This isn't the case in this program, so the second control point must be "flipped"
				around its automation point)

			x = ( (1-t)^3 * x0 ) + ( 3 * (1-t)^2 * t * x1 ) + ( 3 * (1-t) * t^2 * x2 ) + ( t^3 * x3 )
			y = ( (1-t)^3 * y0 ) + ( 3 * (1-t)^2 * t * y1 ) + ( 3 * (1-t) * t^2 * y2 ) + ( t^3 * y3 )

			0 <= t <= 1
		*/

		int numValues = (v+1).key() - v.key();

		// The x values are essentially twice the distance from their control points
		// to make up for their range being limited.
		int targetX1 = ( m_controlPoints[v.key()][0] - v.key() ) * 2;
		int targetX2 = ( 3 * (v+1).key() - 2 * m_controlPoints[(v+1).key()][0] - v.key() );
		// The y values are the actual y values. Maybe this should be doubled, 
		// but it doesn't seem necessary to me.
		float targetY1 = m_controlPoints[v.key()][1];
		float targetY2 = 2*(v+1).value() - m_controlPoints[(v+1).key()][1];

		// To find the y value on the curve at a certain x, we first have to find the t (between 0 and 1) that gives the x
		float t = 0;
		float x = 3 * pow((1-t), 2) * t * targetX1 + 3 * (1-t) * pow(t, 2) * targetX2 + pow(t, 3) * numValues;
		while (offset > x)
		{
			t += 0.05;
			x = 3 * pow((1-t), 2) * t * targetX1 + 3 * (1-t) * pow(t, 2) * targetX2 + pow(t, 3) * numValues;
		}

		float ratio = x;
		float y1 = pow((1-t),3) * v.value() + 3 * pow((1-t),2) * t * targetY1 + 
				3 * (1-t) * pow(t,2) * targetY2 + pow(t,3) * (v+1).value();
		t -= 0.05;
		float y2 = pow((1-t),3) * v.value() + 3 * pow((1-t),2) * t * targetY1 + 
				3 * (1-t) * pow(t,2) * targetY2 + pow(t,3) * (v+1).value();
		x = 3 * pow((1-t), 2) * t * targetX1 + 3 * (1-t) * pow(t, 2) * targetX2 + pow(t, 3) * numValues;

		// Ratio is how we get the linear extrapolation between points
		// We have to get the ratio of how close this point is to its left compared to right
		ratio = (offset - x) / (ratio - x);
		return (ratio)*y1 + (1-ratio)*y2;
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




void AutomationPattern::flipY( int min, int max )
{
	timeMap tempMap = m_timeMap;
	timeMap::ConstIterator iterate = m_timeMap.lowerBound(0);
	float tempValue = 0;

	int numPoints = 0;

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
			tempValue = m_controlPoints[(iterate + i).key()][1] * -1;
			m_controlPoints[(iterate + i).key()][1] = tempValue;
		}
		else
		{
			tempValue = max - valueAt( ( iterate + i ).key() );
			putValue( MidiTime( (iterate + i).key() ) , tempValue, false);
			tempValue = max - m_controlPoints[(iterate + i).key()][1];
			m_controlPoints[(iterate + i).key()][1] = tempValue;
		}
	}

	generateTangents();
	emit dataChanged();
}




void AutomationPattern::flipY()
{
	flipY(getMin(), getMax());
}




void AutomationPattern::flipX( int length )
{
	timeMap tempMap;
	controlPointTimeMap tempControlPoints;

	timeMap::ConstIterator iterate = m_timeMap.lowerBound(0);
	float tempValue = 0;
	int numPoints = 0;

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

				int newControlPointX = -( iterate + i ).key() + m_controlPoints[( iterate + i ).key()][0] + newTime;
				tempControlPoints[newTime].insert( 0, newControlPointX );
				tempControlPoints[newTime].insert( 1, 2*tempValue - m_controlPoints[( iterate + i ).key()][1] );

				tempMap[newTime] = tempValue;
			}
		}
		else
		{
			for( int i = 0; i <= numPoints; i++ )
			{
				tempValue = valueAt( ( iterate + i ).key() );
				MidiTime newTime;

				int newControlPointX = -( iterate + i ).key() + m_controlPoints[( iterate + i ).key()][0] + newTime;
				tempControlPoints[newTime].insert( 0, newControlPointX );
				tempControlPoints[newTime].insert( 1, 2*tempValue - m_controlPoints[( iterate + i ).key()][1] );

				if ( ( iterate + i ).key() <= length )
				{
					newTime = MidiTime( length - ( iterate + i ).key() );
				}
				else
				{
					newTime = MidiTime( ( iterate + i ).key() );
				}
				tempMap[newTime] = tempValue;
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
			int newControlPointX = -( iterate + i ).key() + m_controlPoints[( iterate + i ).key()][0] + newTime;

			tempMap[newTime] = tempValue;
			tempControlPoints[newTime].insert( 0, newControlPointX );
			tempControlPoints[newTime].insert( 1, 2*tempValue - m_controlPoints[( iterate + i ).key()][1] );
		}
	}

	m_timeMap.clear();
	m_controlPoints.clear();

	m_timeMap = tempMap;
	m_controlPoints = tempControlPoints;

	generateTangents();
	emit dataChanged();
}




void AutomationPattern::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "pos", startPosition() );
	_this.setAttribute( "len", TrackContentObject::length() );
	_this.setAttribute( "name", name() );
	_this.setAttribute( "prog", QString::number( progressionType() ) );
	_this.setAttribute( "tens", QString::number( getTension() ) );
	_this.setAttribute( "mute", QString::number( isMuted() ) );

	for( timeMap::const_iterator it = m_timeMap.begin();
						it != m_timeMap.end(); ++it )
	{
		QDomElement element = _doc.createElement( "time" );
		element.setAttribute( "pos", it.key() );
		element.setAttribute( "value", it.value() );
		_this.appendChild( element );
	}

	for( controlPointTimeMap::const_iterator it = m_controlPoints.begin();
						it != m_controlPoints.end(); ++it )
	{
		QDomElement element = _doc.createElement( "ctrlpnt" );
		element.setAttribute( "pos", it.key() );
		element.setAttribute( "value1", it.value()[0] );
		element.setAttribute( "value2", it.value()[1] );
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
			m_timeMap[element.attribute( "pos" ).toInt()]
				= element.attribute( "value" ).toFloat();
		}
		else if( element.tagName() == "ctrlpnt" )
		{
			m_controlPoints[element.attribute( "pos" ).toInt()].insert( 0, element.attribute( "value1" ).toInt() );
			m_controlPoints[element.attribute( "pos" ).toInt()].insert( 1, element.attribute( "value2" ).toFloat() );
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

	// Very important for reading older files
	cleanControlPoints();
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
	return tr( "Drag a control while pressing <%1>" ).arg(
	#ifdef LMMS_BUILD_APPLE
		"⌘");
	#else
		"Ctrl");
	#endif
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




void AutomationPattern::clampControlPoints(bool clampVertical)
{
	timeMap::const_iterator it;
	for (it = m_timeMap.begin(); it != m_timeMap.end(); it++)
	{
		int new_x = m_controlPoints[it.key()][0];
		float new_y = m_controlPoints[it.key()][1];
		// Clamp X positions
		// If the control point x is less than its automation point
		if ( it.key() > m_controlPoints[it.key()][0] )
		{
			new_x = it.key();
		}
		// The control point x must not pass the midpoints of its automation point and the automation points
		// its left and right
		else if ( it != m_timeMap.begin() && it.key() * 2 - m_controlPoints[it.key()][0] < ( (it-1).key() + it.key() ) / 2 )
		{
			new_x = it.key() * 2 - ( (it-1).key() + it.key() )/2;
		}
		else if ( it+1 != m_timeMap.end() && m_controlPoints[it.key()][0] > ( (it+1).key() + it.key() )/2 )
		{
			new_x = ( (it+1).key() + it.key() )/2;
		}

		if (clampVertical)
		{
			// Clamp y positions between the top and bottom of the screen
			// Clamp the right control point (keep in mind the last control point isn't clamped)
			if ( it+1 != m_timeMap.end() && m_controlPoints[it.key()][1] > getMax() )
			{
				new_y = getMax();
			}
			else if ( it+1 != m_timeMap.end() && m_controlPoints[it.key()][1] < getMin() )
			{
				new_y = getMin();
			}
			// Clamp the left control point (keep in mind the first control point isn't clamped)
			if ( it != m_timeMap.begin() && 2 * it.value() - m_controlPoints[it.key()][1] > getMax() )
			{
				new_y = 2 * it.value() - getMax();
			}
			else if ( it != m_timeMap.begin() && 2 * it.value() - m_controlPoints[it.key()][1] < getMin() )
			{
				new_y =  2 * it.value() - getMin();
			}
		}

		m_controlPoints.remove( it.key() );
		
		m_controlPoints[it.key()].insert( 0, new_x );
		m_controlPoints[it.key()].insert( 1, new_y );
	}
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
	m_controlPoints.clear();
	m_tangents.clear();

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




void AutomationPattern::cleanControlPoints()
{
	// If there's any control points that aren't connected to an automation point then destroy it
	for( controlPointTimeMap::iterator it = m_controlPoints.begin(); it != m_controlPoints.end(); )
	{
		if(m_timeMap.contains( (int)it.key()) )
		{
			it++;
		}
		else
		{
			it = m_controlPoints.erase( it );
		}
	}

	// If there's any automation points without a control point then destroy it
	for( timeMap::iterator it = m_timeMap.begin(); it != m_timeMap.end(); )
	{
		if(m_controlPoints.contains( (int)it.key()) )
		{
			it++;
		}
		else
		{
			m_controlPoints[it.key()].insert( 0, it.key() + 50 );
			m_controlPoints[it.key()].insert( 1, it.value() );
		}
	}

	clampControlPoints(false);
}




void AutomationPattern::generateTangents()
{
	generateTangents(m_timeMap.begin(), m_timeMap.size());
}




void AutomationPattern::generateTangents( timeMap::const_iterator it,
							int numToGenerate )
{
	if( m_timeMap.size() < 2 && numToGenerate > 0 )
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





