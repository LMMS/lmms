/*
 * DummyEffect.h - effect used as fallback if an effect couldn't be loaded
 *
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "Effect.h"
#include "EffectControls.h"
#include "EffectControlDialog.h"


class DummyEffectControlDialog : public EffectControlDialog
{
public:
	DummyEffectControlDialog( EffectControls * _controls ) :
		EffectControlDialog( _controls )
	{
	}

} ;


class DummyEffectControls : public EffectControls
{
public:
	DummyEffectControls( Effect * _eff ) :
		EffectControls( _eff )
	{
	}

	virtual ~DummyEffectControls()
	{
	}

	virtual int controlCount()
	{
		return 0;
	}

	inline virtual void saveSettings( QDomDocument &, QDomElement & )
	{
	}

	inline virtual void loadSettings( const QDomElement & )
	{
	}

	inline virtual QString nodeName() const
	{
		return "DummyControls";
	}

	virtual EffectControlDialog * createView()
	{
		return new DummyEffectControlDialog( this );
	}
} ;



class DummyEffect : public Effect
{
public:
	inline DummyEffect( Model * _parent ) :
		Effect( NULL, _parent, NULL ),
		m_controls( this )
	{
	}

	inline virtual ~DummyEffect()
	{
	}

	inline virtual EffectControls * controls()
	{
		return &m_controls;
	}

	bool processAudioBuffer( sampleFrame *, const fpp_t )
	{
		return false;
	}


private:
	DummyEffectControls m_controls;

} ;


#endif
