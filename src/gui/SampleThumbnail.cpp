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

namespace lmms {

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

SampleThumbnail::Thumbnail SampleThumbnail::generate(
	const size_t thumbnailSize, const SampleFrame* buffer, const size_t size)
{
	const auto sampleChunk = (size + thumbnailSize) / thumbnailSize;
	auto thumbnail = Thumbnail(thumbnailSize);

	for (auto tIndex = size_t{0}; tIndex < thumbnailSize; tIndex++)
	{
		const auto sampleIndex = tIndex * size / thumbnailSize;
		const auto sampleChunkBound = std::min(sampleIndex + sampleChunk, size);
		thumbnail[tIndex] = std::accumulate(buffer + sampleIndex, buffer + sampleChunkBound, Bit{});
	}

	return thumbnail;
}

SampleThumbnail::SampleThumbnail(const Sample& inputSample)
{
	if (selectFromGlobalThumbnailMap(inputSample)) { return; }

	cleanUpGlobalThumbnailMap();

	const auto sampleBufferSize = inputSample.sampleSize();
	const auto thumbnailSizeDivisor = size_t{10};
	const auto firstThumbnailSize = std::max<size_t>(sampleBufferSize / thumbnailSizeDivisor, 1);

	const auto firstThumbnail = generate(firstThumbnailSize, inputSample.data(), sampleBufferSize);

	m_thumbnailCache->push_back(firstThumbnail);

	// Generate the remaining thumbnails using the first one, each one's
	// size is the size of the previous one divided by the thumbnail size divisor.
	auto thumbnailSize = size_t{firstThumbnailSize / thumbnailSizeDivisor};
	while (thumbnailSize >= 1)
	{
		const auto& biggerThumbnail = m_thumbnailCache->back();

		auto thumbnail = Thumbnail(thumbnailSize);

		for (auto smallIndex = size_t{0}; smallIndex < thumbnail.size(); smallIndex += 1)
		{
			auto bigIndex = smallIndex * thumbnailSizeDivisor;
			auto bigItStart = biggerThumbnail.cbegin() + bigIndex;
			auto bigItEnd = std::min(bigItStart + thumbnailSizeDivisor, biggerThumbnail.end());

			thumbnail[smallIndex] = std::accumulate(bigItStart, bigItEnd, Bit{});
		}

		m_thumbnailCache->push_back(thumbnail);
		thumbnailSize /= thumbnailSizeDivisor;
	}
}

void SampleThumbnail::draw(QPainter& painter, const Bit& bit, float lineX, int centerY, float scalingFactor,
	const QColor& color, const QColor& innerColor)
{
	const auto lengthY1 = bit.max * scalingFactor;
	const auto lengthY2 = bit.min * scalingFactor;

	const auto lineY1 = centerY - lengthY1;
	const auto lineY2 = centerY - lengthY2;

	const auto innerLineY1 = centerY - bit.max * 0.6 * scalingFactor;
	const auto innerLineY2 = centerY - bit.min * 0.6 * scalingFactor;

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
	} while (thumbnailIt != m_thumbnailCache->begin() && thumbnailIt->size() < widthSelect);

	const auto& thumbnail = *thumbnailIt;

	const auto thumbnailLastSample
		= std::max<size_t>(static_cast<size_t>(parameters.sampleEnd * thumbnail.size()), 1) - 1;
	const auto tViewStart = static_cast<long>(parameters.sampleStart * thumbnail.size());
	const auto tViewLast = std::min<size_t>(thumbnailLastSample, thumbnail.size() - 1);
	const auto tLast = thumbnail.size() - 1;
	const auto thumbnailViewSize = thumbnailLastSample + 1 - tViewStart;

	const auto pixelIndexStart = std::max(std::max(clipRect.x(), drawRect.x()), x);
	const auto pixelIndexEnd = std::min(std::min(clipRect.width(), drawRect.width()), width) + pixelIndexStart;

	const auto tChunk = (thumbnailViewSize + width) / width;

	for (auto pixelIndex = pixelIndexStart; pixelIndex <= pixelIndexEnd; pixelIndex++)
	{
		auto tIndex = tViewStart + (pixelIndex - x) * thumbnailViewSize / width;

		if (tIndex > tViewLast) { break; }

		const auto tChunkBound = tIndex + tChunk;

		auto thumbnailBit = Bit{};

		if (parameters.reversed)
		{
			thumbnailBit
				= std::accumulate(thumbnail.cbegin() + tLast - tChunkBound, thumbnail.cbegin() + tLast - tIndex, Bit{});
		}
		else { thumbnailBit = std::accumulate(thumbnail.cbegin() + tIndex, thumbnail.cbegin() + tChunkBound, Bit{}); }

		draw(painter, thumbnailBit, pixelIndex, centerY, scalingFactor, color, innerColor);
	}
}

} // namespace lmms
