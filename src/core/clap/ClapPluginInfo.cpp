/*
 * ClapPluginInfo.cpp - Implementation of ClapPluginInfo class
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

#include "ClapPluginInfo.h"

#ifdef LMMS_HAVE_CLAP

#include "ClapLog.h"
#include "ClapManager.h"

namespace lmms
{

auto ClapPluginInfo::create(const clap_plugin_factory& factory, std::uint32_t index) -> std::optional<ClapPluginInfo>
{
	auto info = std::optional{ClapPluginInfo{factory, index}};
	return info->type() != Plugin::Type::Undefined ? info : std::nullopt;
}

ClapPluginInfo::ClapPluginInfo(const clap_plugin_factory& factory, std::uint32_t index)
	: m_factory{&factory}
	, m_index{index}
{
	m_descriptor = m_factory->get_plugin_descriptor(m_factory, m_index);
	if (!m_descriptor)
	{
		ClapLog::globalLog(CLAP_LOG_ERROR, "No plugin descriptor");
		return;
	}

	if (!m_descriptor->id || !m_descriptor->name)
	{
		ClapLog::globalLog(CLAP_LOG_ERROR, "Invalid plugin descriptor");
		return;
	}

	if (!clap_version_is_compatible(m_descriptor->clap_version))
	{
		std::string msg = "Plugin '";
		msg += m_descriptor->id;
		msg += "' uses unsupported CLAP version '";
		msg += std::to_string(m_descriptor->clap_version.major) + "."
			+ std::to_string(m_descriptor->clap_version.minor) + "."
			+ std::to_string(m_descriptor->clap_version.revision);
		msg += "'. Must be at least 1.0.0.";
		ClapLog::globalLog(CLAP_LOG_ERROR, msg);
		return;
	}

	if (ClapManager::debugging())
	{
		std::string msg = "name: ";
		msg += m_descriptor->name;
		msg += "\nid: ";
		msg += m_descriptor->id;
		msg += "\nversion: ";
		msg += (m_descriptor->version ? m_descriptor->version : "");
		msg += "\nCLAP version: ";
		msg += std::to_string(m_descriptor->clap_version.major) + "."
			+ std::to_string(m_descriptor->clap_version.minor) + "."
			+ std::to_string(m_descriptor->clap_version.revision);
		msg += "\ndescription: ";
		msg += (m_descriptor->description ? m_descriptor->description : "");
		ClapLog::plainLog(CLAP_LOG_DEBUG, msg);
	}

	auto features = m_descriptor->features;
	while (features && *features)
	{
		auto feature = std::string_view{*features};
		if (ClapManager::debugging())
		{
			std::string msg = "feature: " + std::string{feature};
			ClapLog::plainLog(CLAP_LOG_DEBUG, msg);
		}

		if (feature == CLAP_PLUGIN_FEATURE_INSTRUMENT)
		{
			m_type = Plugin::Type::Instrument;
		}
		else if (feature == CLAP_PLUGIN_FEATURE_AUDIO_EFFECT
			|| feature == "effect" /* non-standard, but used by Surge XT Effects */)
		{
			m_type = Plugin::Type::Effect;
		}
		/*else if (feature == CLAP_PLUGIN_FEATURE_ANALYZER)
		{
			m_type = Plugin::Type::Tool;
		}*/
		++features;
	}

	if (m_type == Plugin::Type::Undefined)
	{
		std::string msg = "Plugin '" + std::string{m_descriptor->id}
			+ "' is not recognized as an instrument or audio effect";
		ClapLog::globalLog(CLAP_LOG_ERROR, msg);
	}
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
