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
	assert(parameters.sampleStart <= parameters.sampleEnd && "Invalid sample range");

	if (m_thumbnailCache->empty())
	{
		qDebug() << "SampleThumbnail::visualize: Cannot draw waveform with an empty thumbnail cache";
		return;
	}

	const auto& sampleRect = parameters.sampleRect;
	const auto& viewportRect = parameters.viewportRect.isNull() ? sampleRect : parameters.viewportRect;

	const auto numSamples = parameters.sampleEnd - parameters.sampleStart;
	const auto idealSamplesPerPeak = std::max(static_cast<double>(numSamples) / sampleRect.width(), 1.0);

	const auto smallestLargerThumbnail = std::find_if(m_thumbnailCache->rbegin(), m_thumbnailCache->rend(),
		[&](const auto& thumbnail) { return thumbnail.samplesPerPeak() <= idealSamplesPerPeak; });

	if (smallestLargerThumbnail == m_thumbnailCache->rend())
	{
		qDebug() << "SampleThumbnail::visualize: Smallest thumbnail of larger or equal width not found within "
					"thumbnail cache for a "
					"samples per peak of"
				 << idealSamplesPerPeak;
		return;
	}

	const auto smallestLargerThumbnailBegin
		= static_cast<double>(viewportRect.x()) / sampleRect.width() * smallestLargerThumbnail->width();
	const auto smallestLargerThumbnailEnd = static_cast<double>(viewportRect.x() + viewportRect.width())
		/ sampleRect.width() * smallestLargerThumbnail->width();

	const auto extractedThumbnail = smallestLargerThumbnail->extract(
		std::floor(smallestLargerThumbnailBegin), std::ceil(smallestLargerThumbnailEnd));
	const auto zoomedOutThumbnail
		= extractedThumbnail.zoomOut(idealSamplesPerPeak / smallestLargerThumbnail->samplesPerPeak());

	painter.setClipRect(viewportRect.intersected(sampleRect));

	const auto scalingFactor = sampleRect.center().y() / 2 * parameters.amplification;
	for (auto xPos = viewportRect.x(); xPos < viewportRect.x() + viewportRect.width(); ++xPos)
	{
		const auto peak = zoomedOutThumbnail[xPos - viewportRect.x()];
		const auto yMin = sampleRect.center().y() - peak.min * scalingFactor;
		const auto yMax = sampleRect.center().y() - peak.max * scalingFactor;
		painter.drawLine(xPos, yMin, xPos, yMax);
	}
}

} // namespace lmms
