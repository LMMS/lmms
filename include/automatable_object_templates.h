/*
 * automatable_object_templates.h - definition of automatableObject templates
 *
 * Copyright (c) 2006-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _AUTOMATABLE_OBJECT_TEMPLATES_H
#define _AUTOMATABLE_OBJECT_TEMPLATES_H


#include "automatable_object.h"
#include "automation_editor.h"
#include "automation_pattern.h"
#include "engine.h"
#include "templates.h"

#ifndef QT3

#include <QtXml/QDomElement>

#else

#include <qdom.h>

#endif


template<typename T, typename EDIT_STEP_TYPE>
automatableObject<T, EDIT_STEP_TYPE>::automatableObject( track * _track,
						const T _val, const T _min,
						const T _max, const T _step ) :
	m_value( _val ),
	m_minValue( _min ),
	m_maxValue( _max ),
	m_step( _step ),
	m_automation_pattern( NULL ),
	m_track( _track ),
	m_journalEntryReady( FALSE )
{
	m_curLevel = level( _val );
	m_minLevel = level( _min );
	m_maxLevel = level( _max );
}




template<typename T, typename EDIT_STEP_TYPE>
automatableObject<T, EDIT_STEP_TYPE>::~automatableObject()
{
	delete m_automation_pattern;
	while( m_linkedObjects.empty() == FALSE )
	{
		m_linkedObjects.last()->unlinkObject( this );
		m_linkedObjects.erase( m_linkedObjects.end() - 1 );
	}
}




template<typename T, typename EDIT_STEP_TYPE>
T automatableObject<T, EDIT_STEP_TYPE>::fittedValue( T _value ) const
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
void automatableObject<T, EDIT_STEP_TYPE>::setInitValue( const T _value )
{
	bool journalling = testAndSetJournalling( FALSE );
	setValue( _value );
	if( m_automation_pattern )
	{
		setFirstValue();
	}
	setJournalling( journalling );
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableObject<T, EDIT_STEP_TYPE>::setValue( const T _value )
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

		// notify linked objects

		// doesn't work because of implicit typename T
		// for( autoObjVector::iterator it =
		// 			m_linkedObjects.begin();
		//		it != m_linkedObjects.end(); ++it )
		for( int i = 0; i < m_linkedObjects.size(); ++i )
		{
			autoObj * it = m_linkedObjects[i];
			if( value() != it->value() && it->fittedValue( value() )
								!= it->value() )
			{
				bool journalling = it->testAndSetJournalling(
							isJournalling() );
				it->setValue( value() );
				it->setJournalling( journalling );
			}
		}
	}
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableObject<T, EDIT_STEP_TYPE>::setRange( const T _min, const T _max,
								const T _step )
{
	m_minValue = _min;
	m_maxValue = _max;
	if( m_minValue > m_maxValue )
	{
		qSwap<T>( m_minValue, m_maxValue );
	}
	setStep( _step );
	// re-adjust value
	autoObj::setInitValue( value() );
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableObject<T, EDIT_STEP_TYPE>::setStep( const T _step )
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
	m_step = _step;
	m_curLevel = level( m_value );
	m_minLevel = level( m_minValue );
	m_maxLevel = level( m_maxValue );
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableObject<T, EDIT_STEP_TYPE>::linkObjects( autoObj * _object1,
							autoObj * _object2 )
{
	_object1->linkObject( _object2 );
	_object2->linkObject( _object1 );

	if( _object1->m_automation_pattern != _object2->m_automation_pattern )
	{
		delete _object2->m_automation_pattern;
		_object2->m_automation_pattern = _object1->m_automation_pattern;
	}
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableObject<T, EDIT_STEP_TYPE>::unlinkObjects( autoObj * _object1,
							autoObj * _object2 )
{
	_object1->unlinkObject( _object2 );
	_object2->unlinkObject( _object1 );

	if( _object1->m_automation_pattern && _object1->m_automation_pattern
					== _object2->m_automation_pattern )
	{
		_object2->m_automation_pattern = new automationPattern(
				*_object1->m_automation_pattern, _object2 );
	}
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableObject<T, EDIT_STEP_TYPE>::saveSettings( QDomDocument & _doc,
							QDomElement & _this,
							const QString & _name )
{
	if( m_automation_pattern && m_automation_pattern->getTimeMap().size()
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
		m_automation_pattern->saveSettings( _doc, element );
		pattern_element.appendChild( element );
	}
	else
	{
		_this.setAttribute( _name, value() );
	}
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableObject<T, EDIT_STEP_TYPE>::loadSettings(
						const QDomElement & _this,
						const QString & _name )
{
	QDomNode node = _this.namedItem( automationPattern::classNodeName() );
	if( node.isElement() )
	{
		node = node.namedItem( _name );
		if( node.isElement() )
		{
			m_automation_pattern->loadSettings( node.toElement() );
			setLevel( m_automation_pattern->valueAt( 0 ) );
			return;
		}
	}

	setInitValue( attributeValue( _this.attribute( _name ) ) );
}




template<typename T, typename EDIT_STEP_TYPE>
automationPattern * automatableObject<T, EDIT_STEP_TYPE>::getAutomationPattern(
									void )
{
	if( !m_automation_pattern )
	{
		m_automation_pattern = new automationPattern( m_track, this );
		syncAutomationPattern();
	}
	return( m_automation_pattern );
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableObject<T, EDIT_STEP_TYPE>::redoStep( journalEntry & _je )
{
	bool journalling = testAndSetJournalling( FALSE );
/*#ifndef QT3
	setValue( static_cast<T>( value() +
					_je.data().value<EDIT_STEP_TYPE>() ) );
#else*/
	setValue( static_cast<T>( value() + static_cast<EDIT_STEP_TYPE>(
						_je.data().toDouble() ) ) );
