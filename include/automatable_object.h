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

#include "editable_object.h"
#include "templates.h"


template<typename T, typename EDIT_STEP_TYPE = T>
class automatableObject : public editableObject
{
public:
	typedef automatableObject<T, EDIT_STEP_TYPE> autoObj;

	automatableObject( engine * _engine, const T _val = 0, const T _min = 0,
				const T _max = 0,
				const T _step = defaultRelStep() ) :
		editableObject( _engine ),
		m_oldValue( _val ),
		m_value( _val ),
		m_minValue( _min ),
		m_maxValue( _max ),
		m_step( _step )
	{
	}

	virtual ~automatableObject()
	{
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

	inline T fittedValue( T _value )
	{
		_value = tLimit<T>( _value, minValue(), maxValue() );

		if( m_step != 0 )
		{
			_value = static_cast<T>( floorf( _value / step() ) *
								step() );
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
		const bool sr = isRecordingSteps();
		setStepRecording( FALSE );
		setValue( _value );
		setStepRecording( sr );
	}

	inline virtual void setValue( const T _value )
	{
		const T old_val = m_value;

		m_value = fittedValue( _value );
		if( old_val != m_value )
		{
			// add changes to history so user can undo it
			addStep( editStep( 0, static_cast<EDIT_STEP_TYPE>(
							m_value ) -
						static_cast<EDIT_STEP_TYPE>(
							old_val ) ) );

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
					const bool sr = it->isRecordingSteps();
					it->setStepRecording(
							isRecordingSteps() );
					it->setValue( value() );
					it->setStepRecording( sr );
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
		setStep( _step );
		if( m_minValue > m_maxValue )
		{
			qSwap<T>( m_minValue, m_maxValue );
		}
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
	}

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

	static inline void linkObjects( autoObj * _object1,
					autoObj * _object2 )
	{
		_object1->linkObject( _object2 );
		_object2->linkObject( _object1 );
	}


protected:
	virtual void redoStep( const editStep & _edit_step )
	{
		const bool sr = isRecordingSteps();
		setStepRecording( FALSE );
#ifndef QT3
		setValue( static_cast<T>( value() +
				_edit_step.data().value<EDIT_STEP_TYPE>() ) );
#else
		setValue( static_cast<T>( value() + static_cast<EDIT_STEP_TYPE>(
					_edit_step.data().toDouble() ) ) );
#endif
		setStepRecording( sr );
	}

	virtual void undoStep( const editStep & _edit_step )
	{
#ifndef QT3
		redoStep( editStep( _edit_step.actionID(),
				-_edit_step.data().value<EDIT_STEP_TYPE>() ) );
#else
		redoStep( editStep( _edit_step.actionID(),
				static_cast<EDIT_STEP_TYPE>(
					-_edit_step.data().toDouble() ) ) );
#endif
	}

	// most objects will need this temporarily
	T m_oldValue;

	inline void addStepFromOldToCurVal( void )
	{
		addStep( editStep( 0, value() - m_oldValue ) );
	}


private:
	T m_value;
	T m_minValue;
	T m_maxValue;
	T m_step;

	typedef vvector<autoObj *> autoObjVector;
	autoObjVector m_linkedObjects;

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




#endif

