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
	if (!parameters.buffer || parameters.size == 0) { return; }

	const auto centerY = rect.center().y();
	const auto halfHeight = rect.height() / 2;

	const auto color = painter.pen().color();
	const auto rmsColor = color.lighter(123);

	const auto samplesPerPixel = std::max(1.0, static_cast<double>(parameters.size) / rect.width());
	const auto numPixelsToDraw = std::min(static_cast<int>(parameters.size), rect.width());

	const auto squaredSumFn = [](auto acc, auto x) { return acc + x * x; };

	for (auto i = 0; i < numPixelsToDraw; ++i)
	{
		const auto start = parameters.buffer + static_cast<int>(std::floor(i * samplesPerPixel));
		const auto end = parameters.buffer + static_cast<int>(std::ceil((i + 1) * samplesPerPixel));
		const auto [minPeak, maxPeak] = std::minmax_element(&start->left(), &end->right() + 1);

		const auto lineY1 = centerY - *maxPeak * halfHeight * parameters.amplification;
		const auto lineY2 = centerY - *minPeak * halfHeight * parameters.amplification;
		const auto lineX = rect.x() + (parameters.reversed ? numPixelsToDraw - i : i);

		const auto squaredSum = std::accumulate(&start->left(), &end->right() + 1, 0.0f, squaredSumFn);
		const auto rms = std::sqrt(squaredSum / (samplesPerPixel * DEFAULT_CHANNELS));
		const auto rmsLineY1 = centerY - rms * halfHeight * parameters.amplification;
		const auto rmsLineY2 = centerY + rms * halfHeight * parameters.amplification;

		painter.drawLine(lineX, lineY1, lineX, lineY2);
		painter.setPen(rmsColor);
		painter.drawLine(lineX, rmsLineY1, lineX, rmsLineY2);
		painter.setPen(color);
	}
}

} // namespace lmms::gui