//#endif
	setJournalling( journalling );
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableObject<T, EDIT_STEP_TYPE>::undoStep( journalEntry & _je )
{
	journalEntry je( _je.actionID(),
/*#ifndef QT3
					-_je.data().value<EDIT_STEP_TYPE>()
#else*/
			static_cast<EDIT_STEP_TYPE>( -_je.data().toDouble() )
//#endif
				);
	redoStep( je );
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableObject<T, EDIT_STEP_TYPE>::prepareJournalEntryFromOldVal( void )
{
	m_oldValue = value();
	saveJournallingState( FALSE );
	m_journalEntryReady = TRUE;
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableObject<T, EDIT_STEP_TYPE>::addJournalEntryFromOldToCurVal(
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
void automatableObject<T, EDIT_STEP_TYPE>::setFirstValue( void )
{
	if( m_automation_pattern && m_automation_pattern->updateFirst() )
	{
		m_automation_pattern->putValue( 0, m_curLevel, FALSE );
		if( engine::getAutomationEditor() &&
				engine::getAutomationEditor()->currentPattern()
						== m_automation_pattern )
		{
			engine::getAutomationEditor()->update();
		}
	}
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableObject<T, EDIT_STEP_TYPE>::linkObject( autoObj * _object )
{
	if( qFind( m_linkedObjects.begin(), m_linkedObjects.end(), _object )
						== m_linkedObjects.end() )
	{
		m_linkedObjects.push_back( _object );
	}
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableObject<T, EDIT_STEP_TYPE>::unlinkObject( autoObj * _object )
{
	if( qFind( m_linkedObjects.begin(), m_linkedObjects.end(), _object )
						!= m_linkedObjects.end() )
	{
		m_linkedObjects.erase( qFind( m_linkedObjects.begin(),
							m_linkedObjects.end(),
							_object ) );
	}
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableObject<T, EDIT_STEP_TYPE>::syncAutomationPattern( void )
{
	for( int i = 0; i < m_linkedObjects.size(); ++i )
	{
		autoObj * it = m_linkedObjects[i];
		if( m_automation_pattern != it->m_automation_pattern )
		{
			it->m_automation_pattern = m_automation_pattern;
		}
	}
}




template<typename T, typename EDIT_STEP_TYPE>
void automatableObject<T, EDIT_STEP_TYPE>::setLevel( int _level )
{
	if( m_curLevel == _level )
	{
		return;
	}
	bool journalling = testAndSetJournalling( FALSE );
	m_automation_pattern->setUpdateFirst( FALSE );
	setValue( _level * m_step );
	m_automation_pattern->setUpdateFirst( TRUE );
	setJournalling( journalling );
}




template<>
inline float automatableObject<float>::minRelStep( void )
{
	return( 1.0e-10 );
}




template<>
inline float automatableObject<float>::defaultRelStep( void )
{
	return( 1.0e-2 );
}




template<>
inline float automatableObject<float>::minEps( void )
{
	return( 1.0e-10 );
}




template<>
inline float automatableObject<float>::attributeValue( QString _value )
{
	return( _value.toFloat() );
}




template<>
inline int automatableObject<int>::attributeValue( QString _value )
{
	return( _value.toInt() );
}




template<>
inline bool automatableObject<bool, signed char>::attributeValue(
								QString _value )
{
	return( static_cast<bool>( _value.toInt() ) );
}




#endif

