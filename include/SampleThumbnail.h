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

#include <QPainter>
#include <QRect>
#include <memory>
#include "Sample.h"
#include "lmms_export.h"

namespace lmms {

/**
   Insert this into your class when you want to implement
   thumbnails. This is optional, but without this class, there will be no
   indication that this thumbnail list is being used and it will be destroyed
   when you generate thumbnails somewhere else, and have to be regenerated
   before being rendered, which is slower than rendering the sample directly.
*/
class LMMS_EXPORT SampleThumbnail
{
public:
	struct VisualizeParameters
	{
		QRect sampleRect; //!< A rectangle that covers the entire range of samples.

		QRect drawRect; //!< Specifies the location in `sampleRect` where the waveform will be drawn. Equals
						//!< `sampleRect` when null.

		QRect viewportRect; //!< Clips `drawRect`. Equals `drawRect` when null.

		float amplification = 1.0f; //!< The amount of amplification to apply to the waveform.

		float sampleStart = 0.0f; //!< Where the sample begins for drawing.

		float sampleEnd = 1.0f; //!< Where the sample ends for drawing.

		bool reversed = false; //!< Determines if the waveform is drawn in reverse or not.
	};

	SampleThumbnail() = default;
	SampleThumbnail(const Sample& sample);

	void visualize(const VisualizeParameters& parameters, QPainter& painter) const;

	bool selectFromGlobalThumbnailMap(const Sample&);
	static void cleanUpGlobalThumbnailMap();

private:
	class Thumbnail
	{
	public:
		static constexpr auto AggregationPerZoomStep = 2;

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

			float min = std::numeric_limits<float>::max();
			float max = -std::numeric_limits<float>::max();
		};

		Thumbnail() = default;
		Thumbnail(std::vector<Peak> peaks, double samplesPerPeak);
		Thumbnail(const SampleFrame* buffer, size_t size, int width);

		Thumbnail zoomOut(float factor) const;
		void clip(size_t from, size_t to);
		void reverse() { std::reverse(m_peaks.begin(), m_peaks.end()); }

		Peak& operator[](size_t index) { return m_peaks[index]; }
		const Peak& operator[](size_t index) const { return m_peaks[index]; }

		Peak* peaks() { return m_peaks.data(); }
		const Peak* peaks() const { return m_peaks.data(); }
		
		int width() const { return m_peaks.size(); }
		double samplesPerPeak() const { return m_samplesPerPeak; }

	private:
		std::vector<Peak> m_peaks;
		double m_samplesPerPeak = 0.0;
	};

	using ThumbnailCache = std::vector<Thumbnail>;
	std::shared_ptr<ThumbnailCache> m_thumbnailCache = nullptr;

	/* DEPRECATED; functionality is kept for testing conveniences */
	inline static std::map<const QString, std::shared_ptr<ThumbnailCache>> s_sampleThumbnailCacheMap;
};

} // namespace lmms

#endif // LMMS_SAMPLE_THUMBNAIL_H
