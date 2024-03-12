/*
 * QuantizationGrid.cpp - Linear grid for quantizing positions and durations
 *
 * Copyright (c) 2024 Dominic Clark
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
 */

#include "QuantizationGrid.h"

namespace lmms {

auto QuantizationGrid::quantize(TimePos pos) const noexcept -> TimePos
{
	// Adjust the position to be a whole number of intervals from the offset
	return m_offset + quantizeDuration(pos - m_offset);
}

auto QuantizationGrid::quantizeDuration(tick_t duration) const noexcept -> tick_t
{
	// Compute how many whole intervals make up the duration, and the remaining ticks
	auto intervals = duration / m_interval;
	auto remainder = duration % m_interval;

	// Transfer an interval to the remainder if necessary to ensure it is non-negative
	if (remainder < 0) {
		remainder += m_interval;
		--intervals;
	}

	// Round up midpoint or later
	if (remainder * 2 >= m_interval) { ++intervals; }

	// Convert final interval count back to ticks
	return intervals * m_interval;
}

} // namespace lmms
