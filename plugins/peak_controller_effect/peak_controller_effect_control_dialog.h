/*
 * stereomatrix_control_dialog.h - control dialog for stereoMatrix-effect
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
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

#ifndef _PEAK_CONTROLLER_EFFECT_CONTROL_DIALOG_H
#define _PEAK_CONTROLLER_EFFECT_CONTROL_DIALOG_H

#include "effect_control_dialog.h"

class peakControllerEffectControls;
class knob;
class tempoSyncKnob;
class ledCheckBox;


class peakControllerEffectControlDialog : public effectControlDialog
{
public:
	peakControllerEffectControlDialog( peakControllerEffectControls * _controls );
	virtual ~peakControllerEffectControlDialog()
	{
	}

protected:
	knob * m_baseKnob;
	knob * m_amountKnob;
	tempoSyncKnob * m_decayKnob;
	ledCheckBox * m_muteLed;

};


#endif
