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
#include "PathUtil.h"

namespace lmms {

std::map<const QString, SharedSampleThumbnailList> 
	SampleThumbnailListManager::SAMPLE_THUMBNAIL_MAP{};

SampleThumbnailBit::SampleThumbnailBit(const SampleFrame& frame):
	maxRMS(-0.0),
	minRMS( 0.0)
{
	const float l = frame.left();
	const float r = frame.right();
	
	if (l > r)
	{
		max = l;
		min = r;
	}
	else
	{
		max = r;
		min = l;
	}
}


SampleThumbnailBit::SampleThumbnailBit():
	max(-100.0),
	min( 100.0),
	maxRMS(-0.0),
	minRMS( 0.0)
{}

void SampleThumbnailBit::merge(const SampleThumbnailBit& other)
{
	min = std::min(min, other.min);
	max = std::max(max, other.max);
	
	minRMS = std::min(minRMS, other.minRMS);
	maxRMS = std::max(maxRMS, other.maxRMS);
	
	//maxRMS = std::clamp(maxRMS, min, max);
	//minRMS = std::clamp(minRMS, min, max);
}

void SampleThumbnailBit::mergeFrame(const SampleFrame& frame) 
{
	const auto other = SampleThumbnailBit(frame);
	min = std::min(min, other.min);
	max = std::max(max, other.max);
}

SampleThumbnailListManager::SampleThumbnailListManager()
{
	this->list = nullptr;
}

bool SampleThumbnailListManager::selectFromGlobalThumbnailMap(
	const Sample& inputSample
)
{
	const auto samplePtr 	= inputSample.buffer();
	const QString name 		= inputSample.sampleFile();
	
	const auto end = SAMPLE_THUMBNAIL_MAP.end();
	auto list = SAMPLE_THUMBNAIL_MAP.find(name);
	
	if (list != end)
	{
		this->list = list->second;			
		return true;
	}

	this->list = std::make_shared<SampleThumbnailList>(SampleThumbnailList());
	
	qDebug("Generating thumbnails for file: %s", qUtf8Printable(name));
	
	SAMPLE_THUMBNAIL_MAP.insert(
		std::pair<const QString, SharedSampleThumbnailList>
		(
			name,
			this->list
		)
	);
	
	return false;
}

void SampleThumbnailListManager::cleanUpGlobalThumbnailMap()
{
	auto map = SAMPLE_THUMBNAIL_MAP.begin();
	while (map != SAMPLE_THUMBNAIL_MAP.end()) 
	{
		// All instances of SampleThumbnailListManager are destroyed,
		// a.k.a sample goes out of use
		if (map->second.use_count() == 1)
		{
			qDebug("Deleting an orphaned thumbnaillist...");
			SAMPLE_THUMBNAIL_MAP.erase(map);
			map = SAMPLE_THUMBNAIL_MAP.begin();
			continue;
		}
		
		map++;
	}
	
	qDebug("Now holding %lu thumbnaillists", SAMPLE_THUMBNAIL_MAP.size());
}

SampleThumbnail SampleThumbnailListManager::generate(
	const size_t thumbnailSize, 
	const SampleFrame* sampleBuffer,
	const SampleFrame* sampleBufferEnd
) {
	const size_t sampleSize  = sampleBufferEnd - sampleBuffer;
	
	const size_t sampleChunk = (sampleSize + thumbnailSize) / thumbnailSize;
	
	SampleThumbnail thumbnail(thumbnailSize, SampleThumbnailBit());
	
	for (size_t tIndex = 0; tIndex < thumbnailSize; tIndex++)
	{
		size_t sampleIndex = tIndex * sampleSize / thumbnailSize;
		
		const size_t sampleChunkBound = std::min(
			sampleIndex + sampleChunk,
			sampleSize
		);
		
		auto& bit = thumbnail[tIndex];
		
		float rms = 0.0;
		
		while (sampleIndex < sampleChunkBound)
		{
			const auto& frame = sampleBuffer[sampleIndex];
			bit.mergeFrame(frame);
			const float ave = frame.average();
			rms += ave * ave;
			
			sampleIndex++;
		}
		
		rms = std::sqrt(rms / sampleChunk);
				
		bit.maxRMS = std::clamp( rms, bit.min, bit.max);
		bit.minRMS = std::clamp(-rms, bit.min, bit.max);
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
	const size_t firstThumbnailSize = 
		std::min<size_t>(
			std::max<size_t>(sampleBufferSize, 1),
			MAX_THUMBNAIL_SIZE
		);
		
	qDebug("Sample to be prepared is of size: %lu", sampleBufferSize);
	qDebug("Max thumbnail size: %lu", firstThumbnailSize); 
	
	auto& thumbnaillist = *this->list;
	
	SampleThumbnail firstThumbnail = generate(
		firstThumbnailSize, 
		buffer, 
		buffer + sampleBufferSize
	);
	
	thumbnaillist.push_back(firstThumbnail);
	
	// Generate the remaining thumbnails using the first one, each one's
	// size is the size of the previous one divided by THUMBNAIL_SIZE_DIVISOR
	for (
		size_t thumbnailSize = firstThumbnailSize / THUMBNAIL_SIZE_DIVISOR; 
		thumbnailSize >= MIN_THUMBNAIL_SIZE;
		thumbnailSize /= THUMBNAIL_SIZE_DIVISOR
	) {
		const auto& biggerThumbnail = thumbnaillist.back();
		const auto biggerThumbnailSize = biggerThumbnail.size();
		size_t bitIndex = 0;
		
		SampleThumbnail thumbnail = std::vector(thumbnailSize, lmms::SampleThumbnailBit());
		
		qDebug("Generating for size %lu", thumbnail.size());
		
		for (const auto& biggerBit: biggerThumbnail) 
		{
			auto& bit = thumbnail[bitIndex * thumbnailSize / biggerThumbnailSize];

			bit.merge(biggerBit);
			
			++bitIndex;
		}
		
		thumbnaillist.push_back(thumbnail);
	}
}

void SampleThumbnailListManager::draw(
		QPainter& painter, const SampleThumbnailBit& bit,
		int lineX, int centerY, float scalingFactor, 
		QColor color, QColor rmsColor
) {
	const int lengthY1 = bit.max * scalingFactor;
	const int lengthY2 = bit.min * scalingFactor;
	
	const int lineY1 = centerY - lengthY1;
	const int lineY2 = centerY - lengthY2;
	
	const int rmsLineY1 = centerY - bit.maxRMS * scalingFactor;
	const int rmsLineY2 = centerY - bit.minRMS * scalingFactor;

	painter.drawLine(lineX, lineY1, lineX, lineY2);
	painter.setPen(rmsColor);
	painter.drawLine(lineX, rmsLineY1, lineX, rmsLineY2);
	painter.setPen(color);
}


void SampleThumbnailListManager::visualize(
	const SampleThumbnailVisualizeParameters& parameters, 
	QPainter& painter
) const {
	
	const float sampleView = parameters.sampleEnd - parameters.sampleStart;
	
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
		const float sampleSize = static_cast<float>(parameters.originalSample->sampleSize());
		const long sampleViewSize = static_cast<long>(sampleSize * sampleView);

		if (sampleViewSize / parameters.width < 882)
		{
			visualize_original(parameters, painter);
			return;
		}
	}
	
	const long x = parameters.x;
	
	// If the clip extends to the left past the start of the sample,
	// start starting at the start of the sample and skip the blank space. 
	const long absXOr0 = (x < 0) ? -x : 0;
	
	const long height 		= parameters.height;
	const long halfHeight 	= height / 2;
	const long width 		= parameters.width;
	const long centerY 		= parameters.y + halfHeight;
	
	const auto color 	= painter.pen().color();
	const auto rmsColor = color.lighter(123);

	const float scalingFactor = 
		halfHeight * parameters.amplification 
	;
	
	auto list 			= this->list->end()-1;
	const auto begin 	= this->list->begin();
	
	const long widthSelect = static_cast<long>(1.0f * width / sampleView);
	//qDebug("%ld", widthSelect);
	
	while (list != begin && list->size() < widthSelect)
	{
		list--;
	}
	
	const auto& thumbnail = *list;
	
	const long thumbnailSize = thumbnail.size();
	const long thumbnailLastSample = std::max<long>(parameters.sampleEnd*thumbnailSize, 1) - 1;

	//qDebug("Using thumbnail of size %ld", thumbnailSize);
	
	const long tStart = static_cast<long>(parameters.sampleStart * thumbnailSize);

	const long thumbnailViewSize = thumbnailLastSample + 1 - tStart;
	
	const long tLast = std::min(thumbnailLastSample, thumbnailSize-1);
	long tIndex = 0;
		
	const long tChunk = (thumbnailSize + width) / width;
	
	const long pixelBound = std::min(width, parameters.clipWidthSinceSampleStart);
	
	// Don't draw out of bounds.
	long pixelIndex = absXOr0;
	
	//qDebug("%ld", width);
	
	do
	{
		tIndex = tStart + pixelIndex * thumbnailViewSize / width;
		
		auto thumbnailBit = SampleThumbnailBit();
		
		const long tChunkBound = tIndex + tChunk;
		
		while (tIndex < tChunkBound)
		{
			thumbnailBit.merge(thumbnail[
				parameters.reversed ? tLast - tIndex : tIndex 
			]);
			tIndex += 1;
		}
		
		draw(
			painter, thumbnailBit, 
			pixelIndex + x, centerY, scalingFactor, 
			color, rmsColor
		);
		
		pixelIndex++;
	}
	while (pixelIndex <= pixelBound && tIndex < tLast);
	
}

// Method is made public to be easily accessed when 
// one needs to quickly draw a sample without a thumbnail.
//
// As performant as the original SampleWaveform code.
void SampleThumbnailListManager::visualize_original(
	const SampleThumbnailVisualizeParameters& parameters, 
	QPainter& painter
) {	
	const long x = parameters.x;
	
	// If the clip extends to the left past the start of the 
	// sample, start drawing at the start of the sample and 
	// skip the blank space.
	const int absXOr0 = (x < 0) ? -x : 0;
	
	const long height 		= parameters.height;
	const long halfHeight 	= height / 2;
	const long width 		= parameters.width;
	const long centerY 		= parameters.y + halfHeight;
	
	const float scalingFactor = 
		halfHeight * parameters.amplification 
	;
	
	const auto color 	= painter.pen().color();
	const auto rmsColor = color.lighter(123);
	
	const auto originalSampleBuffer = parameters.originalSample->data();
	const long originalSampleSize = parameters.originalSample->sampleSize();
	
	const long sampleStartFrame = static_cast<long>(parameters.sampleStart * originalSampleSize);
	const long sampleEndFrame = std::min<long>(
		parameters.sampleEnd * originalSampleSize,
		originalSampleSize
	);
	
	const auto thumbnail = generate(
		width, 
		originalSampleBuffer + sampleStartFrame, 
		originalSampleBuffer + sampleEndFrame
	);
	
	for (long pixelIndex = absXOr0; pixelIndex < width; pixelIndex++) 
	{
		draw(
			painter, 
			thumbnail[pixelIndex], 
			pixelIndex + x, centerY, scalingFactor, 
			color, rmsColor
		);
	}
}


} // namespace lmms
