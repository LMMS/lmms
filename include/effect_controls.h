/*
 * effect_controls.h - model for effect-controls
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

#ifndef _EFFECT_CONTROLS_H
#define _EFFECT_CONTROLS_H

#include "mv_base.h"
#include "journalling_object.h"
#include "effect.h"


class effectControlDialog;


class effectControls : public journallingObject, public model
{
public:
	effectControls( effect * _eff ) :
		journallingObject(),
		model( _eff ),
		m_effect( _eff )
	{
	}

	virtual ~effectControls()
	{
	}

	virtual int getControlCount( void ) = 0;
	virtual effectControlDialog * createView( void ) = 0;


//	template<class T>
	effect * getEffect( void )
	{
		return( m_effect );
		//return( dynamic_cast<T *>( m_effect ) );
	}


private:
	effect * m_effect;

} ;

#endif
