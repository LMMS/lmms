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
	: m_host(this), m_pluginInfo(pluginInfo)
{
}

ClapInstance::ClapInstance(ClapInstance&& other) noexcept
	: m_host(std::move(other.m_host)), m_pluginInfo(other.m_pluginInfo)
{
	m_plugin = std::exchange(other.m_plugin, nullptr);
}

ClapInstance& ClapInstance::operator=(ClapInstance&& rhs) noexcept
{
	if (this != &rhs)
	{
		m_host = std::move(rhs.m_host);
		m_plugin = std::exchange(rhs.m_plugin, nullptr);
		m_pluginInfo = rhs.m_pluginInfo;
	}
	return *this;
}

ClapInstance::~ClapInstance()
{
	//if (ClapManager::kDebug)
	//	qDebug() << "ClapInstance::~ClapInstance()";
	destroy();
}

void ClapInstance::destroy()
{
	m_host.idle(); // ???

	// Deactivates and destroys clap_plugin* as needed
	m_plugin.reset();
	m_host.destroy();
}

void ClapInstance::load()
{
	qDebug() << "ClapInstance::load()";
	if (!m_host.isValid())
	{
		m_host.destroy();
		return;
	}

	// Create plugin instance, destroying any previous plugin instance first
	m_plugin = std::make_unique<ClapPluginInstance>(this, m_pluginInfo);
	if (!m_plugin || !m_plugin->isValid())
	{
		qWarning() << "Failed to create instance of CLAP plugin";
		m_plugin.reset();
		m_host.destroy();
		return;
	}

	// For testing:
	m_plugin->activate();

	// TODO
}

auto ClapInstance::isValid() const -> bool
{
	if (!m_plugin)
		return false;

	return m_plugin->isValid() && m_host.isValid();
}

////////////////////////////////
// ClapInstance::Host
////////////////////////////////

ClapInstance::Host::Host(const ClapInstance* parent)
	: m_parent(parent)
{
	setHost();
}

ClapInstance::Host::Host(Host&& other) noexcept
	: m_parent(other.m_parent)
{
	setHost();
	m_idleQueue = std::move(other.m_idleQueue);
}

ClapInstance::Host& ClapInstance::Host::operator=(Host&& rhs) noexcept
{
	if (this != &rhs)
	{
		m_parent = rhs.m_parent;
		setHost();
		m_idleQueue = std::move(rhs.m_idleQueue);
	}
	return *this;
}

ClapInstance::Host::~Host()
{
	//if (ClapManager::kDebug)
	//	qDebug() << "ClapInstance::Host::~Host()";
	destroy();
}

void ClapInstance::Host::destroy()
{
	// Clear queue just in case
	while (!m_idleQueue.empty())
	{
		m_idleQueue.pop();
	}
}

auto ClapInstance::Host::getPlugin() const -> const clap_plugin*
{
	return m_parent->getPlugin();
}

void ClapInstance::Host::idle()
{
	// NOTE: Must run on main thread
	while (!m_idleQueue.empty())
	{
		// Execute task then pop
		m_idleQueue.front()();
		m_idleQueue.pop();
	}
}

void ClapInstance::Host::setHost()
{
	m_host.host_data = this;
	m_host.clap_version = CLAP_VERSION;
	m_host.name = "LMMS";
	m_host.version = LMMS_VERSION;
	m_host.vendor = nullptr;
	m_host.url = "https://lmms.io/";
	m_host.get_extension = getExtension;
	m_host.request_callback = requestCallback;
	m_host.request_process = requestProcess;
	m_host.request_restart = requestRestart;
}

void ClapInstance::Host::pushToIdleQueue(std::function<bool()>&& functor)
{
	m_idleQueue.push(std::move(functor));
}

auto ClapInstance::Host::fromHost(const clap_host* host) -> Host*
{
	if (!host)
		throw std::invalid_argument("Passed a null host pointer");

	auto h = static_cast<Host*>(host->host_data);
	if (!h)
		throw std::invalid_argument("Passed an invalid host pointer because the host_data is null");

	if (!h->getPlugin())
		throw std::logic_error("The plugin can't query for extensions during the create method. Wait "
								"for clap_plugin.init() call.");

	return h;
}

auto ClapInstance::Host::getExtension(const clap_host* host, const char* extension_id) -> const void*
{
	auto h = fromHost(host);
	(void)h;
	// TODO
	return nullptr;
}

void ClapInstance::Host::requestCallback(const clap_host* host)
{
	const auto h = fromHost(host);

	auto mainThreadCallback = [h]() -> bool {
		const auto plugin = h->getPlugin();
		plugin->on_main_thread(plugin);
		return false;
	};

	h->pushToIdleQueue(std::move(mainThreadCallback));
	qDebug() << "Host::requestCallback called";
}

void ClapInstance::Host::requestProcess(const clap_host* host)
{
	auto h = fromHost(host);
	(void)h;
	//h->_scheduleProcess = true;
	//qDebug() << "hostRequestProcess called";
}

void ClapInstance::Host::requestRestart(const clap_host* host)
{
	auto h = fromHost(host);
	(void)h;
	//h->_scheduleRestart = true;
	//qDebug() << "hostRequestRestart called";
}


} // namespace lmms

#endif // LMMS_HAVE_CLAP
