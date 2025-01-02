/*
 * SampleThumbnail.cpp
 *
 * Copyright (c) Copyright (c) 2024 Khoi Dau <casboi86@gmail.com>
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

#include <QDebug>
#include <QGuiApplication>
#include <QScreen>

namespace lmms {

SampleThumbnail::Thumbnail::Thumbnail(std::vector<Peak> peaks, double samplesPerPeak)
	: m_peaks(std::move(peaks))
	, m_samplesPerPeak(samplesPerPeak)
{
}

SampleThumbnail::Thumbnail::Thumbnail(const SampleFrame* buffer, size_t size, int width)
	: m_peaks(width)
	, m_samplesPerPeak(static_cast<double>(size) / width)
{
	for (auto peakIndex = 0; peakIndex < width; ++peakIndex)
	{
		const auto beginSample = buffer + static_cast<size_t>(std::floor(peakIndex * m_samplesPerPeak));
		const auto endSample
			= buffer + std::min(static_cast<size_t>(std::ceil((peakIndex + 1) * m_samplesPerPeak)), size);
		const auto [min, max] = std::minmax_element(&beginSample->left(), &endSample->left());
		m_peaks[peakIndex] = Peak{*min, *max};
	}
}

SampleThumbnail::Thumbnail SampleThumbnail::Thumbnail::zoomOut(float factor) const
{
	assert(factor >= 1 && "Invalid zoom out factor");

	auto peaks = std::vector<Peak>(width() / factor);
	for (auto peakIndex = 0; peakIndex < peaks.size(); ++peakIndex)
	{
		const auto beginAggregationAt = m_peaks.begin() + static_cast<int>(std::floor(peakIndex * factor));
		const auto endAggregationAt
			= m_peaks.begin() + std::min(static_cast<int>(std::ceil((peakIndex + 1) * factor)), width());
		const auto aggregatedPeak = std::accumulate(beginAggregationAt, endAggregationAt, Peak{});
		peaks[peakIndex] = aggregatedPeak;
	}

	return Thumbnail{std::move(peaks), m_samplesPerPeak * factor};
}

SampleThumbnail::Thumbnail SampleThumbnail::Thumbnail::extract(size_t from, size_t to) const
{
	auto peaks = std::vector<Peak>(to - from);
	std::copy(m_peaks.begin() + from, m_peaks.begin() + to, peaks.begin());
	return Thumbnail{std::move(peaks), m_samplesPerPeak};
}

SampleThumbnail::SampleThumbnail(const Sample& sample)
{
	if (selectFromGlobalThumbnailMap(sample)) { return; }
	cleanUpGlobalThumbnailMap();

	m_thumbnailCache->emplace_back(sample.buffer()->data(), sample.sampleSize(), sample.sampleSize());

	while (m_thumbnailCache->back().width() > QGuiApplication::primaryScreen()->geometry().width())
	{
		const auto zoomedOutThumbnail = m_thumbnailCache->back().zoomOut(Thumbnail::AggregationPerZoomStep);
		m_thumbnailCache->emplace_back(zoomedOutThumbnail);
	}
}

/* DEPRECATED; functionality is kept for testing conveniences */
bool SampleThumbnail::selectFromGlobalThumbnailMap(const Sample& inputSample)
{
	const auto samplePtr = inputSample.buffer();
	const auto name = inputSample.sampleFile();
	const auto end = s_sampleThumbnailCacheMap.end();

	if (const auto list = s_sampleThumbnailCacheMap.find(name); list != end)
	{
		m_thumbnailCache = list->second;
		return true;
	}

	m_thumbnailCache = std::make_shared<ThumbnailCache>();
	s_sampleThumbnailCacheMap.insert(std::make_pair(name, m_thumbnailCache));
	return false;
}

/* DEPRECATED; functionality is kept for testing conveniences */
void SampleThumbnail::cleanUpGlobalThumbnailMap()
{
	auto map = s_sampleThumbnailCacheMap.begin();
	while (map != s_sampleThumbnailCacheMap.end())
	{
		// All sample thumbnails are destroyed, a.k.a sample goes out of use
		if (map->second.use_count() == 1)
		{
			s_sampleThumbnailCacheMap.erase(map);
			map = s_sampleThumbnailCacheMap.begin();
			continue;
		}

		map++;
	}
}

void SampleThumbnail::visualize(const VisualizeParameters& parameters, QPainter& painter) const
{
	const auto& sampleRect = parameters.sampleRect;
	const auto clipRect = parameters.clipRect.isNull() ? sampleRect : parameters.clipRect;
	const auto viewportRect = parameters.viewportRect.isNull() ? clipRect : parameters.viewportRect;

	const auto thumbnail = std::find_if(m_thumbnailCache->rbegin(), m_thumbnailCache->rend(),
		[&](const auto& thumbnail) { return thumbnail.width() >= sampleRect.width(); });

	assert(thumbnail != m_thumbnailCache->rend());

	const auto drawBeginIndex = std::max({clipRect.x(), sampleRect.x(), viewportRect.x()});
	const auto drawEndIndex = std::min({clipRect.x() + clipRect.width(), sampleRect.x() + sampleRect.width(), viewportRect.x() + viewportRect.width()});

	const auto peakBeginIndex = -sampleRect.x() + drawBeginIndex;
	const auto peakEndIndex = -sampleRect.x() + drawEndIndex;

	const auto thumbnailScaleFactor = static_cast<double>(thumbnail->width()) / sampleRect.width();
	const auto thumbnailBeginIndex = static_cast<int>(peakBeginIndex * thumbnailScaleFactor);
	const auto thumbnailEndIndex = static_cast<int>(peakEndIndex * thumbnailScaleFactor);

	const auto extractedThumbnail = thumbnail->extract(thumbnailBeginIndex, thumbnailEndIndex);
	auto outputThumbnail = extractedThumbnail.zoomOut(thumbnailScaleFactor);
	if (parameters.reversed) { outputThumbnail.reverse(); }

	painter.save();
	painter.setRenderHint(QPainter::Antialiasing, true);

	for (auto x = drawBeginIndex; x < drawEndIndex; ++x)
	{
		const auto peak = outputThumbnail[x - drawBeginIndex];
		const auto yMin = sampleRect.center().y() - peak.min * sampleRect.center().y() / 2 * parameters.amplification;
		const auto yMax = sampleRect.center().y() - peak.max * sampleRect.center().y() / 2 * parameters.amplification;
		painter.drawLine(x, yMin, x, yMax);
	}

	painter.restore();
}

} // namespace lmms
