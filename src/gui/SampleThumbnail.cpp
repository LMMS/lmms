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

#include "SampleThumbnail.h"

#include <QFileInfo>
#include <QPainter>

namespace {
constexpr auto AggregationPerZoomStep = 10;
} // namespace

namespace lmms::gui {

SampleThumbnailCache::SampleThumbnailCache(const SampleBuffer& buffer)
	: m_sampleCount{buffer.size()}
{
	if (buffer.empty()) { return; }

	auto highestResolutionThumbnail = Thumbnail{static_cast<int>(m_sampleCount)};
	for (auto i = 0; i < highestResolutionThumbnail.width(); ++i)
	{
		const auto value = buffer.data()[i].average();
		highestResolutionThumbnail[i] = Thumbnail::Peak{value, value};
	}

	// The thumbnail with the highest resolution is the entire sample buffer summed down to mono
	m_thumbnails.emplace_back(std::move(highestResolutionThumbnail));

	// The thumbnails after it are increasingly compressed via the peaks being aggregated
	while (m_thumbnails.back().width() >= AggregationPerZoomStep)
	{
		auto thumbnail = m_thumbnails.back().compress(AggregationPerZoomStep);
		m_thumbnails.emplace_back(std::move(thumbnail));
	}
}

auto SampleThumbnailCache::thumbnailForWidth(int width) -> Thumbnail&
{
	assert(1 <= width && width <= m_sampleCount && "invalid width");

	// The thumbnails are in descending order, starting from the largest width to the smallest
	// We want to find the last element that is still greater than the requested width
	// This is the same as finding the first element smaller than the requested width and moving back one
	const auto it = std::lower_bound(m_thumbnails.begin(), m_thumbnails.end(), width,
		[](const Thumbnail& thumbnail, int width) { return thumbnail.width() > width; });

	// It should exist, the highest resolution stored is always the longest possible width
	assert(it != m_thumbnails.end());

	// Check if the one we found is equal before moving back
	if (it->width() == width) { return *it; }

	// Move back by one to get the nearest larger thumbnail
	const auto nearest = std::prev(it);

	auto thumbnail = nearest->compress(nearest->width() / width);
	return *m_thumbnails.insert(it, std::move(thumbnail));
}

auto SampleThumbnailCache::thumbnailForSamplesPerPixel(int samplesPerPixel) -> Thumbnail&
{
	assert(1 <= samplesPerPixel && samplesPerPixel <= m_sampleCount && "invalid samples per pixel");
	const auto width = (m_sampleCount + samplesPerPixel - 1) / samplesPerPixel;
	return thumbnailForWidth(width);
}

auto SampleThumbnailCache::Thumbnail::compress(int factor) const -> Thumbnail
{
	assert(factor > 0 && "factor must be greater than 0");

	// A ceiling needs to be applied when dealing with factors that do not cleanly divide the number of peaks
	// An extra peak is needed for the trailing remainder in this case
	auto compressedThumbnail = Thumbnail{static_cast<int>((m_peaks.size() + factor - 1) / factor)};

	for (auto in = 0, out = 0; in < m_peaks.size(); in += factor, ++out)
	{
		const auto end = std::min(in + factor, static_cast<int>(m_peaks.size()));
		compressedThumbnail[out] = std::accumulate(m_peaks.begin() + in, m_peaks.begin() + end, Thumbnail::Peak{});
	}

	return compressedThumbnail;
}

void SampleThumbnailCache::visualize(VisualizeParameters parameters, QPainter& painter)
{
	parameters.sampleEnd = std::min(parameters.sampleEnd, m_sampleCount);

	if (parameters.sampleStart >= m_sampleCount || parameters.sampleEnd <= parameters.sampleStart) { return; }

	const auto numSamples = parameters.sampleEnd - parameters.sampleStart;
	const auto samplesPerPixel = numSamples / parameters.rect.width();
	const auto& thumbnail = thumbnailForSamplesPerPixel(samplesPerPixel);

	const auto drawStart = std::max(parameters.viewportStart, parameters.rect.x());
	const auto drawEnd = std::max(parameters.viewportEnd, parameters.rect.x() + parameters.rect.width());

	const auto yScale = parameters.rect.height() / 2 * parameters.amplification;
	const auto yCenter = parameters.rect.center().y();

	painter.save();
	painter.setRenderHint(QPainter::Antialiasing, true);

	for (auto x = drawStart; x <= drawEnd; ++x)
	{
		const auto peakIndex = x - parameters.rect.x();

		if (0 <= peakIndex && peakIndex < thumbnail.width())
		{
			const auto yMin = yCenter - thumbnail[peakIndex].min * yScale;
			const auto yMax = yCenter - thumbnail[peakIndex].max * yScale;
			painter.drawLine(x, yMin, x, yMax);
		}
		else
		{
			painter.drawLine(x, yCenter, x, yCenter);
		}
	}

	painter.restore();
}

} // namespace lmms::gui
