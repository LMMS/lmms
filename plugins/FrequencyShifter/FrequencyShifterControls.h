/*
 * FrequencyShifterControls.h
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
 */

#ifndef LMMS_FREQUENCY_SHIFTER_CONTROLS_H
#define LMMS_FREQUENCY_SHIFTER_CONTROLS_H

#include "EffectControls.h"
#include "FrequencyShifterControlDialog.h"

namespace lmms
{

class FrequencyShifterEffect;

class FrequencyShifterControls : public EffectControls
{
	Q_OBJECT
public:
	FrequencyShifterControls(FrequencyShifterEffect* e);
	~FrequencyShifterControls() override = default;

	void saveSettings(QDomDocument& doc, QDomElement& e) override;
	void loadSettings(const QDomElement& e) override;
	QString nodeName() const override
	{
		return "FrequencyShifterControls";
	}
	gui::EffectControlDialog* createView() override
	{
		return new gui::FrequencyShifterControlDialog(this);
	}
	int controlCount() override
	{
		return 20;
	}

	FrequencyShifterEffect* m_effect;
	FloatModel m_mix;
	FloatModel m_freqShift;
	FloatModel m_spreadShift;
	FloatModel m_ring;
	FloatModel m_feedback;
	FloatModel m_delayLengthLong;
	FloatModel m_delayLengthShort;
	FloatModel m_delayDamp;
	FloatModel m_delayGlide;
	FloatModel m_lfoAmount;
	FloatModel m_lfoRate;
	FloatModel m_lfoStereoPhase;
	FloatModel m_glide;
	FloatModel m_tone;

	FloatModel m_phase;

	BoolModel m_antireflect;
	IntModel m_routeMode;

	FloatModel m_harmonics;

	BoolModel m_resetShifter;
	BoolModel m_resetLfo;
};

} // namespace lmms

#endif // LMMS_FREQUENCY_SHIFTER_CONTROLS_H
