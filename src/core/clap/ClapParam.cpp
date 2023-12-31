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
	: QObject{pluginHost}, m_info{info}, m_value{value}
{
	qDebug() << "ClapParam::ClapParam";
	const auto flags = m_info.flags;
	// Assume ClapParam::check() has already been called at this point

	m_id = "p" + std::to_string(m_info.id);
	m_displayName = m_info.name;

	qDebug().nospace() << "--id:" << info.id << "; name:'" << info.name << "'; module:'" << info.module << "'; flags:" << info.flags;

	// If the user cannot control this param, no AutomatableModel is needed
	if (flags & CLAP_PARAM_IS_HIDDEN
		|| flags & CLAP_PARAM_IS_READONLY
		|| flags & CLAP_PARAM_IS_BYPASS /* TODO */) { return; }

	if (value > m_info.max_value || value < m_info.min_value)
	{
		throw std::logic_error{"CLAP param: Error: value is out of range"};
		//value = std::clamp(value, m_info.min_value, m_info.max_value);
	}

	auto name = QString::fromUtf8(displayName().data());

	// TODO: CLAP_PARAM_IS_BYPASS
	if (flags & CLAP_PARAM_IS_STEPPED)
	{
		const auto minVal = static_cast<int>(m_info.min_value);
		const auto valueInt = static_cast<int>(value);
		const auto maxVal = static_cast<int>(m_info.max_value);

		if (minVal == 0 && maxVal == 1)
		{
			qDebug() << "CLAP param: Creating BoolModel";
			m_connectedModel = std::make_unique<BoolModel>(valueInt, pluginHost, name);
			m_valueType = ParamType::Bool;
		}
		else
		{
			qDebug() << "CLAP param: Creating IntModel";
			m_connectedModel = std::make_unique<IntModel>(valueInt, minVal, maxVal, pluginHost, name);
			m_valueType = ParamType::Integer;
		}
	}
	else
	{
		qDebug() << "CLAP param: Creating FloatModel";

		// Allow ~1000 steps
		double stepSize = (m_info.max_value - m_info.min_value) / 1000.0f;

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

		m_valueType = ParamType::Float;
	}
}

auto ClapParam::getValueText(const clap_plugin* plugin, const clap_plugin_params* params) const -> std::string
{
	constexpr std::uint32_t bufferSize = 50;
	std::string buffer(bufferSize, '\0');
	if (!params->value_to_text(plugin, m_info.id, m_value, &buffer[0], bufferSize))
	{
		return std::to_string(m_value);
	}

	auto temp = std::to_string(m_value) + " (";
	temp += buffer.c_str();
	temp += ')';
	return temp;
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

void ClapParam::printShortInfo(std::ostream& os) const
{
	os << "id: " << m_info.id << ", name: '" << m_info.name << "', module: '" << m_info.module << "'";
}

void ClapParam::printInfo(std::ostream& os) const
{
	printShortInfo(os);
	os << ", min: " << m_info.min_value << ", max: " << m_info.max_value;
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
	const std::uint32_t criticalFlags =
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
		//qWarning() << "CLAP param: Error: min value > max value";
	}

	if (info.default_value > info.max_value || info.default_value < info.min_value)
	{
		std::string msg = "CLAP param: Error: default value is out of range\ndefault: " + std::to_string(info.default_value)
			+ "; min: " + std::to_string(info.min_value) + "; max: " + std::to_string(info.max_value);
		throw std::logic_error{msg};
		//qDebug() << "CLAP param: Warning: default value is out of range; clamping to valid range";
		//info.default_value = std::clamp(info.default_value, info.min_value, info.max_value);
		//return false;
	}
	//return true;
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
