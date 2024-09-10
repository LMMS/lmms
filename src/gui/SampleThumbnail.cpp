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

namespace lmms {

SampleThumbnailBit::SampleThumbnailBit(const SampleFrame& frame)
	: max(std::max(frame.left(), frame.right()))
	, min(std::min(frame.left(), frame.right()))
	, rms(0.0)
{
}

void SampleThumbnailBit::merge(const SampleThumbnailBit& other)
{
	min = std::min(min, other.min);
	max = std::max(max, other.max);
	rms = std::sqrt((rms * rms + other.rms * other.rms) / 2.0);
}

void SampleThumbnailBit::merge(const SampleFrame& frame)
{
	merge(SampleThumbnailBit{frame});
}

bool SampleThumbnailListManager::selectFromGlobalThumbnailMap(const Sample& inputSample)
{
	const auto samplePtr = inputSample.buffer();
	const auto name = inputSample.sampleFile();
	const auto end = s_sampleThumbnailListMap.end();

	if (const auto list = s_sampleThumbnailListMap.find(name); list != end)
	{
		m_list = list->second;
		return true;
	}

	m_list = std::make_shared<SampleThumbnailList>();
	s_sampleThumbnailListMap.insert(std::make_pair(name, m_list));
	return false;
}

void SampleThumbnailListManager::cleanUpGlobalThumbnailMap()
{
	auto map = s_sampleThumbnailListMap.begin();
	while (map != s_sampleThumbnailListMap.end())
	{
		// All instances of SampleThumbnailListManager are destroyed,
		// a.k.a sample goes out of use
		if (map->second.use_count() == 1)
		{
			s_sampleThumbnailListMap.erase(map);
			map = s_sampleThumbnailListMap.begin();
			continue;
		}

		map++;
	}
}

SampleThumbnail SampleThumbnailListManager::generate(const size_t thumbnailSize, const SampleFrame* buffer, const size_t size)
{
	const auto sampleChunk = (size + thumbnailSize) / thumbnailSize;
	auto thumbnail = SampleThumbnail(thumbnailSize);

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

SampleThumbnailListManager::SampleThumbnailListManager(const Sample& inputSample)
{
	if (selectFromGlobalThumbnailMap(inputSample)) { return; }

	cleanUpGlobalThumbnailMap();

	const auto sampleBufferSize = inputSample.sampleSize();
	const auto& buffer = inputSample.data();

	// For very small samples, use the sample size.
	const auto firstThumbnailSize = std::min(std::max(sampleBufferSize, std::size_t{1}), std::size_t{MaxThumbnailSize});

	auto& thumbnaillist = *m_list;

	SampleThumbnail firstThumbnail = generate(firstThumbnailSize, buffer, sampleBufferSize);

	thumbnaillist.push_back(firstThumbnail);

	// Generate the remaining thumbnails using the first one, each one's
	// size is the size of the previous one divided by the thumbnail size divisor.
	for (auto thumbnailSize = std::size_t{firstThumbnailSize / ThumbnailSizeDivisor}; thumbnailSize >= MinThumbnailSize;
		 thumbnailSize /= ThumbnailSizeDivisor)
	{
		const auto& biggerThumbnail = thumbnaillist.back();
		const auto biggerThumbnailSize = biggerThumbnail.size();
		auto bitIndex = std::size_t{0};

		SampleThumbnail thumbnail = std::vector(thumbnailSize, lmms::SampleThumbnailBit());

		for (const auto& biggerBit : biggerThumbnail)
		{
			auto& bit = thumbnail[bitIndex * thumbnailSize / biggerThumbnailSize];

			bit.merge(biggerBit);

			++bitIndex;
		}

		thumbnaillist.push_back(thumbnail);
	}
}

void SampleThumbnailListManager::draw(QPainter& painter, const SampleThumbnailBit& bit, int lineX, int centerY,
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

	painter.drawLine(lineX, lineY1, lineX, lineY2);
	painter.setPen(rmsColor);

	painter.drawLine(lineX, rmsLineY1, lineX, rmsLineY2);
	painter.setPen(color);
}

void SampleThumbnailListManager::visualize(
	const SampleThumbnailVisualizeParameters& parameters, QPainter& painter) const
{
	const auto sampleLength = parameters.sampleEnd - parameters.sampleStart;

	// We specify that the existence of the original sample
	// means we may need the sample to be drawn
	// with the highest quaility possible.
	//
	// For AFP and SlicerT where the sample isn't drawn a whole lot
	// of times and waveform is required to be crisp.
	//
	// However when the sample too large, we still use thumbnails.
	if (parameters.originalSample)
	{
		const auto sampleSize = static_cast<float>(parameters.originalSample->sampleSize());
		const auto sampleViewSize = static_cast<long>(sampleSize * sampleLength);

		if (sampleViewSize / parameters.width < 882)
		{
			visualizeOriginal(parameters, painter);
			return;
		}
	}

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

	auto list = m_list->end() - 1;
	const auto begin = m_list->begin();
	const auto widthSelect = static_cast<long>(static_cast<float>(width) / sampleLength);
	while (list != begin && list->size() < widthSelect)
	{
		list--;
	}

	const auto& thumbnail = *list;

	const auto thumbnailSize = thumbnail.size();
	const auto thumbnailLastSample = std::max(static_cast<std::size_t>(parameters.sampleEnd * thumbnailSize), std::size_t{1}) - 1;
	const auto tStart = static_cast<long>(parameters.sampleStart * thumbnailSize);
	const auto thumbnailViewSize = thumbnailLastSample + 1 - tStart;
	const auto tLast = std::min(thumbnailLastSample, thumbnailSize - 1);
	const auto pixelBound = std::min(width, parameters.clipWidthSinceSampleStart);

	auto tIndex = 0;
	auto pixelIndex = absXOr0;
	const auto tChunk = (thumbnailSize + width) / width;

	do
	{
		tIndex = tStart + pixelIndex * thumbnailViewSize / width;
		auto thumbnailBit = SampleThumbnailBit();
		const auto tChunkBound = tIndex + tChunk;

		while (tIndex < tChunkBound)
		{
			thumbnailBit.merge(thumbnail[parameters.reversed ? tLast - tIndex : tIndex]);
			tIndex += 1;
		}

		draw(painter, thumbnailBit, pixelIndex + x, centerY, scalingFactor, color, rmsColor);

		pixelIndex++;
	} while (pixelIndex <= pixelBound && tIndex < tLast);
}

void SampleThumbnailListManager::visualizeOriginal(
	const SampleThumbnailVisualizeParameters& parameters, QPainter& painter) const
{
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

	const auto originalSampleBuffer = parameters.originalSample->data();
	const auto originalSampleSize = parameters.originalSample->sampleSize();

	const auto sampleStartFrame = static_cast<long>(parameters.sampleStart * originalSampleSize);
	const auto sampleEndFrame = std::min(static_cast<std::size_t>(parameters.sampleEnd * originalSampleSize), originalSampleSize);

	const auto thumbnail
		= generate(width, originalSampleBuffer + sampleStartFrame, sampleEndFrame);

	for (auto pixelIndex = absXOr0; pixelIndex < width; pixelIndex++)
	{
		draw(painter, thumbnail[pixelIndex], pixelIndex + x, centerY, scalingFactor, color, rmsColor);
	}
}

} // namespace lmms
