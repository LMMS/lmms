/*
 * GranularPitchShifterControls.h
 *
 * Copyright (c) 2024 Lost Robot <r94231/at/gmail/dot/com>
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

#ifndef LMMS_GRANULAR_PITCH_SHIFTER_CONTROL_DIALOG_H
#define LMMS_GRANULAR_PITCH_SHIFTER_CONTROL_DIALOG_H

#include "ComboBoxModel.h"
#include "EffectControls.h"
#include "GranularPitchShifterControlDialog.h"

namespace lmms
{

class GranularPitchShifterEffect;

class GranularPitchShifterControls : public EffectControls
{
	Q_OBJECT
public:
	GranularPitchShifterControls(GranularPitchShifterEffect* effect);
	~GranularPitchShifterControls() override = default;

	void saveSettings(QDomDocument& doc, QDomElement& parent) override;
	void loadSettings(const QDomElement& parent) override;
	inline QString nodeName() const override
	{
		return "GranularPitchShifterControls";
	}
	gui::EffectControlDialog* createView() override
	{
		return new gui::GranularPitchShifterControlDialog(this);
	}
	int controlCount() override { return 4; }
	
public slots:
	void updateRange();
	
private:
	GranularPitchShifterEffect* m_effect;
	FloatModel m_pitchModel;
	FloatModel m_sizeModel;
	FloatModel m_sprayModel;
	FloatModel m_jitterModel;
	FloatModel m_twitchModel;
	FloatModel m_pitchSpreadModel;
	FloatModel m_spraySpreadModel;
	FloatModel m_shapeModel;
	FloatModel m_fadeLengthModel;
	FloatModel m_feedbackModel;
	FloatModel m_minLatencyModel;
	BoolModel m_prefilterModel;
	FloatModel m_densityModel;
	FloatModel m_glideModel;
	ComboBoxModel m_rangeModel;

	friend class gui::GranularPitchShifterControlDialog;
	friend class GranularPitchShifterEffect;
};

} // namespace lmms

#endif // LMMS_GRANULAR_PITCH_SHIFTER_CONTROL_DIALOG_H
