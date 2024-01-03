/*
 * ClapState.h - Implements CLAP state extension
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

#ifndef LMMS_CLAP_STATE_H
#define LMMS_CLAP_STATE_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include <optional>
#include <string>
#include <string_view>
#include <clap/ext/state.h>

#include "ClapExtension.h"

namespace lmms
{

class ClapState final : public ClapExtension<clap_host_state, clap_plugin_state>
{
public:
	using ClapExtension::ClapExtension;

	auto extensionId() const -> std::string_view override { return CLAP_EXT_STATE; }
	auto hostExt() const -> const clap_host_state* override;

	//! Whether the plugin has indicated its state has changes that need to be saved
	auto dirty() const { return m_dirty; }

	//! Tells plugin to load the given base64-encoded state data; Returns true if successful
	auto load(std::string_view base64) -> bool;

	//! Tells plugin to load state data from encodedState(); Returns true if successful
	auto load() -> bool;

	//! Tells plugin to save its state data; Sets and returns encodedState()'s data if successful
	auto save() -> std::optional<std::string_view>;

	//! Base-64 encoded state data from the last time save() was successfully called
	auto encodedState() const -> std::string_view { return m_state; }

private:
	auto checkSupported(const clap_plugin_state& ext) -> bool override;

	/**
	 * clap_host_state implementation
	 */
	static void clapMarkDirty(const clap_host* host);

	std::string m_state; //!< base64-encoded state cache
	bool m_dirty = false;
};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_STATE_H
