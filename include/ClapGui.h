/*
 * ClapGui.h - Implements CLAP GUI extension
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

#ifndef LMMS_CLAP_GUI_H
#define LMMS_CLAP_GUI_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include "ClapFile.h"
#include "WindowEmbed.h"

#include <string_view>

#include "lmms_basics.h"

#include <clap/ext/gui.h>

struct clap_plugin;

namespace lmms
{

class ClapGui
{
public:
	friend class ClapInstance;

	ClapGui(const ClapPluginInfo* info, const clap_plugin* plugin, const clap_plugin_gui* gui);

	auto create() -> bool;

	static auto extensionSupported(const clap_plugin_gui* gui) noexcept -> bool;

	auto gui() const { return m_gui; }
	auto isFloating() const { return m_embedMethod == WindowEmbed::Method::None; }

private:

	/**
	 * clap_host_gui implementation
	 */
	void clapResizeHintsChanged();
	auto clapRequestResize(std::uint32_t width, std::uint32_t height) -> bool;
	auto clapRequestShow() -> bool;
	auto clapRequestHide() -> bool;
	void clapRequestClosed(bool wasDestroyed);

	const ClapPluginInfo* m_pluginInfo = nullptr;
	const clap_plugin* m_plugin = nullptr;
	const clap_plugin_gui* m_gui = nullptr;
	WindowEmbed::Method m_embedMethod = WindowEmbed::Method::Headless;
	bool m_guiCreated = false;
	bool m_guiVisible = false;
};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_GUI_H
