/*
 * gui_templates.h - GUI-specific templates
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_GUI_TEMPLATES_H
#define LMMS_GUI_TEMPLATES_H

#include "lmmsconfig.h"

#include <algorithm>
#include <QApplication>
#include <QFont>
#include <QGuiApplication>

// TODO: remove once qt5 support is dropped
#if (QT_VERSION < QT_VERSION_CHECK(6,0,0))
	#include <QScreen>
#endif

namespace lmms::gui
{


// return DPI-independent font-size - font with returned font-size has always
// the same size in pixels
inline QFont pointSize(QFont fontPointer, float fontSize)
{
	// to calculate DPI of a screen to make it HiDPI ready
	qreal devicePixelRatio = QGuiApplication::primaryScreen()->devicePixelRatio();
    qreal scaleFactor = std::max(devicePixelRatio, 1.0); // Ensure scaleFactor is at least 1.0

	fontPointer.setPointSizeF(fontSize * scaleFactor);
	return fontPointer;
}


} // namespace lmms::gui

#endif // LMMS_GUI_TEMPLATES_H
