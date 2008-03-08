/*
 * automatable_model_templates.h - definition of automatableModel templates
 *
 * Copyright (c) 2007-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _AUTOMATABLE_MODEL_TEMPLATES_H
#define _AUTOMATABLE_MODEL_TEMPLATES_H

#include <QtXml/QDomElement>

#include "automatable_model.h"
#include "automation_editor.h"
#include "automation_pattern.h"
#include "engine.h"
#include "templates.h"



template<typename T, typename EDIT_STEP_TYPE>
automatableModel<T, EDIT_STEP_TYPE>::automatableModel(
						const T _val,
						const T _min,
						const T _max,
						const T _step,
						::model * _parent,
						bool _default_constructed ) :
	model( _parent, _default_constructed ),
	m_value( _val ),
	m_initValue( _val ),
	m_minValue( _min ),
	m_maxValue( _max ),
	m_step( _step ),
	m_automationPattern( NULL ),
	m_track( NULL ),
	m_journalEntryReady( FALSE )
{
	m_curLevel = level( _val );
	m_minLevel = level( _min );
	m_maxLevel = level( _max );
}




template<typename T, typename EDIT_STEP_TYPE>
automatableModel<T, EDIT_STEP_TYPE>::~automatableModel()
{
	delete m_automationPattern;
	while( m_linkedModels.empty() == FALSE )
	{
		m_linkedModels.last()->unlinkModel( this );
		m_linkedModels.erase( m_linkedModels.end() - 1 );
	}
}




template<typename T, typename EDIT_STEP_TYPE>
T automatableModel<T, EDIT_STEP_TYPE>::fittedValue( T _value ) const
{
	_value = tLimit<T>( _value, minValue(), maxValue() );

	if( m_step != 0 )
	{
		_value = static_cast<T>( roundf( _value / (float)step() )
								* step() );
	}
	else
	{
		_value = minValue();
	}

	// correct rounding error at the border
	if( tAbs<T>( _value - maxValue() ) < minEps() * tAbs<T>( step() ) )
	{
		_value = maxValue();
	}

	// correct rounding error if value = 0
	if( tAbs<T>( _value ) < minEps() * tAbs<T>( step() ) )
	{
		_value = 0;
	}

	return( _value );
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableModel<T, EDIT_STEP_TYPE>::setInitValue( const T _value )
{
	m_initValue = _value;
	bool journalling = testAndSetJournalling( FALSE );
	setValue( _value );
	if( m_automationPattern )
	{
		setFirstValue();
	}
	setJournalling( journalling );
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableModel<T, EDIT_STEP_TYPE>::setValue( const T _value )
{
	const T old_val = m_value;

	m_value = fittedValue( _value );
	if( old_val != m_value )
	{
		m_curLevel = level( m_value );

		// add changes to history so user can undo it
		addJournalEntry( journalEntry( 0,
				static_cast<EDIT_STEP_TYPE>( m_value ) -
				static_cast<EDIT_STEP_TYPE>( old_val ) ) );

		// notify linked models

		// doesn't work because of implicit typename T
		// for( autoModelVector::iterator it =
		// 			m_linkedModels.begin();
		//		it != m_linkedModels.end(); ++it )
		for( int i = 0; i < m_linkedModels.size(); ++i )
		{
			autoModel * it = m_linkedModels[i];
			if( value() != it->value() && it->fittedValue( value() )
								!= it->value() )
			{
				bool journalling = it->testAndSetJournalling(
							isJournalling() );
				it->setValue( value() );
				it->setJournalling( journalling );
			}
		}
		setFirstValue();
		emit dataChanged();
	}
	else
	{
		emit dataUnchanged();
	}
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableModel<T, EDIT_STEP_TYPE>::setRange( const T _min, const T _max,
								const T _step )
{
        if( ( m_maxValue != _max ) || ( m_minValue != _min ) )
	{
		m_minValue = _min;
		m_maxValue = _max;
		if( m_minValue > m_maxValue )
		{
			qSwap<T>( m_minValue, m_maxValue );
		}
		setStep( _step );
		// re-adjust value
		autoModel::setInitValue( value() );

		emit propertiesChanged();
	}
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableModel<T, EDIT_STEP_TYPE>::setStep( const T _step )
{
	/*
	const T intv = maxValue() - minValue();

	if( _step == 0 )
	{
		m_step = intv * defaultRelStep();
	}
	else
	{
		if( ( intv > 0 ) && ( _step < 0 ) || ( intv < 0 ) &&
								( _step > 0 ) )
		{
			m_step = -_step;
		}
		else
		{
			m_step = _step;
		}
		if( tAbs<T>( m_step ) < tAbs<T>( minRelStep() * intv ) )
		{
			m_step = minRelStep() * intv;
		}
	}*/
	if( m_step != _step )
	{
		m_step = _step;
		m_curLevel = level( m_value );
		m_minLevel = level( m_minValue );
		m_maxLevel = level( m_maxValue );

		emit propertiesChanged();
	}
}





