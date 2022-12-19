/*
 * ClapManager.h - Implementation of ClapManager class
 *
 * Copyright (c) 2022 Dalton Messmer <messmer/dot/dalton/at/gmail/dot/com>
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

#ifndef CLAPMANAGER_H
#define CLAPMANAGER_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include <filesystem>
#include <map>
#include <set>
#include <clap/clap.h>

#include "Plugin.h"

#include <QLibrary>

namespace lmms
{

//! Class to keep track of all CLAP plugins
class ClapManager
{
public:
	void initPlugins();

	ClapManager();
	~ClapManager();

	//! Class representing info for one plugin
	class ClapInfo
	{
	public:
		//! use only for std::map internals
		ClapInfo() : m_library(nullptr) {}
		//! ctor used inside ClapManager
		ClapInfo(const std::shared_ptr<QLibrary>& lib, const clap_plugin_descriptor_t* desc, Plugin::PluginTypes type, bool valid)
			: m_library(lib), m_descriptor(desc), m_type(type), m_valid(valid) {}
		ClapInfo(ClapInfo&& other) = default;
		ClapInfo& operator=(ClapInfo&& other) = default;

		std::shared_ptr<QLibrary> library() const { return m_library; }
		const clap_plugin_descriptor_t* descriptor() const { return m_descriptor; }
		Plugin::PluginTypes type() const { return m_type; }
		bool isValid() const { return m_valid; }

	private:
		std::shared_ptr<QLibrary> m_library;
		const clap_plugin_descriptor_t* m_descriptor;
		Plugin::PluginTypes m_type;
		bool m_valid{false};
	};

	// CLAP API plugin -> host
	static const void* hostGetExtension(const clap_host* host, const char* extension);
	static void hostRequestCallback(const clap_host* host);
	static void hostRequestProcess(const clap_host* host);
	static void hostRequestRestart(const clap_host* host);

private:
	// data
	bool m_debug; //!< if set, debug output will be printed
	std::set<std::filesystem::path> m_pluginPaths;
	clap_host m_host;
	int m_pluginIndex;
	std::vector<ClapInfo> m_loadedPlugins;

	// functions

	//! Finds all ".clap" files in CLAP search paths
	void findPlugins();
};


} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // CLAPMANAGER_H
