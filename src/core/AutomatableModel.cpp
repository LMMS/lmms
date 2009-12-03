/*
 * AutomatableModel.cpp - some implementations of AutomatableModel-class
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include "automation_recorder.h"
#include "automation_pattern.h"
#include "ControllerConnection.h"


float AutomatableModel::__copiedValue = 0;




AutomatableModel::AutomatableModel( DataType _type,
						const float _val,
						const float _min,
						const float _max,
						const float _step,
						::Model * _parent,
						const QString & _display_name,
						bool _default_constructed ) :
	Model( _parent, _display_name, _default_constructed ),
	m_minValue( _min ),
	m_maxValue( _max ),
	m_dataType( _type ),
	m_value( _val ),
	m_initValue( _val ),
	m_step( _step ),
	m_range( _max - _min ),
	m_journalEntryReady( false ),
	m_setValueDepth( 0 ),
	m_hasLinkedModels( false ),
	m_controllerConnection( NULL ),
	m_armed( false )
{
	// we need to handle our own dataChanged signal so we can
	// alert AutomationRecorder and pass a pointer to this
	QObject::connect( this, SIGNAL( dataChanged() ),
					this, SLOT( handleDataChanged() ) );
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
	return automationPattern::isAutomated( this );
}




void AutomatableModel::saveSettings( QDomDocument & _doc, QDomElement & _this,
							const QString & _name )
{
	if( isAutomated() )
	{
		QDomElement me = _doc.createElement( _name );
		me.setAttribute( "id", id() );
		me.setAttribute( "value", m_value );
		_this.appendChild( me );
	}
	else
	{
		_this.setAttribute( _name, m_value );
	}

	if( m_controllerConnection )
	{
		QDomElement controller_element;
		QDomNode node = _this.namedItem( "connection" );
		if( node.isElement() )
		{
			controller_element = node.toElement();
		}
		else
		{
			controller_element = _doc.createElement( "connection" );
			_this.appendChild( controller_element );
		}
		QDomElement element = _doc.createElement( _name );
		m_controllerConnection->saveSettings( _doc, element );
		controller_element.appendChild( element );
	}
}




void AutomatableModel::loadSettings( const QDomElement & _this,
						const QString & _name )
{
	// compat code
	QDomNode node = _this.namedItem( automationPattern::classNodeName() );
	if( node.isElement() )
	{
		node = node.namedItem( _name );
		if( node.isElement() )
		{
			automationPattern * p = automationPattern::
						globalAutomationPattern( this );
			p->loadSettings( node.toElement() );
			setValue( p->valueAt( 0 ) );
			// in older projects we sometimes have odd automations
			// with just one value in - eliminate if neccessary
			if( !p->hasAutomation() )
			{
				delete p;
			}
			return;
		}
	}

	node = _this.namedItem( _name );
	if( node.isElement() )
	{
		changeID( node.toElement().attribute( "id" ).toInt() );
		setValue( node.toElement().attribute( "value" ).toFloat() );
		return;
	}

	node = _this.namedItem( "connection" );
	if( node.isElement() )
	{
		node = node.namedItem( _name );
		if( node.isElement() )
		{
			setControllerConnection( new ControllerConnection( (Controller*)NULL ) );
			m_controllerConnection->loadSettings( node.toElement() );
			//m_controllerConnection->setTargetName( displayName() );
		}
	}

	setInitValue( _this.attribute( _name ).toFloat() );
}




void AutomatableModel::setValue( const float _value )
{
	++m_setValueDepth;
	const float old_val = m_value;

	m_value = fittedValue( _value );
	if( old_val != m_value )
	{
		// add changes to history so user can undo it
		addJournalEntry( JournalEntry( 0, m_value - old_val ) );

		// notify linked models
		for( AutoModelVector::Iterator it =
		 			m_linkedModels.begin();
				it != m_linkedModels.end(); ++it )
		{
			if( (*it)->m_setValueDepth < 1 &&
				(*it)->fittedValue( _value ) !=
							 (*it)->m_value )
			{
				bool journalling = (*it)->testAndSetJournalling(
							isJournalling() );
				(*it)->setValue( _value );
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




void AutomatableModel::setAutomatedValue( const float _value )
{
	++m_setValueDepth;
	const float old_val = m_value;

	m_value = fittedValue( _value );
	if( old_val != m_value )
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




void AutomatableModel::setRange( const float _min, const float _max,
							const float _step )
{
        if( ( m_maxValue != _max ) || ( m_minValue != _min ) )
	{
		m_minValue = _min;
		m_maxValue = _max;
		if( m_minValue > m_maxValue )
		{
			qSwap<float>( m_minValue, m_maxValue );
		}
		m_range = m_maxValue - m_minValue;
		setStep( _step );

		// re-adjust value
		setInitValue( value<float>() );

		emit propertiesChanged();
	}
}




void AutomatableModel::setStep( const float _step )
{
	if( m_step != _step )
	{
		m_step = _step;
		emit propertiesChanged();
	}
}




float AutomatableModel::fittedValue( float _value ) const
{
	_value = tLimit<float>( _value, m_minValue, m_maxValue );

	if( m_step != 0 )
	{
		_value = roundf( _value / m_step ) * m_step;
	}
	else
	{
		_value = m_minValue;
	}

	// correct rounding error at the border
	if( qAbs<float>( _value - m_maxValue ) <
			typeInfo<float>::minEps() * qAbs<float>( m_step ) )
	{
		_value = m_maxValue;
	}

	// correct rounding error if value = 0
	if( qAbs<float>( _value ) < typeInfo<float>::minEps() *
							qAbs<float>( m_step ) )
	{
		_value = 0;
	}

	if( _value < m_minValue )
	{
		return m_minValue;
	}
	else if( _value > m_maxValue )
	{
		return m_maxValue;
	}

	return _value;
}





void AutomatableModel::redoStep( JournalEntry & _je )
{
	bool journalling = testAndSetJournalling( false );
	setValue( value<float>() + (float) _je.data().toDouble() );
	setJournalling( journalling );
}




void AutomatableModel::undoStep( JournalEntry & _je )
{
	JournalEntry je( _je.actionID(), -_je.data().toDouble() );
	redoStep( je );
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
			addJournalEntry( JournalEntry( 0, value<float>() -
								m_oldValue ) );
		}
		m_journalEntryReady = false;
	}
}




void AutomatableModel::linkModel( AutomatableModel * _model )
{
	if( !m_linkedModels.contains( _model ) )
	{
		m_linkedModels.push_back( _model );
		m_hasLinkedModels = true;
		if( !_model->m_hasLinkedModels )
		{
			QObject::connect( this, SIGNAL( dataChanged() ),
					_model, SIGNAL( dataChanged() ) );
		}
	}
}




void AutomatableModel::unlinkModel( AutomatableModel * _model )
{
	AutoModelVector::Iterator it =
		qFind( m_linkedModels.begin(), m_linkedModels.end(), _model );
	if( it != m_linkedModels.end() )
	{
		m_linkedModels.erase( it );
	}
	m_hasLinkedModels = !m_linkedModels.isEmpty();
}






void AutomatableModel::linkModels( AutomatableModel * _model1,
						AutomatableModel * _model2 )
{
	_model1->linkModel( _model2 );
	_model2->linkModel( _model1 );
}




void AutomatableModel::unlinkModels( AutomatableModel * _model1,
						AutomatableModel * _model2 )
{
	_model1->unlinkModel( _model2 );
	_model2->unlinkModel( _model1 );
}




void AutomatableModel::setControllerConnection( ControllerConnection * _c )
{
	m_controllerConnection = _c;
	if( _c )
	{
		QObject::connect( m_controllerConnection,
						SIGNAL( valueChanged() ),
					this, SIGNAL( dataChanged() ) );
		QObject::connect( m_controllerConnection,
						SIGNAL( destroyed() ),
				this, SLOT( unlinkControllerConnection() ) );
		emit dataChanged();
	}
}



float AutomatableModel::controllerValue( int _frameOffset ) const
{
	if( m_controllerConnection )
	{
		const float v = m_minValue +
			( m_range * m_controllerConnection->currentValue(
							_frameOffset ) );
		if( typeInfo<float>::isEqual( m_step, 1 ) )
		{
			return qRound( v );
		}
		return v;
	}
	AutomatableModel * lm = m_linkedModels.first();
	if( lm->m_controllerConnection )
	{
		return lm->controllerValue( _frameOffset );
	}
	return lm->m_value;
}




void AutomatableModel::unlinkControllerConnection()
{
	if( m_controllerConnection )
	{
		m_controllerConnection->disconnect( this );
	}

	m_controllerConnection = NULL;
}



void AutomatableModel::handleDataChanged()
{
	// report the data changed to AutomationRecorder
	engine::getAutomationRecorder()->modelDataEvent( this );
}



void AutomatableModel::setInitValue( const float _value )
{
	m_initValue = _value;
	bool journalling = testAndSetJournalling( false );
	setValue( _value );
	setJournalling( journalling );
	emit initValueChanged( _value );
}




void AutomatableModel::reset()
{
	setValue( initValue<float>() );
}




void AutomatableModel::copyValue()
{
	__copiedValue = value<float>();
}




void AutomatableModel::pasteValue()	
{
	setValue( __copiedValue );
}




#include "moc_AutomatableModel.cxx"

