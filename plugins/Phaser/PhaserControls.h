/*
 * PhaserControls.h
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

#ifndef PHASER_CONTROLS_H
#define PHASER_CONTROLS_H

#include "PhaserControlDialog.h"

#include "ComboBox.h"
#include "EffectControls.h"
#include "Knob.h"
#include "LcdSpinBox.h"

namespace lmms
{

class PhaserEffect;


class PhaserControls : public EffectControls
{
	Q_OBJECT
public:
	PhaserControls(PhaserEffect* effect);

	void saveSettings(QDomDocument & _doc, QDomElement & _parent) override;
	void loadSettings(const QDomElement & _this) override;
	inline QString nodeName() const override
	{
		return "PhaserControls";
	}

	int controlCount() override
	{
		return 24;
	}

	gui::EffectControlDialog * createView() override
	{
		return new gui::PhaserControlDialog(this);
	}

	float m_inPeakL = 0;
	float m_inPeakR = 0;
	float m_outPeakL = 0;
	float m_outPeakR = 0;

private:
	PhaserEffect* m_effect;

	FloatModel m_cutoffModel;
	FloatModel m_resonanceModel;
	FloatModel m_feedbackModel;
	IntModel m_orderModel;
	FloatModel m_delayModel;
	TempoSyncKnobModel m_rateModel;
	FloatModel m_amountModel;
	BoolModel m_enableLFOModel;
	FloatModel m_phaseModel;
	FloatModel m_inFollowModel;
	FloatModel m_attackModel;
	FloatModel m_releaseModel;
	FloatModel m_distortionModel;
	FloatModel m_analogDistModel;
	FloatModel m_cutoffControlModel;
	FloatModel m_delayControlModel;
	FloatModel m_outGainModel;
	FloatModel m_inGainModel;
	BoolModel m_invertModel;
	BoolModel m_wetModel;
	BoolModel m_analogModel;
	BoolModel m_doubleModel;
	BoolModel m_aliasModel;
	ComboBoxModel m_modeModel;

	friend class gui::PhaserControlDialog;
	friend class PhaserEffect;

} ;

} // namespace lmms

#endif

