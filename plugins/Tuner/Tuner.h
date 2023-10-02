/*
 * Tuner.h - estimate the pitch of an audio signal
 *
 * Copyright (c) 2022 sakertooth <sakertooth@gmail.com>
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

#ifndef LMMS_TUNER_H
#define LMMS_TUNER_H

#include <array>
#include <aubio/aubio.h>
#include <chrono>

#include "Effect.h"
#include "TunerControls.h"

namespace lmms {

class Tuner : public Effect
{
	Q_OBJECT
public:
	static constexpr auto WINDOW_SIZE = 8192;
	static constexpr auto HOP_SIZE = WINDOW_SIZE / 4;

	Tuner(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
	~Tuner() override;

	auto processAudioBuffer(sampleFrame* buf, const fpp_t frames) -> bool override;
	auto controls() -> EffectControls* override;

	auto detectPitch(float* data, size_t size) -> float;

signals:
	auto frequencyCalculated(float frequency) -> void;

private:
	TunerControls m_tunerControls;
	aubio_pitch_t* m_aubioPitch;
	std::array<float, HOP_SIZE> m_aubioInput;
	int m_numAubioInputFrames;
	friend class TunerControls;
};
} // namespace lmms

#endif // LMMS_TUNER_H
