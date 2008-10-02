/*
 * dummy_effect.h - effect used as fallback if an effect couldn't be loaded
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _DUMMY_EFFECT_H
#define _DUMMY_EFFECT_H

#include "effect.h"
#include "effect_controls.h"
#include "effect_control_dialog.h"


class dummyEffectControlDialog : public effectControlDialog
{
public:
	dummyEffectControlDialog( effectControls * _controls ) :
		effectControlDialog( _controls )
	{
	}

} ;


class dummyEffectControls : public effectControls
{
public:
	dummyEffectControls( effect * _eff ) :
		effectControls( _eff )
	{
	}

	virtual ~dummyEffectControls()
	{
	}

	virtual int getControlCount( void )
	{
		return( 0 );
	}

	inline virtual void saveSettings( QDomDocument &, QDomElement & )
	{
	}

	inline virtual void loadSettings( const QDomElement & )
	{
	}

	inline virtual QString nodeName( void ) const
	{
		return( "dummycontrols" );
	}

	virtual effectControlDialog * createView( void )
	{
		return( new dummyEffectControlDialog( this ) );
	}
} ;



class dummyEffect : public effect
{
public:
	inline dummyEffect( model * _parent ) :
		effect( NULL, _parent, NULL ),
		m_controls( this )
	{
	}

	inline virtual ~dummyEffect()
	{
	}

	inline virtual effectControls * getControls( void )
	{
		return &m_controls;
	}

	bool processAudioBuffer( sampleFrame *, const fpp_t )
	{
		return false;
	}


private:
	dummyEffectControls m_controls;

} ;


#endif
