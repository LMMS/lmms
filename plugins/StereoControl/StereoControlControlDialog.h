/*
 * StereoControlControlDialog.h
 *
 * Copyright (c) 2020 Lost Robot <r94231@gmail.com>
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

#ifndef STEREOCONTROL_CONTROL_DIALOG_H
#define STEREOCONTROL_CONTROL_DIALOG_H

#include "ComboBox.h"
#include "EffectControlDialog.h"
#include "LedCheckbox.h"
#include "Knob.h"
#include "PixmapButton.h"


class StereoControlControls;


class StereoControlControlDialog : public EffectControlDialog
{
	Q_OBJECT
public:
	StereoControlControlDialog(StereoControlControls* controls);

private:
	StereoControlControls * m_controls;

	Knob * m_gainKnob;
	Knob * m_stereoizerKnob;
	Knob * m_widthKnob;
	Knob * m_monoBassFreqKnob;
	Knob * m_stereoizerLPKnob;
	Knob * m_stereoizerHPKnob;
	Knob * m_panSpectralKnob;
	Knob * m_panDelayKnob;
	Knob * m_panDualLKnob;
	Knob * m_panDualRKnob;
	Knob * m_panKnob;
	PixmapButton * m_gainButton;
	PixmapButton * m_stereoButton;
	PixmapButton * m_haasButton;
	automatableButtonGroup * m_panModeGroup;
	PixmapButton * m_monoButton;
	PixmapButton * m_dcRemovalButton;
	PixmapButton * m_muteButton;
	LedCheckBox * m_monoBassButton;
	PixmapButton * m_auditionButton;
	PixmapButton * m_invertLButton;
	PixmapButton * m_invertRButton;
	ComboBox * m_soloChannelBox;

private slots:
	void updateKnobVisibility();

};

#endif
