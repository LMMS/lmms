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

#include "Sample.h"

namespace {
	constexpr auto MaxSampleThumbnailCacheSize = 32;
	constexpr auto AggregationPerZoomStep = 10;
}

namespace lmms::gui {

SampleThumbnail::Thumbnail::Thumbnail(std::vector<Peak> peaks, double samplesPerPeak)
	: m_peaks(std::move(peaks))
	, m_samplesPerPeak(samplesPerPeak)
{
}

SampleThumbnail::Thumbnail::Thumbnail(PlanarBufferView<const float> buffer)
{
	const auto width = buffer.frames() / AggregationPerZoomStep;

	m_peaks.resize(width);
	m_samplesPerPeak = std::max(static_cast<double>(buffer.frames()) / width, 1.0);

	if (buffer.empty()) { return; }

	const float* const* bufferData = buffer.data();
	if (buffer.channels() >= 2)
	{
		// Stereo sample or multi-channel sample truncated to stereo
		for (auto peakIndex = std::size_t{0}; peakIndex < width; ++peakIndex)
		{
			const auto beginOffset = static_cast<size_t>(std::floor(peakIndex * m_samplesPerPeak));
			const auto endOffset = static_cast<size_t>(std::ceil((peakIndex + 1) * m_samplesPerPeak));

			const float* beginSample0 = bufferData[0] + beginOffset;
			const float* endSample0 = bufferData[0] + endOffset;
			const auto [min0, max0] = std::minmax_element(beginSample0, endSample0);

			const float* beginSample1 = bufferData[1] + beginOffset;
			const float* endSample1 = bufferData[1] + endOffset;
			const auto [min1, max1] = std::minmax_element(beginSample1, endSample1);

			m_peaks[peakIndex] = Peak{std::min(*min0, *min1), std::max(*max0, *max1)};
		}
	}
	else
	{
		// Mono sample
		for (auto peakIndex = std::size_t{0}; peakIndex < width; ++peakIndex)
		{
			const auto beginOffset = static_cast<size_t>(std::floor(peakIndex * m_samplesPerPeak));
			const auto endOffset = static_cast<size_t>(std::ceil((peakIndex + 1) * m_samplesPerPeak));

			const float* beginSample = bufferData[0] + beginOffset;
			const float* endSample = bufferData[0] + endOffset;
			const auto [min, max] = std::minmax_element(beginSample, endSample);

			m_peaks[peakIndex] = Peak{*min, *max};
		}
	}
}

SampleThumbnail::Thumbnail SampleThumbnail::Thumbnail::zoomOut(float factor) const
{
	assert(factor >= 1 && "Invalid zoom out factor");

	auto peaks = std::vector<Peak>(m_peaks.size() / factor);
	for (auto peakIndex = std::size_t{0}; peakIndex < peaks.size(); ++peakIndex)
	{
		const auto beginAggregationAt = m_peaks.begin() + static_cast<size_t>(std::floor(peakIndex * factor));
		const auto endAggregationAt = m_peaks.begin() + static_cast<size_t>(std::ceil((peakIndex + 1) * factor));
		peaks[peakIndex] = std::accumulate(beginAggregationAt, endAggregationAt, Peak{});
	}

	return Thumbnail{std::move(peaks), m_samplesPerPeak * factor};
}

SampleThumbnail::SampleThumbnail(const Sample& sample)
	: m_buffer(sample.buffer())
{
	auto entry = SampleThumbnailEntry{sample.sampleFile(), QFileInfo{sample.sampleFile()}.lastModified()};
	if (!entry.filePath.isEmpty())
	{
		const auto it = s_sampleThumbnailCacheMap.find(entry);
		if (it != s_sampleThumbnailCacheMap.end())
		{
			m_thumbnailCache = it->second;
			return;
		}

		if (s_sampleThumbnailCacheMap.size() == MaxSampleThumbnailCacheSize)
		{
			const auto leastUsed = std::min_element(s_sampleThumbnailCacheMap.begin(), s_sampleThumbnailCacheMap.end(),
				[](const auto& a, const auto& b) { return a.second.use_count() < b.second.use_count(); });
			s_sampleThumbnailCacheMap.erase(leastUsed->first);
		}

		s_sampleThumbnailCacheMap[std::move(entry)] = m_thumbnailCache;
	}

	m_thumbnailCache->emplace_back(m_buffer->data());

	while (m_thumbnailCache->back().width() >= AggregationPerZoomStep)
	{
		auto zoomedOutThumbnail = m_thumbnailCache->back().zoomOut(AggregationPerZoomStep);
		m_thumbnailCache->emplace_back(std::move(zoomedOutThumbnail));
	}
}

void SampleThumbnail::visualize(VisualizeParameters parameters, QPainter& painter) const
{
	const auto& sampleRect = parameters.sampleRect;
	const auto& viewportRect = parameters.viewportRect.isNull() ? sampleRect : parameters.viewportRect;

	const auto renderRect = sampleRect.intersected(viewportRect);
	if (renderRect.isNull()) { return; }

	const auto sampleRange = parameters.sampleEnd - parameters.sampleStart;
	if (sampleRange <= 0.0f || sampleRange > 1.0f) { return; }

	const auto targetThumbnailWidth = static_cast<std::size_t>(sampleRect.width() / sampleRange);
	const auto finerThumbnail = std::find_if(m_thumbnailCache->rbegin(), m_thumbnailCache->rend(),
		[&](const auto& thumbnail) { return thumbnail.width() >= targetThumbnailWidth; });

	const auto useOriginalBuffer = finerThumbnail == m_thumbnailCache->rend();
	const auto drawOriginalBuffer = targetThumbnailWidth == m_buffer->frames();

	painter.save();
	painter.setRenderHint(QPainter::Antialiasing, true);

	const auto thumbnailBeginForward = std::max<std::size_t>(renderRect.x() - sampleRect.x(), parameters.sampleStart * targetThumbnailWidth);
	const auto thumbnailEndForward = std::max<std::size_t>(renderRect.x() + renderRect.width() - sampleRect.x(), parameters.sampleEnd * targetThumbnailWidth);
	const auto thumbnailBegin = parameters.reversed ? targetThumbnailWidth - thumbnailBeginForward - 1 : thumbnailBeginForward;
	const auto thumbnailEnd = parameters.reversed ? targetThumbnailWidth - thumbnailEndForward : thumbnailEndForward;
	const auto advanceThumbnailBy = parameters.reversed ? -1 : 1;

	const auto finerThumbnailWidth = useOriginalBuffer ? m_buffer->frames() : finerThumbnail->width();
	const auto finerThumbnailScaleFactor = static_cast<double>(finerThumbnailWidth) / targetThumbnailWidth;
	const auto yScale = renderRect.height() / 2 * parameters.amplification;

	const float* const* buffer = m_buffer->data().data();

	const auto getPoint = m_buffer->channels() >= 2
		? +[](const float* const* b, f_cnt_t frame) -> float {
			return (b[0][frame] + b[1][frame]) / 2;
		}
		: +[](const float* const* b, f_cnt_t frame) -> float {
			return b[0][frame];
		};

	const auto getMinMax = m_buffer->channels() >= 2
		? +[](const float* const* b, std::size_t begin, std::size_t end) -> std::pair<float, float> {
			const auto [min0, max0] = std::minmax_element(b[0] + begin, b[0] + end);
			const auto [min1, max1] = std::minmax_element(b[1] + begin, b[1] + end);
			return {std::min(*min0, *min1), std::max(*max0, *max1)};
		}
		: +[](const float* const* b, std::size_t begin, std::size_t end) -> std::pair<float, float> {
			const auto [min, max] = std::minmax_element(b[0] + begin, b[0] + end);
			return {*min, *max};
		};

	auto i = thumbnailBegin;
	for (auto x = renderRect.x(); x < renderRect.x() + renderRect.width() && i != thumbnailEnd;
		++x, i += advanceThumbnailBy)
	{
		if (useOriginalBuffer && drawOriginalBuffer)
		{
			const auto value = getPoint(buffer, i);
			painter.drawPoint(x, renderRect.center().y() - value * yScale);
			continue;
		}
		else
		{
			const auto beginIndex = std::clamp<size_t>(std::floor(i * finerThumbnailScaleFactor), 0, finerThumbnail->width() - 1);
			const auto endIndex = std::clamp<size_t>(std::ceil((i + 1) * finerThumbnailScaleFactor), 0, finerThumbnail->width() - 1);

			auto minPeak = 0.f;
			auto maxPeak = 0.f;

			if (useOriginalBuffer)
			{
				const auto [min, max] = getMinMax(buffer, beginIndex, endIndex);
				minPeak = min;
				maxPeak = max;
			}
			else
			{
				const auto beginAggregationAt = finerThumbnail->data() + beginIndex;
				const auto endAggregationAt = finerThumbnail->data() + endIndex;
				const auto peak = std::accumulate(beginAggregationAt, endAggregationAt, Thumbnail::Peak{});
				minPeak = peak.min;
				maxPeak = peak.max;
			}

			const auto yMin = renderRect.center().y() - minPeak * yScale;
			const auto yMax = renderRect.center().y() - maxPeak * yScale;
			painter.drawLine(x, yMin, x, yMax);
		}
	}

	painter.restore();
}

} // namespace lmms::gui
