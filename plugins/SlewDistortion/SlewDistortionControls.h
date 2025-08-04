/*
 * SlewDistortionControls.h
 *
 * Copyright (c) 2025 Lost Robot <r94231/at/gmail/dot/com>
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

#ifndef LMMS_SLEW_DISTORTION_CONTROLS_H
#define LMMS_SLEW_DISTORTION_CONTROLS_H

#include "EffectControls.h"
#include "SlewDistortionControlDialog.h"
#include "ComboBox.h"

namespace lmms
{

constexpr int SLEWDIST_MAX_OVERSAMPLE_STAGES = 5;

class SlewDistortion;

namespace gui
{
class SlewDistortionControlDialog;
}

enum class SlewDistortionType : int
{
	HardClip = 0,
	Tanh,
	FastSoftClip1,
	FastSoftClip2,
	Sinusoidal,
	Foldback,
	FullRectify,
	HalfRectify,
	SmoothRectify,
	Bitcrush,
	Count
};

class SlewDistortionControls : public EffectControls
{
	Q_OBJECT
public:
	SlewDistortionControls(SlewDistortion* effect);
	~SlewDistortionControls() override = default;

	void saveSettings(QDomDocument& doc, QDomElement& parent) override;
	void loadSettings(const QDomElement& parent) override;
	inline QString nodeName() const override
	{
		return "SlewDistortionControls";
	}
	gui::EffectControlDialog* createView() override
	{
		return new gui::SlewDistortionControlDialog(this);
	}
	int controlCount() override { return 32; }

private:
	SlewDistortion* m_effect;
	
	ComboBoxModel m_distType1Model;
	ComboBoxModel m_distType2Model;
	FloatModel m_drive1Model;
	FloatModel m_drive2Model;
	FloatModel m_slewUp1Model;
	FloatModel m_slewUp2Model;
	FloatModel m_slewDown1Model;
	FloatModel m_slewDown2Model;
	FloatModel m_bias1Model;
	FloatModel m_bias2Model;
	FloatModel m_warp1Model;
	FloatModel m_warp2Model;
	FloatModel m_crush1Model;
	FloatModel m_crush2Model;
	FloatModel m_outVol1Model;
	FloatModel m_outVol2Model;
	FloatModel m_attack1Model;
	FloatModel m_attack2Model;
	FloatModel m_release1Model;
	FloatModel m_release2Model;
	FloatModel m_dynamics1Model;
	FloatModel m_dynamics2Model;
	FloatModel m_dynamicSlew1Model;
	FloatModel m_dynamicSlew2Model;
	BoolModel m_dcRemoveModel;
	BoolModel m_multibandModel;
	IntModel m_oversamplingModel;
	FloatModel m_splitModel;
	FloatModel m_mix1Model;
	FloatModel m_mix2Model;
	BoolModel m_slewLink1Model;
	BoolModel m_slewLink2Model;

	friend class gui::SlewDistortionControlDialog;
	friend class SlewDistortion;
};

} // namespace lmms

#endif // LMMS_SLEW_DISTORTION_CONTROLS_H
