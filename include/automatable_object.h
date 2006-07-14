/*
 * automatable_object.h - declaration of class automatableObject
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _AUTOMATABLE_OBJECT_H
#define _AUTOMATABLE_OBJECT_H

#include <math.h>

#include "qt3support.h"
#include "automation_editor.h"
#include "automation_pattern.h"
#include "journalling_object.h"
#include "templates.h"
#include "midi_time.h"
#include "level_object.h"

#ifndef QT3

#include <Qt/QtXml>
#include <QtCore/QVariant>
#include <QtCore/QPointer>

#else

#include <qdom.h>
#include <qvariant.h>
#include <qguardedptr.h>

#endif


template<typename T, typename EDIT_STEP_TYPE = T>
class automatableObject : public journallingObject, public levelObject
{
public:
	typedef automatableObject<T, EDIT_STEP_TYPE> autoObj;

	automatableObject( engine * _engine, track * _track = NULL,
					const T _val = 0, const T _min = 0,
					const T _max = 0,
					const T _step = defaultRelStep() ) :
		journallingObject( _engine ),
		m_oldValue( _val ),
		m_value( _val ),
		m_minValue( _min ),
		m_maxValue( _max ),
		m_step( _step ),
		m_automation_pattern( NULL ),
		m_track( _track )
	{
		m_curLevel = level( _val );
		m_minLevel = level( _min );
		m_maxLevel = level( _max );
	}

	virtual ~automatableObject()
	{
		if( m_automation_pattern )
		{
			delete m_automation_pattern;
		}
		while( m_linkedObjects.empty() == FALSE )
		{
			m_linkedObjects.last()->unlinkObject( this );
			m_linkedObjects.erase( m_linkedObjects.end() - 1 );
		}
	}

	static inline T minRelStep( void )
	{
		return( 1 );
	}

	static inline T defaultRelStep( void )
	{
		return( 1 );
	}

	static inline T minEps( void )
	{
		return( 1 );
	}


	inline virtual T value( void ) const
	{
		return( m_value );
	}

	inline virtual T minValue( void ) const
	{
		return( m_minValue );
	}

	inline virtual T maxValue( void ) const
	{
		return( m_maxValue );
	}

	inline virtual T step( void ) const
	{
		return( m_step );
	}

	inline int curLevel( void ) const
	{
		return( m_curLevel );
	}

	inline T fittedValue( T _value )
	{
		_value = tLimit<T>( _value, minValue(), maxValue() );

		if( m_step != 0 )
		{
			_value = static_cast<T>( roundf( _value
						/ (float)step() ) * step() );
		}
		else
		{
			_value = minValue();
		}

		// correct rounding error at the border
		if( tAbs<T>( _value - maxValue() ) < minEps() *
							tAbs<T>( step() ) )
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

	inline virtual void setInitValue( const T _value )
	{
		saveJournallingState( FALSE );
		setValue( _value );
		if( m_automation_pattern )
		{
			setFirstValue();
		}
		restoreJournallingState();
	}

	inline virtual void setValue( const T _value )
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
			for( csize i = 0; i < m_linkedObjects.size(); ++i )
			{
				autoObj * it = m_linkedObjects[i];
				if( value() != it->value() &&
					it->fittedValue( value() ) !=
								it->value() )
				{
					it->saveJournallingState(
							isJournalling() );
					it->setValue( value() );
					it->restoreJournallingState();
				}
			}
		}
	}

	inline virtual void incValue( int _steps )
	{
		setValue( m_value + _steps * m_step );
	}

	inline virtual void setRange( const T _min, const T _max,
					const T _step = defaultRelStep() )
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

	inline virtual void setStep( const T _step )
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
			if( tAbs<T>( m_step ) <
					tAbs<T>( minRelStep() * intv ) )
			{
				m_step = minRelStep() * intv;
			}
		}*/
		m_step = _step;
		m_curLevel = level( m_value );
		m_minLevel = level( m_minValue );
		m_maxLevel = level( m_maxValue );
	}

	static inline void linkObjects( autoObj * _object1,
					autoObj * _object2 )
	{
		_object1->linkObject( _object2 );
		_object2->linkObject( _object1 );

		if( _object1->m_automation_pattern
					!= _object2->m_automation_pattern )
		{
			if( _object2->m_automation_pattern )
			{
				delete _object2->m_automation_pattern;
			}
			_object2->m_automation_pattern
					= _object1->m_automation_pattern;
		}
	}

	virtual void FASTCALL saveSettings( QDomDocument & _doc,
					QDomElement & _this,
					const QString & _name = "value" )
	{
		if( m_automation_pattern )
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

	virtual void FASTCALL loadSettings( const QDomElement & _this,
					const QString & _name = "value" )
	{
		QDomNode node = _this.namedItem(
					automationPattern::classNodeName() );
		if( node.isElement() )
		{
			node = node.namedItem( _name );
			if( node.isElement() )
			{
				m_automation_pattern->loadSettings(
							node.toElement() );
				setLevel( m_automation_pattern->valueAt(
							midiTime( 0 ) ) );
				return;
			}
		}

		setInitValue( attributeValue( _this.attribute( _name ) ) );
	}

	virtual QString nodeName( void ) const
	{
		return( "automatableobject" );
	}

	inline const QVariant & data( void ) const
	{
		return( m_data );
	}

	void setData( const QVariant & _data )
	{
		m_data = _data;
	}

	inline automationPattern * getAutomationPattern( void )
	{
		if( !m_automation_pattern )
		{
			m_automation_pattern = new automationPattern( m_track,
									this );
			syncAutomationPattern();
		}
		return( m_automation_pattern );
	}

	inline bool nullTrack( void )
	{
		return( m_track == NULL );
	}

	void initAutomationPattern( engine * _engine )
	{
		m_automation_pattern = new automationPattern( _engine, this );
	}


protected:
	virtual void redoStep( journalEntry & _je )
	{
		saveJournallingState( FALSE );
/*#ifndef QT3
		setValue( static_cast<T>( value() +
					_je.data().value<EDIT_STEP_TYPE>() ) );
#else*/
		setValue( static_cast<T>( value() + static_cast<EDIT_STEP_TYPE>(
						_je.data().toDouble() ) ) );
//#endif
		restoreJournallingState();
	}

	virtual void undoStep( journalEntry & _je )
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


	// most objects will need this temporarily
	T m_oldValue;

	inline void addJournalEntryFromOldToCurVal( void )
	{
		addJournalEntry( journalEntry( 0, value() - m_oldValue ) );
	}

	inline void setFirstValue( void )
	{
		if( m_automation_pattern
					&& m_automation_pattern->updateFirst() )
		{
			m_automation_pattern->putValue( midiTime( 0 ),
							m_curLevel, FALSE );
			if( eng()->getAutomationEditor() &&
				eng()->getAutomationEditor()->currentPattern()
						== m_automation_pattern )
			{
				eng()->getAutomationEditor()->update();
			}
		}
	}


private:
	T m_value;
	T m_minValue;
	T m_maxValue;
	T m_step;
	int m_curLevel;
	QPointer<automationPattern> m_automation_pattern;
	track * m_track;

	QVariant m_data;

	typedef vvector<autoObj *> autoObjVector;
	autoObjVector m_linkedObjects;

	inline void linkObject( autoObj * _object )
	{
		if( qFind( m_linkedObjects.begin(), m_linkedObjects.end(),
					_object ) == m_linkedObjects.end() )
		{
			m_linkedObjects.push_back( _object );
		}
	}

	inline void unlinkObject( autoObj * _object )
	{
		m_linkedObjects.erase( qFind( m_linkedObjects.begin(),
						m_linkedObjects.end(),
						_object ) );
	}

	static T attributeValue( QString _value );

	inline void syncAutomationPattern( void )
	{
		for( csize i = 0; i < m_linkedObjects.size(); ++i )
		{
			autoObj * it = m_linkedObjects[i];
			if( m_automation_pattern != it->m_automation_pattern )
			{
				it->m_automation_pattern = m_automation_pattern;
			}
		}
	}

	void setLevel( int _level )
	{
		saveJournallingState( FALSE );
		m_automation_pattern->setUpdateFirst( FALSE );
		setValue( _level * m_step );
		m_automation_pattern->setUpdateFirst( TRUE );
		restoreJournallingState();
	}

	inline int level( T _value ) const
	{
		return( (int)roundf( _value / (float)m_step ) );
	}

	QString levelToLabel( int _level )
	{
		return( QString::number( _level * m_step ) );
	}

	int labelToLevel( QString _label )
	{
		return( level( attributeValue( _label ) ) );
	}

} ;



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

