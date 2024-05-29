/*
 * ClapManager.h - Implementation of ClapManager class
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

#ifndef LMMS_CLAP_MANAGER_H
#define LMMS_CLAP_MANAGER_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include <QString>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ClapFile.h"
#include "NoCopyNoMove.h"
#include "lmms_export.h"

namespace lmms
{

//! Manages loaded .clap files, plugin info, and plugin instances
class LMMS_EXPORT ClapManager : public NoCopyNoMove
{
public:
	ClapManager();
	~ClapManager();

	//! Allows access to loaded .clap files
	auto files() const -> auto& { return m_files; }

	//! Returns a cached plugin info vector
	auto pluginInfo() const -> auto& { return m_pluginInfo; }

	//! Returns a URI-to-PluginInfo map
	auto uriInfoMap() const -> auto& { return m_uriInfoMap; }

	//! Return plugin info for plugin with the given uri or nullptr if none exists
	auto pluginInfo(const std::string& uri) const -> const ClapPluginInfo*;

	//! Return preset database for plugin with the given uri or nullptr if none exists
	auto presetDatabase(const std::string& uri) -> ClapPresetDatabase*;

	//! Called by Engine at LMMS startup
	void initPlugins();

	static auto debugging() { return s_debugging; }

private:
	//! For hashing since std::hash<std::filesystem::path> is not available until C++23's LWG issue 3657 for god knows why
	struct PathHash
	{
		auto operator()(const std::filesystem::path& path) const noexcept -> std::size_t
		{
			return std::filesystem::hash_value(path);
		}
	};

	using UniquePaths = std::unordered_set<std::filesystem::path, PathHash>;

	//! Finds all CLAP search paths and populates m_searchPaths
	void findSearchPaths();

	//! Returns search paths found by prior call to findSearchPaths()
	auto searchPaths() const -> auto& { return m_searchPaths; }

	//! Finds and loads all .clap files in the provided search paths @p searchPaths
	void loadClapFiles(const UniquePaths& searchPaths);

	UniquePaths m_searchPaths; //!< Owns all CLAP search paths; Populated by findSearchPaths()
	std::vector<ClapFile> m_files; //!< Owns all loaded .clap files; Populated by loadClapFiles()

	// Non-owning plugin caches (for fast iteration/lookup)

	std::vector<const ClapPluginInfo*> m_pluginInfo; //!< successfully loaded plugins
	std::unordered_map<std::string, ClapPluginInfo> m_uriInfoMap; //!< maps plugin URIs (IDs) to ClapPluginInfo
	std::unordered_map<std::string, std::size_t> m_uriFileIndexMap; //!< maps plugin URIs (IDs) to ClapFile index in `m_files`

	static inline bool s_debugging = false; //!< If LMMS_CLAP_DEBUG is set, debug output will be printed
};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_MANAGER_H
