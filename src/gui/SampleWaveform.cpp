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
	const int x = rect.x();
	const int height = rect.height();
	const int width = rect.width();
	const int centerY = rect.center().y();

	const int halfHeight = height / 2;

	const auto color = painter.pen().color();
	const auto rmsColor = color.lighter(123);

	const float framesPerPixel = std::max(1.0f, static_cast<float>(parameters.size) / width);

	constexpr float maxFramesPerPixel = 512.0f;
	const float resolution = std::max(1.0f, framesPerPixel / maxFramesPerPixel);
	const float framesPerResolution = framesPerPixel / resolution;

	const size_t numPixels = std::min<size_t>(parameters.size, width);
	auto min = std::vector<float>(numPixels, 1);
	auto max = std::vector<float>(numPixels, -1);
	auto squared = std::vector<float>(numPixels, 0);

	const size_t maxFrames = numPixels * static_cast<size_t>(framesPerPixel);

	auto pixelIndex = std::size_t{0};

	for (auto i = std::size_t{0}; i < maxFrames; i += static_cast<std::size_t>(resolution))
	{
		pixelIndex = i / framesPerPixel;
		const auto frameIndex = !parameters.reversed ? i : maxFrames - i;

		const auto& frame = parameters.buffer[frameIndex];
		const auto value = frame.average();

		if (value > max[pixelIndex]) { max[pixelIndex] = value; }
		if (value < min[pixelIndex]) { min[pixelIndex] = value; }

		squared[pixelIndex] += value * value;
	}
	
	while (pixelIndex < numPixels)
	{
		max[pixelIndex] = 0.0;
		min[pixelIndex] = 0.0;
		
		pixelIndex++;
	}

	for (auto i = std::size_t{0}; i < numPixels; i++)
	{
		const int lineY1 = centerY - max[i] * halfHeight * parameters.amplification;
		const int lineY2 = centerY - min[i] * halfHeight * parameters.amplification;
		const int lineX = static_cast<int>(i) + x;
		painter.drawLine(lineX, lineY1, lineX, lineY2);

		const float rms = std::sqrt(squared[i] / framesPerResolution);
		const float maxRMS = std::clamp(rms, min[i], max[i]);
		const float minRMS = std::clamp(-rms, min[i], max[i]);

		const int rmsLineY1 = centerY - maxRMS * halfHeight * parameters.amplification;
		const int rmsLineY2 = centerY - minRMS * halfHeight * parameters.amplification;

		painter.setPen(rmsColor);
		painter.drawLine(lineX, rmsLineY1, lineX, rmsLineY2);
		painter.setPen(color);
	}
}

} // namespace lmms::gui
