/*
 * ClapPluginInfo.h - Implementation of ClapPluginInfo class
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

#ifndef LMMS_CLAP_PLUGIN_INFO_H
#define LMMS_CLAP_PLUGIN_INFO_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include <optional>
#include <clap/entry.h>
#include <clap/factory/plugin-factory.h>

#include "Plugin.h"
#include "lmms_export.h"

namespace lmms
{

//! Represents a CLAP plugin within a .clap file
class LMMS_EXPORT ClapPluginInfo
{
public:
	//! Creates plugin info, populated via a quick scan of the plugin; may fail
	static auto create(const clap_plugin_factory& factory, std::uint32_t index) -> std::optional<ClapPluginInfo>;

	~ClapPluginInfo() = default;

	ClapPluginInfo(const ClapPluginInfo&) = default;
	ClapPluginInfo(ClapPluginInfo&&) noexcept = default;
	auto operator=(const ClapPluginInfo&) -> ClapPluginInfo& = default;
	auto operator=(ClapPluginInfo&&) noexcept -> ClapPluginInfo& = default;

	auto factory() const -> const clap_plugin_factory& { return *m_factory; };
	auto index() const { return m_index; }
	auto type() const { return m_type; }
	auto descriptor() const -> const clap_plugin_descriptor& { return *m_descriptor; }

private:
	ClapPluginInfo(const clap_plugin_factory& factory, std::uint32_t index);

	const clap_plugin_factory* m_factory;
	std::uint32_t m_index; //!< plugin index within .clap file
	const clap_plugin_descriptor* m_descriptor = nullptr;
	Plugin::Type m_type = Plugin::Type::Undefined;
};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_PLUGIN_INFO_H
