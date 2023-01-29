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

	auto getHost() const -> const clap_host_t* { return &m_host; }

	// CLAP API implementation: plugin -> host
	static const void* hostGetExtension(const clap_host_t* host, const char* extension);
	static void hostRequestCallback(const clap_host_t* host);
	static void hostRequestProcess(const clap_host_t* host);
	static void hostRequestRestart(const clap_host_t* host);

	// For iterating over m_uriToPlugin
	auto begin() const { return m_uriToPlugin.cbegin(); }
	auto end() const { return m_uriToPlugin.cend(); }

	//! Return descriptor with URI @p uri or nullptr if none exists
	const ClapPlugin* getPlugin(const std::string& uri);
	//! Return descriptor with URI @p uri or nullptr if none exists
	const ClapPlugin* getPlugin(const QString& uri);

	void initPlugins(); //!< Called by Engine at LMMS startup

	static bool kDebug; //!< If set, debug output will be printed

private:

	clap_host_t m_host; //!< CLAP host info and functions for plugins to use
	std::vector<std::filesystem::path> m_searchPaths; //!< Owns all CLAP search paths; Populated by findSearchPaths()
	std::vector<ClapFile> m_clapFiles; //!< Owns all loaded .clap files; Populated by findClapFiles()

	// Non-owning plugin caches (for fast iteration/lookup)
	std::vector<const clap_plugin_t*> m_plugins; //!< Non-owning vector of all loaded CLAP plugins
	//std::vector<const clap_plugin_t*> m_instrumentPlugins; //!< Non-owning vector of all loaded CLAP instrument plugins
	//std::vector<const clap_plugin_t*> m_effectPlugins; //!< Non-owning vector of all loaded CLAP effect plugins
	std::unordered_map<std::string, const ClapPlugin*> m_uriToPlugin; //!< Non-owning map of plugin URIs (IDs) to ClapPlugin

	//! Finds all CLAP search paths and populates m_searchPaths
	void findSearchPaths();

	//! Returns search paths found by prior call to findSearchPaths()
	auto getSearchPaths() const -> const std::vector<std::filesystem::path>& { return m_searchPaths; }

	//! Finds and loads all .clap files in the provided search paths @p searchPaths
	void findClapFiles(const std::vector<std::filesystem::path>& searchPaths);

	//! Returns ClapFile vector found by prior call to findClapFiles()
	auto getClapFiles() const -> const std::vector<ClapFile>& { return m_clapFiles; }
};


} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_MANAGER_H
