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
 * a new clap_host instance needs to be passed to the plugin instance,
 * creating a CLAP host instance / CLAP plugin instance pair.
 * The plugin instance will pass the host pointer whenever it calls the
 * host's API (instead of passing the plugin pointer), and that is how
 * the host instance can know which plugin instance called the host API.
 * 
 * The ClapInstance class implements the CLAP host API and
 *     stores the pointer to the clap_host object.
 * The ClapInstance::Plugin class provides access to the plugin instance
 *     for making plugin API calls. TODO: Merge into ClapInstance?
 *
 * Every ClapInstance is owned by the ClapManager, though other classes
 * may borrow from the ClapManager.
 */
class ClapInstance
{
public:
	ClapInstance() = delete;
	ClapInstance(const ClapPluginInfo* pluginInfo);
	ClapInstance(const ClapInstance&) = delete;
	ClapInstance& operator=(const ClapInstance&) = delete;
	ClapInstance(ClapInstance&& other) noexcept;
	ClapInstance& operator=(ClapInstance&& rhs) noexcept = delete;
	~ClapInstance();

	void hostDestroy();

	//! Executes tasks in idle queue
	void hostIdle();

	//! Destroy the plugin instance
	void destroy();

	//! Loads/reloads the plugin instance
	void load();

	auto getHost() const -> const clap_host* { return &m_host; }
	auto getPlugin() const -> const clap_plugin* { return m_plugin != nullptr ? m_plugin->getPlugin() : nullptr; }
	auto getPluginInstance() const -> std::weak_ptr<ClapPluginInstance> { return m_plugin; }

	//! The value obtained may become invalid later, so do not store it
	auto getPluginInfo() const -> const ClapPluginInfo& { return *m_pluginInfo; }
	auto isValid() const -> bool;

private:

	/**
	 * Host-related methods
	 */
	void setHost();
	void hostPushToIdleQueue(std::function<bool()>&& functor);
	static auto fromHost(const clap_host* host) -> ClapInstance*;
	static auto hostGetExtension(const clap_host* host, const char* extension_id) -> const void*;
	static void hostRequestCallback(const clap_host* host);
	static void hostRequestProcess(const clap_host* host);
	static void hostRequestRestart(const clap_host* host);

	/**
	 * Host-related member variables
	 */
	clap_host m_host;
	std::queue<std::function<bool()>> m_idleQueue;

	/**
	 * Misc.
	 */
	std::shared_ptr<ClapPluginInstance> m_plugin; //!< Plugin instance
	const ClapPluginInfo* m_pluginInfo; // TODO: Use weak_ptr instead?
	//std::weak_ptr<const ClapPluginInfo> m_pluginInfo;
};

}

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_INSTANCE_H
