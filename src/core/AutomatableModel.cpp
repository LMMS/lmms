/*
 * AutomatableModel.cpp - some implementations of AutomatableModel-class
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "AutomatableModel.h"
#include "AutomationPattern.h"
#include "ControllerConnection.h"


float AutomatableModel::s_copiedValue = 0;




AutomatableModel::AutomatableModel( DataType type,
						const float val, const float min, const float max, const float step,
						Model* parent, const QString & displayName, bool defaultConstructed ) :
	Model( parent, displayName, defaultConstructed ),
	m_dataType( type ),
	m_value( val ),
	m_initValue( val ),
	m_minValue( min ),
	m_maxValue( max ),
	m_step( step ),
	m_range( max - min ),
	m_centerValue( m_minValue ),
	m_journalEntryReady( false ),
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
	if( isAutomated() )
	{
		QDomElement me = doc.createElement( name );
		me.setAttribute( "id", id() );
		me.setAttribute( "value", m_value );
		element.appendChild( me );
	}
	else
	{
		element.setAttribute( name, m_value );
	}

	if( m_controllerConnection )
	{
		QDomElement controllerElement;
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
	}

	QDomNode connectionNode = element.namedItem( "connection" );
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

	node = element.namedItem( name );
	if( node.isElement() )
	{
		changeID( node.toElement().attribute( "id" ).toInt() );
		setValue( node.toElement().attribute( "value" ).toFloat() );
	}
	else if( element.hasAttribute( name ) )
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
		addJournalEntry( JournalEntry( 0, m_value - old_val ) );

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
		emit dataChanged();
	}
	else
	{
		emit dataUnchanged();
	}
	--m_setValueDepth;
}




void AutomatableModel::setAutomatedValue( const float value )
{
	++m_setValueDepth;
	const float oldValue = m_value;

	m_value = fittedValue( value );
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
				(*it)->setAutomatedValue( m_value );
			}
		}
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

	// correct rounding error at the border
	if( qAbs<float>( value - m_maxValue ) < typeInfo<float>::minEps() * qAbs<float>( m_step ) )
	{
		value = m_maxValue;
	}

	// correct rounding error if value = 0
	if( qAbs<float>( value ) < typeInfo<float>::minEps() * qAbs<float>( m_step ) )
	{
		value = 0;
	}

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





void AutomatableModel::redoStep( JournalEntry& je )
{
	bool journalling = testAndSetJournalling( false );
	setValue( value<float>() + (float) je.data().toDouble() );
	setJournalling( journalling );
}




void AutomatableModel::undoStep( JournalEntry& je )
{
	JournalEntry inv( je.actionID(), -je.data().toDouble() );
	redoStep( inv );
}




void AutomatableModel::prepareJournalEntryFromOldVal()
{
	m_oldValue = value<float>();
	saveJournallingState( false );
	m_journalEntryReady = true;
}




void AutomatableModel::addJournalEntryFromOldToCurVal()
{
	if( m_journalEntryReady )
	{
		restoreJournallingState();
		if( value<float>() != m_oldValue )
		{
			addJournalEntry( JournalEntry( 0, value<float>() - m_oldValue ) );
		}
		m_journalEntryReady = false;
	}
}




void AutomatableModel::linkModel( AutomatableModel* model )
{
	if( !m_linkedModels.contains( model ) )
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
		emit dataChanged();
	}
}



float AutomatableModel::controllerValue( int frameOffset ) const
{
	if( m_controllerConnection )
	{
		const float v = minValue<float>() + ( range() * controllerConnection()->currentValue( frameOffset ) );
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




#include "moc_AutomatableModel.cxx"

