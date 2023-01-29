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

//! Class representing info for one .clap file, which contains 1 or more CLAP plugins
class ClapFile
{
public:
	//! Loads .clap file and plugin info
	ClapFile(const class ClapManager* manager, std::filesystem::path&& filename);
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
		ClapPlugin(const ClapFile* parent, uint32_t index, const clap_plugin_descriptor_t* desc);

		~ClapPlugin() = default;
		ClapPlugin(const ClapPlugin&) = delete;
		ClapPlugin(ClapPlugin&& other) noexcept;
		ClapPlugin& operator=(const ClapPlugin&) = delete;
		ClapPlugin& operator=(ClapPlugin&&) noexcept = delete;

		enum class State
		{
			kUnknown,
			kInvalid,

		};

		auto activate() -> bool;
		void deactivate();
		void initExtensions();
		auto isValid() const -> bool { return m_valid; }
		auto isActivated() const -> bool { return m_plugin != nullptr; }

		auto getParent() const -> const ClapFile* { return m_parent; }
		auto getIndex() const -> uint32_t { return m_index; }
		auto getType() const -> Plugin::PluginTypes { return m_type; }
		auto getDescriptor() const -> const clap_plugin_descriptor_t* { return m_descriptor; }
		auto getPlugin() const -> const clap_plugin_t* { return m_plugin.get(); }

	private:

		struct ClapPluginDeleter
		{
			void operator()(const clap_plugin_t* p) const noexcept;
		};

		using ClapPluginPtr = std::unique_ptr<const clap_plugin_t, ClapPluginDeleter>;

		auto check() -> bool;

		template<typename T>
		void initPluginExtension(const T*& ext, const char* id)
		{
			// Must be on main thread
			if (!ext)
				ext = static_cast<const T*>(m_plugin->get_extension(m_plugin.get(), id));
		}

		// Are set when the .clap file is loaded:
		const ClapFile* m_parent;
		const clap_host_t* m_host{nullptr};
		uint32_t m_index{0}; //!< Plugin index within the .clap file
		const clap_plugin_descriptor_t* m_descriptor{nullptr};
		Plugin::PluginTypes m_type{Plugin::PluginTypes::Undefined};
		bool m_valid{false};
		std::unordered_set<PluginIssue, PluginIssueHash> m_issues;

		//! Is set after plugin activization
		ClapPluginPtr m_plugin{nullptr};

		// Pointers to the plugin's extensions; Set in activate()
		const clap_plugin_params_t* m_extParams{nullptr};
		//const clap_plugin_quick_controls_t* m_extQuickControls{nullptr};
		const clap_plugin_audio_ports_t* m_extAudioPorts{nullptr};
		const clap_plugin_gui_t* m_extGui{nullptr};
		const clap_plugin_timer_support_t* m_extTimerSupport{nullptr};
		const clap_plugin_posix_fd_support_t* m_extPosixFdSupport{nullptr};
		const clap_plugin_thread_pool_t* m_extThreadPool{nullptr};
		const clap_plugin_preset_load_t* m_extPresetLoad{nullptr};
		const clap_plugin_state_t* m_extState{nullptr};
	};

	auto load() -> bool;
	void unload();
	auto getParent() const -> const ClapManager* { return m_parent; }
	auto getFilename() const -> const std::filesystem::path& { return m_filename; }
	auto getFactory() const -> const clap_plugin_factory_t* { return m_factory; }

	//! Only includes plugins that successfully loaded
	auto getPlugins() const -> const std::vector<ClapPlugin>& { return m_plugins; }

	//! Includes plugins that failed to load
	auto getPluginCount() const -> uint32_t { return m_pluginCount; }
	auto isValid() const -> bool { return m_valid; }

private:
	// Are set when the .clap file is loaded:
	const class ClapManager* m_parent;
	std::filesystem::path m_filename;
	std::unique_ptr<QLibrary> m_library;
	const clap_plugin_factory_t* m_factory{nullptr};
	std::vector<ClapPlugin> m_plugins; //!< Only includes plugins that successfully loaded
	uint32_t m_pluginCount{0}; //!< Includes plugins that failed to load
	bool m_valid{false};
};

using ClapPlugin = ClapFile::ClapPlugin;

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_FILE_H
