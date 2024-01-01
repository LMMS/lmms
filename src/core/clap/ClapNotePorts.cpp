/*
 * ClapNotePorts.cpp - Implements CLAP note ports extension
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

#include "ClapNotePorts.h"

#ifdef LMMS_HAVE_CLAP

#include <QTimerEvent>

#include <utility>
#include <cassert>

#include "ClapInstance.h"

namespace lmms
{

auto ClapNotePorts::initImpl(const clap_host* host, const clap_plugin* plugin) noexcept -> bool
{
	m_portIndex = 0;
	m_dialect = 0;
	clapRescan(host, CLAP_NOTE_PORTS_RESCAN_ALL);
	return true;
}

auto ClapNotePorts::hostExt() const -> const clap_host_note_ports*
{
	static clap_host_note_ports ext {
		&clapSupportedDialects,
		&clapRescan
	};
	return &ext;
}

auto ClapNotePorts::checkSupported(const clap_plugin_note_ports& ext) -> bool
{
	return ext.count && ext.get;
}

auto ClapNotePorts::clapSupportedDialects([[maybe_unused]] const clap_host* host) -> std::uint32_t
{
	return CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI;
}

void ClapNotePorts::clapRescan(const clap_host* host, std::uint32_t flags)
{
	auto h = fromHost(host);
	if (!h) { return; }
	auto& notePorts = h->notePorts();

	if (!notePorts.supported()) { return; }

	if (flags & CLAP_NOTE_PORTS_RESCAN_ALL)
	{
		if (h->isActive())
		{
			h->logger().log(CLAP_LOG_PLUGIN_MISBEHAVING, "Host cannot rescan note ports while plugin is active");
			return;
		}

		/*
		 * I'm using a priority system to choose the note port we use.
		 * This may not be very useful in practice.
		 *
		 * Highest to lowest priority:
		 *   - CLAP preferred + MIDI (and CLAP) supported
		 *   - MIDI preferred + CLAP (and MIDI) supported
		 *   - Other preferred + CLAP and MIDI supported
		 *   - CLAP supported
		 *   - MIDI supported
		 *   - (no note input support)
		 */
		class PriorityHelper
		{
		public:
			using IndexDialectPair = std::pair<std::uint16_t, std::uint32_t>;
			auto check(std::uint16_t index, const clap_note_port_info& info)
			{
				// TODO: Check for CLAP_NOTE_DIALECT_MIDI_MPE too
				// MIDI preferred + CLAP (and MIDI) supported
				if (info.preferred_dialect == CLAP_NOTE_DIALECT_MIDI && (info.supported_dialects & CLAP_NOTE_DIALECT_CLAP))
				{
					m_cache[0] = { index, CLAP_NOTE_DIALECT_MIDI };
					m_best = std::min(0u, m_best);
				}
				// CLAP and MIDI supported
				else if ((info.supported_dialects & CLAP_NOTE_DIALECT_CLAP) && (info.supported_dialects & CLAP_NOTE_DIALECT_MIDI))
				{
					m_cache[1] = { index, CLAP_NOTE_DIALECT_CLAP };
					m_best = std::min(1u, m_best);
				}
				// CLAP supported
				else if (info.supported_dialects & CLAP_NOTE_DIALECT_CLAP)
				{
					m_cache[2] = { index, CLAP_NOTE_DIALECT_CLAP };
					m_best = std::min(2u, m_best);
				}
				// MIDI supported
				else if (info.supported_dialects & CLAP_NOTE_DIALECT_MIDI)
				{
					m_cache[3] = { index, CLAP_NOTE_DIALECT_MIDI };
					m_best = std::min(3u, m_best);
				}
			}
			auto getBest() -> IndexDialectPair
			{
				if (m_best == m_cache.size())
				{
					// No note port supported by host
					return {0, 0};
				}
				return m_cache[m_best];
			}
		private:
			std::array<IndexDialectPair, 4> m_cache; // priority 2 thru 5
			unsigned m_best = static_cast<unsigned>(m_cache.size()); // best port seen so far
		} priorityHelper;

		const auto count = notePorts.pluginExt()->count(h->plugin(), true);
		assert(count < (std::numeric_limits<std::int16_t>::max)()); // just in case
		for (std::uint16_t idx = 0; idx < static_cast<std::uint16_t>(count); ++idx)
		{
			auto info = clap_note_port_info{};
			info.id = CLAP_INVALID_ID;
			if (!notePorts.pluginExt()->get(h->plugin(), idx, true, &info))
			{
				h->logger().log(CLAP_LOG_DEBUG, "Failed to read note port info");
				return;
			}

			// Just in case
			if (info.id == CLAP_INVALID_ID)
			{
				h->logger().log(CLAP_LOG_PLUGIN_MISBEHAVING, "Note port info contains invalid id");
				continue;
			}

			// Check for #1 priority option: CLAP preferred + MIDI (and CLAP) supported
			if (info.preferred_dialect == CLAP_NOTE_DIALECT_CLAP && (info.supported_dialects & CLAP_NOTE_DIALECT_MIDI))
			{
				notePorts.m_portIndex = idx;
				notePorts.m_dialect = CLAP_NOTE_DIALECT_CLAP;
				return;
			}

			// Check for match with lesser-priority options
			priorityHelper.check(idx, info);
		}

		std::tie(notePorts.m_portIndex, notePorts.m_dialect) = priorityHelper.getBest();
		// TODO: Also set the 2nd supported note dialect? Or use a boolean to indicate both CLAP and MIDI are supported?
	}
	else if (flags & CLAP_NOTE_PORTS_RESCAN_NAMES)
	{
		// Not implemented
	}
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
