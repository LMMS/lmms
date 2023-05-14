/*
 * DeprecationHelper.h - This file contains the declarations of helper functions
 * which helps centralize the #ifdefs preprocessors regarding deprecation based on Qt versions.
 * The functions are defined differently based on the callers' Qt versions.
 *
 * Copyright (c) 2020 Tien Dat Nguyen <ntd.bk.k56/at/gmail.com>
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

#ifndef LMMS_DEPRECATIONHELPER_H
#define LMMS_DEPRECATIONHELPER_H

#include <QFontMetrics>
#include <QWheelEvent>

namespace lmms
{

/**
 * @brief horizontalAdvance is a backwards-compatible adapter for
 * QFontMetrics::horizontalAdvance and width functions.
 * @param metrics
 * @param text
 * @return text's horizontal advance based on metrics.
 */
inline int horizontalAdvance(const QFontMetrics& metrics, const QString& text)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
	return metrics.horizontalAdvance(text);
#else
	return metrics.width(text);
#endif
}

/**
 * @brief position is a backwards-compatible adapter for
 * QWheelEvent::position and pos functions.
 * @param wheelEvent
 * @return the position of wheelEvent
 */
inline QPoint position(QWheelEvent *wheelEvent)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
	return wheelEvent->position().toPoint();
#else
	return wheelEvent->pos();
#endif
}

} // namespace lmms

#endif // LMMS_DEPRECATIONHELPER_H
