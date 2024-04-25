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

//~ SampleThumbnailBit::SampleThumbnailBit(float max, float min):
	//~ max(max),
	//~ min(min)
//~ {}

SampleThumbnailBit::SampleThumbnailBit(const sampleFrame& frame):
	maxRMS(-0.0),
	minRMS( 0.0)
{
	if (frame[0] >= frame[1]) 
	{	
		this->max = frame[0];
		this->min = frame[1];
	} 
	else 
	{
		this->max = frame[1];
		this->min = frame[0];
	}
}

SampleThumbnailBit::SampleThumbnailBit():
	max(-100.0),
	min( 100.0),
	maxRMS(-0.0),
	minRMS( 0.0)
{}

SampleThumbnailBit::~SampleThumbnailBit()
{}

void SampleThumbnailBit::merge(const SampleThumbnailBit& other)
{
	this->min = std::min(this->min, other.min);
	this->max = std::max(this->max, other.max);
	
	this->minRMS = std::min(this->minRMS, other.minRMS);
	this->maxRMS = std::max(this->maxRMS, other.maxRMS);
	
	this->maxRMS = std::clamp(this->maxRMS, this->min, this->max);
	this->minRMS = std::clamp(this->minRMS, this->min, this->max);
}

void SampleThumbnailBit::mergeFrame(const sampleFrame& frame) 
{
	const auto newLine = SampleThumbnailBit(frame);
	
	this->merge(newLine);
}

SampleThumbnailBit SampleThumbnailBit::linear(const SampleThumbnailBit& other, float t) const 
{
	float t2 = 1.0 - t;
	
	SampleThumbnailBit o;
	
	o.max = this->max * t2 + other.max * t;
	o.min = this->min * t2 + other.min * t;
	
	o.maxRMS = this->maxRMS * t2 + other.maxRMS * t;
	o.minRMS = this->minRMS * t2 + other.minRMS * t;
	
	return o;
}

SampleThumbnailListManager::SampleThumbnailListManager()
{
	this->list = std::make_shared< SampleThumbnailList >( SampleThumbnailList() );
}

SampleThumbnailListManager::SampleThumbnailListManager(const Sample& inputSample)
{
	const auto sampleBufferSize = inputSample.sampleSize();
	const auto& buffer = inputSample.data();
	
	const auto samplePtr = inputSample.buffer();
	const auto end = SAMPLE_THUMBNAIL_MAP.end();
	
	auto list = SAMPLE_THUMBNAIL_MAP.begin();
	while (list != end && list->first.get() != samplePtr.get()) list++; 
	
	if (list != end) 
	{
		this->list = list->second;
		
		return;
	}
	
	qDebug("Generating new thumbnails...");
	
	this->list = std::make_shared<SampleThumbnailList>( SampleThumbnailList() );
	
	SAMPLE_THUMBNAIL_MAP.insert(
		std::pair<const std::shared_ptr<const SampleBuffer>, SharedSampleThumbnailList>
		(
			samplePtr,
			this->list
		)
	);
	
	// Clean up SAMPLE_THUMBNAIL_MAP
	{
		auto start = SAMPLE_THUMBNAIL_MAP.begin();
		while (start != SAMPLE_THUMBNAIL_MAP.end()) 
		{
			if (start->first.use_count() == 1) 
			{
				qDebug("Deleting an unusued thumbnail...");
				SAMPLE_THUMBNAIL_MAP.erase(start);
				start = SAMPLE_THUMBNAIL_MAP.begin();
				continue;
			}
			
			start++;
		}
	}
	
	qDebug("Now holding %lu thumbnaillists", SAMPLE_THUMBNAIL_MAP.size());
	
	const size_t maxResoThumbnailSize = 
		std::min<size_t>(
			sampleBufferSize,
			MAX_THUMBNAIL_SIZE
		);
	
	auto& thumbnaillist = *this->list;
	
	SampleThumbnail maxResoThumbnail = std::vector(maxResoThumbnailSize, lmms::SampleThumbnailBit());
	
	size_t sampleFrameIndex = 0;
	
	float rms = 0.0;
	
	for (; sampleFrameIndex < sampleBufferSize; sampleFrameIndex++)
	{
		size_t sampleThumbnailIndex = 
			sampleFrameIndex *
			maxResoThumbnailSize / 
			sampleBufferSize
		;
		
		const auto& frame = buffer[sampleFrameIndex];
				
		auto& bit = maxResoThumbnail[sampleThumbnailIndex];
		
		rms = rms*0.5 + std::abs(bit.max + bit.min)*0.5;
		
		bit.maxRMS = std::clamp( rms, bit.min, bit.max);
		bit.minRMS = std::clamp(-rms, bit.min, bit.max);
		
		bit.mergeFrame(frame);
	}
	
	thumbnaillist.push_back(maxResoThumbnail);
	
	for (
		size_t thumbnailSize = maxResoThumbnailSize / THUMBNAIL_SIZE_DIVISOR; 
		thumbnailSize >= MIN_THUMBNAIL_SIZE; 
		thumbnailSize /= THUMBNAIL_SIZE_DIVISOR
	)
	{
		const auto& biggerThumbnail = thumbnaillist.back();
		const auto biggerThumbnailSize = biggerThumbnail.size();
		size_t bitIndex = 0;
		
		SampleThumbnail thumbnail = std::vector(thumbnailSize, lmms::SampleThumbnailBit());
		
		qDebug("Size %lu", thumbnailSize);
		
		for (const auto& biggerBit: biggerThumbnail) 
		{
			auto& bit = thumbnail[bitIndex * thumbnailSize / biggerThumbnailSize];
			
			//~ rms = rms*0.5 + std::abs(bit.max + bit.min)*0.5;
			
			//~ bit.maxRMS = std::clamp( rms, bit.min, bit.max);
			//~ bit.minRMS = std::clamp(-rms, bit.min, bit.max);
			
			bit.merge(biggerBit);
			
			++bitIndex;
		}
		
		thumbnaillist.push_back(thumbnail);
	}
}

