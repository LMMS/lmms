/*
 * peak_controller_EffectControlDialog.h - control dialog for
 *                                           peakControllerEffect
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
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

#ifndef _PEAK_CONTROLLER_EFFECT_CONTROL_DIALOG_H
#define _PEAK_CONTROLLER_EFFECT_CONTROL_DIALOG_H

#include "EffectControlDialog.h"

class PeakControllerEffectControls;
class Knob;
class LedCheckBox;


class PeakControllerEffectControlDialog : public EffectControlDialog
{
	Q_OBJECT
public:
	PeakControllerEffectControlDialog(
				PeakControllerEffectControls * _controls );
	virtual ~PeakControllerEffectControlDialog()
	{
	}


protected:
	Knob * m_baseKnob;
	Knob * m_amountKnob;
	Knob * m_attackKnob;
	Knob * m_decayKnob;
	Knob * m_tresholdKnob;
	LedCheckBox * m_muteLed;

	LedCheckBox * m_absLed;
	Knob * m_amountMultKnob;

} ;


#endif
