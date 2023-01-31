/*
 * ClapPluginInstance.h - Implementation of ClapPluginInstance class
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

#ifndef LMMS_CLAP_PLUGIN_INSTANCE_H
#define LMMS_CLAP_PLUGIN_INSTANCE_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include "ClapFile.h"

namespace lmms
{

class ClapInstance;

//! CLAP plugin instance
class ClapPluginInstance
{
public:
	//! Creates a clap_plugin plugin instance
	ClapPluginInstance(const ClapInstance* parent, const ClapPluginInfo* info);

	//! Deactivates and destroys plugin instance as needed
	~ClapPluginInstance();

	//! Inits plugin instance, returning true if successful
	auto init() -> bool;

	//! Activates plugin instance, returning true if successful
	auto activate() -> bool;

	//! Deactivates plugin instance, returning true if successful
	auto deactivate() -> bool;

	auto getHost() const -> const clap_host*;
	auto getPlugin() const -> const clap_plugin* { return m_plugin; }
	auto getInfo() const -> const ClapPluginInfo& { return *m_info; }
	auto isValid() const -> bool { return m_plugin != nullptr; }
	auto isInitialized() const -> bool { return m_initialized; }
	auto isActive() const -> bool { return m_active; }
	auto isProcessing() const -> bool { return m_processing; }

private:

	//! Initializes extensions for plugin instance, returning true if successful
	auto initExtensions() -> bool;

	template<typename T>
	void initExtension(const T*& ext, const char* id)
	{
		// Must be on main thread
		if (!ext)
			ext = static_cast<const T*>(m_plugin->get_extension(m_plugin, id));
	}

	const ClapInstance* m_parent; //!< Never null
	const clap_plugin* m_plugin;
	const ClapPluginInfo* m_info; //!< Never null

	/**
	 * Plugin instance state
	 */
	bool m_initialized{false};
	bool m_active{false};
	bool m_processing{false};

	/**
	 * Plugin extension pointers
	 * TODO: Add plugin extension pointers here as support is implemented in the host
	 */
	const clap_plugin_audio_ports* m_extAudioPorts{nullptr};
	const clap_plugin_params* m_extParams{nullptr};
};


}

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_PLUGIN_INSTANCE_H
