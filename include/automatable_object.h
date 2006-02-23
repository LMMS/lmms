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

#include "templates.h"

template<typename T>
class automatableObject
{
public:
	automatableObject( const T _val = 0, const T _min = 0,
				const T _max = 0, const T _step = 1 ) :
		m_value( _val ),
		m_minValue( _min ),
		m_maxValue( _max ),
		m_step( _step )
	{
	}

	virtual ~automatableObject()
	{
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


	inline virtual void setValue( const T _value )
	{
		m_value = tLimit<T>( _value, minValue(), maxValue() );
		if( m_step != 0 )
		{
			m_value = static_cast<T>( floorf( m_value / m_step ) *
								m_step );
		}
		else
		{
			m_value = m_minValue;
		}

		// correct rounding error at the border
		if( tAbs<T>( m_value - m_maxValue ) < minEps() *
							tAbs<T>( m_step ) )
		{
			m_value = m_maxValue;
		}

		// correct rounding error if value = 0
		if( tAbs<T>( m_value ) < minEps() * tAbs<T>( m_step ) )
		{
			m_value = 0;
		}
	}

	inline virtual void incValue( int _steps )
	{
		setValue( m_value + _steps * m_step );
	}

	inline virtual void setRange( const T _min, const T _max,
							const T _step = 0 )
	{
		m_minValue = _min;
		m_maxValue = _max;
		setStep( _step );
		if( m_minValue > m_maxValue )
		{
			qSwap<T>( m_minValue, m_maxValue );
		}
		// re-adjust value
		setValue( value() );
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


private:
	T m_value;
	T m_minValue;
	T m_maxValue;
	T m_step;

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

