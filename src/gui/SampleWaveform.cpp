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

	const auto scaling_factor = 
		halfHeight * parameters.amplification 
		/ static_cast<float>(parameters.buffer[0].size())
	;

	const auto maxFrames = numPixels * framesPerPixel;
	
	const auto color = painter.pen().color();
	const auto rmsColor = color.lighter(123);
	
	// Cap at this many frames per pixel index.
	constexpr size_t MAX_FRAMES_PER_PIXEL = 16;
	
	constexpr float MAX_FRAMES_PER_PIXEL_RECIP = 
		1.0 / static_cast<float>(MAX_FRAMES_PER_PIXEL)
	;
	
	const size_t STEP_SIZE = std::max<size_t>(framesPerPixel / MAX_FRAMES_PER_PIXEL, 1);
	
	for (size_t pixelIndex = 0; pixelIndex < numPixels; pixelIndex++) 
	{
		
		const auto i = pixelIndex * maxFrames / numPixels;
		size_t frameIndex = !parameters.reversed ? i : maxFrames - i;
		const auto frameIndex_bound = std::min(maxFrames, frameIndex + framesPerPixel);
		
		float max = -100.0;
		float min =  100.0;
		float squared = 0.0;
		
		while (frameIndex < frameIndex_bound)
		{
			const auto& frame = parameters.buffer[frameIndex];
			const auto value = std::accumulate(frame.begin(), frame.end(), 0.0f);

			if (max < value) max = value;
			if (min > value) min = value;
			
			squared += value * value;
			
			frameIndex += STEP_SIZE;
		}
		
		const auto lineY1 = centerY - max * scaling_factor;
		const auto lineY2 = centerY - min * scaling_factor;
		const auto lineX = pixelIndex + x;
		painter.drawLine(lineX, lineY1, lineX, lineY2);
		
		const auto rms = std::sqrt(squared * MAX_FRAMES_PER_PIXEL_RECIP);
		const auto maxRMS = std::clamp(rms, min, max);
		const auto minRMS = std::clamp(-rms, min, max);

		const auto rmsLineY1 = centerY - maxRMS * scaling_factor;
		const auto rmsLineY2 = centerY - minRMS * scaling_factor;

		painter.setPen(rmsColor);
		painter.drawLine(lineX, rmsLineY1, lineX, rmsLineY2);
		painter.setPen(color);
	}
}

} // namespace lmms::gui
