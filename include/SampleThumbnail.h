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

constexpr unsigned long long MIN_THUMBNAIL_SIZE = 1;
constexpr unsigned long long MAX_THUMBNAIL_SIZE = 32768;
constexpr unsigned long long THUMBNAIL_SIZE_DIVISOR = 32;

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
	const Sample* originalSample = nullptr;
	
	const float amplification;
	const bool reversed;
	
	const float sampleStartPercent = 0.0;
	const float sampleEndPercent   = 1.0;
		
	const long x;
	const long y;
	const long width;
	const long height;
};

struct SampleThumbnailBit 
{
	float max;
	float min;
	float maxRMS;
	float minRMS;
	
	SampleThumbnailBit();
	
	SampleThumbnailBit(const SampleFrame&);
	
	void merge(const SampleThumbnailBit&);
	
	void mergeFrame(const SampleFrame&);
};

using SampleThumbnail = std::vector<SampleThumbnailBit>;
using SampleThumbnailList = std::vector<SampleThumbnail>;
using SharedSampleThumbnailList = std::shared_ptr<SampleThumbnailList>;

class LMMS_EXPORT SampleThumbnailListManager
{
private:
	SharedSampleThumbnailList list;
	static std::map<const QString, SharedSampleThumbnailList> SAMPLE_THUMBNAIL_MAP;
	
protected:
	static void draw(
		QPainter& painter, const SampleThumbnailBit& bit,
		int lineX, int centerY, float scalingFactor, 
		QColor color, QColor rmsColor
	);
	
	static SampleThumbnail generate(
		const size_t thumbnailsize, 
		const SampleFrame* sampleBuffer,
		const SampleFrame* sampleBufferEnd
	);

public:
	SampleThumbnailListManager();

	SampleThumbnailListManager(const Sample&);

	bool selectFromGlobalThumbnailMap(const Sample&); 
	
	void cleanUpGlobalThumbnailMap();
		
	void visualize(const SampleThumbnailVisualizeParameters&, QPainter&) const;
	static void visualize_original(const SampleThumbnailVisualizeParameters&, QPainter&);

};
	
}



#endif // LMMS_SAMPLE_THUMBNAIL_H
