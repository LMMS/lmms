/*
 * Lv2Options.h - Lv2Options class
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

#ifndef LV2OPTIONS_H
#define LV2OPTIONS_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_LV2

#include <lv2/lv2plug.in/ns/ext/options/options.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <map>
#include <set>
#include <vector>

/**
	Option container

	References all available options for a plugin and maps them to their URIDs.
	This class is used per Lv2 processor (justification in Lv2Proc::initMOptions())

	The public member functions should be called in descending order:

	1. supportOption: set all supported option URIDs
	2. initOption: initialize options with values
	3. createOptionVectors: create the option vectors required for
		the feature
	4. access the latter using feature()
*/
class Lv2Options
{
public:
	//! Initialize an option
	template<class T>
	void initOption(LV2_URID key,
		LV2_URID type,
		const T* value,
		LV2_Options_Context context = LV2_Options_Context::LV2_OPTIONS_INSTANCE,
		uint32_t subject = 0)
	{
		initOption(key, sizeof(T), type, value, context, subject);
	}
	//! Fill m_options and m_optionPointers with all options
	void createOptionVectors();
	//! Return the feature
	const LV2_Options_Option* feature() const
	{
		return m_options.data();
	}


	//! Return if a option is supported by LMMS
	static bool isOptionSupported(LV2_URID key);
	//! Mark option as supported
	static void supportOption(LV2_URID key);

private:
	void initOption(LV2_URID key,
		uint32_t size,
		LV2_URID type,
		const void* value,
		LV2_Options_Context context,
		uint32_t subject);

	//! options that are supported by every processor
	static std::set<LV2_URID> s_supportedOptions;
	//! options + data, ordered by URID
	std::map<LV2_URID, LV2_Options_Option> m_optionByUrid;
	//! option storage
	std::vector<LV2_Options_Option> m_options;
};

#endif // LMMS_HAVE_LV2

#endif // LV2OPTIONS_H
