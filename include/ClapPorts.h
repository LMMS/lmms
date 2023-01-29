/*
 * ClapPorts.h - Implementation of ClapPorts class
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

#ifndef LMMS_CLAP_PORTS_H
#define LMMS_CLAP_PORTS_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include "ClapFile.h"
#include "Plugin.h"
#include "PluginIssue.h"

#include <unordered_set>

#include <clap/clap.h>

namespace lmms
{

//! Manages CLAP ports for a CLAP plugin
class ClapPorts
{
public:
	ClapPorts(const clap_plugin_t* plugin, const clap_plugin_audio_ports* ports);

	auto check(const clap_plugin_audio_ports_t* ports, std::unordered_set<PluginIssue, PluginIssueHash>& issues) -> bool;

private:
	const clap_plugin_t* m_plugin;
	const clap_plugin_audio_ports* m_ports;
	uint32_t m_inputCount{0};
	uint32_t m_outputCount{0};

	bool m_valid{false};
	std::unordered_set<PluginIssue, PluginIssueHash> m_issues;
};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_PORTS_H
