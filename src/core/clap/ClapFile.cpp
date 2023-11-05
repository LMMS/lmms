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

#ifdef LMMS_HAVE_CLAP

#include "ClapManager.h"

#include <QDebug>
#include <clap/clap.h>

namespace lmms
{

////////////////////////////////
// ClapFile
////////////////////////////////

ClapFile::ClapFile(std::filesystem::path filename)
	: m_filename{std::move(filename)}
{
	m_filename.make_preferred();
}

ClapFile::ClapFile(ClapFile&& other) noexcept
	: m_filename{std::move(other.m_filename)}
	, m_library{std::exchange(other.m_library, nullptr)}
	, m_entry{std::exchange(other.m_entry, nullptr)}
	, m_factory{std::exchange(other.m_factory, nullptr)}
	, m_pluginInfo{std::move(other.m_pluginInfo)}
	, m_pluginCount{other.m_pluginCount}
	, m_valid{std::exchange(other.m_valid, false)}
{
}

auto ClapFile::operator=(ClapFile&& rhs) noexcept -> ClapFile&
{
	if (this != &rhs)
	{
		m_filename = std::move(rhs.m_filename);
		m_library = std::exchange(rhs.m_library, nullptr);
		m_entry = std::exchange(rhs.m_entry, nullptr);
		m_factory = std::exchange(rhs.m_factory, nullptr);
		m_pluginInfo = std::move(rhs.m_pluginInfo);
		m_pluginCount = rhs.m_pluginCount;
		m_valid = std::exchange(rhs.m_valid, false);
	}
	return *this;
}

ClapFile::~ClapFile()
{
	unload();
}

auto ClapFile::load() -> bool
{
	// Do not allow reloading yet
	if (m_library && m_library->isLoaded()) { return false; }

	m_valid = false;

	// TODO: Replace QLibrary with in-house non-Qt alternative
	const auto filenameStr = filename().u8string();
	m_library = std::make_unique<QLibrary>(QString::fromUtf8(filenameStr.c_str(), filenameStr.size()));
	if (!m_library->load())
	{
		qWarning() << m_library->errorString();
		return false;
	}

	m_entry = reinterpret_cast<const clap_plugin_entry*>(m_library->resolve("clap_entry"));
	if (!m_entry)
	{
		qWarning().nospace() << "Unable to resolve entry point 'clap_entry' in CLAP file '" << filename().c_str() << "'";
		m_library->unload();
		return false;
	}

	if (!m_entry->init(filenameStr.c_str()))
	{
		qWarning().nospace() << "CLAP file '" << filename().c_str() << "' failed to initialize";
		m_entry = nullptr; // Prevent deinit() from being called
		return false;
	}

	m_factory = static_cast<const clap_plugin_factory*>(m_entry->get_factory(CLAP_PLUGIN_FACTORY_ID));
	if (!m_factory)
	{
		qWarning().nospace() << "Failed to get the plugin factory in CLAP file '" << filename().c_str() << "'";
		return false;
	}

	m_pluginCount = m_factory->get_plugin_count(m_factory);
	if (ClapManager::debugging()) { qDebug() << "plugin count:" << m_pluginCount; }
	if (m_pluginCount <= 0)
	{
		qWarning().nospace() << "CLAP file '" << filename().c_str() << "' contains no plugins";
		return false;
	}

	m_pluginInfo.clear();
	for (std::uint32_t i = 0; i < m_pluginCount; ++i)
	{
		auto& plugin = m_pluginInfo.emplace_back(std::make_shared<ClapPluginInfo>(m_factory, i));
		if (!plugin || !plugin->isValid())
		{
			m_pluginInfo.pop_back();
			continue;
		}
	}

	m_valid = true;
	return true;
}

void ClapFile::unload()
{
	// NOTE: Need to destroy any plugin instances from this .clap file before
	// calling this method. This should be okay as long as the ClapManager
	// singleton is destroyed after any ClapControlBase objects.

	if (m_entry)
	{
		// No more calls into the shared library can be made after this
		m_entry->deinit();
		m_entry = nullptr;
	}

	if (m_library)
	{
		m_library->unload();
		m_library = nullptr;
	}

	m_valid = false;
}

void ClapFile::purgeInvalidPlugins()
{
	m_pluginInfo.erase(std::remove_if(
		m_pluginInfo.begin(), m_pluginInfo.end(), [](const auto& pi) {
			return !pi || !pi->isValid();
		})
	);
}

////////////////////////////////
// ClapPluginInfo
////////////////////////////////

ClapPluginInfo::ClapPluginInfo(const clap_plugin_factory* factory, std::uint32_t index)
	: m_factory{factory}
	, m_index{index}
{
	assert(m_factory != nullptr);
	if (ClapManager::debugging()) { qDebug() << ""; }

	m_descriptor = m_factory->get_plugin_descriptor(m_factory, m_index);
	if (!m_descriptor)
	{
		qWarning() << "No CLAP plugin descriptor";
		return;
	}

	if (!m_descriptor->id || !m_descriptor->name)
	{
		qWarning() << "Invalid CLAP plugin descriptor";
		return;
	}

	if (!clap_version_is_compatible(m_descriptor->clap_version))
	{
		qWarning() << "Incompatible CLAP version: Plugin is: " << m_descriptor->clap_version.major << "."
			<< m_descriptor->clap_version.minor << "." << m_descriptor->clap_version.revision << " Host is "
			<< CLAP_VERSION.major << "." << CLAP_VERSION.minor << "." << CLAP_VERSION.revision;
		return;
	}

	if (ClapManager::debugging())
	{
		qDebug() << "name:" << m_descriptor->name;
		qDebug().nospace() << "CLAP version: "
			<< m_descriptor->clap_version.major << "."
			<< m_descriptor->clap_version.minor << "."
			<< m_descriptor->clap_version.revision;
		qDebug() << "description:" << m_descriptor->description;
	}

	m_type = Plugin::Type::Undefined;
	auto features = m_descriptor->features;
	while (features && *features)
	{
		auto feature = std::string_view{*features};
		if (ClapManager::debugging()) { qDebug() << "feature:" << feature.data(); }

		if (feature == CLAP_PLUGIN_FEATURE_INSTRUMENT)
		{
			m_type = Plugin::Type::Instrument;
		}
		else if (feature == CLAP_PLUGIN_FEATURE_AUDIO_EFFECT
			|| feature == "effect" /* non-standard, but used by Surge XT Effects */)
		{
			m_type = Plugin::Type::Effect;
		}
		/*else if (feature == CLAP_PLUGIN_FEATURE_ANALYZER)
		{
			m_type = Plugin::Type::Tool;
		}*/
		++features;
	}

	if (m_type == Plugin::Type::Undefined)
	{
		qWarning() << "CLAP plugin is not recognized as an instrument or audio effect";
		return;
	}

	// So far the plugin is valid, but it may be invalidated later TODO: ???
	m_valid = true;
}

ClapPluginInfo::ClapPluginInfo(ClapPluginInfo&& other) noexcept
	: m_factory{std::exchange(other.m_factory, nullptr)}
	, m_index{other.m_index}
	, m_descriptor{std::exchange(other.m_descriptor, nullptr)}
	, m_type{std::exchange(other.m_type, Plugin::Type::Undefined)}
	, m_valid{std::exchange(other.m_valid, false)}
	, m_issues{std::move(other.m_issues)}
{
}


} // namespace lmms

#endif // LMMS_HAVE_CLAP
