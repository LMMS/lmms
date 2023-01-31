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

#include "ClapPluginInstance.h"

#include <memory>
#include <queue>
#include <functional>

namespace lmms
{

/**
 * @brief ClapInstance stores a CLAP host/plugin instance pair.
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
	ClapInstance() = delete;
	ClapInstance(const ClapPluginInfo* pluginInfo);
	ClapInstance(const ClapInstance&) = delete;
	ClapInstance& operator=(const ClapInstance&) = delete;
	ClapInstance(ClapInstance&& other) noexcept;
	ClapInstance& operator=(ClapInstance&& rhs) noexcept;
	~ClapInstance();

	//! Host instance
	class Host
	{
	public:
		//! Creates a clap_host host instance
		Host() = delete;
		Host(const ClapInstance* parent);
		Host(const Host&) = delete;
		Host& operator=(const Host&) = delete;
		Host(Host&& other) noexcept;
		Host& operator=(Host&& rhs) noexcept;
		~Host();

		void destroy();
		auto getHost() const -> const clap_host* { return &m_host; }
		auto getPlugin() const -> const clap_plugin*;
		auto isValid() const -> bool { return true; }

		//! Executes tasks in idle queue
		void idle();

	private:

		void setHost();
		void pushToIdleQueue(std::function<bool()>&& functor);

		static Host* fromHost(const clap_host* host);
		static auto getExtension(const clap_host* host, const char* extension_id) -> const void*;
		static void requestCallback(const clap_host* host);
		static void requestProcess(const clap_host* host);
		static void requestRestart(const clap_host* host);

		const ClapInstance* m_parent;
		clap_host m_host;

		std::queue<std::function<bool()>> m_idleQueue;
	};

	//! Destroy the plugin instance
	void destroy();

	//! Loads/reloads the plugin instance
	void load();

	auto getHost() const -> const clap_host* { return m_host.getHost(); }
	auto getPlugin() const -> const clap_plugin* { return m_plugin != nullptr ? m_plugin->getPlugin() : nullptr; }
	auto getHostInstance() const -> const Host& { return m_host; }
	auto getPluginInstance() const -> const ClapPluginInstance* { return m_plugin.get(); }
	auto getPluginInfo() const -> const ClapPluginInfo& { return *m_pluginInfo; }
	auto isValid() const -> bool;

private:
	Host m_host; //!< Host instance
	std::unique_ptr<ClapPluginInstance> m_plugin; //!< Plugin instance

	const ClapPluginInfo* m_pluginInfo; //!< Never null
};

}

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_INSTANCE_H
