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

#ifndef CLAPMANAGER_H
#define CLAPMANAGER_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include <filesystem>
#include <vector>
#include <map>
#include <set>
#include <clap/clap.h>

#include "Plugin.h"

#include <QLibrary>

namespace lmms
{

//! Manages CLAP plugins and implements CLAP host
class ClapManager
{
public:
	ClapManager();
	~ClapManager();

	struct ClapPluginDeleter
	{
		void operator()(const clap_plugin_t* p) const noexcept { p->destroy(p); }
	};
	using ClapPluginPtr = std::unique_ptr<const clap_plugin_t, ClapPluginDeleter>;

	//! Class representing info for one .clap file, which contains 1 or more CLAP plugins
	class ClapFile
	{
	public:
		//! Loads .clap file and plugin info
		ClapFile(const ClapManager& manager, std::filesystem::path&& filename);
		~ClapFile() = default;

		ClapFile(const ClapFile&) = delete;
		ClapFile(ClapFile&& other) noexcept;
		ClapFile& operator=(const ClapFile&) = delete;
		ClapFile& operator=(ClapFile&&) noexcept = delete;

		//! Represents a CLAP plugin within a .clap file
		class ClapPlugin
		{
		public:
			//! Loads plugin info but does not activate
			ClapPlugin(const ClapFile& parent, uint32_t index, const clap_plugin_descriptor_t* desc);

			~ClapPlugin() = default;
			ClapPlugin(const ClapPlugin&) = delete;
			ClapPlugin(ClapPlugin&& other) noexcept;
			ClapPlugin& operator=(const ClapPlugin&) = delete;
			ClapPlugin& operator=(ClapPlugin&&) noexcept = delete;

			auto activate() -> bool;
			void deactivate();
			auto isValid() const -> bool { return m_valid; }
			auto isActivated() const -> bool { return m_plugin != nullptr; }

			auto getParent() const -> const ClapFile& { return m_parent; }
			auto getIndex() const -> uint32_t { return m_index; }
			auto getType() const -> Plugin::PluginTypes { return m_type; }
			auto getDescriptor() const -> const clap_plugin_descriptor_t* { return m_descriptor; }
			auto getPlugin() const -> const clap_plugin_t* { return m_plugin.get(); }

		private:
			// Are set when the .clap file is loaded:
			const ClapFile& m_parent;
			const clap_host_t* m_host{nullptr};
			uint32_t m_index{0}; //!< Plugin index within the .clap file
			const clap_plugin_descriptor_t* m_descriptor{nullptr};
			Plugin::PluginTypes m_type{Plugin::PluginTypes::Undefined};
			bool m_valid{false};

			//! Is set after plugin activization
			ClapPluginPtr m_plugin{nullptr};
		};

		auto load() -> bool;
		void unload();
		auto getParent() const -> const ClapManager& { return m_parent; }
		auto getFilename() const -> const std::filesystem::path& { return m_filename; }
		auto getFactory() const -> const clap_plugin_factory_t* { return m_factory; }

		//! Only includes plugins that successfully loaded
		auto getPlugins() const -> const std::vector<ClapPlugin>& { return m_plugins; }

		//! Includes plugins that failed to load
		auto getPluginCount() const -> uint32_t { return m_pluginCount; }
		auto isValid() const -> bool { return m_valid; }

	private:
		// Are set when the .clap file is loaded:
		const ClapManager& m_parent;
		std::filesystem::path m_filename;
		std::shared_ptr<QLibrary> m_library;
		const clap_plugin_factory_t* m_factory{nullptr};
		std::vector<ClapPlugin> m_plugins; //!< Only includes plugins that successfully loaded
		uint32_t m_pluginCount{0}; //!< Includes plugins that failed to load
		bool m_valid{false};
	};

	using ClapPlugin = ClapFile::ClapPlugin;

	auto getHost() const -> const clap_host* { return &m_host; }

	// CLAP API implementation: plugin -> host
	static const void* hostGetExtension(const clap_host* host, const char* extension);
	static void hostRequestCallback(const clap_host* host);
	static void hostRequestProcess(const clap_host* host);
	static void hostRequestRestart(const clap_host* host);

	// For iterating over m_clapInfoMap
	//auto begin() { return m_clapInfoMap.begin(); }
	//auto end() { return m_clapInfoMap.end(); }

	//! Return descriptor with URI @p uri or nullptr if none exists
	const clap_plugin_t* getPlugin(const std::string& uri);
	//! Return descriptor with URI @p uri or nullptr if none exists
	const clap_plugin_t* getPlugin(const QString& uri);

	void initPlugins(); //!< Called by Engine at LMMS startup

private:

	clap_host_t m_host; //!< CLAP host info and functions for plugins to use
	std::vector<std::filesystem::path> m_searchPaths; //!< Owns all CLAP search paths; Populated by findSearchPaths()
	std::vector<ClapFile> m_clapFiles; //!< Owns all loaded .clap files; Populated by findClapFiles()

	// Non-owning plugin caches (for fast iteration)
	std::vector<const clap_plugin_t*> m_plugins; //!< Non-owning vector of all loaded CLAP plugins
	//std::vector<const clap_plugin_t*> m_instrumentPlugins; //!< Non-owning vector of all loaded CLAP instrument plugins
	//std::vector<const clap_plugin_t*> m_effectPlugins; //!< Non-owning vector of all loaded CLAP effect plugins

	static bool m_debug; //!< If set, debug output will be printed

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

#endif // CLAPMANAGER_H
