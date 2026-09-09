/*
 * DpiHelper.h - helpers for scaling hard-coded design sizes to the display
 *
 * Copyright (c) 2026 Zachariah Markusson <zachariahmarkusson@gmail.com>
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

#ifndef LMMS_GUI_DPI_HELPER_H
#define LMMS_GUI_DPI_HELPER_H

#include <QGuiApplication>
#include <QScreen>

#include <cmath>

namespace lmms::gui
{

//! Returns the factor by which the display scaling enlarges the UI, i.e. the
//! device pixel ratio of the primary screen. Qt keeps widget geometry in
//! device-independent pixels, so hard-coded design sizes have to be scaled by
//! this factor to follow the display scaling. Returns 1.0 if there is no
//! screen, e.g. when running headless.
inline float displayScaleFactor()
{
	const auto screen = QGuiApplication::primaryScreen();
	return screen ? static_cast<float>(screen->devicePixelRatio()) : 1.0f;
}

//! Scales a hard-coded design size in pixels to the current display scaling.
inline int scaledPixels(int designPixels)
{
	return static_cast<int>(std::lround(designPixels * displayScaleFactor()));
}

//! Inverse of scaledPixels(): converts a display size back to a design size.
inline int unscaledPixels(int displayPixels)
{
	return static_cast<int>(std::lround(displayPixels / displayScaleFactor()));
}

} // namespace lmms::gui

#endif // LMMS_GUI_DPI_HELPER_H
