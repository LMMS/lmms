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

#include <algorithm>
#include "Sample.h"

namespace lmms {

SampleThumbnail::Bit::Bit(const SampleFrame& frame)
	: max(std::max(frame.left(), frame.right()))
	, min(std::min(frame.left(), frame.right()))
	, rms(0.0)
{
}

void SampleThumbnail::Bit::merge(const Bit& other)
{
	min = std::min(min, other.min);
	max = std::max(max, other.max);
	rms = std::sqrt((rms * rms + other.rms * other.rms) / 2.0);
}

void SampleThumbnail::Bit::merge(const SampleFrame& frame)
{
	merge(Bit{frame});
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

SampleThumbnail::Thumbnail SampleThumbnail::generate(const size_t thumbnailSize, const SampleFrame* buffer, const size_t size)
{
	const auto sampleChunk = (size + thumbnailSize) / thumbnailSize;
	auto thumbnail = SampleThumbnail::Thumbnail(thumbnailSize);

	for (auto tIndex = std::size_t{0}; tIndex < thumbnailSize; tIndex++)
	{
		auto sampleIndex = tIndex * size / thumbnailSize;
		const auto sampleChunkBound = std::min(sampleIndex + sampleChunk, size);

		auto& bit = thumbnail[tIndex];
		while (sampleIndex < sampleChunkBound)
		{
			const auto& frame = buffer[sampleIndex];
			bit.merge(frame);

			const auto ave = frame.average();
			bit.rms += ave * ave;

			sampleIndex++;
		}

		bit.rms = std::sqrt(bit.rms / sampleChunk);
	}

	return thumbnail;
}

SampleThumbnail::SampleThumbnail(const Sample& inputSample)
{
	if (selectFromGlobalThumbnailMap(inputSample)) { return; }

	cleanUpGlobalThumbnailMap();

	const auto sampleBufferSize = inputSample.sampleSize();
	const auto& buffer = inputSample.data();

	const auto thumbnailSizeDivisor = std::max<size_t>(32, 3*std::log2(sampleBufferSize));
	// I don't think we *really* need to keep a full resolution thumbnail of the sample.
	const auto firstThumbnailSize = std::max<size_t>(sampleBufferSize / 4, 1);

	const auto firstThumbnail = generate(firstThumbnailSize, buffer, sampleBufferSize);
	m_thumbnailCache->push_back(firstThumbnail);

	// Generate the remaining thumbnails using the first one, each one's
	// size is the size of the previous one divided by the thumbnail size divisor.
	for (auto thumbnailSize = std::size_t{firstThumbnailSize / thumbnailSizeDivisor}; thumbnailSize >= MinThumbnailSize;
		 thumbnailSize /= thumbnailSizeDivisor)
	{
		const auto& biggerThumbnail = m_thumbnailCache->back();
		const auto biggerThumbnailSize = biggerThumbnail.size();
		auto bitIndex = std::size_t{0};

		auto thumbnail = Thumbnail(thumbnailSize);
		for (const auto& biggerBit : biggerThumbnail)
		{
			auto& bit = thumbnail[bitIndex * thumbnailSize / biggerThumbnailSize];

			bit.merge(biggerBit);

			++bitIndex;
		}

		m_thumbnailCache->push_back(thumbnail);
	}
}

void SampleThumbnail::draw(QPainter& painter, const SampleThumbnail::Bit& bit, float lineX, int centerY,
	float scalingFactor, const QColor& color, const QColor& rmsColor)
{
	const auto lengthY1 = bit.max * scalingFactor;
	const auto lengthY2 = bit.min * scalingFactor;

	const auto lineY1 = centerY - lengthY1;
	const auto lineY2 = centerY - lengthY2;

	const auto maxRMS = std::clamp(bit.rms, bit.min, bit.max);
	const auto minRMS = std::clamp(-bit.rms, bit.min, bit.max);

	const auto rmsLineY1 = centerY - maxRMS * scalingFactor;
	const auto rmsLineY2 = centerY - minRMS * scalingFactor;

	painter.drawLine(QPointF{lineX, lineY1}, QPointF{lineX, lineY2});
	painter.setPen(rmsColor);

	painter.drawLine(QPointF{lineX, rmsLineY1}, QPointF{lineX, rmsLineY2});
	painter.setPen(color);
}

void SampleThumbnail::visualize(const SampleThumbnail::VisualizeParameters& parameters, QPainter& painter) const
{
	const auto sampleViewLength = parameters.sampleEnd - parameters.sampleStart;

	const auto x = parameters.x;
	const auto height = parameters.height;
	const auto halfHeight = height / 2;
	const auto width = parameters.width;
	const auto centerY = parameters.y + halfHeight;

	// If the clip extends to the left past the start of the
	// sample, start drawing at the start of the sample and
	// skip the blank space.
	const auto absXOr0 = (x < 0) ? -x : 0;
	const auto scalingFactor = halfHeight * parameters.amplification;

	const auto color = painter.pen().color();
	const auto rmsColor = color.lighter(123);

	const auto widthSelect = static_cast<std::size_t>(static_cast<float>(width) / sampleViewLength);

	auto thumbnailIt = m_thumbnailCache->end()-1;
	const auto thumbnailItStop = m_thumbnailCache->begin() + (parameters.allowHighResolution ? 0 : 1);
	while (thumbnailIt != thumbnailItStop && thumbnailIt->size() < widthSelect)
	{
		thumbnailIt--;
	}

	const auto thumbnail = *thumbnailIt;

	const auto thumbnailSize = thumbnail.size();
	const auto thumbnailLastSample = std::max(static_cast<std::size_t>(parameters.sampleEnd * thumbnailSize), std::size_t{1}) - 1;
	const auto tStart = static_cast<long>(parameters.sampleStart * thumbnailSize);
	const auto thumbnailViewSize = thumbnailLastSample + 1 - tStart;
	const auto tLast = std::min(thumbnailLastSample, thumbnailSize - 1);
	const auto pixelBound = std::min(width, parameters.clipWidthSinceSampleStart);

	auto tIndex = std::size_t{0};
	auto pixelIndex = std::max(absXOr0, parameters.viewX);
	const auto tChunk = (thumbnailViewSize + width) / width;

	while (pixelIndex <= pixelBound)
	{
		tIndex = tStart + pixelIndex * thumbnailViewSize / width;

		if (tIndex > tLast) break;

		auto thumbnailBit = Bit{};

		const auto tChunkBound = tIndex + tChunk;

		while (tIndex < tChunkBound)
		{
			thumbnailBit.merge(thumbnail[parameters.reversed ? tLast - tIndex : tIndex]);
			tIndex += 1;
		}

		draw(painter, thumbnailBit, pixelIndex + x, centerY, scalingFactor, color, rmsColor);

		pixelIndex++;
	}
}

} // namespace lmms
