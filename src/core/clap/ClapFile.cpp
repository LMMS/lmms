/*
 * ClapFile.cpp - Implementation of ClapFile class
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

#include "ClapFile.h"

#include "ClapManager.h"
#include "ClapPorts.h"

#include <QDebug>
#include <clap/clap.h>

namespace lmms
{

////////////////////////////////
// ClapFile
////////////////////////////////

ClapFile::ClapFile(const ClapManager* manager, std::filesystem::path&& filename)
	: m_parent(manager), m_filename(std::move(filename))
{
	m_filename.make_preferred();
	m_library = std::make_unique<QLibrary>();
	m_library->setFileName(QString::fromUtf8(m_filename.c_str()));
	m_library->setLoadHints(QLibrary::ResolveAllSymbolsHint | QLibrary::DeepBindHint);
	load();
}

ClapFile::ClapFile(ClapFile&& other) noexcept
	: m_parent(other.m_parent)
{
	m_filename = std::move(other.m_filename);
	m_library = std::exchange(other.m_library, nullptr);
	m_factory = std::exchange(other.m_factory, nullptr);
	m_pluginCount = other.m_pluginCount;
	m_valid = std::exchange(other.m_valid, false);
	m_plugins = std::move(other.m_plugins);
}

auto ClapFile::load() -> bool
{
	m_valid = false;
	if (!m_library)
		return false;

	// Do not allow reloading yet
	if (m_library->isLoaded())
		return false;

	if (!m_library->load())
	{
		qWarning() << m_library->errorString();
		return false;
	}

	const auto pluginEntry = reinterpret_cast<const clap_plugin_entry_t*>(m_library->resolve("clap_entry"));
	if (!pluginEntry)
	{
		qWarning() << "Unable to resolve entry point 'clap_entry' in '" << getFilename().c_str() << "'";
		m_library->unload();
		return false;
	}

	pluginEntry->init(getFilename().c_str());
	m_factory = static_cast<const clap_plugin_factory_t*>(pluginEntry->get_factory(CLAP_PLUGIN_FACTORY_ID));

	m_pluginCount = m_factory->get_plugin_count(m_factory);
	if (ClapManager::kDebug)
		qDebug() << "plugin count:" << m_pluginCount;
	if (m_pluginCount <= 0)
		return false;

	m_plugins.clear();
	for (uint32_t i = 0; i < m_pluginCount; ++i)
	{
		const auto desc = m_factory->get_plugin_descriptor(m_factory, i);
		if (!desc)
		{
			qWarning() << "no plugin descriptor";
			continue;
		}

		if (!desc->id || !desc->name)
		{
			qWarning() << "invalid plugin descriptor";
			continue;
		}

		if (!clap_version_is_compatible(desc->clap_version))
		{
			qWarning() << "Incompatible CLAP version: Plugin is: " << desc->clap_version.major << "."
						<< desc->clap_version.minor << "." << desc->clap_version.revision << " Host is "
						<< CLAP_VERSION.major << "." << CLAP_VERSION.minor << "." << CLAP_VERSION.revision;
			continue;
		}

		if (ClapManager::kDebug)
		{
			qDebug() << "name:" << desc->name;
			qDebug() << "description:" << desc->description;
		}

		auto plugin = ClapPlugin{this, i, desc}; // Prints warning if anything goes wrong
		if (!plugin.isValid())
			continue;

		if (ClapManager::kDebug)
			plugin.activate(); // TEMPORARY! (debug purposes currently)

		m_plugins.emplace_back(std::move(plugin));
	}

	m_valid = true;
	return true;
}

void ClapFile::unload()
{
	// TODO: Need to implement
	return;
}

////////////////////////////////
// ClapPlugin
////////////////////////////////

ClapPlugin::ClapPlugin(const ClapFile* parent, uint32_t index, const clap_plugin_descriptor_t* desc)
	: m_parent(parent), m_host(parent->getParent()->getHost())
{
	m_valid = false;
	m_index = index;
	m_descriptor = desc;

	m_type = Plugin::PluginTypes::Undefined;
	auto features = desc->features;
	while (features && *features)
	{
		std::string_view feature = *features;
		if (ClapManager::kDebug)
			qDebug() << "feature:" << feature.data();
		if (feature == CLAP_PLUGIN_FEATURE_INSTRUMENT)
			m_type = Plugin::PluginTypes::Instrument;
		else if (feature == CLAP_PLUGIN_FEATURE_AUDIO_EFFECT
				|| feature == CLAP_PLUGIN_FEATURE_NOTE_EFFECT
				|| feature == "effect" /* non-standard, but used by Surge XT Effects */)
			m_type = Plugin::PluginTypes::Effect;
		else if (feature == CLAP_PLUGIN_FEATURE_ANALYZER)
			m_type = Plugin::PluginTypes::Tool;
		++features;
	}

	if (m_type != Plugin::PluginTypes::Undefined)
		m_valid = true;
	else
	{
		qWarning() << "CLAP plugin is not recognized as an instrument, effect, or tool";
	}
}

