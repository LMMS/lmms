/*
 * AutomatableModel.cpp - some implementations of AutomatableModel-class
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "AutomatableModel.h"
#include "AutomationPattern.h"
#include "ControllerConnection.h"
#include "lmms_math.h"

float AutomatableModel::s_copiedValue = 0;




AutomatableModel::AutomatableModel( DataType type,
						const float val, const float min, const float max, const float step,
						Model* parent, const QString & displayName, bool defaultConstructed ) :
	Model( parent, displayName, defaultConstructed ),
	m_dataType( type ),
	m_scaleType( Linear ),
	m_value( val ),
	m_initValue( val ),
	m_minValue( min ),
	m_maxValue( max ),
	m_step( step ),
	m_range( max - min ),
	m_centerValue( m_minValue ),
	m_valueChanged( false ),
	m_setValueDepth( 0 ),
	m_hasLinkedModels( false ),
	m_controllerConnection( NULL )
{
	setInitValue( val );
}




AutomatableModel::~AutomatableModel()
{
	while( m_linkedModels.empty() == false )
	{
		m_linkedModels.last()->unlinkModel( this );
		m_linkedModels.erase( m_linkedModels.end() - 1 );
	}

	if( m_controllerConnection )
	{
		delete m_controllerConnection;
	}

	emit destroyed( id() );
}




bool AutomatableModel::isAutomated() const
{
	return AutomationPattern::isAutomated( this );
}




void AutomatableModel::saveSettings( QDomDocument& doc, QDomElement& element, const QString& name )
{
	if( isAutomated() || m_scaleType != Linear )
	{
		// automation needs tuple of data (name, id, value)
		// scale type also needs an extra value
		// => it must be appended as a node
		QDomElement me = doc.createElement( name );
		me.setAttribute( "id", id() );
		me.setAttribute( "value", m_value );
		me.setAttribute( "scale_type", m_scaleType == Logarithmic ? "log" : "linear" );
		element.appendChild( me );
	}
	else
	{
		// non automation, linear scale (default), can be saved as attribute
		element.setAttribute( name, m_value );
	}

	if( m_controllerConnection )
	{
		QDomElement controllerElement;

		// get "connection" element (and create it if needed)
		QDomNode node = element.namedItem( "connection" );
		if( node.isElement() )
		{
			controllerElement = node.toElement();
		}
		else
		{
			controllerElement = doc.createElement( "connection" );
			element.appendChild( controllerElement );
		}

		QDomElement element = doc.createElement( name );
		m_controllerConnection->saveSettings( doc, element );

		controllerElement.appendChild( element );
	}
}




void AutomatableModel::loadSettings( const QDomElement& element, const QString& name )
{
	// compat code
	QDomNode node = element.namedItem( AutomationPattern::classNodeName() );
	if( node.isElement() )
	{
		node = node.namedItem( name );
		if( node.isElement() )
		{
			AutomationPattern * p = AutomationPattern::globalAutomationPattern( this );
			p->loadSettings( node.toElement() );
			setValue( p->valueAt( 0 ) );
			// in older projects we sometimes have odd automations
			// with just one value in - eliminate if necessary
			if( !p->hasAutomation() )
			{
				delete p;
			}
			return;
		}
		// logscales were not existing at this point of time
		// so they can be ignored
	}

	QDomNode connectionNode = element.namedItem( "connection" );
	// reads controller connection
	if( connectionNode.isElement() )
	{
		QDomNode thisConnection = connectionNode.toElement().namedItem( name );
		if( thisConnection.isElement() )
		{
			setControllerConnection( new ControllerConnection( (Controller*)NULL ) );
			m_controllerConnection->loadSettings( thisConnection.toElement() );
			//m_controllerConnection->setTargetName( displayName() );
		}
	}
	
	// models can be stored as elements (port00) or attributes (port10):
	// <ladspacontrols port10="4.41">
	//   <port00 value="4.41" id="4249278"/>
	// </ladspacontrols>
	// element => there is automation data, or scaletype information
	node = element.namedItem( name );
	if( node.isElement() )
	{
			changeID( node.toElement().attribute( "id" ).toInt() );
			setValue( node.toElement().attribute( "value" ).toFloat() );
			if( node.toElement().hasAttribute( "scale_type" ) )
			{
				if( node.toElement().attribute( "scale_type" ) == "linear" )
				{
					setScaleType( Linear );
				}
				else if( node.toElement().attribute( "scale_type" ) == "log" )
				{
					setScaleType( Logarithmic );
				}
			}
	}
	else if( element.hasAttribute( name ) )
	// attribute => read the element's value from the attribute list
	{
		setInitValue( element.attribute( name ).toFloat() );
	}
	else
	{
		reset();
	}
}




void AutomatableModel::setValue( const float value )
{
	++m_setValueDepth;
	const float old_val = m_value;

	m_value = fittedValue( value );
	if( old_val != m_value )
	{
		// add changes to history so user can undo it
		addJournalCheckPoint();

		// notify linked models
		for( AutoModelVector::Iterator it = m_linkedModels.begin(); it != m_linkedModels.end(); ++it )
		{
			if( (*it)->m_setValueDepth < 1 && (*it)->fittedValue( value ) != (*it)->m_value )
			{
				bool journalling = (*it)->testAndSetJournalling( isJournalling() );
				(*it)->setValue( value );
				(*it)->setJournalling( journalling );
			}
		}
		m_valueChanged = true;
		emit dataChanged();
	}
	else
	{
		emit dataUnchanged();
	}
	--m_setValueDepth;
}




template<class T> T AutomatableModel::logToLinearScale( T value ) const
{
	return castValue<T>( ::logToLinearScale( minValue<float>(), maxValue<float>(), static_cast<float>( value ) ) );
}


float AutomatableModel::scaledValue( float value ) const
{
	return m_scaleType == Linear
		? value
		: logToLinearScale<float>( ( value - minValue<float>() ) / m_range );
}


float AutomatableModel::inverseScaledValue( float value ) const
{
	return m_scaleType == Linear
		? value
		: ::linearToLogScale( minValue<float>(), maxValue<float>(), value );
}



QString AutomatableModel::displayValue( const float val ) const
{
	switch( m_dataType )
	{
		case Float: return QString::number( castValue<float>( scaledValue( val ) ) );
		case Integer: return QString::number( castValue<int>( scaledValue( val ) ) );
		case Bool: return QString::number( castValue<bool>( scaledValue( val ) ) );
	}
	return "0";
}



//! @todo: this should be moved into a maths header
template<class T>
void roundAt( T& value, const T& where, const T& step_size )
{
	if( qAbs<float>( value - where )
		< typeInfo<float>::minEps() * qAbs<float>( step_size ) )
	{
		value = where;
	}
}




template<class T>
void AutomatableModel::roundAt( T& value, const T& where ) const
{
	::roundAt(value, where, m_step);
}




void AutomatableModel::setAutomatedValue( const float value )
{
	++m_setValueDepth;
	const float oldValue = m_value;

	const float scaled_value = scaledValue( value );

	m_value = fittedValue( scaled_value );

	if( oldValue != m_value )
	{
		// notify linked models
		for( AutoModelVector::Iterator it = m_linkedModels.begin();
									it != m_linkedModels.end(); ++it )
		{
			if( (*it)->m_setValueDepth < 1 &&
				!(*it)->fittedValue( m_value ) !=
							 (*it)->m_value )
			{
				(*it)->setAutomatedValue( value );
			}
		}
		m_valueChanged = true;
		emit dataChanged();
	}
	--m_setValueDepth;
}




void AutomatableModel::setRange( const float min, const float max,
							const float step )
{
	if( ( m_maxValue != max ) || ( m_minValue != min ) )
	{
		m_minValue = min;
		m_maxValue = max;
		if( m_minValue > m_maxValue )
		{
			qSwap<float>( m_minValue, m_maxValue );
		}
		m_range = m_maxValue - m_minValue;

		setStep( step );

		// re-adjust value
		setValue( value<float>() );

		emit propertiesChanged();
	}
}




void AutomatableModel::setStep( const float step )
{
	if( m_step != step )
	{
		m_step = step;
		emit propertiesChanged();
	}
}




float AutomatableModel::fittedValue( float value ) const
{
	value = tLimit<float>( value, m_minValue, m_maxValue );

	if( m_step != 0 )
	{
		value = nearbyintf( value / m_step ) * m_step;
	}

	roundAt( value, m_maxValue );
	roundAt( value, m_minValue );
	roundAt( value, 0.0f );

	if( value < m_minValue )
	{
		return m_minValue;
	}
	else if( value > m_maxValue )
	{
		return m_maxValue;
	}

	return value;
}





void AutomatableModel::linkModel( AutomatableModel* model )
{
	if( !m_linkedModels.contains( model ) && model != this )
	{
		m_linkedModels.push_back( model );
		m_hasLinkedModels = true;

		if( !model->hasLinkedModels() )
		{
			QObject::connect( this, SIGNAL( dataChanged() ), model, SIGNAL( dataChanged() ) );
		}
	}
}




void AutomatableModel::unlinkModel( AutomatableModel* model )
{
	AutoModelVector::Iterator it = qFind( m_linkedModels.begin(), m_linkedModels.end(), model );
	if( it != m_linkedModels.end() )
	{
		m_linkedModels.erase( it );
	}
	m_hasLinkedModels = !m_linkedModels.isEmpty();
}






void AutomatableModel::linkModels( AutomatableModel* model1, AutomatableModel* model2 )
{
		model1->linkModel( model2 );
		model2->linkModel( model1 );
}




void AutomatableModel::unlinkModels( AutomatableModel* model1, AutomatableModel* model2 )
{
	model1->unlinkModel( model2 );
	model2->unlinkModel( model1 );
}




void AutomatableModel::unlinkAllModels()
{
	foreach( AutomatableModel* model, m_linkedModels )
	{
		unlinkModels( this, model );
	}

	m_hasLinkedModels = false;
}




void AutomatableModel::setControllerConnection( ControllerConnection* c )
{
	m_controllerConnection = c;
	if( c )
	{
		QObject::connect( m_controllerConnection, SIGNAL( valueChanged() ), this, SIGNAL( dataChanged() ) );
		QObject::connect( m_controllerConnection, SIGNAL( destroyed() ), this, SLOT( unlinkControllerConnection() ) );
		m_valueChanged = true;
		emit dataChanged();
	}
}




float AutomatableModel::controllerValue( int frameOffset ) const
{
	if( m_controllerConnection )
	{
		float v = 0;
		switch(m_scaleType)
		{
		case Linear:
			v = minValue<float>() + ( range() * controllerConnection()->currentValue( frameOffset ) );
			break;
		case Logarithmic:
			v = logToLinearScale(
				controllerConnection()->currentValue( frameOffset ));
			break;
		default:
			qFatal("AutomatableModel::controllerValue(int)"
				"lacks implementation for a scale type");
			break;
		}
		if( typeInfo<float>::isEqual( m_step, 1 ) )
		{
			return qRound( v );
		}
		return v;
	}

	AutomatableModel* lm = m_linkedModels.first();
	if( lm->controllerConnection() )
	{
		return fittedValue( lm->controllerValue( frameOffset ) );
	}

	return fittedValue( lm->m_value );
}




void AutomatableModel::unlinkControllerConnection()
{
	if( m_controllerConnection )
	{
		m_controllerConnection->disconnect( this );
	}

	m_controllerConnection = NULL;
}




void AutomatableModel::setInitValue( const float value )
{
	m_initValue = fittedValue( value );
	bool journalling = testAndSetJournalling( false );
	setValue( value );
	setJournalling( journalling );
	emit initValueChanged( value );
}




void AutomatableModel::reset()
{
	setValue( initValue<float>() );
}




void AutomatableModel::copyValue()
{
	s_copiedValue = value<float>();
}




void AutomatableModel::pasteValue()
{
	setValue( copiedValue() );
}



float AutomatableModel::globalAutomationValueAt( const MidiTime& time )
{
	// get patterns that connect to this model
	QVector<AutomationPattern *> patterns = AutomationPattern::patternsForModel( this );
	if( patterns.isEmpty() )
	{
		// if no such patterns exist, return current value
		return m_value;
	}
	else
	{
		// of those patterns:
		// find the patterns which overlap with the miditime position
		QVector<AutomationPattern *> patternsInRange;
		for( QVector<AutomationPattern *>::ConstIterator it = patterns.begin(); it != patterns.end(); it++ )
		{
			int s = ( *it )->startPosition();
			int e = ( *it )->endPosition();
			if( s <= time && e >= time ) { patternsInRange += ( *it ); } 
		}
		
		AutomationPattern * latestPattern = NULL;
		
		if( ! patternsInRange.isEmpty() )
		{
			// if there are more than one overlapping patterns, just use the first one because
			// multiple pattern behaviour is undefined anyway
			latestPattern = patternsInRange[0];
		}
		else
		// if we find no patterns at the exact miditime, we need to search for the last pattern before time and use that
		{
			int latestPosition = 0;
			
			for( QVector<AutomationPattern *>::ConstIterator it = patterns.begin(); it != patterns.end(); it++ )
			{
				int e = ( *it )->endPosition();
				if( e <= time && e > latestPosition )
				{
					latestPosition = e;
					latestPattern = ( *it );
				}
			}
		}
		
		if( latestPattern )
		{
			// scale/fit the value appropriately and return it
			const float value = latestPattern->valueAt( time - latestPattern->startPosition() );
			const float scaled_value = scaledValue( value );
			return fittedValue( scaled_value );
		}
		// if we still find no pattern, the value at that time is undefined so 
		// just return current value as the best we can do
		else return m_value;
	}
}


#include "moc_AutomatableModel.cxx"

