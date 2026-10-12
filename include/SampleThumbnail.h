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
#include "SampleFrame.h"
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
	SampleThumbnail(const SampleBuffer& buffer);
	void visualize(VisualizeParameters parameters, QPainter& painter) const;

private:
	class Thumbnail
	{
	public:
		struct Peak
		{
			auto operator+(const Peak& other) const -> Peak
			{ return Peak{std::min(min, other.min), std::max(max, other.max)}; }

			float min = std::numeric_limits<float>::infinity();
			float max = -std::numeric_limits<float>::infinity();
		};

		explicit Thumbnail(int width)
			: m_peaks(width)
		{
		}

		auto compress(int factor) const -> Thumbnail;

		auto operator[](std::size_t index) -> Peak& { return m_peaks[index]; }
		auto operator[](std::size_t index) const -> const Peak& { return m_peaks[index]; }

		auto begin() { return m_peaks.begin(); }
		auto begin() const { return m_peaks.begin(); }

		auto end() { return m_peaks.end(); }
		auto end() const { return m_peaks.end(); }

		auto width() const -> int { return m_peaks.size(); }

	private:
		std::vector<Peak> m_peaks;
	};

	// Thumbnails are to be in a sorted order (lowest to highest resolution)
	// These thumbnails are representative of the entire sample data
	std::vector<Thumbnail> m_thumbnails;
};

} // namespace lmms::gui

#endif // LMMS_SAMPLE_THUMBNAIL_H
