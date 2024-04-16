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

void SampleWaveform::visualize(Parameters parameters, QPainter& painter, const QRect& rect)
{
	const auto x = rect.x();
	const auto height = rect.height();
	const auto width = rect.width();
	const auto centerY = rect.center().y();

	const auto halfHeight = height / 2;

	const auto color = painter.pen().color();
	const auto rmsColor = color.lighter(123);

	const auto framesPerPixel = std::max<size_t>(1, parameters.size / width);

	constexpr auto maxFramesPerPixel = 512;
	const auto resolution = std::max<size_t>(1, framesPerPixel / maxFramesPerPixel);
	const auto framesPerResolution = framesPerPixel / resolution;

	const auto numPixels = std::min<size_t>(parameters.size, width);
	auto min = std::vector<float>(numPixels, 1);
	auto max = std::vector<float>(numPixels, -1);
	auto squared = std::vector<float>(numPixels);

	const auto maxFrames = numPixels * framesPerPixel;
	for (int i = 0; i < maxFrames; i += resolution)
	{
		const auto pixelIndex = i / framesPerPixel;
		const auto frameIndex = !parameters.reversed ? i : maxFrames - i;

		const auto& frame = parameters.buffer[frameIndex];
		const auto value = std::accumulate(frame.begin(), frame.end(), 0.0f) / frame.size();

		if (value > max[pixelIndex]) { max[pixelIndex] = value; }
		if (value < min[pixelIndex]) { min[pixelIndex] = value; }

		squared[pixelIndex] += value * value;
	}

	for (int i = 0; i < numPixels; i++)
	{
		const auto lineY1 = centerY - max[i] * halfHeight * parameters.amplification;
		const auto lineY2 = centerY - min[i] * halfHeight * parameters.amplification;
		const auto lineX = i + x;
		painter.drawLine(lineX, lineY1, lineX, lineY2);

		const auto rms = std::sqrt(squared[i] / framesPerResolution);
		const auto maxRMS = std::clamp(rms, min[i], max[i]);
		const auto minRMS = std::clamp(-rms, min[i], max[i]);

		const auto rmsLineY1 = centerY - maxRMS * halfHeight * parameters.amplification;
		const auto rmsLineY2 = centerY - minRMS * halfHeight * parameters.amplification;

		painter.setPen(rmsColor);
		painter.drawLine(lineX, rmsLineY1, lineX, rmsLineY2);
		painter.setPen(color);
	}
}

} // namespace lmms::gui