ClapPlugin::ClapPlugin(ClapPlugin&& other) noexcept
	: m_parent(other.m_parent)
{
	m_host = std::exchange(other.m_host, nullptr);
	m_index = other.m_index;
	m_descriptor = std::exchange(other.m_descriptor, nullptr);
	m_type = std::exchange(other.m_type, Plugin::PluginTypes::Undefined);
	m_valid = std::exchange(other.m_valid, false);
	m_issues = std::move(other.m_issues);
	m_plugin = std::exchange(other.m_plugin, nullptr);

	m_extParams = std::exchange(other.m_extParams, nullptr);
	//m_extQuickControls = std::exchange(m_extQuickControls, nullptr);
	m_extAudioPorts = std::exchange(other.m_extAudioPorts, nullptr);
	m_extGui = std::exchange(other.m_extGui, nullptr);
	m_extTimerSupport = std::exchange(other.m_extTimerSupport, nullptr);
	m_extPosixFdSupport = std::exchange(other.m_extPosixFdSupport, nullptr);
	m_extThreadPool = std::exchange(other.m_extThreadPool, nullptr);
	m_extPresetLoad = std::exchange(other.m_extPresetLoad, nullptr);
	m_extState = std::exchange(other.m_extState, nullptr);
}

auto ClapPlugin::activate() -> bool
{
	if (!isValid() || isActivated())
		return false;

	const auto factory = getParent()->getFactory();
	m_plugin = ClapPluginPtr{ factory->create_plugin(factory, m_host, getDescriptor()->id) };
	if (!m_plugin)
	{
		qWarning() << "could not create the plugin with id: " << getDescriptor()->id;
		return false;
	}

	if (!m_plugin->init(m_plugin.get()))
	{
		qWarning() << "could not init the plugin with id: " << getDescriptor()->id;
		m_plugin.reset(); // Calls destroy
		return false;
	}

	// Need to init extensions before activating the plugin
	initExtensions();

	auto ports = ClapPorts{m_plugin.get(), m_extAudioPorts};

	//m_plugin->activate(m_plugin.get(), );

	return true;
}

void ClapPlugin::deactivate()
{
	// TODO
}

auto ClapPlugin::check() -> bool
{
	m_issues.clear();

	// TODO: Check if blacklisted

	if (!m_extAudioPorts)
	{
		m_issues.emplace(PluginIssueType::noOutputChannel);
		return false; // No ports
	}



	//if (ClapPorts::check(m_extAudioPorts, m_issues))
	//	return false; // failure

	return true;
}

void ClapPlugin::initExtensions()
{
	initPluginExtension(m_extParams, CLAP_EXT_PARAMS);
	//initPluginExtension(m_extQuickControls, CLAP_EXT_QUICK_CONTROLS);
	initPluginExtension(m_extAudioPorts, CLAP_EXT_AUDIO_PORTS);
	initPluginExtension(m_extGui, CLAP_EXT_GUI);
	initPluginExtension(m_extTimerSupport, CLAP_EXT_TIMER_SUPPORT);
	initPluginExtension(m_extPosixFdSupport, CLAP_EXT_POSIX_FD_SUPPORT);
	initPluginExtension(m_extThreadPool, CLAP_EXT_THREAD_POOL);
	initPluginExtension(m_extPresetLoad, CLAP_EXT_PRESET_LOAD);
	initPluginExtension(m_extState, CLAP_EXT_STATE);
}

void ClapPlugin::ClapPluginDeleter::operator()(const clap_plugin_t* p) const noexcept
{
	// NOTE: Need to deactivate plugin first
	p->destroy(p);
}

} // namespace lmms
