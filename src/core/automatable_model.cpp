/*
 * automatable_model.cpp - some implementations of automatableModel-class
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "automatable_model.h"
#include "automation_pattern.h"
#include "controller_connection.h"


float automatableModel::__copiedValue = 0;



template<>
inline float automatableModel::minEps<float>( void )
{
	return( 1.0e-10 );
}




automatableModel::automatableModel( DataType _type,
						const float _val,
						const float _min,
						const float _max,
						const float _step,
						::model * _parent,
						const QString & _display_name,
						bool _default_constructed ) :
	model( _parent, _display_name, _default_constructed ),
	m_dataType( _type ),
	m_value( _val ),
	m_initValue( _val ),
	m_minValue( _min ),
	m_maxValue( _max ),
	m_step( _step ),
	m_range( _max - _min ),
	m_journalEntryReady( FALSE ),
	m_controllerConnection( NULL )
{
}




automatableModel::~automatableModel()
{
	while( m_linkedModels.empty() == FALSE )
	{
		m_linkedModels.last()->unlinkModel( this );
		m_linkedModels.erase( m_linkedModels.end() - 1 );
	}

	if( m_controllerConnection )
	{
		delete m_controllerConnection;
	}
}




bool automatableModel::isAutomated( void ) const
{
	return( automationPattern::isAutomated( this ) );
}




void automatableModel::saveSettings( QDomDocument & _doc, QDomElement & _this,
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




void automatableModel::loadSettings( const QDomElement & _this,
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
			setControllerConnection( new controllerConnection( (controller*)NULL ) );
			m_controllerConnection->loadSettings( node.toElement() );
			//m_controllerConnection->setTargetName( displayName() );
		}
	}

	setInitValue( _this.attribute( _name ).toFloat() );
}




void automatableModel::setValue( const float _value )
{
	const float old_val = m_value;

	m_value = fittedValue( _value );
	if( old_val != m_value )
	{
		// add changes to history so user can undo it
		addJournalEntry( journalEntry( 0, m_value - old_val ) );

		// notify linked models
		for( autoModelVector::iterator it =
		 			m_linkedModels.begin();
				it != m_linkedModels.end(); ++it )
		{
			if( value<float>() != (*it)->value<float>() &&
				(*it)->fittedValue( value<float>() )
						!= (*it)->value<float>() )
			{
				bool journalling = (*it)->testAndSetJournalling(
							isJournalling() );
				(*it)->setValue( value<float>() );
				(*it)->setJournalling( journalling );
			}
		}
		emit dataChanged();
	}
	else
	{
		emit dataUnchanged();
	}
}




void automatableModel::setAutomatedValue( const float _value )
{
	const float old_val = m_value;

	m_value = fittedValue( _value );
	if( old_val != m_value )
	{
		// notify linked models
		for( autoModelVector::iterator it =
		 			m_linkedModels.begin();
				it != m_linkedModels.end(); ++it )
		{
			if( value<float>() != (*it)->value<float>() &&
				(*it)->fittedValue( value<float>() )
						!= (*it)->value<float>() )
			{
				(*it)->setAutomatedValue( value<float>() );
			}
		}
		emit dataChanged();
	}
}




void automatableModel::setRange( const float _min, const float _max,
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




void automatableModel::setStep( const float _step )
{
	if( m_step != _step )
	{
		m_step = _step;
		emit propertiesChanged();
	}
}




float automatableModel::fittedValue( float _value ) const
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
	if( tAbs<float>( _value - m_maxValue ) <
				minEps<float>() * tAbs<float>( m_step ) )
	{
		_value = m_maxValue;
	}

	// correct rounding error if value = 0
	if( tAbs<float>( _value ) < minEps<float>() * tAbs<float>( m_step ) )
	{
		_value = 0;
	}

	return( _value );
}





void automatableModel::redoStep( journalEntry & _je )
{
	bool journalling = testAndSetJournalling( FALSE );
	setValue( value<float>() + _je.data().toDouble() );
	setJournalling( journalling );
}




void automatableModel::undoStep( journalEntry & _je )
{
	journalEntry je( _je.actionID(), -_je.data().toDouble() );
	redoStep( je );
}




void automatableModel::prepareJournalEntryFromOldVal( void )
{
	m_oldValue = value<float>();
	saveJournallingState( FALSE );
	m_journalEntryReady = TRUE;
}




void automatableModel::addJournalEntryFromOldToCurVal( void )
{
	if( m_journalEntryReady )
	{
		restoreJournallingState();
		if( value<float>() != m_oldValue )
		{
			addJournalEntry( journalEntry( 0, value<float>() -
								m_oldValue ) );
		}
		m_journalEntryReady = FALSE;
	}
}




void automatableModel::linkModel( automatableModel * _model )
{
	if( qFind( m_linkedModels.begin(), m_linkedModels.end(), _model )
						== m_linkedModels.end() )
	{
		m_linkedModels.push_back( _model );
	}
}




void automatableModel::unlinkModel( automatableModel * _model )
{
	if( qFind( m_linkedModels.begin(), m_linkedModels.end(), _model )
						!= m_linkedModels.end() )
	{
		m_linkedModels.erase( qFind( m_linkedModels.begin(),
							m_linkedModels.end(),
							_model ) );
	}
}






void automatableModel::linkModels( automatableModel * _model1,
						automatableModel * _model2 )
{
	_model1->linkModel( _model2 );
	_model2->linkModel( _model1 );
}




void automatableModel::unlinkModels( automatableModel * _model1,
						automatableModel * _model2 )
{
	_model1->unlinkModel( _model2 );
	_model2->unlinkModel( _model1 );
}




void automatableModel::setControllerConnection( controllerConnection * _c )
{
	m_controllerConnection = _c;
	if( _c )
	{
		QObject::connect( m_controllerConnection,
						SIGNAL( valueChanged() ),
					this, SIGNAL( dataChanged() ) );
		emit dataChanged();
	}
}




void automatableModel::setInitValue( const float _value )
{
	m_initValue = _value;
	bool journalling = testAndSetJournalling( FALSE );
	setValue( _value );
	setJournalling( journalling );
	emit initValueChanged( _value );
}




void automatableModel::reset( void )
{
	setValue( initValue<float>() );
}




void automatableModel::copyValue( void )
{
	__copiedValue = value<float>();
}




void automatableModel::pasteValue( void )	
{
	setValue( __copiedValue );
}




#include "moc_automatable_model.cxx"

