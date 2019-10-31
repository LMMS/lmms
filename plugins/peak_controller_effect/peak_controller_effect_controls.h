/*
 * peak_controller_EffectControls.h - controls for peakController effect
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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

#include "EffectControls.h"
#include "peak_controller_effect_control_dialog.h"
#include "Knob.h"

class PeakControllerEffect;

class PeakControllerEffectControls : public EffectControls
{
	Q_OBJECT
public:
	PeakControllerEffectControls( PeakControllerEffect * _eff );
	virtual ~PeakControllerEffectControls()
	{
	}

	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;
	inline QString nodeName() const override
	{
		return "peakcontrollereffectcontrols";
	}

	int controlCount() override
	{
		return 1;
	}
	EffectControlDialog * createView() override
	{
		return new PeakControllerEffectControlDialog( this );
	}


private:
	PeakControllerEffect * m_effect;

	FloatModel m_baseModel;
	FloatModel m_amountModel;
	FloatModel m_attackModel;
	FloatModel m_decayModel;
	FloatModel m_tresholdModel;
	BoolModel m_muteModel;
	BoolModel m_absModel;
	FloatModel m_amountMultModel;

	friend class PeakControllerEffectControlDialog;
	friend class PeakControllerEffect;

} ;


#endif
