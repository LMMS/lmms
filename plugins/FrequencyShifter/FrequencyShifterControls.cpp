/*
 * FrequencyShifterControls.cpp
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

#include "FrequencyShifterEffect.h"
#include "FrequencyShifterControls.h"

#include <QDomElement>

namespace lmms
{

FrequencyShifterControls::FrequencyShifterControls(FrequencyShifterEffect* e) :
	EffectControls(e),
	m_effect(e),
	m_mix(1.f, 0.f, 1.f, 0.01f, this, "Mix"),
	m_freqShift(0.f, -20000.f, 20000.f, 0.001f, this, "Frequency Shift"),
	m_spreadShift(0.f, -50.f, 50.f, 0.01f, this, "Spread Shift"),
	m_ring(0.f, 0.f, 1.f, 0.01f, this, "Ring"),
	m_feedback(0.f, 0.f, 1.f, 0.001f, this, "Feedback"),
	m_delayLengthLong(0.f, 0.f, 2000.f, 0.001f, this, "Delay Length"),
	m_delayLengthShort(0.f, 0.f, 20.f, 0.001f, this, "Fine Delay Length"),
	m_delayDamp(22000.f, 100.f, 22000.f, 0.1f, this, "Delay Damping"),
	m_delayGlide(0.05f, 0.f, 1.f, 0.0001f, this, "Delay Glide"),
	m_lfoAmount(0.f, 0.f, 2000.f, 0.001f, this, "LFO Amount"),
	m_lfoRate(0.2f, 0.f, 200.f, 0.001f, this, "LFO Rate"),
	m_lfoStereoPhase(0.f, 0.f, 1.f, 0.001f, this, "LFO StereoPhase"),
	m_glide(0.05f, 0.f, 1.f, 0.0001f, this, "Glide"),
	m_tone(22000.f, 100.f, 22000.f, 0.1f, this, "Tone"),
	m_phase(0.f, -2.f, 2.f, 0.00001f, this, "Phase"),
	m_antireflect(false, this, "Antireflect"),
	m_routeMode(0, 0, 2, this, "Route Mode"),
	m_harmonics(0.f, 0.f, 1.f, 0.0001f, this, "Harmonics"),
	m_resetShifter(false, this, "Reset Shifter"),
	m_resetLfo(false, this, "Reset LFO")
{
	m_spreadShift.setScaleLogarithmic(true);
	m_delayLengthLong.setScaleLogarithmic(true);
	m_delayLengthShort.setScaleLogarithmic(true);
	m_delayDamp.setScaleLogarithmic(true);
	m_delayGlide.setScaleLogarithmic(true);
	m_lfoAmount.setScaleLogarithmic(true);
	m_lfoRate.setScaleLogarithmic(true);
	m_glide.setScaleLogarithmic(true);
	m_tone.setScaleLogarithmic(true);
}

void FrequencyShifterControls::loadSettings(const QDomElement& e)
{
	m_mix.loadSettings(e, "mix");
	m_freqShift.loadSettings(e, "freqShift");
	m_spreadShift.loadSettings(e, "spreadShift");
	m_ring.loadSettings(e, "ring");
	m_feedback.loadSettings(e, "feedback");
	m_delayLengthLong.loadSettings(e, "m_delayLengthLong");
	m_delayLengthShort.loadSettings(e, "delayLengthShort");
	m_delayDamp.loadSettings(e, "delayDamp");
	m_delayGlide.loadSettings(e, "delayGlide");
	m_lfoAmount.loadSettings(e, "lfoAmount");
	m_lfoRate.loadSettings(e, "lfoRate");
	m_lfoStereoPhase.loadSettings(e, "lfoStereoPhase");
	m_antireflect.loadSettings(e, "antireflect");
	m_routeMode.loadSettings(e, "routeMode");
	m_harmonics.loadSettings(e, "harmonics");
	m_glide.loadSettings(e, "glide");
	m_tone.loadSettings(e, "tone");
	m_phase.loadSettings(e, "phase");
}

void FrequencyShifterControls::saveSettings(QDomDocument& doc, QDomElement& e)
{
	m_mix.saveSettings(doc, e, "mix");
	m_freqShift.saveSettings(doc, e, "freqShift");
	m_spreadShift.saveSettings(doc, e, "spreadShift");
	m_ring.saveSettings(doc, e, "ring");
	m_feedback.saveSettings(doc, e, "feedback");
	m_delayLengthLong.saveSettings(doc, e, "m_delayLengthLong");
	m_delayLengthShort.saveSettings(doc, e, "delayLengthShort");
	m_delayDamp.saveSettings(doc, e, "delayDamp");
	m_delayGlide.saveSettings(doc, e, "delayGlide");
	m_lfoAmount.saveSettings(doc, e, "lfoAmount");
	m_lfoRate.saveSettings(doc, e, "lfoRate");
	m_lfoStereoPhase.saveSettings(doc, e, "lfoStereoPhase");
	m_antireflect.saveSettings(doc, e, "antireflect");
	m_routeMode.saveSettings(doc, e, "routeMode");
	m_harmonics.saveSettings(doc, e, "harmonics");
	m_glide.saveSettings(doc, e, "glide");
	m_tone.saveSettings(doc, e, "tone");
	m_phase.saveSettings(doc, e, "phase");
}

} // namespace lmms

