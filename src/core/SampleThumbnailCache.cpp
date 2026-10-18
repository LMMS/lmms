/*
 * SampleThumbnailCache.cpp
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

#include "SampleThumbnailCache.h"

namespace {
constexpr auto AggregationPerZoomStep = 4;
} // namespace

namespace lmms {

SampleThumbnailCache::SampleThumbnailCache(const SampleBuffer& buffer)
	: m_sampleCount{buffer.size()}
{
	if (buffer.empty()) { return; }

	auto highestResolutionThumbnail = Thumbnail{m_sampleCount};
	for (f_cnt_t i = 0; i < highestResolutionThumbnail.width(); ++i)
	{
		const auto value = buffer.data()[i].average();
		highestResolutionThumbnail[i] = Thumbnail::Peak{value, value};
	}

	// The thumbnail with the highest resolution is the entire sample buffer summed down to mono
	m_thumbnails.emplace_back(std::move(highestResolutionThumbnail));

	// The thumbnails after it are increasingly compressed via the peaks being aggregated
	while (m_thumbnails.back().width() >= AggregationPerZoomStep)
	{
		auto thumbnail = m_thumbnails.back().compress(m_thumbnails.back().width() / AggregationPerZoomStep);
		m_thumbnails.emplace_back(std::move(thumbnail));
	}
}

auto SampleThumbnailCache::nearestThumbnailForWidth(f_cnt_t width) const -> const Thumbnail&
{
	assert(1 <= width && width <= m_sampleCount && "invalid width");

	// The thumbnails are in descending order, starting from the largest width to the smallest
	// We want to find the last element that is still greater than the requested width
	// This is the same as finding the first element smaller than the requested width and moving back one
	// If not found, then the requested width is smaller than (ordered after) all the others cached 
	const auto it = std::lower_bound(m_thumbnails.begin(), m_thumbnails.end(), width,
		[](const Thumbnail& thumbnail, f_cnt_t width) { return thumbnail.width() > width; });

	// Check if the one we found is equal before moving back
	if (it != m_thumbnails.end() && it->width() == width) { return *it; }

	// Move back by one to get the nearest larger thumbnail
	return *std::prev(it);
}

auto SampleThumbnailCache::Thumbnail::compress(f_cnt_t outputWidth) const -> Thumbnail
{
	assert(outputWidth <= width() && "output width cannot be greater than this thumbnail's width");

	const auto factor = static_cast<double>(width()) / outputWidth;
	auto compressedThumbnail = Thumbnail{outputWidth};

	for (f_cnt_t out = 0; out < outputWidth; ++out)
	{
		const auto start = m_peaks.begin() + static_cast<f_cnt_t>(std::floor(out * factor));
		const auto end = m_peaks.begin() + static_cast<f_cnt_t>(std::floor((out + 1) * factor));
		compressedThumbnail[out] = std::accumulate(start, end, Peak{});
	}

	return compressedThumbnail;
}

} // namespace lmms::gui
