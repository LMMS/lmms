/*
 * Lv2Options.cpp - Lv2Options implementation
 *
 * Copyright (c) 2020-2020 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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

#include "Lv2Options.h"

#ifdef LMMS_HAVE_LV2

#include <QtGlobal>


namespace lmms
{


std::set<LV2_URID> Lv2Options::s_supportedOptions;




bool Lv2Options::isOptionSupported(LV2_URID key)
{
	return s_supportedOptions.find(key) != s_supportedOptions.end();
}




void Lv2Options::supportOption(LV2_URID key)
{
	const auto result = s_supportedOptions.insert(key);
	Q_ASSERT(result.second);
}




void Lv2Options::createOptionVectors()
{
	// create vector of options
	for(LV2_URID urid : s_supportedOptions)
	{
		auto itr = m_optionByUrid.find(urid);
		Q_ASSERT(itr != m_optionByUrid.end());
		m_options.push_back(itr->second);
	}
	LV2_Options_Option nullOption;
	nullOption.key = 0;
	nullOption.value = nullptr;
	m_options.push_back(nullOption);
}




void Lv2Options::initOption(LV2_URID key, uint32_t size, LV2_URID type,
	std::shared_ptr<void> value,
	LV2_Options_Context context, uint32_t subject)
{
	Q_ASSERT(isOptionSupported(key));

	LV2_Options_Option opt;
	opt.key = key;
	opt.context = context;
	opt.subject = subject;
	opt.size = size;
	opt.type = type;
	opt.value = value.get();

	const auto optResult = m_optionByUrid.emplace(key, opt);
	const auto valResult = m_optionValues.emplace(key, std::move(value));
	Q_ASSERT(optResult.second);
	Q_ASSERT(valResult.second);
}




void Lv2Options::clear()
{
	m_options.clear();
	m_optionValues.clear();
	m_optionByUrid.clear();
}


} // namespace lmms

#endif // LMMS_HAVE_LV2
