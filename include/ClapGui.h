/*
 * ClapGui.h - Implements CLAP gui extension
 *
 * Copyright (c) 2024 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include <string_view>
#include <clap/ext/gui.h>

#include "ClapFile.h"
#include "ClapExtension.h"
#include "WindowEmbed.h"
#include "lmms_basics.h"

namespace lmms
{

class ClapGui : public ClapExtension<clap_host_gui, clap_plugin_gui>
{
public:
	ClapGui(ClapInstance* instance);
	~ClapGui() { destroy(); }

	auto extensionId() const -> std::string_view override { return CLAP_EXT_GUI; }
	auto hostExt() const -> const clap_host_gui* override;

	auto create() -> bool;
	void destroy();

	auto isFloating() const { return m_embedMethod == WindowEmbed::Method::Floating; }

	auto supportsEmbed() const { return m_supportsEmbed; }
	auto supportsFloating() const { return m_supportsFloating; }

private:
	auto initImpl(const clap_host* host, const clap_plugin* plugin) noexcept -> bool override;
	void deinitImpl() noexcept override;
	auto checkSupported(const clap_plugin_gui& ext) -> bool override;

	static auto windowSupported(const clap_plugin_gui& ext, bool floating) -> bool;

	/**
	 * clap_host_gui implementation
	 */
	static void clapResizeHintsChanged(const clap_host* host);
	static auto clapRequestResize(const clap_host* host, std::uint32_t width, std::uint32_t height) -> bool;
	static auto clapRequestShow(const clap_host* host) -> bool;
	static auto clapRequestHide(const clap_host* host) -> bool;
	static void clapRequestClosed(const clap_host* host, bool wasDestroyed);

	clap_window m_window{};

	bool m_created = false;
	bool m_visible = false;

	WindowEmbed::Method m_embedMethod = WindowEmbed::Method::Headless;
	bool m_supportsEmbed = false;
	bool m_supportsFloating = false;
};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_GUI_H
