/*
 * VstEffectControls.h - controls for VST effect plugins
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _VST_EFFECT_CONTROLS_H
#define _VST_EFFECT_CONTROLS_H

#include "EffectControls.h"
#include "VstEffectControlDialog.h"


class VstEffect;


class VstEffectControls : public EffectControls
{
	Q_OBJECT
public:
	VstEffectControls( VstEffect * _eff );
	virtual ~VstEffectControls()
	{
	}

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );
	inline virtual QString nodeName() const
	{
		return "vsteffectcontrols";
	}

	virtual int controlCount();

	virtual EffectControlDialog * createView()
	{
		return new VstEffectControlDialog( this );
	}


private:
	VstEffect * m_effect;

	friend class VstEffectControlDialog;

} ;


#endif
