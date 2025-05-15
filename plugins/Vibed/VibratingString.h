/*
 * VibratingString.h - model of a vibrating string lifted from pluckedSynth
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/yahoo/com>
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

#ifndef LMMS_VIBRATING_STRING_H
#define LMMS_VIBRATING_STRING_H

#include <memory>
#include <cstdlib>

#include "LmmsTypes.h"

namespace lmms
{


class VibratingString
{
public:
	VibratingString() = default;
	VibratingString(float pitch, float pick, float pickup, const float* impulse, int len,
		sample_rate_t sampleRate, int oversample, float randomize, float stringLoss, float detune, bool state);
	~VibratingString() = default;

	VibratingString(const VibratingString&) = delete;
	VibratingString& operator=(const VibratingString&) = delete;
	VibratingString(VibratingString&&) noexcept = delete;
	VibratingString& operator=(VibratingString&&) noexcept = default;

	sample_t nextSample()
	{
		for (int i = 0; i < m_oversample; ++i)
		{
			// Output at pickup position
			m_outsamp[i] = fromBridgeAccess(m_fromBridge.get(), m_pickupLoc);
			m_outsamp[i] += toBridgeAccess(m_toBridge.get(), m_pickupLoc);

			// Sample traveling into "bridge"
			sample_t ym0 = toBridgeAccess(m_toBridge.get(), 1);
			// Sample to "nut"
			sample_t ypM = fromBridgeAccess(m_fromBridge.get(), m_fromBridge->length - 2);

			// String state update

			// Decrement pointer and then update
			fromBridgeUpdate(m_fromBridge.get(), -bridgeReflection(ym0));
			// Update and then increment pointer
			toBridgeUpdate(m_toBridge.get(), -ypM);
		}

		return m_outsamp[m_choice];
	}

private:
	struct DelayLine
	{
		std::unique_ptr<sample_t[]> data;
		int length;
		sample_t* pointer;
		sample_t* end;
	};

	std::unique_ptr<DelayLine> m_fromBridge;
	std::unique_ptr<DelayLine> m_toBridge;
	int m_pickupLoc;
	int m_oversample;
	float m_randomize;
	float m_stringLoss;

	std::unique_ptr<float[]> m_impulse;
	int m_choice;
	float m_state;

	std::unique_ptr<sample_t[]> m_outsamp;

	std::unique_ptr<DelayLine> initDelayLine(int len);
	void resample(const float* src, f_cnt_t srcFrames, f_cnt_t dstFrames);

	/**
	 * setDelayLine initializes the string with an impulse at the pick
	 * position unless the impulse is longer than the string, in which
	 * case the impulse gets truncated.
	 */
	void setDelayLine(DelayLine* dl, int pick, const float* values, int len, float scale, bool state)
	{
		if (!state)
		{
			for (int i = 0; i < pick; ++i)
			{
				float r = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
				float offset = (m_randomize / 2.0f - m_randomize) * r;
				dl->data[i] = scale * values[dl->length - i - 1] + offset;
			}
			for (int i = pick; i < dl->length; ++i)
			{
				float r = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
				float offset = (m_randomize / 2.0f - m_randomize) * r;
				dl->data[i] = scale * values[i - pick]  + offset;
			}
		}
		else
		{
			if (len + pick > dl->length)
			{
				for (int i = pick; i < dl->length; ++i)
				{
					float r = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
					float offset = (m_randomize / 2.0f - m_randomize) * r;
					dl->data[i] = scale * values[i - pick] + offset;
				}
			}
			else
			{
				for (int i = 0; i < len; ++i)
				{
					float r = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
					float offset = (m_randomize / 2.0f - m_randomize) * r;
					dl->data[i+pick] = scale * values[i] + offset;
				}
			}
		}
	}

	/**
	 * toBridgeUpdate(dl, insamp);
	 * Places "nut-reflected" sample from upper delay-line into
	 * current lower delay-line pointer position (which represents
	 * x = 0 position). The pointer is then incremented (i.e. the
	 * wave travels one sample to the left), turning the previous
	 * position into an "effective" x = L position for the next
	 * iteration.
	 */
	void toBridgeUpdate(DelayLine* dl, sample_t insamp)
	{
		sample_t* ptr = dl->pointer;
		*ptr = insamp * m_stringLoss;
		++ptr;
		if (ptr > dl->end)
		{
			ptr = dl->data.get();
		}
		dl->pointer = ptr;
	}

	/**
	 * fromBridgeUpdate(dl, insamp);
	 * Decrements current upper delay-line pointer position (i.e.
	 * the wave travels one sample to the right), moving it to the
	 * "effective" x = 0 position for the next iteration. The
	 * "bridge-reflected" sample from lower delay-line is then placed
	 * into this position.
	 */
	void fromBridgeUpdate(DelayLine* dl, sample_t insamp)
	{
		sample_t* ptr = dl->pointer;
		--ptr;
		if (ptr < dl->data.get())
		{
			ptr = dl->end;
		}
		*ptr = insamp * m_stringLoss;
		dl->pointer = ptr;
	}

	/**
	 * dlAccess(dl, position);
	 * Returns sample "position" samples into delay-line's past.
	 * Position "0" points to the most recently inserted sample.
	 */
	static sample_t dlAccess(DelayLine* dl, int position)
	{
		sample_t* outpos = dl->pointer + position;
		while (outpos < dl->data.get())
		{
			outpos += dl->length;
		}
		while (outpos > dl->end)
		{
			outpos -= dl->length;
		}
		return *outpos;
	}

	/*
	 * Right-going delay line:
	 * -->---->---->---
	 * x=0
	 * (pointer)
	 * Left-going delay line:
	 * --<----<----<---
	 * x=0
	 * (pointer)
	 */

	/**
	 * fromBridgeAccess(dl, position);
	 * Returns spatial sample at position "position", where position zero
	 * is equal to the current upper delay-line pointer position (x = 0).
	 * In a right-going delay-line, position increases to the right, and
	 * delay increases to the right => left = past and right = future.
	 */
	static sample_t fromBridgeAccess(DelayLine* dl, int position)
	{
		return dlAccess(dl, position);
	}

	/**
	 * toBridgeAccess(dl, position);
	 * Returns spatial sample at position "position", where position zero
	 * is equal to the current lower delay-line pointer position (x = 0).
	 * In a left-going delay-line, position increases to the right, and
	 * delay DEcreases to the right => left = future and right = past.
	 */
	static sample_t toBridgeAccess(DelayLine* dl, int position)
	{
		return dlAccess(dl, position);
	}

	sample_t bridgeReflection(sample_t insamp)
	{
		m_state = (m_state + insamp) * 0.5;
		return m_state;
	}
};


} // namespace lmms

#endif // LMMS_VIBRATING_STRING_H
