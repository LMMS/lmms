/*
 * QuantizationGrid.h - Linear grid for quantizing positions and durations
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

#ifndef LMMS_QUANTIZATION_GRID_H
#define LMMS_QUANTIZATION_GRID_H

#include "TimePos.h"

namespace lmms {

class QuantizationGrid
{
public:
	QuantizationGrid() = default;
	QuantizationGrid(tick_t interval, TimePos offset) :
		m_interval{interval},
		m_offset{offset}
	{ }

	auto interval() const noexcept -> tick_t { return m_interval; }
	auto offset() const noexcept -> TimePos { return m_offset; }

	void setInterval(tick_t interval) noexcept { m_interval = interval; }
	void setOffset(TimePos offset) noexcept { m_offset = offset; }

	auto quantize(TimePos pos) const noexcept -> TimePos;
	auto quantizeDuration(tick_t duration) const noexcept -> tick_t;

private:
	tick_t m_interval = 1;
	TimePos m_offset = TimePos{0};
};

} // namespace lmms

#endif // LMMS_QUANTIZATION_GRID_H
