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

#include <limits>

#include "Sample.h"
#include "SampleBuffer.h"

#include "lmms_basics.h"
#include "lmms_export.h"

namespace lmms {

struct SampleThumbnailVisualizeParameters
{
	// Assign to this field when we need to render using the
	// original sample.
	const Sample* originalSample = nullptr;

	const float amplification;
	const bool reversed;

	// Not using these 2 fields in the Song editor clips.
	// [0..1] Range
 	const float sampleStart = 0.0;
	const float sampleEnd   = 1.0;

	// All the fields below are in pixel unit.

	const long x;
	const long y;

	// The length of the rendered sample is proportional to the
	// value of this field in the song editor.
	//
	// This carries the zoom level information in the Song Editor.
	//
	// In AFP, SlicerT or other places where the width and height
	// is always fixed, the sampleStart and sampleEnd fields carries
	// the zoom level information instead.
	const long width;
	const long height;

	// Song editor clips shorter than the sample length (measuring
	// from the start of the sample) can specify this field so
	// rendering cuts off early, reducing computation cost.
	const long clipWidthSinceSampleStart = std::numeric_limits<long>::max();
};

struct SampleThumbnailBit
{
	float max;
	float min;
	float rms;

	SampleThumbnailBit();

	SampleThumbnailBit(const SampleFrame&);

	void merge(const SampleThumbnailBit&);

	void mergeFrame(const SampleFrame&);
};

using SampleThumbnail = std::vector<SampleThumbnailBit>;
using SampleThumbnailList = std::vector<SampleThumbnail>;
using SharedSampleThumbnailList = std::shared_ptr<SampleThumbnailList>;

// Insert this into your class when you want to implement
// thumbnails. This is optional, but without this class, there will be no
// indication that this thumbnail list is being used and it will be destroyed
// when you generate thumbnails somewhere else, and have to be regenerated
// before being rendered, which is slower than rendering the sample directly.
//
// Doing
//
//		m_thumbnaillist = SampleThumbnailListManager(sample)
//
// every time you prepare to render the sample won't have a considerable
// performance hit.
class LMMS_EXPORT SampleThumbnailListManager
{
private:
	SharedSampleThumbnailList m_list;
	static std::map<const QString, SharedSampleThumbnailList> SAMPLE_THUMBNAIL_LIST_MAP;

protected:
	static void draw(
		QPainter& painter, const SampleThumbnailBit& bit,
		int lineX, int centerY, float scalingFactor,
		const QColor& color, const QColor& rmsColor
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

} // namespace lmms



#endif // LMMS_SAMPLE_THUMBNAIL_H
