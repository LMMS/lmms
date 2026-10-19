/*
 * SampleThumbnailRenderer.h
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

#ifndef LMMS_SAMPLE_THUMBNAIL_RENDERER_H
#define LMMS_SAMPLE_THUMBNAIL_RENDERER_H

#include <QRectF>

#include "lmms_export.h"

class QPainter;

namespace lmms {
class SampleThumbnailCache;
}

namespace lmms::gui {

/**
 * @brief A class that can be used to render sample thumbnails.
 */
class LMMS_EXPORT SampleThumbnailRenderer
{
public:
	struct VisualizeParameters
	{
		//!< The sample thumbnail cache to use.
		const SampleThumbnailCache* cache;

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

	//! Visualize sample thumbnails with the given @a parameters and @a painter object.
	static void visualize(VisualizeParameters parameters, QPainter& painter);
};

} // namespace lmms::gui

#endif // LMMS_SAMPLE_THUMBNAIL_RENDERER_H
