/*
 * StereoControlControls.h
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

#ifndef STEREOCONTROL_CONTROLS_H
#define STEREOCONTROL_CONTROLS_H

#include "EffectControls.h"
#include "StereoControlControlDialog.h"

#include "ComboBox.h"


class StereoControlEffect;


class StereoControlControls : public EffectControls
{
	Q_OBJECT
public:
	StereoControlControls(StereoControlEffect* effect);
	~StereoControlControls() override
	{
	}

	void saveSettings(QDomDocument & _doc, QDomElement & _parent) override;
	void loadSettings(const QDomElement & _this);
	inline QString nodeName() const override
	{
		return "StereoControlControls";
	}

	int controlCount() override
	{
		return 20;
	}

	EffectControlDialog* createView() override
	{
		return new StereoControlControlDialog(this);
	}

private:
	StereoControlEffect* m_effect;

	FloatModel m_gainModel;
	FloatModel m_stereoizerModel;
	FloatModel m_widthModel;
	FloatModel m_panModel;
	FloatModel m_monoBassFreqModel;
	FloatModel m_stereoizerLPModel;
	FloatModel m_stereoizerHPModel;
	FloatModel m_panSpectralModel;
	FloatModel m_panDelayModel;
	FloatModel m_panDualLModel;
	FloatModel m_panDualRModel;

	IntModel m_panModeModel;
	ComboBoxModel m_soloChannelModel;

	BoolModel m_monoModel;
	BoolModel m_dcModel;
	BoolModel m_muteModel;
	BoolModel m_monoBassModel;
	BoolModel m_auditionModel;
	BoolModel m_invertLModel;
	BoolModel m_invertRModel;

	friend class StereoControlControlDialog;
	friend class StereoControlEffect;

};

#endif
