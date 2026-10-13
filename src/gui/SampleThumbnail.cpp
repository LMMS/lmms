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
{
	if (buffer.empty()) { return; }

	auto highestResolutionThumbnail = Thumbnail{static_cast<int>(buffer.size())};
	for (auto i = 0; i < highestResolutionThumbnail.width(); ++i)
	{
		const auto value = buffer.data()[i].average();
		highestResolutionThumbnail[i] = Thumbnail::Peak{value, value};
	}

	m_thumbnails.emplace_back(std::move(highestResolutionThumbnail));
	while (m_thumbnails.back().width() >= AggregationPerZoomStep)
	{
		auto thumbnail = m_thumbnails.back().compress(AggregationPerZoomStep);
		m_thumbnails.emplace_back(std::move(thumbnail));
	}
}

auto SampleThumbnailCache::thumbnailForWidth(int width) -> Thumbnail&
{
	assert(1 <= width && width <= m_thumbnails.begin()->width() && "invalid width");

	const auto it = std::lower_bound(m_thumbnails.begin(), m_thumbnails.end(), width,
		[](const Thumbnail& thumbnail, int width) { return thumbnail.width() < width; });

	// This should not happen, the finest resolution stored is always 1 sample per pixel
	assert(it != m_thumbnails.end());

	if (it->width() == width) { return *it; }

	auto thumbnail = it->compress(it->width() / width);
	return *m_thumbnails.insert(it, std::move(thumbnail));
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

void SampleThumbnailCache::visualize(VisualizeParameters parameters, QPainter& painter) const
{
	const auto& sampleRect = parameters.sampleRect;
	const auto& viewportRect = parameters.viewportRect.isNull() ? sampleRect : parameters.viewportRect;

	const auto renderRect = sampleRect.intersected(viewportRect);
	if (renderRect.isNull()) { return; }

	const auto sampleRange = parameters.sampleEnd - parameters.sampleStart;
	if (sampleRange <= 0.0f || sampleRange > 1.0f) { return; }

	const auto targetThumbnailWidth = static_cast<int>(sampleRect.width() / sampleRange);
	const auto finerThumbnail = std::find_if(m_thumbnails.rbegin(), m_thumbnails.rend(),
		[&](const auto& thumbnail) { return thumbnail.width() >= targetThumbnailWidth; });

	if (finerThumbnail == m_thumbnails.rend()) { return; }

	painter.save();
	painter.setRenderHint(QPainter::Antialiasing, true);

	const auto thumbnailBeginForward
		= std::max<int>(renderRect.x() - sampleRect.x(), parameters.sampleStart * targetThumbnailWidth);
	const auto thumbnailEndForward = std::max<int>(
		renderRect.x() + renderRect.width() - sampleRect.x(), parameters.sampleEnd * targetThumbnailWidth);
	const auto thumbnailBegin
		= parameters.reversed ? targetThumbnailWidth - thumbnailBeginForward - 1 : thumbnailBeginForward;
	const auto thumbnailEnd = parameters.reversed ? targetThumbnailWidth - thumbnailEndForward : thumbnailEndForward;
	const auto advanceThumbnailBy = parameters.reversed ? -1 : 1;

	const auto finerThumbnailScaleFactor = static_cast<double>(finerThumbnail->width()) / targetThumbnailWidth;
	const auto yScale = renderRect.height() / 2 * parameters.amplification;

	for (auto x = renderRect.x(), i = thumbnailBegin; x < renderRect.x() + renderRect.width() && i != thumbnailEnd;
		++x, i += advanceThumbnailBy)
	{
		const auto beginIndex
			= std::clamp<size_t>(std::floor(i * finerThumbnailScaleFactor), 0, finerThumbnail->width() - 1);
		const auto endIndex
			= std::clamp<size_t>(std::ceil((i + 1) * finerThumbnailScaleFactor), 0, finerThumbnail->width() - 1);

		auto minPeak = 0.f;
		auto maxPeak = 0.f;

		const auto beginAggregationAt = finerThumbnail->begin() + beginIndex;
		const auto endAggregationAt = finerThumbnail->begin() + endIndex;
		const auto peak = std::accumulate(beginAggregationAt, endAggregationAt, Thumbnail::Peak{});
		minPeak = peak.min;
		maxPeak = peak.max;

		const auto yMin = renderRect.center().y() - minPeak * yScale;
		const auto yMax = renderRect.center().y() - maxPeak * yScale;
		painter.drawLine(x, yMin, x, yMax);
	}

	painter.restore();
}

} // namespace lmms::gui
