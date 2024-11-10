/*
 * SampleWaveform.cpp
 *
 * Copyright (c) 2023 saker <sakertooth@gmail.com>
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

#include "SampleWaveform.h"
#include "interpolation.h"

namespace lmms::gui {

SampleWaveform::SampleWaveform(const SampleFrame* buffer, size_t size)
	: m_buffer(buffer)
	, m_size(size)
{
}

void SampleWaveform::generate()
{
	if (!m_buffer || m_size == 0) { return; }

	m_min.clear();
	m_max.clear();

	const auto maxLevel = static_cast<int>(std::log2(m_size));
	for (auto level = 1; level <= maxLevel; ++level)
	{
		const auto downsampledSize = m_size / static_cast<int>(std::exp2(level));
		m_min[level].resize(downsampledSize);
		m_max[level].resize(downsampledSize);

		if (level == 1)
		{
			for (auto i = std::size_t{0}; i < downsampledSize; ++i)
			{
				static auto peakComp = [](const SampleFrame& a, const SampleFrame& b) { return a.average() < b.average(); };
				m_min[level][i] = std::min_element(&m_buffer[i * 2], &m_buffer[i * 2 + 2], peakComp)->average();
				m_max[level][i] = std::max_element(&m_buffer[i * 2], &m_buffer[i * 2 + 2], peakComp)->average();
			}
		}
		else
		{
			for (auto i = std::size_t{0}; i < downsampledSize; ++i)
			{
				m_min[level][i] = *std::min_element(&m_min[level - 1][i * 2], &m_min[level - 1][i * 2 + 2]);
				m_max[level][i] = *std::max_element(&m_max[level - 1][i * 2], &m_max[level - 1][i * 2 + 2]);
			}
		}
	}
}

void SampleWaveform::visualize(QPainter& painter, const QRect& rect, float amplification, bool reversed, std::optional<size_t> from, std::optional<size_t> to)
{
	if (!m_buffer || m_size == 0) { return; }

	const auto sampleBegin = from.has_value() ? from.value() : 0;
	const auto sampleEnd = to.has_value() ? to.value() : m_size;
	const auto sampleRange = sampleEnd - sampleBegin;
	const auto samplesPerPixel = std::max(1, static_cast<int>(sampleRange) / rect.width());

	const auto downsampledLevelLow = static_cast<int>(std::log2(samplesPerPixel));
	const auto downsampledLevelHigh = static_cast<int>(std::ceil(std::log2(samplesPerPixel)));

	const auto downsampledFactorLow = static_cast<int>(std::exp2(downsampledLevelLow));
	const auto downsampledFactorHigh = static_cast<int>(std::exp2(downsampledLevelHigh));

	const auto ratio = downsampledFactorHigh == downsampledFactorLow
		? 1.0f
		: static_cast<float>(samplesPerPixel - downsampledFactorLow) / (downsampledFactorHigh - downsampledFactorLow);

	const auto centerY = rect.center().y();
	const auto centerHeight = rect.height() / 2.0f;

	for (auto i = 0; i < rect.width(); ++i)
	{	
		const auto index = static_cast<float>(i * samplesPerPixel);
		const auto lowIndex = static_cast<int>(std::floor(index / downsampledFactorLow));
		const auto highIndex = static_cast<int>(std::ceil(index / downsampledFactorHigh));

		const auto minPeak = linearInterpolate(m_min[downsampledLevelLow][lowIndex], m_min[downsampledLevelHigh][highIndex], ratio);
		const auto maxPeak = linearInterpolate(m_max[downsampledLevelLow][lowIndex], m_max[downsampledLevelHigh][highIndex], ratio);

		const auto lineMin = centerY - minPeak * centerHeight * amplification;
		const auto lineMax = centerY - maxPeak * centerHeight * amplification;
		const auto lineX = rect.x() + static_cast<int>(i);
		painter.drawLine(lineX, lineMax, lineX, lineMin);
	}
}

void SampleWaveform::reset(const SampleFrame* buffer, size_t size)
{
	m_buffer = buffer;
	m_size = size;
	generate();
}

} // namespace lmms::gui
