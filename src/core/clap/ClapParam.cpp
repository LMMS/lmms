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

namespace lmms
{

ClapParam::ClapParam(ClapInstance* pluginHost, const clap_param_info& info, double value)
	: QObject{pluginHost}, m_info{info}, m_value{value} {}

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
	const uint32_t criticalFlags =
		CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_AUTOMATABLE_PER_NOTE_ID |
		CLAP_PARAM_IS_AUTOMATABLE_PER_KEY | CLAP_PARAM_IS_AUTOMATABLE_PER_CHANNEL |
		CLAP_PARAM_IS_AUTOMATABLE_PER_PORT | CLAP_PARAM_IS_MODULATABLE |
		CLAP_PARAM_IS_MODULATABLE_PER_NOTE_ID | CLAP_PARAM_IS_MODULATABLE_PER_KEY |
		CLAP_PARAM_IS_MODULATABLE_PER_CHANNEL | CLAP_PARAM_IS_MODULATABLE_PER_PORT |
		CLAP_PARAM_IS_READONLY | CLAP_PARAM_REQUIRES_PROCESS;
	return (m_info.flags & criticalFlags) == (info.flags & criticalFlags)
		|| m_info.min_value != m_info.min_value || m_info.max_value != m_info.max_value;
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
