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

#include <QDomElement>
#include <QMouseEvent>
#include <QPainter>

#include "AutomationPattern.h"
#include "AutomationPatternView.h"
#include "AutomationEditor.h"
#include "AutomationTrack.h"
#include "ProjectJournal.h"
#include "BBTrackContainer.h"
#include "Song.h"
#include "TextFloat.h"
#include "embed.h"
#include "TempoTrack.h"


const float AutomationPattern::DEFAULT_MIN_VALUE = 0;
const float AutomationPattern::DEFAULT_MAX_VALUE = 1;


AutomationPattern::AutomationPattern( AutomationTrack * auto_track ) :
	TrackContentObject( auto_track ),
	m_autoTrack( auto_track ),
	m_tension( 1.0 ),
	m_progressionType( DiscreteProgression ),
	m_dragging( false ),
	m_isRecording( false ),
	m_lastRecordedValue( 0 ),
	m_inlineObject( NULL ),
	m_isTempoPattern( false )
{
	changeLength( MidiTime( 1, 0 ) );
	
	if( m_autoTrack && ! m_autoTrack->objects()->isEmpty() )
	{
		const AutomatableModel * obj = firstObject();
		putValue( MidiTime(0), obj->inverseScaledValue( obj->value<float>() ), false );
	}
}


AutomationPattern::AutomationPattern( const AutomationPattern & _pat_to_copy ) :
	TrackContentObject( _pat_to_copy.m_autoTrack ),
	m_autoTrack( _pat_to_copy.m_autoTrack ),
	m_tension( _pat_to_copy.m_tension ),
	m_progressionType( _pat_to_copy.m_progressionType ),
	m_dragging( false ),
	m_isRecording( _pat_to_copy.m_isRecording ),
	m_lastRecordedValue( _pat_to_copy.m_lastRecordedValue ),
	m_inlineObject( _pat_to_copy.m_inlineObject ),
	m_isTempoPattern( _pat_to_copy.m_isTempoPattern )
{
	for( TimeMap::const_iterator it = _pat_to_copy.m_timeMap.begin();
				it != _pat_to_copy.m_timeMap.end(); ++it )
	{
		m_timeMap[it.key()] = it.value();
		m_tangents[it.key()] = _pat_to_copy.m_tangents[it.key()];
	}
}




AutomationPattern::~AutomationPattern()
{
	if( Engine::automationEditor() &&
		Engine::automationEditor()->currentPattern() == this )
	{
		Engine::automationEditor()->setCurrentPattern( NULL );
	}
}




void AutomationPattern::addObject( AutomatableModel * obj, bool search_dup )
{
	if( m_isTempoPattern ) { return; } // no adding objects to tempo patterns
	m_autoTrack->addObject( obj, search_dup );
	
	// if there is nothing in the pattern..
	if( hasAutomation() == false )
	{
		// then initialize first value
		putValue( MidiTime(0), obj->inverseScaledValue( obj->value<float>() ), false );
	}
}


void AutomationPattern::addInlineObject( InlineAutomation * i )
{
	if( m_isTempoPattern ) { return; } // no adding objects to tempo patterns
	m_inlineObject = i;
	if( hasAutomation() == false )
	{
		putValue( MidiTime(0), i->inverseScaledValue( i->value() ), false );
	}
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
	return m_autoTrack->firstObject();
}





MidiTime AutomationPattern::length() const
{
	if( m_timeMap.isEmpty() ) return 0;
	TimeMap::const_iterator it = m_timeMap.end();	
	return MidiTime( qMax( MidiTime( (it-1).key() ).getTact() + 1, 1 ), 0 );
}




