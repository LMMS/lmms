/* ColorHelper.h - Helper methods for color related algorithms, etc.
 *
 * Copyright (c) 2024- Michael Gregorius
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

#ifndef LMMS_GUI_COLOR_HELPER_H
#define LMMS_GUI_COLOR_HELPER_H

#include <cmath>
#include <QColor>

namespace lmms::gui
{

class ColorHelper
{
public:
	static QColor interpolateInRgb(const QColor& a, const QColor& b, float t)
	{
		qreal ar, ag, ab, aa;
		a.getRgbF(&ar, &ag, &ab, &aa);

		qreal br, bg, bb, ba;
		b.getRgbF(&br, &bg, &bb, &ba);

		const auto t2 = static_cast<qreal>(t);
		const float interH = std::lerp(ar, br, t2);
		const float interS = std::lerp(ag, bg, t2);
		const float interV = std::lerp(ab, bb, t2);
		const float interA = std::lerp(aa, ba, t2);

		return QColor::fromRgbF(interH, interS, interV, interA);
	}
};

} // namespace lmms::gui

#endif // LMMS_GUI_COLOR_HELPER_H
