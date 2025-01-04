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

SampleThumbnail::Thumbnail::Thumbnail(const SampleFrame* buffer, size_t size, size_t width)
	: m_peaks(width)
	, m_samplesPerPeak(static_cast<double>(size) / width)
{
	for (auto peakIndex = std::size_t{0}; peakIndex < width; ++peakIndex)
	{
		const auto beginSample = buffer + static_cast<size_t>(std::floor(peakIndex * m_samplesPerPeak));
		const auto endSample = buffer + static_cast<size_t>(std::ceil((peakIndex + 1) * m_samplesPerPeak));
		const auto [min, max] = std::minmax_element(&beginSample->left(), &endSample->left());
		m_peaks[peakIndex] = Peak{*min, *max};
	}
}

SampleThumbnail::Thumbnail SampleThumbnail::Thumbnail::zoomOut(float factor, size_t from, size_t to) const
{
	assert(factor >= 1 && "Invalid zoom out factor");

	auto peaks = std::vector<Peak>((to - from) / factor);
	for (auto peakIndex = std::size_t{0}; peakIndex < peaks.size(); ++peakIndex)
	{
		const auto beginAggregationAt = m_peaks.begin() + from + static_cast<size_t>(std::floor(peakIndex * factor));
		const auto endAggregationAt = m_peaks.begin() + from + static_cast<size_t>(std::ceil((peakIndex + 1) * factor));
		peaks[peakIndex] = std::accumulate(beginAggregationAt, endAggregationAt, Peak{});
	}

	return Thumbnail{std::move(peaks), m_samplesPerPeak * factor};
}

SampleThumbnail::Thumbnail SampleThumbnail::Thumbnail::zoomOut(float factor) const
{
	return zoomOut(factor, 0, m_peaks.size());
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
	const auto& drawRect = parameters.drawRect.isNull() ? sampleRect : parameters.drawRect;
	const auto& viewportRect = parameters.viewportRect.isNull() ? drawRect : parameters.viewportRect;

	const auto sampleRange = parameters.sampleEnd - parameters.sampleStart;
	assert(sampleRange <= 1);

	const auto targetThumbnailWidth = static_cast<double>(sampleRect.width()) / sampleRange;
	const auto finerThumbnail = std::find_if(m_thumbnailCache->rbegin(), m_thumbnailCache->rend(),
		[&](const auto& thumbnail) { return thumbnail.width() >= targetThumbnailWidth; });

	if (finerThumbnail == m_thumbnailCache->rend())
	{
		qDebug() << "Could not find closest finer thumbnail for a target width of" << targetThumbnailWidth;
		return;
	}

	const auto finerThumbnailBegin = (parameters.reversed ? 1.0 - parameters.sampleEnd : parameters.sampleStart) * finerThumbnail->width();
	const auto finerThumbnailEnd =  (parameters.reversed ? 1.0 - parameters.sampleStart : parameters.sampleEnd) * finerThumbnail->width();
	const auto finerThumbnailScaleFactor = static_cast<double>(finerThumbnail->width()) / targetThumbnailWidth;
	const auto thumbnail = finerThumbnail->zoomOut(finerThumbnailScaleFactor, finerThumbnailBegin, finerThumbnailEnd);

	const auto drawBegin = std::max({sampleRect.x(), drawRect.x(), viewportRect.x()});
	const auto drawEnd = std::min({sampleRect.x() + sampleRect.width(), drawRect.x() + drawRect.width(),
		viewportRect.x() + viewportRect.width()});

	const auto thumbnailBeginForward = std::clamp(drawBegin - sampleRect.x(), 0, thumbnail.width());
	const auto thumbnailEndForward = std::clamp(drawEnd - sampleRect.x(), 0, thumbnail.width());
	const auto thumbnailBegin = parameters.reversed ? thumbnail.width() - thumbnailBeginForward - 1 : thumbnailBeginForward;
	const auto thumbnailEnd = parameters.reversed ? thumbnail.width() - thumbnailEndForward - 1 : thumbnailEndForward;
	const auto thumbnailIndexOffset = parameters.reversed ? -1 : 1;

	painter.save();
	painter.setRenderHint(QPainter::Antialiasing, true);

	const auto yScale = drawRect.height() / 2 * parameters.amplification;
	for (auto x = drawBegin, i = thumbnailBegin; x < drawEnd && i != thumbnailEnd; ++x, i += thumbnailIndexOffset)
	{
		const auto& peak = thumbnail[i];
		const auto yMin = drawRect.center().y() - peak.min * yScale;
		const auto yMax = drawRect.center().y() - peak.max * yScale;
		painter.drawLine(x, yMin, x, yMax);
	}

	painter.restore();
}

} // namespace lmms
