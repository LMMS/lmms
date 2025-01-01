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
		const auto endSample = buffer + std::min(static_cast<size_t>(std::ceil((peakIndex + 1) * m_samplesPerPeak)), size);
		const auto [min, max] = std::minmax_element(&beginSample->left(), &endSample->left());
		m_peaks[peakIndex] = Peak{*min, *max};
	}
}

SampleThumbnail::Thumbnail SampleThumbnail::Thumbnail::zoomOut(float factor)
{
	assert(width() >= factor);

	auto peaks = std::vector<Peak>(width() / factor);
	for (auto peakIndex = 0; peakIndex < peaks.size(); ++peakIndex)
	{
		const auto beginAggregationAt = m_peaks.begin() + static_cast<int>(std::floor(peakIndex * factor));
		const auto endAggregationAt = m_peaks.begin() + std::min(static_cast<int>(std::ceil((peakIndex + 1) * factor)), width());
		const auto aggregatedPeak = std::accumulate(beginAggregationAt, endAggregationAt, Peak{});
		peaks[peakIndex] = aggregatedPeak;
	}

	return Thumbnail{std::move(peaks), (m_samplesPerPeak * m_peaks.size()) / peaks.size()};
}

SampleThumbnail::SampleThumbnail(const Sample& sample)
{
	if (selectFromGlobalThumbnailMap(sample)) { return; }
	cleanUpGlobalThumbnailMap();

	m_thumbnailCache->emplace_back(sample.buffer()->data(), sample.sampleSize(), sample.sampleSize());

	do
	{
		const auto zoomedOutThumbnail = m_thumbnailCache->back().zoomOut(Thumbnail::AggregationPerZoomStep);
		m_thumbnailCache->emplace_back(zoomedOutThumbnail);
	}
	while (m_thumbnailCache->back().width() > QGuiApplication::primaryScreen()->geometry().width());
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

void SampleThumbnail::Thumbnail::draw(QPainter& painter, const Thumbnail::Peak& peak, float lineX, int centerY, float scalingFactor,
	const QColor& color, const QColor& innerColor) const
{
	const auto lengthY1 = peak.max * scalingFactor;
	const auto lengthY2 = peak.min * scalingFactor;

	const auto lineY1 = centerY - lengthY1;
	const auto lineY2 = centerY - lengthY2;

	const auto innerLineY1 = centerY - peak.max * 0.6 * scalingFactor;
	const auto innerLineY2 = centerY - peak.min * 0.6 * scalingFactor;

	painter.drawLine(QPointF{lineX, lineY1}, QPointF{lineX, lineY2});
	painter.setPen(innerColor);

	painter.drawLine(QPointF{lineX, innerLineY1}, QPointF{lineX, innerLineY2});
	painter.setPen(color);
}

void SampleThumbnail::visualize(const VisualizeParameters& parameters, QPainter& painter) const
{
	const auto& clipRect = parameters.clipRect;
	const auto& sampRect = parameters.sampRect.isNull() ? clipRect : parameters.sampRect;
	const auto& drawRect = parameters.drawRect.isNull() ? clipRect : parameters.drawRect;

	const auto sampleViewLength = parameters.sampleEnd - parameters.sampleStart;

	const auto x = sampRect.x();
	const auto height = clipRect.height();
	const auto halfHeight = height / 2;
	const auto width = sampRect.width();
	const auto centerY = clipRect.y() + halfHeight;

	if (width < 1) { return; }

	const auto scalingFactor = halfHeight * parameters.amplification;

	const auto color = painter.pen().color();
	const auto innerColor = color.lighter(123);

	const auto widthSelect = static_cast<std::size_t>(static_cast<float>(width) / sampleViewLength);

	auto thumbnailIt = m_thumbnailCache->end();

	do
	{
		thumbnailIt--;
	} while (thumbnailIt != m_thumbnailCache->begin() && thumbnailIt->width() < widthSelect);

	const auto& thumbnail = *thumbnailIt;

	const auto thumbnailLastSample = std::max(static_cast<size_t>(parameters.sampleEnd * thumbnail.width()), std::size_t{1}) - 1;
	const auto tViewStart = static_cast<long>(parameters.sampleStart * thumbnail.width());
	const auto tViewLast = std::min<size_t>(thumbnailLastSample, thumbnail.width() - 1);
	const auto tLast = thumbnail.width() - 1;
	const auto thumbnailViewSize = thumbnailLastSample + 1 - tViewStart;

	const auto pixelIndexStart = std::max(std::max(clipRect.x(), drawRect.x()), x);
	const auto pixelIndexEnd = std::min(std::min(clipRect.width(), drawRect.width()), width) + pixelIndexStart;

	const auto tChunk = (thumbnailViewSize + width) / width;

	for (auto pixelIndex = pixelIndexStart; pixelIndex <= pixelIndexEnd; pixelIndex++)
	{
		auto tIndex = tViewStart + (pixelIndex - x) * thumbnailViewSize / width;
		if (tIndex > tViewLast) { break; }

		const auto tChunkBound = tIndex + tChunk;

		auto peak = Thumbnail::Peak{};
		if (parameters.reversed)
		{
			peak = std::accumulate(thumbnail.peaks() + tLast - tChunkBound, thumbnail.peaks() + tLast - tIndex, Thumbnail::Peak{});
		}
		else { peak = std::accumulate(thumbnail.peaks() + tIndex, thumbnail.peaks() + tChunkBound, Thumbnail::Peak{}); }

		thumbnail.draw(painter, peak, pixelIndex, centerY, scalingFactor, color, innerColor);
	}
}

} // namespace lmms
