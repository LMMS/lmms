/*
 * SampleThumbnailCache.h
 *
 * Copyright (c) 2026 Khoi Dau <casboi86@gmail.com>
 * Copyright (c) 2026 saker <sakertooth@gmail.com>
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

#ifndef LMMS_SAMPLE_THUMBNAIL_CACHE_H
#define LMMS_SAMPLE_THUMBNAIL_CACHE_H

#include "SampleBuffer.h"
#include "lmms_export.h"

namespace lmms {
class Sample;

/**
 * @class SampleThumbnailCache
 * @brief Holds increasingly downsampled versions of an audio waveform for efficient rendering of them.
 * 
 * Instead of reading a lot of audio samples every time the screen redraws or zooms, 
 * this class keeps a list of small, pre-calculated waveforms (known as thumbnails, @see Thumbnail)
 * at different zoom levels.
 */
class LMMS_EXPORT SampleThumbnailCache
{
public:
	/**
	 * @class Thumbnail
	 * @brief Encapsulates a fixed-width vector of audio peak min/max values.
	 *
	 * A Thumbnail represents the entire sample downsampled to a specific width.
	 * It provides a compression method to further downsample or stretch itself into a smaller target.
	 */
	class Thumbnail
	{
	public:
		/**
		 * @struct Peak
		 * @brief Stores a audio peak value pairing (i.e., minimum and maximum sample over some amount of time).
		 *
		 * Peaks track the absolute lowest and highest sample values over a specific time window.
		 */
		struct Peak
		{
			/**
			 * @brief Combines two peaks together by getting the smallest minimum and largest maximum of the two.
			 * @return A new Peak containing the minimum and maximum values of both peaks.
			 */
			friend auto operator+(const Peak& left, const Peak& right) -> Peak
			{ return Peak{std::min(left.min, right.min), std::max(left.max, right.max)}; }

			/**
			 * @brief In-place accumulates another peak into this peak, expanding this one as necessary.
			 * @return Reference to this peak being modified.
			 */
			auto operator+=(const Peak& other) -> Peak&
			{
				min = std::min(min, other.min);
				max = std::max(max, other.max);
				return *this;
			}

			/**
			 * @brief The lowest audio sample found in this frame window.
			 * Initialized to positive infinity so that any real sample value will be immediately lower than it.
			 */
			float min = std::numeric_limits<float>::infinity();

			/**
			 * @brief The highest audio sample found in this frame window.
			 * Initialized to negative infinity so that any real sample value will be immediately higher than it.
			 */
			float max = -std::numeric_limits<float>::infinity();
		};

		/**
		 * @brief Constructs an empty thumbnail of a certain width.
		 * @param width The number of @ref Peak elements this thumbnail will contain.
		 */
		explicit Thumbnail(f_cnt_t width)
			: m_peaks(width)
		{
		}

		//! @brief Downsamples this thumbnail to fit a smaller screen width.
		auto compress(f_cnt_t outputWidth) const -> Thumbnail;

		//! @returns The peak at the given index.
		auto operator[](f_cnt_t index) -> Peak& { return m_peaks[index]; }

		//! @returns The peak at the given index.
		auto operator[](f_cnt_t index) const -> const Peak& { return m_peaks[index]; }

		//! @returns An iterator to the first peak.
		auto begin() { return m_peaks.begin(); }

		//! @returns An iterator to the first peak.
		auto begin() const { return m_peaks.begin(); }

		//! @returns An iterator to the last peak.
		auto end() { return m_peaks.end(); }

		//! @returns An iterator to the last peak.
		auto end() const { return m_peaks.end(); }

		//! @returns The number of peaks stored inside this thumbnail.
		auto width() const -> f_cnt_t { return m_peaks.size(); }

	private:
		std::vector<Peak> m_peaks;
	};

	/**
	 * @brief Constructs an uninitialized, empty thumbnail cache.
	 */
	SampleThumbnailCache() = default;

	/**
	 * @brief Constructs a cache and populates its multi-resolution levels from a sample buffer.
	 * @param buffer The sample buffer to downsample and analyze.
	 */
	SampleThumbnailCache(const SampleBuffer& buffer);

	/**
	 * @brief Get a thumbnail of the requested width.
	 * If no thumbnail of the requested width exists, the nearest larger thumbnail is returned.
	 * @param width The requested thumbnail width.
	 * @pre @a width is in the range [1, N] where N is the number of frames in the original buffer.
	 * @note This function is not thread safe.
	 */
	auto nearestThumbnailForWidth(f_cnt_t width) -> Thumbnail&;

	//! @returns The number of samples in the original audio buffer.
	auto sampleCount() -> f_cnt_t { return m_sampleCount; }

private:
	// Thumbnails are to be in descending order (highest to lowest resolution, or longest to shortest width)
	// These thumbnails are representative of the entire sample
	std::vector<Thumbnail> m_thumbnails;
	f_cnt_t m_sampleCount;
};

} // namespace lmms

#endif // LMMS_SAMPLE_THUMBNAIL_CACHE_H
