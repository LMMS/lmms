/*
 * SampleWaveform.h
 *
 * Copyright (c) 2023 saker <sakertooth@gmail.com>
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

#ifndef LMMS_GUI_SAMPLE_WAVEFORM_H
#define LMMS_GUI_SAMPLE_WAVEFORM_H

#include <QPainter>
#include <optional>

#include "SampleFrame.h"
#include "lmms_export.h"

namespace lmms::gui {
class LMMS_EXPORT SampleWaveform
{
public:
	SampleWaveform() = default;
	SampleWaveform(const SampleFrame* buffer, size_t size);

	void generate();
	void visualize(QPainter& painter, const QRect& rect,
		float amplification = 1.0f, bool reversed = false,
		std::optional<size_t> from = std::nullopt,
		std::optional<size_t> to = std::nullopt);
	void reset(const SampleFrame* buffer, size_t size);

private:
	const SampleFrame* m_buffer = nullptr;
	size_t m_size = 0;
	std::unordered_map<int, std::vector<float>> m_min;
	std::unordered_map<int, std::vector<float>> m_max;
};
} // namespace lmms::gui

#endif // LMMS_GUI_SAMPLE_WAVEFORM_H
