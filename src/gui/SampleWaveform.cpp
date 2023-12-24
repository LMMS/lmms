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

namespace lmms::gui {

void SampleWaveform::visualize(const Sample& sample, QPainter& p, const QRect& dr, int fromFrame, int toFrame)
{
	if (sample.sampleSize() == 0) { return; }

	const auto x = dr.x();
	const auto height = dr.height();
	const auto width = dr.width();
	const auto centerY = dr.center().y();

	const auto halfHeight = height / 2;
	const auto buffer = sample.data() + fromFrame;

	const auto color = p.pen().color();
	const auto rmsColor = color.lighter(123);

	auto numFrames = toFrame - fromFrame;
	if (numFrames == 0) { numFrames = sample.sampleSize(); }

	const auto framesPerPixel = std::max(1, numFrames / width);

	constexpr auto maxFramesPerPixel = 512;
	const auto resolution = std::max(1, framesPerPixel / maxFramesPerPixel);
	const auto framesPerResolution = framesPerPixel / resolution;

	const auto numPixels = std::min(numFrames, width);
	auto min = std::vector<float>(numPixels, 1);
	auto max = std::vector<float>(numPixels, -1);
	auto squared = std::vector<float>(numPixels);

	const auto maxFrames = numPixels * framesPerPixel;
	for (int i = 0; i < maxFrames; i += resolution)
	{
		const auto pixelIndex = i / framesPerPixel;
		const auto value = std::accumulate(buffer[i].begin(), buffer[i].end(), 0.0f) / buffer[i].size();
		if (value > max[pixelIndex]) { max[pixelIndex] = value; }
		if (value < min[pixelIndex]) { min[pixelIndex] = value; }
		squared[pixelIndex] += value * value;
	}

	const auto amplification = sample.amplification();
	const auto reversed = sample.reversed();

	for (int i = 0; i < numPixels; i++)
	{
		const auto lineY1 = centerY - max[i] * halfHeight * amplification;
		const auto lineY2 = centerY - min[i] * halfHeight * amplification;

		auto lineX = i + x;
		if (reversed) { lineX = width - lineX; }

		p.drawLine(lineX, lineY1, lineX, lineY2);

		const auto rms = std::sqrt(squared[i] / framesPerResolution);
		const auto maxRMS = std::clamp(rms, min[i], max[i]);
		const auto minRMS = std::clamp(-rms, min[i], max[i]);

		const auto rmsLineY1 = centerY - maxRMS * halfHeight * amplification;
		const auto rmsLineY2 = centerY - minRMS * halfHeight * amplification;

		p.setPen(rmsColor);
		p.drawLine(lineX, rmsLineY1, lineX, rmsLineY2);
		p.setPen(color);
	}
}

} // namespace lmms::gui
