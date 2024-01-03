/*
 * ClapFile.h - Implementation of ClapFile class
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

#ifndef LMMS_CLAP_FILE_H
#define LMMS_CLAP_FILE_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include <QLibrary>
#include <memory>
#include <vector>

#include "ClapPluginInfo.h"
#include "NoCopyNoMove.h"
#include "lmms_filesystem.h"
#include "lmms_export.h"

namespace lmms
{

//! Class representing info for one .clap file, which contains 1 or more CLAP plugins
class LMMS_EXPORT ClapFile
{
public:
	class AccessKey : public NoCopyNoMove
	{
		AccessKey() = default;
		friend class ClapManager;
	};

	explicit ClapFile(fs::path filename);
	~ClapFile();

	ClapFile(const ClapFile&) = delete;
	ClapFile(ClapFile&& other) noexcept;
	auto operator=(const ClapFile&) -> ClapFile& = delete;
	auto operator=(ClapFile&& rhs) noexcept -> ClapFile&;

	//! Loads the .clap file and scans for plugins
	auto load() -> bool;

	auto filename() const -> const fs::path& { return m_filename; }
	auto factory() const { return m_factory; }

	//! Only includes plugins that successfully loaded; Some may be invalidated later
	auto pluginInfo() const -> auto& { return m_pluginInfo; }

	//! Only includes plugins that successfully loaded; Some may be invalidated later
	auto pluginInfo(AccessKey) -> auto& { return m_pluginInfo; }

	//! Includes plugins that failed to load
	auto pluginCount() const { return m_pluginCount; }

private:
	void unload() noexcept;

	fs::path m_filename;

	std::unique_ptr<QLibrary> m_library;

	struct EntryDeleter
	{
		void operator()(const clap_plugin_entry* ptr);
	};

	std::unique_ptr<const clap_plugin_entry, EntryDeleter> m_entry;

	const clap_plugin_factory* m_factory = nullptr;

	//! Only includes info for plugins that successfully loaded
	std::vector<std::optional<ClapPluginInfo>> m_pluginInfo;

	//! Includes plugins that failed to load
	std::uint32_t m_pluginCount = 0;
};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_FILE_H
