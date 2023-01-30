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

ClapInstance::ClapInstance(const ClapPluginInfo& pluginInfo)
	: m_pluginInfo(pluginInfo)
{
	m_host = std::make_unique<Host>(*this);
	if (!m_host || !m_host->isValid())
	{
		m_host.reset();
		return;
	}

	m_plugin = std::make_unique<Plugin>(*this, m_pluginInfo);
	if (!m_plugin || !m_plugin->isValid())
	{
		m_plugin.reset();
		m_host.reset();
		return;
	}

	// TODO
}

ClapInstance::~ClapInstance()
{
	if (ClapManager::kDebug)
		qDebug() << "ClapInstance::~ClapInstance()";
	destroy();
}

void ClapInstance::destroy()
{
	m_plugin.reset(); // Deactivates and destroys clap_plugin_t* as needed
	m_host.reset();
}

auto ClapInstance::isValid() const -> bool
{
	if (!m_plugin || !m_host)
		return false;

	return m_plugin->isValid() && m_host->isValid();
}

////////////////////////////////
// ClapInstance::Host
////////////////////////////////

ClapInstance::Host::Host(const ClapInstance& parent)
	: m_parent(parent)
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

auto ClapInstance::Host::getExtension(const clap_host_t* host, const char* extension_id) -> const void*
{
	auto h = fromHost(host);
	(void)h;
	// TODO
	return nullptr;
}

void ClapInstance::Host::requestCallback(const clap_host* host)
{
	auto h = fromHost(host);
	(void)h;
	//h->_scheduleMainThreadCallback = true;
	//qDebug() << "hostRequestCallback called";
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

////////////////////////////////
// ClapInstance::Plugin
////////////////////////////////

ClapInstance::Plugin::Plugin(const ClapInstance& parent, const ClapPluginInfo& info)
	: m_parent(parent), m_info(info)
{
	const auto factory = m_info.getFile()->getFactory();
	m_plugin = factory->create_plugin(factory, m_parent.getHost(), m_info.getDescriptor()->id);
}

ClapInstance::Plugin::~Plugin()
{
	if (ClapManager::kDebug)
		qDebug() << "ClapInstance::Plugin::~Plugin()";
	if (m_plugin)
	{
		if (m_active)
			m_plugin->deactivate(m_plugin);

		m_plugin->destroy(m_plugin);
		m_plugin = nullptr;
	}
}

auto ClapInstance::Plugin::initExtensions() -> bool
{
	if (!m_plugin)
		return false;

	initExtension(m_extAudioPorts, CLAP_EXT_AUDIO_PORTS);
	// ...

	return true;
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
