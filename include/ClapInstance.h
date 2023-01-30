/*
 * ClapInstance.h - Implementation of ClapInstance class
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

#ifndef LMMS_CLAP_INSTANCE_H
#define LMMS_CLAP_INSTANCE_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include "ClapFile.h"

#include <memory>

namespace lmms
{

/**
 * ClapInstance stores a CLAP host/plugin instance pair.
 *
 * When a new CLAP plugin instance is created by the ClapManager,
 * a new CLAP host instance needs to be passed to the plugin instance,
 * creating a CLAP host instance / CLAP plugin instance pair.
 * The plugin instance will pass the host pointer whenever it calls the
 * host's API (instead of passing the plugin pointer), and that is how
 * the host instance can know which plugin instance called the host API.
 * 
 * The ClapInstance::Host class implements the CLAP host API and
 *     stores the pointer to the clap_host_t object.
 * The ClapInstance::Plugin class provides access to the plugin instance
 *     for making plugin API calls
 *
 * Every ClapInstance is owned by the ClapManager, though other classes may
 * work with non-owned references borrowed from the ClapManager.
 */
class ClapInstance
{
public:
	class Host;
	class Plugin;

	ClapInstance(const ClapPluginInfo& pluginInfo);
	~ClapInstance();

	//! Host instance
	class Host
	{
	public:
		//! Creates a clap_host_t host instance
		Host(const ClapInstance& parent);

		auto getHost() const -> const clap_host_t* { return &m_host; }
		auto getPlugin() const -> const clap_plugin_t* { return m_parent.getPlugin(); }
		auto isValid() const -> bool { return true; }

	private:
		static Host* fromHost(const clap_host* host);
		static auto getExtension(const clap_host_t* host, const char* extension_id) -> const void*;
		static void requestCallback(const clap_host* host);
		static void requestProcess(const clap_host* host);
		static void requestRestart(const clap_host* host);

		const ClapInstance& m_parent;
		clap_host_t m_host;
	};

	//! Plugin instance
	class Plugin
	{
	public:
		//! Creates a clap_plugin_t plugin instance
		Plugin(const ClapInstance& parent, const ClapPluginInfo& info);

		//! Deactivates and destroys plugin instance as needed
		~Plugin();

		//! Initializes extensions for plugin instance; Returns true when successful
		auto initExtensions() -> bool;

		auto getHost() const -> const clap_host_t* { return m_parent.getHost(); }
		auto getPlugin() const -> const clap_plugin_t* { return m_plugin; }
		auto getPluginInfo() const -> const ClapPluginInfo& { return m_info; }
		auto isValid() const -> bool { return m_plugin != nullptr; }

	private:

		template<typename T>
		void initExtension(const T*& ext, const char* id)
		{
			// Must be on main thread
			if (!ext)
				ext = static_cast<const T*>(m_plugin->get_extension(m_plugin, id));
		}

		const ClapInstance& m_parent;
		const clap_plugin_t* m_plugin;
		const ClapPluginInfo& m_info;
		bool m_active{false};

		// TODO: Add plugin extension pointers here as support is implemented in the host
		const clap_plugin_audio_ports_t* m_extAudioPorts{nullptr};
		const clap_plugin_params_t* m_extParams{nullptr};
	};

	void destroy();
	auto getHost() const -> const clap_host_t* { return m_host != nullptr ? m_host->getHost() : nullptr; }
	auto getPlugin() const -> const clap_plugin_t* { return m_plugin != nullptr ? m_plugin->getPlugin() : nullptr; }
	auto getHostInstance() const -> const Host* { return m_host.get(); }
	auto getPluginInstance() const -> const Plugin* { return m_plugin.get(); }
	auto getPluginInfo() const -> const ClapPluginInfo& { return m_pluginInfo; }
	auto isValid() const -> bool;

private:
	std::unique_ptr<Host> m_host; //!< Host instance
	std::unique_ptr<Plugin> m_plugin; //!< Plugin instance

	const ClapPluginInfo& m_pluginInfo;
};

}

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_INSTANCE_H
