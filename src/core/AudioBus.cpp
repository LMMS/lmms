/*
 * AudioBus.cpp
 *
 * Copyright (c) 2025 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include "AudioBus.h"

#include <algorithm>
#include <type_traits>

#include "LmmsPolyfill.h"
#include "MixHelpers.h"

namespace lmms
{

namespace
{
/*
 * In the past, the RMS was calculated then compared with a threshold of 10^(-10).
 * Now we use a different algorithm to determine whether a buffer is non-quiet, so
 * a new threshold is needed for the best compatibility. The following is how it's derived.
 *
 * Old method:
 * RMS = average (L^2 + R^2) across stereo buffer.
 * RMS threshold = 10^(-10)
 *
 * So for a single channel, it would be:
 * RMS/2 = average M^2 across single channel buffer.
 * RMS/2 threshold = 5^(-11)
 *
 * The new algorithm for determining whether a buffer is non-silent compares M with the threshold,
 * not M^2, so the square root of M^2's threshold should give us the most compatible threshold for
 * the new algorithm:
 *
 * (RMS/2)^0.5 = (5^(-11))^0.5 = 0.0001431 (approx.)
 *
 * In practice though, the exact value shouldn't really matter so long as it's sufficiently small.
 */
constexpr auto SilenceThreshold = 0.0001431f;

} // namespace

auto AudioBus::hasInputNoise(const std::bitset<MaxTrackChannels>& usedChannels) const -> bool
{
	auto nonQuiet = ~m_quietChannels;
	nonQuiet &= usedChannels;
	return nonQuiet.any();
}

void AudioBus::sanitize(const std::bitset<MaxTrackChannels>& channels, track_ch_t upperBound)
{
	if (!MixHelpers::useNaNHandler()) { return; }

	const auto numTrackChannels = std::min(upperBound, this->channels());

	// Like MixHelpers::sanitize(), but works for a single channel
	auto sanitizeChannel = [](InterleavedBufferView<float, 2> output, auto channelIndex) -> bool {
		constexpr auto channel = channelIndex();
		for (float* frame : output.framesView())
		{
			float& sample = frame[channel];

			if (std::isinf(sample) || std::isnan(sample))
			{
#ifdef LMMS_DEBUG
				// TODO: don't use std::cout here?
				std::cout << "Bad data, clearing buffer. frame: " << static_cast<std::size_t>(frame - output.data());
				std::cout << ": value " << sample << " (" << (channel == 0 ? "L" : "R") << ")\n";
#endif

				// Clear the whole channel if a problem is found
				for (float* f : output.framesView())
				{
					f[channel] = 0.f;
				}

				return true;
			}
			else
			{
				sample = std::clamp(sample, sample_t(-1000.0), sample_t(1000.0));
			}
		}
		return false;
	};

	for (track_ch_t tc = 0; tc < numTrackChannels; tc += 2)
	{
		switch ((channels[tc] << 1) | std::uint8_t{channels[tc + 1]})
		{
			case 0b11:
			{
				const auto tcPair = m_bus[tc / 2];
				if (MixHelpers::sanitize(tcPair, m_frames))
				{
					// Inf/NaN detected and buffer cleared
					m_quietChannels[tc] = true;
					m_quietChannels[tc + 1] = true;
				}
				break;
			}
			case 0b10:
			{
				if (sanitizeChannel(trackChannelPair(tc / 2), std::integral_constant<proc_ch_t, 0>{}))
				{
					// Inf/NaN detected and buffer cleared
					m_quietChannels[tc] = true;
				}
				break;
			}
			case 0b01:
			{
				if (sanitizeChannel(trackChannelPair(tc / 2), std::integral_constant<proc_ch_t, 1>{}))
				{
					// Inf/NaN detected and buffer cleared
					m_quietChannels[tc + 1] = true;
				}
				break;
			}
			case 0b00:
				// Neither track channel needs to be sanitized
				break;
			default:
				unreachable();
				break;
		}
	}
}

void AudioBus::sanitizeAll()
{
	if (!MixHelpers::useNaNHandler()) { return; }

	for (track_ch_t tc = 0; tc < channelPairs(); ++tc)
	{
		if (MixHelpers::sanitize(m_bus[tc], m_frames))
		{
			// Inf/NaN detected and buffer cleared
			m_quietChannels[tc * 2] = true;
			m_quietChannels[tc * 2 + 1] = true;
		}
	}
}

