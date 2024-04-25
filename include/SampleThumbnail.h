/*
 * SampleThumbnail.h
 *
 * Copyright (c) 2024 Khoi Dau <casboi86@gmail.com>
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

#ifndef LMMS_SAMPLE_THUMBNAIL_H
#define LMMS_SAMPLE_THUMBNAIL_H

constexpr unsigned long long MIN_THUMBNAIL_SIZE = 32;
constexpr unsigned long long MAX_THUMBNAIL_SIZE = 32768;
constexpr unsigned long long THUMBNAIL_SIZE_DIVISOR = 2;

#include<vector>
#include<memory>

#include <QPainter>

#include "Sample.h"
#include "SampleBuffer.h"

#include "lmms_basics.h"
#include "lmms_export.h"

namespace lmms {

struct SampleThumbnailVisualizeParameters
{
	float amplification;
	bool reversed;
	float pixelsTillSampleEnd;
};

struct SampleThumbnailBit 
{
	float max;
	float min;
	float maxRMS;
	float minRMS;
	
	SampleThumbnailBit();
	
	SampleThumbnailBit(const sampleFrame& sample);
		
	~SampleThumbnailBit();
	
	void merge(const SampleThumbnailBit& other);
	
	void mergeFrame(const sampleFrame& sample);
	
	SampleThumbnailBit linear(const SampleThumbnailBit& other, float t) const;
};

using SampleThumbnail = std::vector<SampleThumbnailBit>;
using SampleThumbnailList = std::vector<SampleThumbnail>;
using SharedSampleThumbnailList = std::shared_ptr< SampleThumbnailList >;
using SampleBufferSharedPtrVec = std::vector< std::shared_ptr<const SampleBuffer> >;

// SharedSampleThumbnailList and SampleBufferSharedPtrVec
struct SSTLandSBSPV 
{
	SharedSampleThumbnailList sharedSampleList;
	SampleBufferSharedPtrVec vectorPtr;
};

static std::map<const QString, SSTLandSBSPV> SAMPLE_THUMBNAIL_MAP;

class LMMS_EXPORT SampleThumbnailListManager 
{
private:
	SharedSampleThumbnailList list;

public:
	SampleThumbnailListManager();

	SampleThumbnailListManager(const Sample& inputSample);

	bool selectFromGlobalThumbnailMap(const Sample& inputSample); 
	
	void cleanUpGlobalThumbnailMap();
	
	void visualize(SampleThumbnailVisualizeParameters parameters, QPainter& painter, const QRect& rect);
};
	
}



#endif // LMMS_SAMPLE_THUMBNAIL_H