void SampleThumbnailListManager::visualize(
	SampleThumbnailVisualizeParameters parameters, 
	QPainter& painter, 
	const QRect& rect
) {
	const int x = rect.x();
	const int height = rect.height();
	const int width = rect.width();
	const int centerY = rect.center().y();
	
	const auto color = painter.pen().color();
	const auto rmsColor = color.lighter(123);
	//qDebug("offset_end %d", parameters.offset_end);

	const int halfHeight = height / 2;
	
	const float scalingFactor = 
		halfHeight * parameters.amplification 
		// static_cast<float>(parameters.numChannels)
	;
	
	size_t last = this->list->size()-1;
	size_t setIndex = 0;
	while (setIndex < last && (*this->list)[setIndex].size() > width)
	{
		setIndex++;
	}
	
	const auto& thumbnail = (*this->list)[setIndex];
	
	const float thumbnailSize = static_cast<float>(thumbnail.size());
	const size_t tS = thumbnail.size();
	// const float xThumbnail = (x - parameters.offset_start) * thumbnailSize / width;
	
	const size_t bound = std::min<size_t>(width, parameters.offset_end);
	
	for (size_t pixelIndex = 0; pixelIndex < bound; pixelIndex++) 
	{
		float thumbnailIndex = thumbnailSize * pixelIndex / width; 
		float thumbnailIndexNext = 0.0;
		
		if (thumbnailIndex >= thumbnailSize || thumbnailIndex < 0) continue;
		
		if (parameters.reversed)
		{
			thumbnailIndexNext = thumbnailSize - thumbnailIndex;
			thumbnailIndex = std::floor(thumbnailIndexNext);
		} 
		else
		{	
			thumbnailIndexNext = std::ceil(thumbnailIndex);	
		}
		
		float t = thumbnailIndexNext - thumbnailIndex;
		
		size_t tI = thumbnailIndex;
		size_t tIN = thumbnailIndexNext;
		
		if (tIN >= tS) tIN = thumbnail.size()-1;
		
		const auto thumbnailBit = 
			thumbnail[tI]
			.linear(thumbnail[tIN], t)
		;
		
		const int lengthY1 = thumbnailBit.max * scalingFactor;
		const int lengthY2 = thumbnailBit.min * scalingFactor;
		
		const int lineY1 = centerY - lengthY1;
		const int lineY2 = centerY - lengthY2;
		const int lineX = pixelIndex + x;
		painter.drawLine(lineX, lineY1, lineX, lineY2);
		
		const int rmsLineY1 = centerY - thumbnailBit.maxRMS * scalingFactor;
		const int rmsLineY2 = centerY - thumbnailBit.minRMS * scalingFactor;

		painter.setPen(rmsColor);
		painter.drawLine(lineX, rmsLineY1, lineX, rmsLineY2);
		painter.setPen(color);
	}
}

}