auto AudioBus::update(const std::bitset<MaxTrackChannels>& channels, track_ch_t upperBound) -> bool
{
	assert(upperBound <= MaxTrackChannels);
	assert(upperBound % 2 == 0);

	bool allQuiet = true;

	const auto numTrackChannels = std::min(upperBound, this->channels());
	for (track_ch_t tc = 0; tc < numTrackChannels; tc += 2)
	{
		switch ((channels[tc] << 1) | std::uint8_t{channels[tc + 1]})
		{
			case 0b11:
			{
				const float* tcPair = (*this)[tc / 2];
				const auto samples = frames() * 2;

				bool quietL = true;
				bool quietR = true;
				for (std::size_t idx = 0; idx < samples; idx += 2)
				{
					if (std::abs(tcPair[idx]) > SilenceThreshold)
					{
						quietL = false;
						++idx;
						for (; idx < samples; idx += 2)
						{
							if (std::abs(tcPair[idx]) > SilenceThreshold)
							{
								quietR = false;
								break;
							}
						}
						break;
					}
					if (std::abs(tcPair[idx + 1]) > SilenceThreshold)
					{
						quietR = false;
						for (; idx < samples; idx += 2)
						{
							if (std::abs(tcPair[idx]) > SilenceThreshold)
							{
								quietL = false;
								break;
							}
						}
						break;
					}
				}
				m_quietChannels[tc] = quietL;
				m_quietChannels[tc + 1] = quietR;
				allQuiet = allQuiet && quietL && quietR;
				break;
			}
			case 0b10:
			{
				const auto tcPair = trackChannelPair(tc / 2);
				const auto quiet = std::ranges::all_of(tcPair.framesView(), [](const float* frame) {
					return std::abs(frame[0]) < SilenceThreshold;
				});
				m_quietChannels[tc] = quiet;
				allQuiet = allQuiet && quiet;
				break;
			}
			case 0b01:
			{
				const auto tcPair = trackChannelPair(tc / 2);
				const auto quiet = std::ranges::all_of(tcPair.framesView(), [](const float* frame) {
					return std::abs(frame[1]) < SilenceThreshold;
				});
				m_quietChannels[tc + 1] = quiet;
				allQuiet = allQuiet && quiet;
				break;
			}
			case 0b00:
				// Neither track channel needs to be updated
				break;
			default:
				unreachable();
				break;
		}
	}

	return allQuiet;
}

auto AudioBus::updateAll() -> bool
{
	return update(std::bitset<MaxTrackChannels>{}.set());
}

void AudioBus::silenceChannels(const std::bitset<MaxTrackChannels>& channels, track_ch_t upperBound)
{
	auto needSilenced = ~m_quietChannels;
	needSilenced &= channels;

	const auto numTrackChannels = std::min(upperBound, this->channels());
	for (track_ch_t tc = 0; tc < numTrackChannels; tc += 2)
	{
		switch ((needSilenced[tc] << 1) | std::uint8_t{needSilenced[tc + 1]})
		{
			case 0b11:
				// Zero both track channels
				std::ranges::fill(trackChannelPair(tc / 2).dataView(), 0.f);
				break;
			case 0b10:
			{
				// Zero left track channel
				auto tcPair = trackChannelPair(tc / 2);
				for (float* frame : tcPair.framesView())
				{
					frame[0] = 0.f;
				}
				break;
			}
			case 0b01:
			{
				// Zero right track channel
				auto tcPair = trackChannelPair(tc / 2);
				for (float* frame : tcPair.framesView())
				{
					frame[1] = 0.f;
				}
				break;
			}
			case 0b00:
				// Neither track channel is used
				break;
			default:
				unreachable();
				break;
		}
	}

	m_quietChannels |= channels;
}

void AudioBus::silenceAllChannels()
{
	for (track_ch_t tc = 0; tc < m_channelPairs; tc += 2)
	{
		std::fill_n(m_bus[tc], m_frames, SampleFrame{});
	}

	m_quietChannels.set();
}

} // namespace lmms
