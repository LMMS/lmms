/*
 * ClapNotePorts.h - Implements CLAP note ports extension
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

#ifndef LMMS_CLAP_NOTE_PORTS_H
#define LMMS_CLAP_NOTE_PORTS_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include <clap/ext/note-ports.h>

#include "ClapExtension.h"

namespace lmms
{

class ClapNotePorts final : public ClapExtension<clap_host_note_ports, clap_plugin_note_ports>
{
public:
	using ClapExtension::ClapExtension;

	auto extensionId() const -> std::string_view override { return CLAP_EXT_NOTE_PORTS; }

	auto portIndex() const { return m_portIndex; }
	auto dialect() const { return m_dialect; }

	auto hasInput() const -> bool { return m_dialect != 0; }

private:
	auto initImpl() noexcept -> bool override;
	auto hostExtImpl() const -> const clap_host_note_ports* override;
	auto checkSupported(const clap_plugin_note_ports& ext) -> bool override;

	/**
	 * clap_host_note_ports implementation
	 */
	static auto clapSupportedDialects(const clap_host* host) -> std::uint32_t;
	static void clapRescan(const clap_host* host, std::uint32_t flags);

	std::uint16_t m_portIndex = 0; // Chosen plugin note port index (not the port id!)
	std::uint32_t m_dialect = 0;   // Chosen plugin input dialect (0 == no note input)
};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_NOTE_PORTS_H
