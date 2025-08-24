/*
 * Oscilloscope.h - Oscilloscope effect to preview the incoming waveform
 *
 * Copyright (c) 2025 Keratin
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

#ifndef LMMS_OSCILLOSCOPE_H
#define LMMS_OSCILLOSCOPE_H

#include "Effect.h"
#include "OscilloscopeControls.h"

namespace lmms
{

class Oscilloscope : public Effect
{
public:
	Oscilloscope(Model* parent, const Descriptor::SubPluginFeatures::Key* key);

	ProcessStatus processImpl(SampleFrame* buf, const fpp_t frames) override;

	EffectControls* controls() override
	{
		return &m_controls;
	}

	static const int BUFFER_SIZE = 44100*3;

	std::span<const SampleFrame, BUFFER_SIZE> buffer() const { return m_ringBuffer; }
	int bufferIndex() const { return m_ringBufferIndex; }

private:
	OscilloscopeControls m_controls;

	std::array<SampleFrame, BUFFER_SIZE> m_ringBuffer = {};
	int m_ringBufferIndex = 0;

	friend class OscilloscopeControls;
};

} // namespace lmms

#endif // LMMS_OSCILLOSCOPE_H
