/*
 * ClapFile.h - Implementation of ClapFile class
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

#ifndef LMMS_CLAP_FILE_H
#define LMMS_CLAP_FILE_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include "Plugin.h"
#include "PluginIssue.h"

#include <filesystem>
#include <vector>
#include <unordered_set>
#include <memory>

#include <QLibrary>
#include <clap/clap.h>

namespace lmms
{

//! Forward declare
class ClapManager;

//! Class representing info for one .clap file, which contains 1 or more CLAP plugins
class ClapFile
{
public:
	//! Loads .clap file and plugin info
	ClapFile(const ClapManager* manager, std::filesystem::path&& filename);
	ClapFile(const ClapFile&) = delete;
	ClapFile(ClapFile&& other) noexcept;
	ClapFile& operator=(const ClapFile&) = delete;
	ClapFile& operator=(ClapFile&& rhs) noexcept;
	~ClapFile();

	//! Represents a CLAP plugin within a .clap file
	class ClapPluginInfo
	{
	public:
		//! Loads plugin info but does not activate
		ClapPluginInfo(const clap_plugin_factory* factory, uint32_t index);
		ClapPluginInfo(const ClapPluginInfo&) = delete;
		ClapPluginInfo(ClapPluginInfo&& other) noexcept;
		ClapPluginInfo& operator=(const ClapPluginInfo&) = delete;
		ClapPluginInfo& operator=(ClapPluginInfo&& rhs) noexcept = delete;
		~ClapPluginInfo() = default;

		auto isValid() const -> bool { return m_valid; }
		void invalidate() const { m_valid = false; }

		auto getFactory() const -> const clap_plugin_factory* { return m_factory; };
		auto getIndex() const -> uint32_t { return m_index; }
		auto getType() const -> Plugin::PluginTypes { return m_type; }
		auto getDescriptor() const -> const clap_plugin_descriptor* { return m_descriptor; }

	private:

		// Are set when the .clap file is loaded:
		const clap_plugin_factory* m_factory;
		uint32_t m_index; //!< Plugin index within the .clap file
		const clap_plugin_descriptor* m_descriptor;
		Plugin::PluginTypes m_type{Plugin::PluginTypes::Undefined};
		mutable bool m_valid{false};
		std::unordered_set<PluginIssue, PluginIssueHash> m_issues;
	};

	//! Call after creating ClapFile to load the .clap file
	auto load() -> bool;

	auto getParent() const -> const ClapManager* { return m_parent; }
	auto getFilename() const -> const std::filesystem::path& { return m_filename; }
	auto getFactory() const -> const clap_plugin_factory* { return m_factory; }

	//! Only includes plugins that successfully loaded
	auto pluginInfo() const -> const std::vector<std::shared_ptr<const ClapPluginInfo>>& { return m_pluginInfo; }

	//! Includes plugins that failed to load
	auto getPluginCount() const -> uint32_t { return m_pluginCount; }
	auto isValid() const -> bool { return m_valid; }

	//! Removes any invalid plugin info objects - be careful when using
	void purgeInvalidPlugins();

private:

	void unload();

	// Are set when the .clap file is loaded:
	const ClapManager* m_parent; //!< Always valid, so storing it is okay
	std::filesystem::path m_filename;
	std::unique_ptr<QLibrary> m_library;
	const clap_plugin_entry* m_entry{nullptr};
	const clap_plugin_factory* m_factory{nullptr};
	std::vector<std::shared_ptr<const ClapPluginInfo>> m_pluginInfo; //!< Only includes info for plugins that successfully loaded
	uint32_t m_pluginCount{0}; //!< Includes plugins that failed to load
	bool m_valid{false};
};

using ClapPluginInfo = ClapFile::ClapPluginInfo;

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_FILE_H
