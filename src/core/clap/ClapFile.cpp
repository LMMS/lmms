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
}

ClapFile::ClapFile(ClapFile&& other) noexcept
	: m_parent(other.m_parent)
{
	m_filename = std::move(other.m_filename);
	m_library = std::exchange(other.m_library, nullptr);
	m_entry = std::exchange(other.m_entry, nullptr);
	m_factory = std::exchange(other.m_factory, nullptr);
	m_pluginCount = other.m_pluginCount;
	m_valid = std::exchange(other.m_valid, false);
	m_pluginInfo = std::move(other.m_pluginInfo);
}

ClapFile::~ClapFile()
{
	//if (ClapManager::kDebug)
	//	qDebug() << "ClapFile::~ClapFile(): m_library=" << (m_library != nullptr ? "non-null" : "null");
	unload();
}

auto ClapFile::load() -> bool
{
	m_valid = false;

	m_library = std::make_unique<QLibrary>();
	if (!m_library)
		return false;

	m_library->setFileName(QString::fromUtf8(m_filename.c_str()));
	m_library->setLoadHints(QLibrary::ResolveAllSymbolsHint | QLibrary::DeepBindHint);

	// Do not allow reloading yet
	if (m_library->isLoaded())
		return false;

	if (!m_library->load())
	{
		qWarning() << m_library->errorString();
		return false;
	}

	const auto pluginEntry = reinterpret_cast<const clap_plugin_entry*>(m_library->resolve("clap_entry"));
	if (!pluginEntry)
	{
		qWarning() << "Unable to resolve entry point 'clap_entry' in '" << getFilename().c_str() << "'";
		m_library->unload();
		return false;
	}

	pluginEntry->init(getFilename().c_str());
	m_factory = static_cast<const clap_plugin_factory*>(pluginEntry->get_factory(CLAP_PLUGIN_FACTORY_ID));

	m_pluginCount = m_factory->get_plugin_count(m_factory);
	if (ClapManager::kDebug)
		qDebug() << "plugin count:" << m_pluginCount;
	if (m_pluginCount <= 0)
		return false;

	m_pluginInfo.clear();
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

		auto plugin = ClapPluginInfo{this, i, desc}; // Prints warning if anything goes wrong
		if (!plugin.isValid())
			continue;

		//if (ClapManager::kDebug)
		//	plugin.activate(); // TEMPORARY! (debug purposes currently)

		m_pluginInfo.emplace_back(std::move(plugin));
	}

	m_valid = true;
	return true;
}

void ClapFile::unload()
{
	// Need to destroy any plugin instances from this .clap file before
	// calling this method. See ClapManager::unload().

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
}

////////////////////////////////
// ClapPluginInfo
////////////////////////////////

ClapPluginInfo::ClapPluginInfo(const ClapFile* file, uint32_t index, const clap_plugin_descriptor* desc)
	: m_file(file)
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

ClapPluginInfo::ClapPluginInfo(ClapPluginInfo&& other) noexcept
	: m_file(other.m_file)
{
	m_index = other.m_index;
	m_descriptor = std::exchange(other.m_descriptor, nullptr);
	m_type = std::exchange(other.m_type, Plugin::PluginTypes::Undefined);
	m_valid = std::exchange(other.m_valid, false);
	m_issues = std::move(other.m_issues);
}

auto ClapPluginInfo::getFactory() const -> const clap_plugin_factory*
{
	return getFile()->getFactory();
}


} // namespace lmms

#endif // LMMS_HAVE_CLAP
