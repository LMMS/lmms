/*
 * DisintegratorControlDialog.h
 *
 * Copyright (c) 2019 Lost Robot <r94231@gmail.com>
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

#ifndef DISINTEGRATOR_CONTROL_DIALOG_H
#define DISINTEGRATOR_CONTROL_DIALOG_H

#include "ComboBox.h"
#include "EffectControlDialog.h"
#include "Knob.h"


class DisintegratorControls;


class DisintegratorControlDialog : public EffectControlDialog
{
	Q_OBJECT
public:
	DisintegratorControlDialog(DisintegratorControls* controls);
	DisintegratorControls * m_controls;

private slots:
	void updateKnobVisibility();

private:
	Knob * m_lowCutKnob;
	Knob * m_highCutKnob;
	Knob * m_amountKnob;
	ComboBox * m_typeBox;
	Knob * m_freqKnob;

	friend class DisintegratorControls;
};

#endif
