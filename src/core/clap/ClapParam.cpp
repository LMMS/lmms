/*
 * ClapParam.cpp - Implementation of ClapParam class
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

#include "ClapParam.h"

#ifdef LMMS_HAVE_CLAP

#include "ClapInstance.h"

#include <cmath>

#include <QString>
#include <QDebug>

namespace lmms
{

ClapParam::ClapParam(ClapInstance* pluginHost, const clap_param_info& info, double value)
	: QObject{pluginHost}
	, m_info{info}
	, m_value{value}
	, m_id{"p" + std::to_string(m_info.id)}
	, m_displayName{m_info.name}
{
	// Assume ClapParam::check() has already been called at this point

#if 0
	qDebug().nospace() << "CLAP param --- id:" << info.id << "; name:'" << info.name << "'; module:'" << info.module << "'; flags:" << info.flags;
#endif

	// If the user cannot control this param, no AutomatableModel is needed
	const auto flags = m_info.flags;
	if ((flags & CLAP_PARAM_IS_HIDDEN) || (flags & CLAP_PARAM_IS_READONLY)) { return; }

	if (value > m_info.max_value || value < m_info.min_value)
	{
		throw std::logic_error{"CLAP param: Error: value is out of range"};
		//value = std::clamp(value, m_info.min_value, m_info.max_value);
	}

	const auto name = QString::fromUtf8(displayName().data());

	if ((flags & CLAP_PARAM_IS_STEPPED) || (flags & CLAP_PARAM_IS_BYPASS))
	{
		const auto minVal = static_cast<int>(std::trunc(m_info.min_value));
		const auto valueInt = static_cast<int>(std::trunc(value));
		const auto maxVal = static_cast<int>(std::trunc(m_info.max_value));

		if (minVal == 0 && maxVal == 1)
		{
			m_connectedModel = std::make_unique<BoolModel>(valueInt, pluginHost, name);
			m_valueType = ValueType::Bool;
		}
		else
		{
			if (flags & CLAP_PARAM_IS_BYPASS)
			{
				pluginHost->log(CLAP_LOG_PLUGIN_MISBEHAVING, "Bypass parameter doesn't have range [0, 1]");
			}

			m_connectedModel = std::make_unique<IntModel>(valueInt, minVal, maxVal, pluginHost, name);
			// TODO: Use CLAP_PARAM_IS_ENUM
			m_valueType = ValueType::Integer;
		}
	}
	else
	{
		// Allow ~1000 steps
		double stepSize = (m_info.max_value - m_info.min_value) / 1000.0;

		// Make multiples of 0.01 (or 0.1 for larger values)
		const double minStep = (stepSize >= 1.0) ? 0.1 : 0.01;
		stepSize -= std::fmod(stepSize, minStep);
		stepSize = std::max(stepSize, minStep);

		m_connectedModel = std::make_unique<FloatModel>(
			static_cast<float>(value),
			static_cast<float>(m_info.min_value),
			static_cast<float>(m_info.max_value),
			static_cast<float>(stepSize),
			pluginHost, name);

		m_valueType = ValueType::Float;
	}
}

auto ClapParam::getValueText(const clap_plugin* plugin, const clap_plugin_params* params) const -> std::string
{
	const auto valueStr = std::to_string(m_value);
	if (!params->value_to_text)
	{
		return valueStr;
	}

	auto buffer = std::array<char, CLAP_NAME_SIZE>{};
	if (!params->value_to_text(plugin, m_info.id, m_value, buffer.data(), CLAP_NAME_SIZE - 1))
	{
		return valueStr;
	}

	if (valueStr == buffer.data())
	{
		// No point in displaying two identical strings
		return valueStr;
	}

	// Use CLAP-provided string + internal value in brackets for automation purposes
	return std::string{buffer.data()} + "\n[" + valueStr + "]";
}

void ClapParam::setValue(double v)
{
	if (m_value == v) { return; }
	m_value = v;
	valueChanged();
}

void ClapParam::setModulation(double v)
{
	if (m_modulation == v) { return; }
	m_modulation = v;
	modulatedValueChanged();
}

auto ClapParam::isValueValid(const double v) const -> bool
{
	return m_info.min_value <= v && v <= m_info.max_value;
}

auto ClapParam::getShortInfoString() const -> std::string
{
	return "id: " + std::to_string(m_info.id) + ", name: '" + std::string{m_info.name} + "', module: '" + std::string{m_info.module} + "'";
}

auto ClapParam::getInfoString() const -> std::string
{
	return getShortInfoString() + ", min: " + std::to_string(m_info.min_value) + ", max: " + std::to_string(m_info.max_value);
}

auto ClapParam::isInfoEqualTo(const clap_param_info& info) const -> bool
{
	return info.cookie == m_info.cookie
		&& info.default_value == m_info.default_value
		&& info.max_value == m_info.max_value
		&& info.min_value == m_info.min_value
		&& info.flags == m_info.flags
		&& info.id == m_info.id
		&& !::std::strncmp(info.name, m_info.name, sizeof(info.name))
		&& !::std::strncmp(info.module, m_info.module, sizeof(info.module));
}

auto ClapParam::isInfoCriticallyDifferentTo(const clap_param_info& info) const -> bool
{
	assert(m_info.id == info.id);
	constexpr std::uint32_t criticalFlags =
		CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_AUTOMATABLE_PER_NOTE_ID |
		CLAP_PARAM_IS_AUTOMATABLE_PER_KEY | CLAP_PARAM_IS_AUTOMATABLE_PER_CHANNEL |
		CLAP_PARAM_IS_AUTOMATABLE_PER_PORT | CLAP_PARAM_IS_MODULATABLE |
		CLAP_PARAM_IS_MODULATABLE_PER_NOTE_ID | CLAP_PARAM_IS_MODULATABLE_PER_KEY |
		CLAP_PARAM_IS_MODULATABLE_PER_CHANNEL | CLAP_PARAM_IS_MODULATABLE_PER_PORT |
		CLAP_PARAM_IS_READONLY | CLAP_PARAM_REQUIRES_PROCESS;
	return (m_info.flags & criticalFlags) == (info.flags & criticalFlags)
		|| m_info.min_value != m_info.min_value || m_info.max_value != m_info.max_value;
}

void ClapParam::check(clap_param_info& info)
{
	if (info.min_value > info.max_value)
	{
		throw std::logic_error{"CLAP param: Error: min value > max value"};
		// TODO: Use PluginIssueType::MinGreaterMax ??
	}

	if (info.default_value > info.max_value || info.default_value < info.min_value)
	{
		std::string msg = "CLAP param: Error: default value is out of range\ndefault: " + std::to_string(info.default_value)
			+ "; min: " + std::to_string(info.min_value) + "; max: " + std::to_string(info.max_value);
		throw std::logic_error{msg};
		// TODO: Use PluginIssueType::DefaultValueNotInRange ??
		//qDebug() << "CLAP param: Warning: default value is out of range; clamping to valid range";
		//info.default_value = std::clamp(info.default_value, info.min_value, info.max_value);
		//return false;
	}
	//return true;
}

auto ClapParam::extensionSupported(const clap_plugin_params* ext) noexcept -> bool
{
	// NOTE: ext->value_to_text and ext->text_to_value are optional
	return ext && ext->count && ext->get_info && ext->get_value && ext->flush;
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
