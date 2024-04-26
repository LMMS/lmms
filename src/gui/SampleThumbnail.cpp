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

bool SampleThumbnailListManager::selectFromGlobalThumbnailMap(
	const Sample& inputSample
)
{
	const auto samplePtr = inputSample.buffer();
	const QString name = inputSample.sampleFile();
	
	const auto end = SAMPLE_THUMBNAIL_MAP.end();
	auto list = SAMPLE_THUMBNAIL_MAP.begin();
	
	while (list != end && list->first != name) list++; 
	
	if (list == end)
	{
		this->list = std::make_shared< SampleThumbnailList >( SampleThumbnailList() );
			
		qDebug("Generating thumbnails for file: %s", qUtf8Printable(name));
		
		SAMPLE_THUMBNAIL_MAP.insert(
			std::pair<const QString, SSTLandSBSPV>
			(
				name,
				SSTLandSBSPV {
					this->list,
					SampleBufferSharedPtrVec{ samplePtr }
				}
			)
		);
		
		return false;
	}
	else
	{
		this->list = list->second.sharedSampleList;
		
		list->second.vectorPtr.push_back(samplePtr);
		
		return true;
	}
}

void SampleThumbnailListManager::cleanUpGlobalThumbnailMap()
{
	// This code looks complicated because of the way LMMS behaves.
	// When you open a project in LMMS, the clips that contain the same
	// audio file end up getting their own samples (and therefore 
	// sampleBuffers). Thus, we need to track 2 things:
	//
	// 1. The name of the sampleBuffer, to avoid making duplicates.
	//
	// 2. A vector of shared_ptrs to the sampleBuffers constructed from
	// the sample audio file. When this vector goes empty. The sample 
	// has gone out of use, and we can then delete the thumbnail  
	// associated with this audio file.
	
	auto map = SAMPLE_THUMBNAIL_MAP.begin();
	while (map != SAMPLE_THUMBNAIL_MAP.end()) 
	{
		// Find and erase all orphaned sampleBuffers
		SampleBufferSharedPtrVec& vec = map->second.vectorPtr;
		auto ptr = vec.end();
		for (;;)
		{
			ptr = std::find_if(
				vec.begin(),
				vec.end(), 
				[](const auto& a) -> bool { return a.use_count() == 1; } 
			);
			
			if (ptr == vec.end()) { break; }
			
			qDebug("Erasing a shared_ptr");
			
			vec.erase(ptr); 
		}
		
		
		// all shared pointers are orphaned a.k.a the sample is out of 
		// use
		if (vec.empty()) 
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

SampleThumbnailListManager::SampleThumbnailListManager(const Sample& inputSample)
{
	if (selectFromGlobalThumbnailMap(inputSample)) { return; }
	
	cleanUpGlobalThumbnailMap();
	
	const auto sampleBufferSize = inputSample.sampleSize();
	const auto& buffer = inputSample.data();
	
	const size_t firstThumbnailSize = 
		std::max<size_t>(
			std::min<size_t>(sampleBufferSize, 1),
			MAX_THUMBNAIL_SIZE
		);
		
	qDebug("Sample to be prepared is of size: %lu", sampleBufferSize);
	qDebug("Max thumbnail size: %lu", firstThumbnailSize); 
	
	auto& thumbnaillist = *this->list;
	
	SampleThumbnail firstThumbnail = std::vector(firstThumbnailSize, lmms::SampleThumbnailBit());
	
	size_t sampleFrameIndex = 0;
	
	float rms = 0.0;
	
	// Prepare the first thumbnail with the largest size.
	for (; sampleFrameIndex < sampleBufferSize; sampleFrameIndex++)
	{
		size_t sampleThumbnailIndex = 
			sampleFrameIndex *
			firstThumbnailSize / 
			sampleBufferSize
		;
		
		const auto& frame = buffer[sampleFrameIndex];
				
		auto& bit = firstThumbnail[sampleThumbnailIndex];
		
		rms = rms*0.85 + std::abs(bit.max + bit.min)*0.1;
		
		bit.maxRMS = std::clamp( rms, bit.min, bit.max);
		bit.minRMS = std::clamp(-rms, bit.min, bit.max);
		
		bit.mergeFrame(frame);
	}
	
	thumbnaillist.push_back(firstThumbnail);
	
	// Generate the remaining thumbnails using the first one, each one
	// is half the size of the previous.
	for (
		size_t thumbnailSize = firstThumbnailSize / THUMBNAIL_SIZE_DIVISOR; 
		thumbnailSize >= MIN_THUMBNAIL_SIZE;
		thumbnailSize /= THUMBNAIL_SIZE_DIVISOR
	)
	{
		const auto& biggerThumbnail = thumbnaillist.back();
		const auto biggerThumbnailSize = biggerThumbnail.size();
		size_t bitIndex = 0;
		
		SampleThumbnail thumbnail = std::vector(thumbnailSize, lmms::SampleThumbnailBit());
		
		qDebug("Size %lu", thumbnail.size());
		
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
	// Prevent unnecessary out of bounds drawing.
	const int absXOr0 = (x < 0) ? ( -x ) : ( 0 );
	
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
	
	long last = this->list->size()-1;
	long setIndex = 0;
	while (setIndex < last && (*this->list)[setIndex].size() > width)
	{
		setIndex++;
	}
	
	const auto& thumbnail = (*this->list)[setIndex];
	
	//~ qDebug("Using thumbnail of size: %lu", thumbnail.size());
	
	const float thumbnailSize = static_cast<float>(thumbnail.size());
	const long tS = thumbnail.size()-1;
	//~ const float xThumbnail = (x - parameters.offset_start) * thumbnailSize / width;
	
	const long bound = std::min<long>(width, parameters.pixelsTillSampleEnd);
	
	const float ratio = thumbnailSize / static_cast<float>(width);
	
	long tI = 0;
	// Don't draw out of bounds.
	long pixelIndex = absXOr0;
	do 
	{
		tI = static_cast<long>(pixelIndex * ratio);
		
		const auto thumbnailBit = thumbnail[
			(parameters.reversed) ? ( tS - tI ) : ( tI )
		];
		
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
		
		pixelIndex++;
	
	} while (pixelIndex <= bound && tI < tS);
	
}

}
