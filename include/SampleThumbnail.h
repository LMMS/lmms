/*
 * SampleThumbnail.h
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

#ifndef LMMS_SAMPLE_THUMBNAIL_H
#define LMMS_SAMPLE_THUMBNAIL_H

#include <QDateTime>
#include <QRect>
#include <memory>

#include "lmms_export.h"
#include "SampleBuffer.h"
#include "SampleFrame.h"

class QPainter;

namespace lmms {

class Sample;

/**
   Allows for visualizing sample data.

   On construction, thumbnails will be generated
   at logarathmic intervals of downsampling. Those cached thumbnails will then be further downsampled on the fly and
   transformed in various ways to create the desired waveform.

   Given that we are dealing with far less data to generate
   the visualization however (i.e., we are not reading from original sample data when drawing), this provides a
   significant performance boost that wouldn't be possible otherwise.
 */
class LMMS_EXPORT SampleThumbnail
{
public:
	struct VisualizeParameters
	{
		QRect sampleRect; //!< A rectangle that covers the entire range of samples.

		QRect viewportRect; //!< Specifies the location in `sampleRect` where the waveform will be drawn. Equals
							//!< `sampleRect` when null.

		float amplification = 1.0f; //!< The amount of amplification to apply to the waveform.

		float sampleStart = 0.0f; //!< Where the sample begins for drawing.

		float sampleEnd = 1.0f; //!< Where the sample ends for drawing.

		bool reversed = false; //!< Determines if the waveform is drawn in reverse or not.
	};

	SampleThumbnail() = default;
	SampleThumbnail(const Sample& sample);
	void visualize(VisualizeParameters parameters, QPainter& painter) const;

private:
	class Thumbnail
	{
	public:
		struct Peak
		{
			Peak() = default;

			Peak(float min, float max)
				: min(min)
				, max(max)
			{
			}

			Peak(const SampleFrame& frame)
				: min(std::min(frame.left(), frame.right()))
				, max(std::max(frame.left(), frame.right()))
			{
			}

			Peak operator+(const Peak& other) const { return Peak(std::min(min, other.min), std::max(max, other.max)); }
			Peak operator+(const SampleFrame& frame) const { return *this + Peak{frame}; }

			float min = std::numeric_limits<float>::infinity();
			float max = -std::numeric_limits<float>::infinity();
		};

		Thumbnail() = default;
		Thumbnail(std::vector<Peak> peaks, double samplesPerPeak);
		Thumbnail(const float* buffer, size_t size, size_t width);

		Thumbnail zoomOut(float factor) const;

		Peak* data() { return m_peaks.data(); }
		Peak& operator[](size_t index) { return m_peaks[index]; }
		const Peak& operator[](size_t index) const { return m_peaks[index]; }

		int width() const { return m_peaks.size(); }
		double samplesPerPeak() const { return m_samplesPerPeak; }

	private:
		std::vector<Peak> m_peaks;
		double m_samplesPerPeak = 0.0;
	};

	struct SampleThumbnailEntry
	{
		QString filePath;
		QDateTime lastModified;

		friend bool operator==(const SampleThumbnailEntry& first, const SampleThumbnailEntry& second)
		{
			return first.filePath == second.filePath && first.lastModified == second.lastModified;
		}
	};

	struct Hash
	{
		std::size_t operator()(const SampleThumbnailEntry& entry) const noexcept { return qHash(entry.filePath); }
	};

	using ThumbnailCache = std::vector<Thumbnail>;
	std::shared_ptr<ThumbnailCache> m_thumbnailCache = std::make_shared<ThumbnailCache>();
	std::shared_ptr<const SampleBuffer> m_buffer = SampleBuffer::emptyBuffer();
	inline static std::unordered_map<SampleThumbnailEntry, std::shared_ptr<ThumbnailCache>, Hash> s_sampleThumbnailCacheMap;
};

} // namespace lmms

#endif // LMMS_SAMPLE_THUMBNAIL_H
