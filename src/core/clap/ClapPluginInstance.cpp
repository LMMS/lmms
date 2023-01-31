/*
 * ClapPluginInstance.cpp - Implementation of ClapPluginInstance class
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

#include "ClapPluginInstance.h"

#ifdef LMMS_HAVE_CLAP

#include "ClapManager.h"
#include "ClapPorts.h"
#include "Engine.h"
#include "AudioEngine.h"

#include <QDebug>

#include "lmmsversion.h"

namespace lmms
{

ClapPluginInstance::ClapPluginInstance(const ClapInstance* parent, const ClapPluginInfo* info)
	: m_parent(parent), m_info(info)
{
	const auto factory = getInfo().getFactory();
	m_plugin = factory->create_plugin(factory, m_parent->getHost(), m_info->getDescriptor()->id);
}

ClapPluginInstance::~ClapPluginInstance()
{
	if (ClapManager::kDebug)
		qDebug() << "ClapPluginInstance::~ClapPluginInstance()";
	if (m_plugin)
	{
		deactivate();
		m_plugin->destroy(m_plugin);
		m_plugin = nullptr;
	}
}

auto ClapPluginInstance::init() -> bool
{
	if (isInitialized())
		return false;

	if (!m_plugin->init(m_plugin))
	{
		qWarning() << "Could not init the plugin with id:" << getInfo().getDescriptor()->id;
		m_plugin->destroy(m_plugin);
		return false;
	}

	// Need to init extensions before activating the plugin
	initExtensions();

	//auto ports = ClapPorts{m_plugin, m_extAudioPorts};
	return true;
}

auto ClapPluginInstance::activate() -> bool
{
	// Must be initialized and deactivated
	if (!isValid() || !isInitialized() || isActive())
		return false;

	const auto sampleRate = Engine::audioEngine()->processingSampleRate();

	static_assert(DEFAULT_BUFFER_SIZE > MINIMUM_BUFFER_SIZE);
	bool success = m_plugin->activate(m_plugin, sampleRate, MINIMUM_BUFFER_SIZE, DEFAULT_BUFFER_SIZE);

	qDebug() << "success:" << success;

	m_active = true;

	return false; // TODO: Implement this
}

auto ClapPluginInstance::deactivate() -> bool
{
	if (m_plugin && m_active)
	{
		m_plugin->deactivate(m_plugin);
		m_active = false;
		return true;
	}
	return false;
}

auto ClapPluginInstance::getHost() const -> const clap_host*
{
	return m_parent->getHost();
}

auto ClapPluginInstance::initExtensions() -> bool
{
	if (!m_plugin)
		return false;

	//initExtension(m_extAudioPorts, CLAP_EXT_AUDIO_PORTS);
	// ...

	return true;
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
