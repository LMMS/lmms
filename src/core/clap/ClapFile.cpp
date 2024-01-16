/*
 * ClapFile.cpp - Implementation of ClapFile class
 *
 * Copyright (c) 2024 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include "ClapLog.h"
#include "ClapManager.h"

namespace lmms
{

ClapFile::ClapFile(fs::path filename)
	: m_filename{std::move(filename.make_preferred())}
{
}

ClapFile::~ClapFile()
{
	unload();
}

auto ClapFile::load() -> bool
{
	// Do not allow reloading yet
	if (m_library && m_library->isLoaded()) { return false; }

	// TODO: Replace QLibrary with in-house non-Qt alternative
	const auto file = filename().u8string();
	m_library = std::make_unique<QLibrary>(QString::fromUtf8(file.c_str(), file.size()));
	if (!m_library->load())
	{
		ClapLog::globalLog(CLAP_LOG_ERROR, m_library->errorString().toStdString());
		return false;
	}

	m_entry.reset(reinterpret_cast<const clap_plugin_entry*>(m_library->resolve("clap_entry")));
	if (!m_entry)
	{
		std::string msg = "Unable to resolve entry point in '" + filename().string() + "'";
		ClapLog::globalLog(CLAP_LOG_ERROR, msg);
		m_library->unload();
		return false;
	}

	if (!m_entry->init(file.c_str()))
	{
		std::string msg = "Failed to initialize '" + filename().string() + "'";
		ClapLog::globalLog(CLAP_LOG_ERROR, msg);
		m_entry = nullptr; // Prevent deinit() from being called
		return false;
	}

	m_factory = static_cast<const clap_plugin_factory*>(m_entry->get_factory(CLAP_PLUGIN_FACTORY_ID));
	if (!m_factory)
	{
		std::string msg = "Failed to retrieve plugin factory from '" + filename().string() + "'";
		ClapLog::globalLog(CLAP_LOG_ERROR, msg);
		return false;
	}

	m_pluginCount = m_factory->get_plugin_count(m_factory);
	if (m_pluginCount == 0)
	{
		std::string msg = "Plugin file '" + filename().string() + "' contains no plugins";
		ClapLog::globalLog(CLAP_LOG_DEBUG, msg);
		return false;
	}

	m_pluginInfo.clear();
	for (std::uint32_t idx = 0; idx < m_pluginCount; ++idx)
	{
		if (auto plugin = ClapPluginInfo::create(*m_factory, idx))
		{
			m_pluginInfo.emplace_back(std::move(plugin));
		}
	}

	return true;
}

void ClapFile::unload() noexcept
{
	// NOTE: Need to destroy any plugin instances from this .clap file before
	// calling this method. This should be okay as long as the ClapManager
	// singleton is destroyed after any ClapControlBase objects.
	// TODO: Enforce this?

	m_entry.reset();

	if (m_library)
	{
		m_library->unload();
		m_library = nullptr;
	}
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
