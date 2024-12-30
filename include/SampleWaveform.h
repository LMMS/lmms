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
#include <cmath>

#include "SampleFrame.h"
#include "lmms_export.h"

namespace lmms::gui {
class LMMS_EXPORT SampleWaveform
{
public:
	struct Parameters
	{
		const SampleFrame* buffer; //!< The buffer to all the samples contained within the rectangle
		size_t size;			   //!< The number of samples contained within the rectangle
		float amplification;	   //!< Amplfication to apply to the waveform
		bool reversed;			   //!< If the waveform should be drawn in reverse
	};

	//! Visualize a sample waveform.
	//! `rect` is the rectangle that contains the all the samples. It specifies where the full waveform would be drawn
	//! and for how long. `viewport` is a viewport into `rect` and specifies where the drawing will happen and for how
	//! long. The position of `rect` should be specified relative to the `viewport`.
	static void visualize(Parameters parameters, QPainter& painter, const QRect& rect, const QRect& viewport);
};
} // namespace lmms::gui

#endif // LMMS_GUI_SAMPLE_WAVEFORM_H
