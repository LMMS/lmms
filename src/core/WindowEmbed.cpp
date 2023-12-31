/*
 * WindowEmbed.cpp - Window embedding helper
 *
 * Copyright (c) 2023 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include "WindowEmbed.h"

#include <QString>
#include <QApplication>
#include <QGuiApplication>

#include "lmmsconfig.h"

namespace lmms
{

auto WindowEmbed::availableMethods() -> std::vector<Method>
{
	auto methods = std::vector{
#if QT_VERSION >= 0x050100
		Method::Qt,
#endif
#ifdef LMMS_BUILD_APPLE
		Method::Cocoa,
#endif
#ifdef LMMS_BUILD_WIN32
		Method::Win32,
#endif
	};

#ifdef LMMS_BUILD_LINUX
	if (static_cast<QGuiApplication*>(QApplication::instance())->platformName() == "xcb")
	{
		methods.push_back(Method::XEmbed);
	}
#endif
	return methods;
}

auto WindowEmbed::embeddable() -> bool
{
	return !availableMethods().empty();
}

} // namespace lmms
