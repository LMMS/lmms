/*
 * ClapInstance.cpp - Implementation of ClapInstance class
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

#include "ClapInstance.h"

#ifdef LMMS_HAVE_CLAP

#include "ClapManager.h"

#include <QDebug>

#include "lmmsversion.h"

namespace lmms
{


////////////////////////////////
// ClapInstance
////////////////////////////////

ClapInstance::ClapInstance(const ClapPluginInfo* pluginInfo)
	: m_pluginInfo(pluginInfo)
{
	setHost();
}

ClapInstance::ClapInstance(ClapInstance&& other) noexcept
	: m_pluginInfo(std::move(other.m_pluginInfo))
{
	m_idleQueue = std::move(other.m_idleQueue);
	m_plugin = std::exchange(other.m_plugin, nullptr);

	// Update the host's host_data pointer
	setHost();
}

ClapInstance::~ClapInstance()
{
	destroy();
}

void ClapInstance::destroy()
{
	hostIdle(); // ???

	// Deactivates and destroys clap_plugin* as needed
	m_plugin.reset();
	hostDestroy();
}

void ClapInstance::load()
{
	/*
	if (!isHostValid())
	{
		hostDestroy();
		return;
	}
	*/

	// Create plugin instance, destroying any previous plugin instance first
	m_plugin = std::make_shared<ClapPluginInstance>(this, m_pluginInfo);
	if (!m_plugin || !m_plugin->isValid())
	{
		qWarning() << "Failed to create instance of CLAP plugin";
		m_plugin.reset();
		hostDestroy();
		return;
	}

	// For testing:
	//m_plugin->activate();

	// TODO
}

auto ClapInstance::isValid() const -> bool
{
	if (!m_plugin)
		return false;

	return m_plugin->isValid() /*&& isHostValid()*/;
}

////////////////////////////////
// ClapInstance host
////////////////////////////////


void ClapInstance::hostDestroy()
{
	// Clear queue just in case
	while (!m_idleQueue.empty())
	{
		m_idleQueue.pop();
	}
}

void ClapInstance::hostIdle()
{
	// NOTE: Must run on main thread
	while (!m_idleQueue.empty())
	{
		// Execute task then pop
		m_idleQueue.front()();
		m_idleQueue.pop();
	}
}

void ClapInstance::setHost()
{
	m_host.host_data = this;
	m_host.clap_version = CLAP_VERSION;
	m_host.name = "LMMS";
	m_host.version = LMMS_VERSION;
	m_host.vendor = nullptr;
	m_host.url = "https://lmms.io/";
	m_host.get_extension = hostGetExtension;
	m_host.request_callback = hostRequestCallback;
	m_host.request_process = hostRequestProcess;
	m_host.request_restart = hostRequestRestart;
}

void ClapInstance::hostPushToIdleQueue(std::function<bool()>&& functor)
{
	m_idleQueue.push(std::move(functor));
}

auto ClapInstance::fromHost(const clap_host* host) -> ClapInstance*
{
	if (!host)
		throw std::invalid_argument("Passed a null host pointer");

	auto h = static_cast<ClapInstance*>(host->host_data);
	if (!h)
		throw std::invalid_argument("Passed an invalid host pointer because the host_data is null");

	if (!h->getPlugin())
		throw std::logic_error("The plugin can't query for extensions during the create method. Wait "
								"for clap_plugin.init() call.");

	return h;
}

auto ClapInstance::hostGetExtension(const clap_host* host, const char* extension_id) -> const void*
{
	[[maybe_unused]] auto h = fromHost(host);
	// TODO
	return nullptr;
}

void ClapInstance::hostRequestCallback(const clap_host* host)
{
	const auto h = fromHost(host);

	auto mainThreadCallback = [h]() -> bool {
		const auto plugin = h->getPlugin();
		plugin->on_main_thread(plugin);
		return false;
	};

	h->hostPushToIdleQueue(std::move(mainThreadCallback));
}

void ClapInstance::hostRequestProcess(const clap_host* host)
{
	[[maybe_unused]] auto h = fromHost(host);
	// TODO
	//h->_scheduleProcess = true;
	//qDebug() << "hostRequestProcess called";
}

void ClapInstance::hostRequestRestart(const clap_host* host)
{
	[[maybe_unused]] auto h = fromHost(host);
	// TODO
	//h->_scheduleRestart = true;
	//qDebug() << "hostRequestRestart called";
}


} // namespace lmms

#endif // LMMS_HAVE_CLAP
