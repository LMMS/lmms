/*
 * peak_controller_effect_controls.h - controls for peakController effect
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _PEAK_CONTROLLER_EFFECT_CONTROLS_H
#define _PEAK_CONTROLLER_EFFECT_CONTROLS_H

#include "effect_controls.h"
#include "peak_controller_effect_control_dialog.h"
#include "knob.h"

class peakControllerEffect;

class peakControllerEffectControls : public effectControls
{
	Q_OBJECT
public:
	peakControllerEffectControls( peakControllerEffect * _eff );
	virtual ~peakControllerEffectControls()
	{
	}

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );
	inline virtual QString nodeName( void ) const
	{
		return "peakcontrollereffectcontrols";
	}

	virtual int getControlCount( void )
	{
		return 1;
	}

	virtual effectControlDialog * createView( void )
	{
		return new peakControllerEffectControlDialog( this );
	}


private:
	peakControllerEffect * m_effect;

	floatModel m_baseModel;
	floatModel m_amountModel;
	floatModel m_attackModel;
	floatModel m_decayModel;
	boolModel m_muteModel;

	friend class peakControllerEffectControlDialog;
	friend class peakControllerEffect;

} ;


#endif
