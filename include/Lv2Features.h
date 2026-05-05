/*
 * Lv2Features.h - Lv2Features class
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

#ifndef LMMS_LV2_FEATURES_H
#define LMMS_LV2_FEATURES_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_LV2

#include <map>
#include <string_view>
#include <vector>

#include <lv2/core/lv2.h>

namespace lmms
{

/**
	Feature container

	References all available features for a plugin and maps them to their URIs.

	The public member functions should be called in descending order:

	1. initCommon: map plugin-common features
	2. operator[]: map plugin-specific features
	3. createFeatureVectors: create the feature vectors required for
		lilv_plugin_instantiate
	4. access the latter
*/
class Lv2Features
{
public:
	//! Return if a feature is supported by LMMS
	static bool isFeatureSupported(const char *featName);

	Lv2Features();

	//! Register only plugin-common features
	void initCommon();
	//! Return reference to feature data with given URI featName
	void*& operator[](const char* featName);
	//! Fill m_features and m_featurePointers with all features
	void createFeatureVectors();
	//! Return LV2_Feature pointer vector, suited for lilv_plugin_instantiate
	const LV2_Feature* const* featurePointers() const
	{
		return m_featurePointers.data();
	}
	//! Clear everything
	void clear();

private:
	//! feature storage
	std::vector<LV2_Feature> m_features;
	//! pointers to m_features, required for lilv_plugin_instantiate
	std::vector<const LV2_Feature*> m_featurePointers;
	//! features + data, ordered by URI
	std::map<std::string_view, void*> m_featureByUri;
};


} // namespace lmms

#endif // LMMS_HAVE_LV2

#endif // LMMS_LV2_FEATURES_H
