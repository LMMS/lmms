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

#include "ClapThreadCheck.h"

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
	ClapExtension(const ClapInstance* instance) : m_instance{instance} {}
	virtual ~ClapExtension() = default;

	ClapExtension(const ClapExtension&) = delete;
	ClapExtension(ClapExtension&&) = delete;
	auto operator=(const ClapExtension&) = delete;
	auto operator=(ClapExtension&&) = delete;

	auto init(const clap_host* host, const clap_plugin* plugin) -> bool
	{
		if (m_pluginExt) { return false; } // already init

		m_host = host;
		m_plugin = plugin;
		const auto ext = static_cast<const PluginExt*>(plugin->get_extension(plugin, extensionId().data()));
		if (!ext || !checkSupported(*ext))
		{
			m_pluginExt = nullptr;
			m_supported = false;
			return false;
		}

		m_pluginExt = ext;
		m_supported = true;
		if (!initImpl(host, plugin))
		{
			m_supported = false;
			return false;
		}

		return true;
	}

	void deinit()
	{
		deinitImpl();
		m_pluginExt = nullptr;
		m_supported = false;
	}

	auto instance() const { return m_instance; }

	/**
	 * Returns whether plugin implements required interface
	 *   and passes any additional checks from initImpl().
	 * Do not use before init().
	 * 
	*/
	auto supported() const { return m_supported; }

	/**
	 * Checks whether the plugin extension implements the required
	 *   API methods for use within LMMS. May not be all the methods.
	*/
	virtual auto checkSupported(const PluginExt& ext) -> bool = 0;

	virtual auto extensionId() const -> std::string_view = 0;
	virtual auto hostExt() const -> const HostExt* = 0;

	//! Non-null after init() is called if plugin implements the needed interface
	auto pluginExt() const { return m_pluginExt; }

protected:
	/**
	 * For additional initialization steps.
	 * - Only called if basic init was successful
	 * - pluginExt() is non-null and supported() == true during this call
	*/
	virtual auto initImpl(const clap_host* host, const clap_plugin* plugin) noexcept -> bool { return true; }

	/**
	 * For additional deinitialization steps.
	*/
	virtual void deinitImpl() noexcept {}

	auto host() const { return m_host; }
	auto plugin() const { return m_plugin; }

private:
	const ClapInstance* m_instance = nullptr;

	const clap_host* m_host = nullptr;
	const clap_plugin* m_plugin = nullptr;
	//const HostExt* m_hostExt = nullptr;
	const PluginExt* m_pluginExt = nullptr;

	bool m_supported = false;
};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_EXTENSION_H
