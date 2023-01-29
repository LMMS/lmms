/*
 * ClapPorts.cpp - Implementation of ClapPorts class
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

#include "ClapPorts.h"

#include <string>

#include <QDebug>

namespace lmms
{

ClapPorts::ClapPorts(const clap_plugin_t* plugin, const clap_plugin_audio_ports* ports)
	: m_plugin(plugin), m_ports(ports)
{
	// NOTE: plugin must not be active
	if (!m_ports || !m_plugin)
		return;

	const auto outCount = m_ports->count(plugin, false);
	if (outCount == 0)
		m_issues.emplace(PluginIssueType::noOutputChannel);
	//if (outCount > 2)
	//	m_issues.emplace(PluginIssueType::tooManyOutputChannels, std::to_string(outCount));

	qDebug() << "~~~Printing port info (outCount:" << outCount << ")";

	for (uint32_t i = 0; i < outCount; ++i)
	{
		clap_audio_port_info_t info;
		if (!m_ports->get(m_plugin, i, false, &info))
		{
			qWarning() << "Unknown error calling m_ports->get(...)";
			continue;
		}
		qDebug() << "- port id:" << info.id;
		qDebug() << "- port name:" << info.name;
		qDebug() << "- port flags:" << info.flags;
		qDebug() << "- port channel_count:" << info.channel_count;
		qDebug() << "- port type:" << info.port_type;
		qDebug() << "- port in place pair:" << info.in_place_pair;
	}

	const auto inCount = m_ports->count(plugin, true);

	qDebug() << "~~~Printing port info (inCount:" << inCount << ")";

	for (uint32_t i = 0; i < inCount; ++i)
	{
		clap_audio_port_info_t info;
		if (!m_ports->get(m_plugin, i, true, &info))
		{
			qWarning() << "Unknown error calling m_ports->get(...)";
			continue;
		}
		qDebug() << "- port id:" << info.id;
		qDebug() << "- port name:" << info.name;
		qDebug() << "- port flags:" << info.flags;
		qDebug() << "- port channel_count:" << info.channel_count;
		qDebug() << "- port type:" << info.port_type;
		qDebug() << "- port in place pair:" << info.in_place_pair;
	}
}

auto ClapPorts::check(const clap_plugin_audio_ports_t* ports, std::unordered_set<PluginIssue, PluginIssueHash>& issues) -> bool
{
	if (!ports)
	{
		issues.emplace(PluginIssueType::noOutputChannel);
		return false; // No ports
	}

	//clap_audio_port_info_t info;
	//ports->get(nullptr, );

	return true;
}


}