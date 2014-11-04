/*
 * transformable_auto_model.h - template transformableAutoModel
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _TRANSFORMABLE_AUTO_MODEL_H
#define _TRANSFORMABLE_AUTO_MODEL_H

#include "AutomatableModel.h"
//#include "automatable_model_templates.h"


template<typename T>
struct AutoModelTransformer
{
	inline virtual T transform( const T & _val ) const
	{
		return( _val );
	}
} ;


template<typename T, typename EDIT_STEP_TYPE>
class transformableAutoModel : public AutomatableModel<T, EDIT_STEP_TYPE>
{
public:
	transformableAutoModel( const AutoModelTransformer<T> * _transformer,
				const T _val = 0,
				const T _min = 0,
				const T _max = 0,
				const T _step = defaultRelStep(),
				Model * _parent = NULL,
				bool _default_constructed = false ) :
		AutomatableModel( _val, _min, _max, _step, _parent,
						_default_constructed ),
		m_transformer( _transformer )
	{
	}

	inline virtual ~transformableAutoModel()
	{
	}

	inline virtual void setValue( const T _value )
	{
		autoModel::setValue( _value );
		if( m_transformer != NULL )
		{
			m_transformedValue = m_transformer->transform(
							autoModel::value() );
		}
		else
		{
			m_transformedValue = autoModel::value();
		}
	}

	inline virtual T value() const
	{
		return( m_transformedValue );
	}

private:
	T m_transformedValue;
	const AutoModelTransformer<T> * m_transformer;

} ;


#endif