MidiTime AutomationPattern::putValue( const MidiTime & _time,
							const float _value,
							const bool _quant_pos )
{
	MidiTime newTime = _quant_pos && Engine::automationEditor() ?
		Note::quantized( _time,
			Engine::automationEditor()->quantization() ) :
		_time;

	m_timeMap[newTime] = _value;
	TimeMap::const_iterator it = m_timeMap.find( newTime );
	if( it != m_timeMap.begin() )
	{
		it--;
	}
	if( m_isTempoPattern ) // if we're a tempo pattern, update the cached tempo maps
	{
		if( (it+2) != m_timeMap.end() )
		{
			updateTempoMaps( it.key(), qMin<int>( (it+2).key(), endPosition() ) );
		}
		else
		{
			updateTempoMaps( it.key(), endPosition() );
		}
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
	MidiTime newTime = _quant_pos && Engine::automationEditor() ?
		Note::quantized( _time,
			Engine::automationEditor()->quantization() ) :
		_time;

	m_timeMap.remove( newTime );
	m_tangents.remove( newTime );
	TimeMap::const_iterator it = m_timeMap.lowerBound( newTime );
	if( it != m_timeMap.begin() )
	{
		it--;
	}
	if( m_isTempoPattern ) // if we're a tempo pattern, update the cached tempo maps
	{
		if( (it+2) != m_timeMap.end() )
		{
			updateTempoMaps( it.key(), qMin<int>( (it+2).key(), endPosition() ) );
		}
		else
		{
			updateTempoMaps( it.key(), endPosition() );
		}
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
		MidiTime newTime = _quant_pos && Engine::automationEditor() ?
			Note::quantized( _time,
				Engine::automationEditor()->quantization() ) :
			_time;
		this->removeValue( newTime );
		m_oldTimeMap = m_timeMap;
		m_dragging = true;
	}

	//Restore to the state before it the point were being dragged
	m_timeMap = m_oldTimeMap;

	for( TimeMap::const_iterator it = m_timeMap.begin(); it != m_timeMap.end(); it++ )
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
	TimeMap::ConstIterator v = m_timeMap.lowerBound( _time );

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




float AutomationPattern::valueAt( TimeMap::const_iterator v, int offset ) const
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
	TimeMap::ConstIterator v = m_timeMap.lowerBound( _time );
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
	_this.setAttribute( "len", TrackContentObject::length() );
	_this.setAttribute( "name", name() );
	_this.setAttribute( "prog", QString::number( progressionType() ) );
	_this.setAttribute( "tens", QString::number( getTension() ) );
	
	for( TimeMap::const_iterator it = m_timeMap.begin();
						it != m_timeMap.end(); ++it )
	{
		QDomElement element = _doc.createElement( "time" );
		element.setAttribute( "pos", it.key() );
		element.setAttribute( "value", it.value() );
		_this.appendChild( element );
	}
	
	if( m_isTempoPattern ) { _this.setAttribute( "istempopattern", 1 ); }
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
	}

	int len = _this.attribute( "len" ).toInt();
	if( len <= 0 )
	{
		len = length();
	}
	
	if( _this.hasAttribute( "istempopattern" ) )
	{
		m_isTempoPattern = true;
	}
	
	changeLength( len );
	generateTangents();
	
	if( m_isTempoPattern ) { updateTempoMaps(); }
}




