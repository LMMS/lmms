/*
 * WindowEmbed.h - Window embedding helper
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

#ifndef LMMS_WINDOW_EMBED_H
#define LMMS_WINDOW_EMBED_H

#include <vector>
#include <string_view>

namespace lmms
{

struct WindowEmbed
{
	enum class Method
	{
		Headless, // no GUI at all
		Floating, // detached
		Qt,
		Win32,
		Cocoa,
		XEmbed
		// ...
	};

	//! The vector returned will not contain None or Headless
	static auto availableMethods() -> std::vector<Method>;

	//! Returns true if embed methods exist
	static auto embeddable() -> bool;

	static auto toString(Method method) -> std::string_view
	{
		// NOTE: These strings must NOT change - used in files saved to disk
		switch (method)
		{
			case Method::Headless: return "headless";
			case Method::Floating: return "none";
			case Method::Qt:       return "qt";
			case Method::Win32:    return "win32";
			case Method::Cocoa:    return "cocoa";
			case Method::XEmbed:   return "xembed";
		}
		//assert("invalid window embed method");
		return "";
	}

	static auto toEnum(std::string_view method) -> Method
	{
		if (method == "headless") { return Method::Headless; }
		if (method == "none") { return Method::Floating; }
		if (method == "qt") { return Method::Qt; }
		if (method == "win32") { return Method::Win32; }
		if (method == "cocoa") { return Method::Cocoa; }
		if (method == "xembed") { return Method::XEmbed; }
		//assert("invalid window embed method");
		return Method::Floating;
	}
};

} // namespace lmms

#endif // LMMS_WINDOW_EMBED_H
