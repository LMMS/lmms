/*
 * ClapManager.h - Implementation of ClapManager class
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

#ifndef LMMS_CLAP_MANAGER_H
#define LMMS_CLAP_MANAGER_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include "ClapFile.h"
#include "ClapInstance.h"

#include <filesystem>
#include <vector>

#include <QString>
#include <clap/clap.h>

namespace lmms
{

//! Manages CLAP plugins and implements CLAP host
class ClapManager
{
public:
	ClapManager();
	~ClapManager();

	// For iterating over m_uriToPluginInfo
	auto begin() const { return m_uriToPluginInfo.cbegin(); }
	auto end() const { return m_uriToPluginInfo.cend(); }

	auto getPluginInfo() const -> const std::vector<const ClapPluginInfo*>& { return m_pluginInfo; }
	auto getInstances() const -> const std::vector<ClapInstance>& { return m_instances; }

	//! Return descriptor with URI @p uri or nullptr if none exists
	auto getPluginInfo(const std::string& uri) -> const ClapPluginInfo*;
	//! Return descriptor with URI @p uri or nullptr if none exists
	auto getPluginInfo(const QString& uri) -> const ClapPluginInfo*;

	void initPlugins(); //!< Called by Engine at LMMS startup

	void unload(ClapFile* file);

	static bool kDebug; //!< If set, debug output will be printed

private:

	std::vector<std::filesystem::path> m_searchPaths; //!< Owns all CLAP search paths; Populated by findSearchPaths()
	std::vector<ClapFile> m_clapFiles; //!< Owns all loaded .clap files; Populated by findClapFiles()
	std::vector<ClapInstance> m_instances; //!< Owns all fully initialized CLAP host/plugin instance pairs

	// Non-owning plugin caches (for fast iteration/lookup)
	std::vector<const ClapPluginInfo*> m_pluginInfo; //!< Non-owning vector of info for all successfully loaded CLAP plugins
	std::unordered_map<std::string, const ClapPluginInfo*> m_uriToPluginInfo; //!< Non-owning map of plugin URIs (IDs) to ClapPluginInfo
	//std::vector<const clap_plugin_t*> m_plugins; //!< Non-owning vector of all CLAP plugin instances

	//! Finds all CLAP search paths and populates m_searchPaths
	void findSearchPaths();

	//! Returns search paths found by prior call to findSearchPaths()
	auto getSearchPaths() const -> const std::vector<std::filesystem::path>& { return m_searchPaths; }

	//! Finds and loads all .clap files in the provided search paths @p searchPaths
	void findClapFiles(const std::vector<std::filesystem::path>& searchPaths);

	//! Returns ClapFile vector found by prior call to findClapFiles()
	auto getFiles() const -> const std::vector<ClapFile>& { return m_clapFiles; }
};


} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_MANAGER_H
