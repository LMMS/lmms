/*
 * EffectControls.h - model for effect-controls
 *
 * Copyright (c) 2008-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _EFFECT_CONTROLS_H
#define _EFFECT_CONTROLS_H

#include "Model.h"
#include "JournallingObject.h"
#include "Effect.h"

class EffectControlDialog;


class EffectControls : public JournallingObject, public Model
{
public:
	EffectControls( Effect * _eff ) :
		JournallingObject(),
		Model( _eff ),
		m_effect( _eff ),
		m_viewVisible( false )
	{
	}

	virtual ~EffectControls()
	{
	}

	virtual int controlCount() = 0;
	virtual EffectControlDialog * createView() = 0;


	void setViewVisible( bool _visible )
	{
		m_viewVisible = _visible;
	}

	bool isViewVisible() const
	{
		return m_viewVisible;
	}

	Effect * effect()
	{
		return m_effect;
	}


private:
	Effect * m_effect;
	bool m_viewVisible;

} ;

#endif
