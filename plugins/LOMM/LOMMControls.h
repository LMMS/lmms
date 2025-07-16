/*
 * LOMMControls.h
 *
 * Copyright (c) 2023 Lost Robot <r94231/at/gmail/dot/com>
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

#ifndef LMMS_LOMM_CONTROLS_H
#define LMMS_LOMM_CONTROLS_H

#include "LOMMControlDialog.h"
#include "EffectControls.h"

namespace lmms
{
class LOMMEffect;

class LOMMControls : public EffectControls
{
	Q_OBJECT
public:
	LOMMControls(LOMMEffect* effect);
	~LOMMControls() override = default;

	void saveSettings(QDomDocument & doc, QDomElement & parent) override;
	void loadSettings(const QDomElement & parent) override;
	inline QString nodeName() const override
	{
		return "LOMMControls";
	}

	int controlCount() override
	{
		return 49;
	}

	gui::EffectControlDialog* createView() override
	{
		m_view = new gui::LOMMControlDialog(this);
		return m_view;
	}
	
	template <typename T>
	void setInitAndReset(AutomatableModel& model, T initValue)
	{
		model.setInitValue(initValue);
		model.reset();
	}
	
public slots:
	void resetAllParameters();

private:
	LOMMEffect* m_effect;
	gui::LOMMControlDialog* m_view;
	
	FloatModel m_depthModel;
	FloatModel m_timeModel;
	FloatModel m_inVolModel;
	FloatModel m_outVolModel;
	FloatModel m_upwardModel;
	FloatModel m_downwardModel;
	FloatModel m_split1Model;
	FloatModel m_split2Model;
	BoolModel m_split1EnabledModel;
	BoolModel m_split2EnabledModel;
	BoolModel m_band1EnabledModel;
	BoolModel m_band2EnabledModel;
	BoolModel m_band3EnabledModel;
	FloatModel m_inHighModel;
	FloatModel m_inMidModel;
	FloatModel m_inLowModel;
	FloatModel m_outHighModel;
	FloatModel m_outMidModel;
	FloatModel m_outLowModel;
	FloatModel m_aThreshHModel;
	FloatModel m_aThreshMModel;
	FloatModel m_aThreshLModel;
	FloatModel m_aRatioHModel;
	FloatModel m_aRatioMModel;
	FloatModel m_aRatioLModel;
	FloatModel m_bThreshHModel;
	FloatModel m_bThreshMModel;
	FloatModel m_bThreshLModel;
	FloatModel m_bRatioHModel;
	FloatModel m_bRatioMModel;
	FloatModel m_bRatioLModel;
	FloatModel m_atkHModel;
	FloatModel m_atkMModel;
	FloatModel m_atkLModel;
	FloatModel m_relHModel;
	FloatModel m_relMModel;
	FloatModel m_relLModel;
	FloatModel m_rmsTimeModel;
	FloatModel m_kneeModel;
	FloatModel m_rangeModel;
	FloatModel m_balanceModel;
	BoolModel m_depthScalingModel;
	BoolModel m_stereoLinkModel;
	FloatModel m_autoTimeModel;
	FloatModel m_mixModel;
	BoolModel m_feedbackModel;
	BoolModel m_midsideModel;
	BoolModel m_lookaheadEnableModel;
	FloatModel m_lookaheadModel;
	BoolModel m_lowSideUpwardSuppressModel;
	
	friend class gui::LOMMControlDialog;
	friend class LOMMEffect;
};

} // namespace lmms

#endif // LMMS_LOMM_CONTROLS_H
