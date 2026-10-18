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

#include "SampleBuffer.h"
#include "lmms_export.h"

class QPainter;

namespace lmms {
class Sample;
}

namespace lmms::gui {

/**
   Allows for visualizing sample data.

   On construction, thumbnails will be generated
   at logarathmic intervals of downsampling. Those cached thumbnails will then be further downsampled on the fly and
   transformed in various ways to create the desired waveform.

   Given that we are dealing with far less data to generate
   the visualization however (i.e., we are not reading from original sample data when drawing), this provides a
   significant performance boost that wouldn't be possible otherwise.
 */
class LMMS_EXPORT SampleThumbnailCache
{
public:
	struct VisualizeParameters
	{
		//!< A rectangle that covers the entire range of samples from [@a sampleStart, @a sampleEnd)
		//!< @a rect must share the same coordinate system as that of the painter in which it is drawn with.
		QRectF rect;

		//!< Where the sample begins for drawing.
		double sampleStart = 0.f;

		//!< Where the sample ends for drawing.
		double sampleEnd = 1.f;

		//!< The amount of amplification to apply to the waveform.
		float amplification = 1.0f;

		//!< Determines if the waveform is drawn in reverse or not.
		bool reversed = false;
	};

	class Thumbnail
	{
	public:
		struct Peak
		{
			friend auto operator+(const Peak& left, const Peak& right) -> Peak
			{ return Peak{std::min(left.min, right.min), std::max(left.max, right.max)}; }

			auto operator+=(const Peak& other) -> Peak&
			{
				min = std::min(min, other.min);
				max = std::max(max, other.max);
				return *this;
			}

			float min = std::numeric_limits<float>::infinity();
			float max = -std::numeric_limits<float>::infinity();
		};

		explicit Thumbnail(f_cnt_t width)
			: m_peaks(width)
		{
		}

		auto compress(f_cnt_t outputWidth) const -> Thumbnail;

		auto operator[](f_cnt_t index) -> Peak& { return m_peaks[index]; }
		auto operator[](f_cnt_t index) const -> const Peak& { return m_peaks[index]; }

		auto begin() { return m_peaks.begin(); }
		auto begin() const { return m_peaks.begin(); }

		auto end() { return m_peaks.end(); }
		auto end() const { return m_peaks.end(); }

		auto width() const -> f_cnt_t { return m_peaks.size(); }

	private:
		std::vector<Peak> m_peaks;
	};

	SampleThumbnailCache() = default;
	SampleThumbnailCache(const SampleBuffer& buffer);

	/**
	 * @brief Get a thumbnail of the requested width.
	 * If no thumbnail of the requested width exists, the nearest larger thumbnail is returned.
	 * @param width The requested thumbnail width.
	 * @pre @a width is in the range [1, N] where N is the number of frames in the original buffer.
	 * @note This function is not thread safe.
	 */
	auto nearestThumbnailForWidth(f_cnt_t width) -> Thumbnail&;

	void visualize(VisualizeParameters parameters, QPainter& painter);

private:
	// Thumbnails are to be in descending order (highest to lowest resolution, or longest to shortest width)
	// These thumbnails are representative of the entire sample
	std::vector<Thumbnail> m_thumbnails;
	f_cnt_t m_sampleCount;
};

} // namespace lmms::gui

#endif // LMMS_SAMPLE_THUMBNAIL_H
