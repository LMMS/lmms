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

void SampleWaveform::visualize(Parameters parameters, QPainter& painter, const QRect& rect, const QRect& viewport)
{
	if (parameters.buffer == nullptr || parameters.size == 0) { return; }

	const auto samplePerPixel = static_cast<double>(parameters.size) / rect.width();
	const auto verticalScale = rect.height() / 2.0;

	painter.save();
	painter.setRenderHint(QPainter::Antialiasing, true);

	for (auto xPos = viewport.x(); xPos < viewport.x() + viewport.width(); ++xPos)
	{
		const auto xPosOffsetInRect = xPos - rect.x();
		const auto pixelPosInRect = parameters.reversed ? (rect.x() + rect.width()) - xPosOffsetInRect : xPosOffsetInRect;

		const auto startSampleIndex = static_cast<std::size_t>(std::floor(pixelPosInRect * samplePerPixel));
		const auto endSampleIndex = static_cast<std::size_t>(std::ceil((pixelPosInRect + 1) * samplePerPixel));

		if (startSampleIndex < 0 || startSampleIndex >= parameters.size) { continue; }
		if (endSampleIndex < 0 || endSampleIndex > parameters.size) { continue; }

		const auto startSample = parameters.buffer + startSampleIndex;
		const auto endSample = parameters.buffer + endSampleIndex;

		const auto [minPeak, maxPeak] = std::minmax_element(&startSample->left(), &endSample->right());
		const auto yMin = rect.center().y() - *minPeak * parameters.amplification * verticalScale;
		const auto yMax = rect.center().y() - *maxPeak * parameters.amplification * verticalScale;

		painter.drawLine(xPos, yMin, xPos, yMax);
	}

	painter.restore();
}
} // namespace lmms::gui