template<typename T, typename EDIT_STEP_TYPE>
void automatableModel<T, EDIT_STEP_TYPE>::linkModels( autoModel * _model1,
							autoModel * _model2 )
{
	_model1->linkModel( _model2 );
	_model2->linkModel( _model1 );

	if( _model1->m_automationPattern != _model2->m_automationPattern )
	{
		delete _model2->m_automationPattern;
		_model2->m_automationPattern = _model1->m_automationPattern;
	}
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableModel<T, EDIT_STEP_TYPE>::unlinkModels( autoModel * _model1,
							autoModel * _model2 )
{
	_model1->unlinkModel( _model2 );
	_model2->unlinkModel( _model1 );

	if( _model1->m_automationPattern && _model1->m_automationPattern
					== _model2->m_automationPattern )
	{
		_model2->m_automationPattern = new automationPattern(
				*_model1->m_automationPattern, _model2 );
	}
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableModel<T, EDIT_STEP_TYPE>::saveSettings( QDomDocument & _doc,
							QDomElement & _this,
							const QString & _name )
{
	if( m_automationPattern && m_automationPattern->getTimeMap().size()
									> 1 )
	{
		QDomElement pattern_element;
		QDomNode node = _this.namedItem(
					automationPattern::classNodeName() );
		if( node.isElement() )
		{
			pattern_element = node.toElement();
		}
		else
		{
			pattern_element = _doc.createElement(
					automationPattern::classNodeName() );
			_this.appendChild( pattern_element );
		}
		QDomElement element = _doc.createElement( _name );
		m_automationPattern->saveSettings( _doc, element );
		pattern_element.appendChild( element );
	}
	else
	{
		_this.setAttribute( _name, value() );
	}
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableModel<T, EDIT_STEP_TYPE>::loadSettings(
						const QDomElement & _this,
						const QString & _name )
{
	QDomNode node = _this.namedItem( automationPattern::classNodeName() );
	if( node.isElement() && getAutomationPattern() )
	{
		node = node.namedItem( _name );
		if( node.isElement() )
		{
			m_automationPattern->loadSettings( node.toElement() );
			setLevel( m_automationPattern->valueAt( 0 ) );
			return;
		}
	}

	setInitValue( attributeValue( _this.attribute( _name ) ) );
}




template<typename T, typename EDIT_STEP_TYPE>
automationPattern * automatableModel<T, EDIT_STEP_TYPE>::getAutomationPattern(
									void )
{
	if( !m_automationPattern )
	{
		m_automationPattern = new automationPattern( m_track, this );
		setFirstValue();
		syncAutomationPattern();
	}
	return( m_automationPattern );
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableModel<T, EDIT_STEP_TYPE>::redoStep( journalEntry & _je )
{
	bool journalling = testAndSetJournalling( FALSE );
	setValue( static_cast<T>( value() + static_cast<EDIT_STEP_TYPE>(
						_je.data().toDouble() ) ) );
	setJournalling( journalling );
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableModel<T, EDIT_STEP_TYPE>::undoStep( journalEntry & _je )
{
	journalEntry je( _je.actionID(),
		 static_cast<EDIT_STEP_TYPE>( -_je.data().toDouble() ) );
	redoStep( je );
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableModel<T, EDIT_STEP_TYPE>::prepareJournalEntryFromOldVal( void )
{
	m_oldValue = value();
	saveJournallingState( FALSE );
	m_journalEntryReady = TRUE;
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableModel<T, EDIT_STEP_TYPE>::addJournalEntryFromOldToCurVal(
									void )
{
	if( m_journalEntryReady )
	{
		restoreJournallingState();
		if( value() != m_oldValue )
		{
			addJournalEntry( journalEntry( 0, value() -
								m_oldValue ) );
		}
		m_journalEntryReady = FALSE;
	}
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableModel<T, EDIT_STEP_TYPE>::setFirstValue( void )
{
	if( m_automationPattern && m_automationPattern->updateFirst() )
	{
		m_automationPattern->putValue( 0, m_curLevel, FALSE );
		if( engine::getAutomationEditor() &&
				engine::getAutomationEditor()->currentPattern()
						== m_automationPattern )
		{
			engine::getAutomationEditor()->update();
		}
	}
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableModel<T, EDIT_STEP_TYPE>::linkModel( autoModel * _model )
{
	if( qFind( m_linkedModels.begin(), m_linkedModels.end(), _model )
						== m_linkedModels.end() )
	{
		m_linkedModels.push_back( _model );
	}
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableModel<T, EDIT_STEP_TYPE>::unlinkModel( autoModel * _model )
{
	if( qFind( m_linkedModels.begin(), m_linkedModels.end(), _model )
						!= m_linkedModels.end() )
	{
		m_linkedModels.erase( qFind( m_linkedModels.begin(),
							m_linkedModels.end(),
							_model ) );
	}
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableModel<T, EDIT_STEP_TYPE>::syncAutomationPattern( void )
{
	for( int i = 0; i < m_linkedModels.size(); ++i )
	{
		autoModel * it = m_linkedModels[i];
		if( m_automationPattern != it->m_automationPattern )
		{
			it->m_automationPattern = m_automationPattern;
		}
	}
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableModel<T, EDIT_STEP_TYPE>::setLevel( int _level )
{
	if( m_curLevel == _level )
	{
		return;
	}
	bool journalling = testAndSetJournalling( FALSE );
	m_automationPattern->setUpdateFirst( FALSE );
	setValue( _level * m_step );
	m_automationPattern->setUpdateFirst( TRUE );
	setJournalling( journalling );
}




template<>
inline float automatableModel<float>::minRelStep( void )
{
	return( 1.0e-10 );
}




template<>
inline float automatableModel<float>::defaultRelStep( void )
{
	return( 1.0e-2 );
}




template<>
inline float automatableModel<float>::minEps( void )
{
	return( 1.0e-10 );
}




template<>
inline float automatableModel<float>::attributeValue( QString _value )
{
	return( _value.toFloat() );
}




template<>
inline int automatableModel<int>::attributeValue( QString _value )
{
	return( _value.toInt() );
}




template<>
inline bool automatableModel<bool>::attributeValue( QString _value )
{
	return( static_cast<bool>( _value.toInt() ) );
}



template<>
inline bool automatableModel<bool, signed char>::attributeValue(
								QString _value )
{
	return( static_cast<bool>( _value.toInt() ) );
}




#endif

