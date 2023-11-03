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

#include <vector>
#include <unordered_set>
#include <memory>

#if __has_include(<filesystem>)
#include <filesystem>
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace std {
	namespace filesystem = experimental::filesystem;
} // namespace std
#else
#error "Standard filesystem library not available"
#endif

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
	explicit ClapFile(std::filesystem::path filename);
	ClapFile(const ClapFile&) = delete;
	ClapFile(ClapFile&& other) noexcept;
	auto operator=(const ClapFile&) -> ClapFile& = delete;
	auto operator=(ClapFile&& rhs) noexcept -> ClapFile&;
	~ClapFile();

	//! Represents a CLAP plugin within a .clap file
	class ClapPluginInfo
	{
	public:
		//! Loads plugin info but does not activate
		ClapPluginInfo(const clap_plugin_factory* factory, std::uint32_t index);
		ClapPluginInfo(const ClapPluginInfo&) = delete;
		ClapPluginInfo(ClapPluginInfo&& other) noexcept;
		auto operator=(const ClapPluginInfo&) -> ClapPluginInfo& = delete;
		auto operator=(ClapPluginInfo&&) noexcept -> ClapPluginInfo& = delete;
		~ClapPluginInfo() = default;

		auto isValid() const { return m_valid; }
		void invalidate() const { m_valid = false; }

		auto factory() const { return m_factory; };
		auto index() const { return m_index; }
		auto type() const { return m_type; }
		auto descriptor() const { return m_descriptor; }

	private:

		// Are set when the .clap file is loaded:
		const clap_plugin_factory* m_factory;
		std::uint32_t m_index; //!< Plugin index within the .clap file
		const clap_plugin_descriptor* m_descriptor = nullptr;
		Plugin::Type m_type = Plugin::Type::Undefined;
		mutable bool m_valid = false;
		std::unordered_set<PluginIssue, PluginIssueHash> m_issues;
	};

	//! Call after creating ClapFile to load the .clap file
	auto load() -> bool;

	auto filename() const -> const std::filesystem::path& { return m_filename; }
	auto factory() const { return m_factory; }

	//! Only includes plugins that successfully loaded
	auto pluginInfo() const -> const auto& { return m_pluginInfo; }

	//! Includes plugins that failed to load
	auto pluginCount() const { return m_pluginCount; }

	auto isValid() const { return m_valid; }

	//! Removes any invalid plugin info objects - be careful when using
	void purgeInvalidPlugins();

private:

	void unload();

	// Are set when the .clap file is loaded:
	std::filesystem::path m_filename;
	std::unique_ptr<QLibrary> m_library;
	const clap_plugin_entry* m_entry = nullptr;
	const clap_plugin_factory* m_factory = nullptr;
	std::vector<std::shared_ptr<const ClapPluginInfo>> m_pluginInfo; //!< Only includes info for plugins that successfully loaded
	std::uint32_t m_pluginCount = 0; //!< Includes plugins that failed to load
	bool m_valid = false;
};

using ClapPluginInfo = ClapFile::ClapPluginInfo;

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_FILE_H
