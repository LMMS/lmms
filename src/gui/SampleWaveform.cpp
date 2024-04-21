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

	const auto framesPerPixel = std::max<size_t>(1, parameters.size / width);

	const auto numPixels = std::min<size_t>(parameters.size, width);

	const auto scalingFactor = 
		halfHeight * parameters.amplification 
		/ static_cast<float>(parameters.buffer[0].size())
	;

	const auto maxFrames = numPixels * framesPerPixel;
	
	const auto color = painter.pen().color();
	const auto rmsColor = color.lighter(123);
	
	constexpr size_t maxFramesPerPixel = 24;
	
	const size_t frameStepSize = std::max<size_t>(framesPerPixel / maxFramesPerPixel, 1);
	
	for (size_t pixelIndex = 0; pixelIndex < numPixels; pixelIndex++) 
	{
		const auto i = pixelIndex * maxFrames / numPixels;
		size_t frameIndex = !parameters.reversed ? i : maxFrames - i;
		const auto frameIndexBound = std::min(maxFrames, frameIndex + framesPerPixel);
		
		float max = -100.0;
		float min =  100.0;
		float squared = 0.0;
		
		while (frameIndex < frameIndexBound)
		{
			const auto& frame = parameters.buffer[frameIndex];
			const auto value = std::accumulate(frame.begin(), frame.end(), 0.0f);

			if (max < value) max = value;
			if (min > value) min = value;
			
			squared += value * value;
			
			frameIndex += frameStepSize;
		}
		
		const auto lineY1 = centerY - max * scalingFactor;
		const auto lineY2 = centerY - min * scalingFactor;
		const auto lineX = pixelIndex + x;
		painter.drawLine(lineX, lineY1, lineX, lineY2);
		
		const auto rms = std::sqrt(squared / maxFramesPerPixel);
		const auto maxRMS = std::clamp(rms, min, max);
		const auto minRMS = std::clamp(-rms, min, max);

		const auto rmsLineY1 = centerY - maxRMS * scalingFactor;
		const auto rmsLineY2 = centerY - minRMS * scalingFactor;

		painter.setPen(rmsColor);
		painter.drawLine(lineX, rmsLineY1, lineX, rmsLineY2);
		painter.setPen(color);
	}
}

} // namespace lmms::gui
