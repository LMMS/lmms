/*
 * SampleThumbnail.cpp
 *
 * Copyright (c) 2024 Khoi Dau <casboi86@gmail.com>
 * Copyright (c) 2024 Sotonye Atemie <sakertooth@gmail.com>
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

#include "SampleThumbnailRenderer.h"

#include <QFileInfo>
#include <QPainter>

#include "SampleThumbnailCache.h"

namespace lmms::gui {

void SampleThumbnailRenderer::visualize(VisualizeParameters parameters, QPainter& painter)
{
	assert(0 <= parameters.sampleStart && parameters.sampleStart < parameters.sampleEnd && parameters.sampleEnd <= 1
		&& "invalid sample range");

	if (parameters.rect.width() == 0) { return; }

	// Get the number of samples that will be drawn in this rectangle
	const auto numSamples = (parameters.sampleEnd - parameters.sampleStart) * parameters.cache->sampleCount();

	// For our rectangle, how many samples each pixel has
	const auto samplesPerPixel = numSamples / parameters.rect.width();

	// We want a thumbnail that has the same SPP as our own rectangle
	// However, it needs to cover the full sample
	const auto nearestThumbnail
		= parameters.cache->nearestThumbnailForWidth(std::round(parameters.cache->sampleCount() / samplesPerPixel));

	const auto yScale = parameters.rect.height() / 2 * parameters.amplification;
	const auto yCenter = parameters.rect.center().y();

	// Calculate how far along we should be in the thumbnail based on where we begin and end in the sample
	const auto nearestThumbnailStart = parameters.sampleStart * nearestThumbnail.width();
	const auto nearestThumbnailEnd = parameters.sampleEnd * nearestThumbnail.width();
	const auto nearestThumbnailRange = nearestThumbnailEnd - nearestThumbnailStart;

	painter.save();
	painter.setRenderHint(QPainter::Antialiasing, true);

	for (auto out = 0; out < parameters.rect.width(); ++out)
	{
		const auto peakStart = nearestThumbnailStart + (out / parameters.rect.width()) * nearestThumbnailRange;
		const auto peakEnd = nearestThumbnailStart + ((out + 1) / parameters.rect.width()) * nearestThumbnailRange;
		const auto peak = std::accumulate(
			nearestThumbnail.begin() + peakStart, nearestThumbnail.begin() + peakEnd, SampleThumbnailCache::Thumbnail::Peak{});

		const auto yMin = yCenter - peak.min * yScale;
		const auto yMax = yCenter - peak.max * yScale;
		const auto x = parameters.rect.x() + out;
		painter.drawLine(x, yMin, x, yMax);
	}

	painter.restore();
}

} // namespace lmms::gui