const QString AutomationPattern::name() const
{
	if( ! TrackContentObject::name().isEmpty() )
	{
		return TrackContentObject::name();
	}
	
	if( m_autoTrack->name() != m_autoTrack->defaultName() )
	{
		return m_autoTrack->name();
	}
	
	if( m_isTempoPattern )
	{
		return tr( "Tempo automation" );
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
			if( m_inlineObject )
			{
				m_inlineObject->setAutomatedValue( val );
			}
			else for( objectVector::iterator it = m_autoTrack->objects()->begin();
							it != m_autoTrack->objects()->end(); ++it )
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
		if( time >= 0 && ! m_autoTrack->objects()->isEmpty() )
		{
			const float value = firstObject()->value<float>();
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



TrackContentObjectView * AutomationPattern::createView( TrackView * _tv )
{
	return new AutomationPatternView( this, _tv );
}





/*! \brief returns a list of all the automation patterns of the track that are connected to a specific model
 *  \param _m the model we want to look for
 */
QVector<AutomationPattern *> AutomationPattern::patternsForModel( const AutomatableModel * _m )
{
	QVector<AutomationPattern *> patterns;
	TrackList l;
	l += Engine::getSong()->tracks();
	l += Engine::getBBTrackContainer()->tracks();
	
	// go through all tracks...
	for( TrackList::ConstIterator it = l.begin(); it != l.end(); ++it )
	{
		// we want only automation tracks...
		if( ( *it )->type() == Track::AutomationTrack ||
			( *it )->type() == Track::HiddenAutomationTrack )
		{
			AutomationTrack * at = dynamic_cast<AutomationTrack *>( *it );
			if( at )
			{
				// check the track for the object we want
				for( objectVector::const_iterator k = at->objects()->begin(); k != at->objects()->end(); ++k )
				{
					if( *k == _m )
					{
						// found, so return patterns of the track
						const tcoVector & v = ( *it )->getTCOs();
						for( tcoVector::ConstIterator j = v.begin(); j != v.end(); ++j )
						{
							AutomationPattern * a = dynamic_cast<AutomationPattern *>( *j );
							// check that the pattern has automation
							if( a && a->hasAutomation() )
							{
								patterns += a;
							}
						}
						break;
					}
				}
			}
		}
	}
	return patterns;
}




void AutomationPattern::clear()
{
	m_timeMap.clear();
	m_tangents.clear();

	emit dataChanged();

	if( Engine::automationEditor() &&
		Engine::automationEditor()->currentPattern() == this )
	{
		Engine::automationEditor()->update();
	}
	
	if( m_isTempoPattern ) { updateTempoMaps(); }
}




void AutomationPattern::openInAutomationEditor()
{
	Engine::automationEditor()->setCurrentPattern( this );
	Engine::automationEditor()->parentWidget()->show();
	Engine::automationEditor()->setFocus();
}




void AutomationPattern::generateTangents()
{
	generateTangents(m_timeMap.begin(), m_timeMap.size());
}




void AutomationPattern::generateTangents( TimeMap::const_iterator it,
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



/**
 * @brief Preserves the auto points over different scale
 */
void AutomationPattern::scaleTimemapToFit( float oldMin, float oldMax )
{
	float newMin = getMin();
	float newMax = getMax();

	if( oldMin == newMin && oldMax == newMax )
	{
		return;
	}

	for( TimeMap::iterator it = m_timeMap.begin();
		it != m_timeMap.end(); ++it )
	{
		if( *it < oldMin )
		{
			*it = oldMin;
		}
		else if( *it > oldMax )
		{
			*it = oldMax;
		}
		*it = (*it-oldMin)*(newMax-newMin)/(oldMax-oldMin)+newMin;
	}

	generateTangents();
}


/**
 * @brief Updates the cached tempo & fpt values with the values in this pattern
 */
void AutomationPattern::updateTempoMaps()
{
	updateTempoMaps( startPosition(), endPosition() );
}
void AutomationPattern::updateTempoMaps( tick_t start, tick_t end )
{
	TimeMap * tm = TempoTrack::s_tempoMap;
	TimeMap * fm = TempoTrack::s_fptMap;
	
	if( ! hasAutomation() ) // clear maps from the area of this pattern
	{
		for( int i = start; i < end; ++i )
		{
			tm->remove( i );
			fm->remove( i );
		}
		return;
	}
	
	if( m_progressionType == DiscreteProgression ) // discrete, so we only need the values of the points
	{
		for( int i = start; i < end; ++i )
		{
			const int j = i - startPosition();
			if( m_timeMap.contains( j ) )
			{
				const float val = roundf( m_timeMap.value( j ) );
				tm->insert( i, val );
				fm->insert( i, Engine::tempoToFramesPerTick( val ) );
			}
			else
			{
				tm->remove( i );
				fm->remove( i );
			}
		}
		return;
	}
	
	// linear or cubic progression, so we need all the in-between values too
	for( int i = start; i < end; ++i )
	{
		const int j = i - startPosition();
		const float val = roundf( valueAt( j ) );
		tm->insert( i, val );
		fm->insert( i, Engine::tempoToFramesPerTick( val ) );
	}
}
