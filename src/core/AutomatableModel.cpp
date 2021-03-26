/*
 * AutomatableModel.cpp - some implementations of AutomatableModel-class
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "AutomatableModel.h"

#include "lmms_math.h"

#include "AutomationPattern.h"
#include "ControllerConnection.h"
#include "LocaleHelper.h"
#include "Mixer.h"
#include "ProjectJournal.h"
#include "Song.h"

long AutomatableModel::s_periodCounter = 0;



AutomatableModel::AutomatableModel(
						const float val, const float min, const float max, const float step,
						Model* parent, const QString & displayName, bool defaultConstructed ) :
	Model( parent, displayName, defaultConstructed ),
	m_scaleType( Linear ),
	m_minValue( min ),
	m_maxValue( max ),
	m_step( step ),
	m_range( max - min ),
	m_centerValue( m_minValue ),
	m_valueChanged( false ),
	m_setValueDepth( 0 ),
	m_hasStrictStepSize( false ),
	m_controllerConnection( NULL ),
	m_valueBuffer( static_cast<int>( Engine::mixer()->framesPerPeriod() ) ),
	m_lastUpdatedPeriod( -1 ),
	m_hasSampleExactData(false),
	m_useControllerValue(true)

{
	m_value = fittedValue( val );
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

	m_valueBuffer.clear();

	emit destroyed( id() );
}




bool AutomatableModel::isAutomated() const
{
	return AutomationPattern::isAutomated( this );
}



bool AutomatableModel::mustQuoteName(const QString& name)
{
	QRegExp reg("^[A-Za-z0-9._-]+$");
	return !reg.exactMatch(name);
}

void AutomatableModel::saveSettings( QDomDocument& doc, QDomElement& element, const QString& name )
{
	bool mustQuote = mustQuoteName(name);

	if( isAutomated() || m_scaleType != Linear )
	{
		// automation needs tuple of data (name, id, value)
		// scale type also needs an extra value
		// => it must be appended as a node

		QDomElement me = doc.createElement( mustQuote ? QString("automatablemodel") : name );
		me.setAttribute( "id", ProjectJournal::idToSave( id() ) );
		me.setAttribute( "value", m_value );
		me.setAttribute( "scale_type", m_scaleType == Logarithmic ? "log" : "linear" );
		if(mustQuote) {
			me.setAttribute( "nodename", name );
		}
		element.appendChild( me );
	}
	else
	{
		if(mustQuote)
		{
			QDomElement me = doc.createElement( "automatablemodel" );
			me.setAttribute( "nodename", name );
			me.setAttribute( "value", m_value );
			element.appendChild( me );
		}
		else
		{
			// non automation, linear scale (default), can be saved as attribute
			element.setAttribute( name, m_value );
		}
	}

	// Skip saving MIDI connections if we're saving project and
	// the discardMIDIConnections option is true.
	auto controllerType = m_controllerConnection
			? m_controllerConnection->getController()->type()
			: Controller::DummyController;
	bool skipMidiController = Engine::getSong()->isSavingProject()
							  && Engine::getSong()->getSaveOptions().discardMIDIConnections.value();
	if (m_controllerConnection && controllerType != Controller::DummyController
		&& !(skipMidiController && controllerType == Controller::MidiController))
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

		bool mustQuote = mustQuoteName(name);
		QString elementName = mustQuote ? "controllerconnection"
						: name;

		QDomElement element = doc.createElement( elementName );
		if(mustQuote)
			element.setAttribute( "nodename", name );
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
		if( !thisConnection.isElement() )
		{
			thisConnection = connectionNode.toElement().namedItem( "controllerconnection" );
			QDomElement tcElement = thisConnection.toElement();
			// sanity check
			if( tcElement.isNull() || tcElement.attribute( "nodename" ) != name )
			{
				// no, that wasn't it, act as if we never found one
				thisConnection.clear();
			}
		}
		if( thisConnection.isElement() )
		{
			setControllerConnection(new ControllerConnection((Controller*)NULL, this));
			m_controllerConnection->loadSettings( thisConnection.toElement() );
			//m_controllerConnection->setTargetName( displayName() );
		}
	}

	// models can be stored as elements (port00) or attributes (port10):
	// <ladspacontrols port10="4.41">
	//   <port00 value="4.41" id="4249278"/>
	// </ladspacontrols>
	// element => there is automation data, or scaletype information

	node = element.namedItem( name ); // maybe we have luck?

	// either: no node with name "name" found
	//  => look for nodes with attribute name="nodename"
	// or: element with namedItem() "name" was found, but it's real nodename
	// is given as attribute and does not match
	//  => look for the right node
	if(node.isNull() ||
		( node.isElement() &&
		node.toElement().hasAttribute("nodename") &&
		node.toElement().attribute("nodename") != name))
	{
		for(QDomElement othernode = element.firstChildElement();
			!othernode.isNull();
			othernode = othernode.nextSiblingElement())
		{
			if((!othernode.hasAttribute("nodename") &&
				othernode.nodeName() == name) ||
				othernode.attribute("nodename") == name)
			{
				node = othernode;
				break;
			}
		}
	}
	if( node.isElement() )
	{
		QDomElement nodeElement = node.toElement();
		changeID( nodeElement.attribute( "id" ).toInt() );
		setValue( LocaleHelper::toFloat( nodeElement.attribute( "value" ) ) );
		if( nodeElement.hasAttribute( "scale_type" ) )
		{
			if( nodeElement.attribute( "scale_type" ) == "linear" )
			{
				setScaleType( Linear );
			}
			else if( nodeElement.attribute( "scale_type" ) == "log" )
			{
				setScaleType( Logarithmic );
			}
		}
	}
	else
	{

		setScaleType( Linear );

		if( element.hasAttribute( name ) )
			// attribute => read the element's value from the attribute list
		{
			setInitValue( LocaleHelper::toFloat( element.attribute( name ) ) );
		}
		else
		{
			reset();
		}
	}
}




void AutomatableModel::setValue( const float value )
{
	m_oldValue = m_value;
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
	setUseControllerValue(false);

	m_oldValue = m_value;
	++m_setValueDepth;
	const float oldValue = m_value;

	const float scaled_value = scaledValue( value );

	m_value = fittedValue( scaled_value );

	if( oldValue != m_value )
	{
		// notify linked models
		for (AutoModelVector::Iterator it = m_linkedModels.begin();
			it != m_linkedModels.end(); ++it)
		{
			if (!((*it)->controllerConnection()) && (*it)->m_setValueDepth < 1 &&
					(*it)->fittedValue(m_value) != (*it)->m_value)
			{
				(*it)->setAutomatedValue(value);
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
	value = qBound<float>( m_minValue, value, m_maxValue );

	if( m_step != 0 && m_hasStrictStepSize )
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

		if( !model->hasLinkedModels() )
		{
			QObject::connect( this, SIGNAL( dataChanged() ),
					model, SIGNAL( dataChanged() ), Qt::DirectConnection );
		}
	}
}




void AutomatableModel::unlinkModel( AutomatableModel* model )
{
	AutoModelVector::Iterator it = std::find( m_linkedModels.begin(), m_linkedModels.end(), model );
	if( it != m_linkedModels.end() )
	{
		m_linkedModels.erase( it );
	}
}






void AutomatableModel::linkModels( AutomatableModel* model1, AutomatableModel* model2 )
{
	if (!model1->m_linkedModels.contains( model2 ) && model1 != model2)
	{
		// copy data
		model1->m_value = model2->m_value;
		if (model1->valueBuffer() && model2->valueBuffer())
		{
			std::copy_n(model2->valueBuffer()->data(),
				model1->valueBuffer()->length(),
				model1->valueBuffer()->data());
		}
		// send dataChanged() before linking (because linking will
		// connect the two dataChanged() signals)
		emit model1->dataChanged();
		// finally: link the models
		model1->linkModel( model2 );
		model2->linkModel( model1 );
	}
}




void AutomatableModel::unlinkModels( AutomatableModel* model1, AutomatableModel* model2 )
{
	model1->unlinkModel( model2 );
	model2->unlinkModel( model1 );
}




void AutomatableModel::unlinkAllModels()
{
	for( AutomatableModel* model : m_linkedModels )
	{
		unlinkModels( this, model );
	}
}




void AutomatableModel::setControllerConnection( ControllerConnection* c )
{
	m_controllerConnection = c;
	if( c )
	{
		QObject::connect( m_controllerConnection, SIGNAL( valueChanged() ),
				this, SIGNAL( dataChanged() ), Qt::DirectConnection );
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
		if( typeInfo<float>::isEqual( m_step, 1 ) && m_hasStrictStepSize )
		{
			return qRound( v );
		}
		return v;
	}

	AutomatableModel* lm = m_linkedModels.first();
	if (lm->controllerConnection() && lm->useControllerValue())
	{
		return fittedValue( lm->controllerValue( frameOffset ) );
	}

	return fittedValue( lm->m_value );
}


ValueBuffer * AutomatableModel::valueBuffer()
{
	QMutexLocker m( &m_valueBufferMutex );
	// if we've already calculated the valuebuffer this period, return the cached buffer
	if( m_lastUpdatedPeriod == s_periodCounter )
	{
		return m_hasSampleExactData
			? &m_valueBuffer
			: NULL;
	}

	float val = m_value; // make sure our m_value doesn't change midway

	ValueBuffer * vb;
	if (m_controllerConnection && m_useControllerValue && m_controllerConnection->getController()->isSampleExact())
	{
		vb = m_controllerConnection->valueBuffer();
		if( vb )
		{
			float * values = vb->values();
			float * nvalues = m_valueBuffer.values();
			switch( m_scaleType )
			{
			case Linear:
				for( int i = 0; i < m_valueBuffer.length(); i++ )
				{
					nvalues[i] = minValue<float>() + ( range() * values[i] );
				}
				break;
			case Logarithmic:
				for( int i = 0; i < m_valueBuffer.length(); i++ )
				{
					nvalues[i] = logToLinearScale( values[i] );
				}
				break;
			default:
				qFatal("AutomatableModel::valueBuffer() "
					"lacks implementation for a scale type");
				break;
			}
			m_lastUpdatedPeriod = s_periodCounter;
			m_hasSampleExactData = true;
			return &m_valueBuffer;
		}
	}

	if (!m_controllerConnection)
	{
		AutomatableModel* lm = NULL;
		if (hasLinkedModels())
		{
			lm = m_linkedModels.first();
		}
		if (lm && lm->controllerConnection() && lm->useControllerValue() &&
				lm->controllerConnection()->getController()->isSampleExact())
		{
			vb = lm->valueBuffer();
			float * values = vb->values();
			float * nvalues = m_valueBuffer.values();
			for (int i = 0; i < vb->length(); i++)
			{
				nvalues[i] = fittedValue(values[i]);
			}
			m_lastUpdatedPeriod = s_periodCounter;
			m_hasSampleExactData = true;
			return &m_valueBuffer;
		}
	}

	if( m_oldValue != val )
	{
		m_valueBuffer.interpolate( m_oldValue, val );
		m_oldValue = val;
		m_lastUpdatedPeriod = s_periodCounter;
		m_hasSampleExactData = true;
		return &m_valueBuffer;
	}

	// if we have no sample-exact source for a ValueBuffer, return NULL to signify that no data is available at the moment
	// in which case the recipient knows to use the static value() instead
	m_lastUpdatedPeriod = s_periodCounter;
	m_hasSampleExactData = false;
	return NULL;
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
	m_oldValue = m_value;
	setJournalling( journalling );
	emit initValueChanged( value );
}




void AutomatableModel::reset()
{
	setValue( initValue<float>() );
}




float AutomatableModel::globalAutomationValueAt( const TimePos& time )
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
		// find the patterns which overlap with the time position
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
		// if we find no patterns at the exact time, we need to search for the last pattern before time and use that
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

void AutomatableModel::setUseControllerValue(bool b)
{
	if (b)
	{
		m_useControllerValue = true;
		emit dataChanged();
	}
	else if (m_controllerConnection && m_useControllerValue)
	{
		m_useControllerValue = false;
		emit dataChanged();
	}
}

float FloatModel::getRoundedValue() const
{
	return qRound( value() / step<float>() ) * step<float>();
}




int FloatModel::getDigitCount() const
{
	float steptemp = step<float>();
	int digits = 0;
	while ( steptemp < 1 )
	{
		steptemp = steptemp * 10.0f;
		digits++;
	}
	return digits;
}



QString FloatModel::displayValue( const float val ) const
{
	return QString::number( castValue<float>( scaledValue( val ) ) );
}

QString IntModel::displayValue( const float val ) const
{
	return QString::number( castValue<int>( scaledValue( val ) ) );
}

QString BoolModel::displayValue( const float val ) const
{
	return QString::number( castValue<bool>( scaledValue( val ) ) );
}
