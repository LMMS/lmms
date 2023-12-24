/*
 * ClapExtension.h - Base class template for implementing CLAP extensions
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

#ifndef LMMS_CLAP_EXTENSION_H
#define LMMS_CLAP_EXTENSION_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include <string_view>

#include <clap/plugin.h>

namespace lmms
{

class ClapInstance;

namespace detail
{

struct ClapExtensionHelper
{
	static auto fromHost(const clap_host* host) -> ClapInstance*;
};

} // namespace detail

template<class HostExt, class PluginExt>
class ClapExtension : public detail::ClapExtensionHelper
{
public:
	ClapExtension() = default;
	virtual ~ClapExtension() = default;

	ClapExtension(const ClapExtension&) = delete;
	ClapExtension(ClapExtension&&) = delete;
	auto operator=(const ClapExtension&) = delete;
	auto operator=(ClapExtension&&) = delete;

	virtual auto init(const clap_host* host, const clap_plugin* plugin) -> bool
	{
		if (m_pluginExt) { return false; } // already init

		m_host = host;
		m_plugin = plugin;
		const auto ext = static_cast<const PluginExt*>(plugin->get_extension(plugin, extensionId().data()));
		if (!ext || !checkSupported(ext)) { return false; }

		m_pluginExt = ext;
		return true;
	}

	virtual void deinit()
	{
		m_pluginExt = nullptr;
		m_plugin = nullptr;
	}

	auto supported() const -> bool { return m_pluginExt != nullptr; }

	virtual auto extensionId() const -> std::string_view = 0;
	virtual auto hostExt() const -> const HostExt* = 0;
	auto pluginExt() const { return m_pluginExt; }

protected:
	auto host() const { return m_host; }
	auto plugin() const { return m_plugin; }

	virtual auto checkSupported(const PluginExt* ext) -> bool = 0;

	const clap_host* m_host = nullptr;
	const clap_plugin* m_plugin = nullptr;
	//const HostExt* m_hostExt = nullptr;
	const PluginExt* m_pluginExt = nullptr;
};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_EXTENSION_H
