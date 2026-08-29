/*
 * LocaleHelper.h - compatibility functions for handling decimal separators
 * Providing helper functions which handle both periods and commas
 * for decimal separators to load old projects correctly
 *
 * Copyright (c) 2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2018 Hyunjin Song <tteu.ingog/at/gmail.com>
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

// NOTE: The LMMS/zynaddsubfx repo contains a copy of this header.
//       If you modify this file, consider modifying it there as well.

#ifndef LMMS_LOCALEHELPER_H
#define LMMS_LOCALEHELPER_H

#include <QLocale>

#include <limits>
#include <cmath>

namespace lmms::LocaleHelper
{

inline double toDouble(const QString& str, bool* ok = nullptr)
{
	bool isOkay;
	QLocale c(QLocale::C);
	c.setNumberOptions(QLocale::RejectGroupSeparator);
	double value = c.toDouble(str, &isOkay);
	if (!isOkay)
	{
		QLocale german(QLocale::German);
		german.setNumberOptions(QLocale::RejectGroupSeparator);
		value = german.toDouble(str, &isOkay);
	}
	if (ok != nullptr) { *ok = isOkay; }
	return value;
}

inline float toFloat(const QString& str, bool* ok = nullptr)
{
	double d = toDouble(str, ok);
	if (!std::isinf(d) && std::fabs(d) > std::numeric_limits<float>::max())
	{
		if (ok != nullptr) { *ok = false; }
		return 0.0f;
	}
	return static_cast<float>(d);
}


} // namespace lmms::LocaleHelper

#endif // LMMS_LOCALEHELPER_H
