/*
 * Lv2Features.cpp - Lv2Features implementation
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

#include "Lv2Features.h"

#ifdef LMMS_HAVE_LV2

#include <QtGlobal>

#include "Engine.h"
#include "Lv2Manager.h"


namespace lmms
{


bool Lv2Features::isFeatureSupported(const char* featName)
{
	return Engine::getLv2Manager()->isFeatureSupported(featName);
}




Lv2Features::Lv2Features()
{
	const Lv2Manager* man = Engine::getLv2Manager();
	// create (yet empty) map feature URI -> feature
	for(auto uri : man->supportedFeatureURIs())
	{
		m_featureByUri.emplace(uri, nullptr);
	}
}




void Lv2Features::initCommon()
{
	Lv2Manager* man = Engine::getLv2Manager();
	// init m_featureByUri with the plugin-common features
	operator[](LV2_URID__map) = man->uridMap().mapFeature();
	operator[](LV2_URID__unmap) = man->uridMap().unmapFeature();
}




void Lv2Features::createFeatureVectors()
{
	// create vector of features
	for(const auto& [uri, feature] : m_featureByUri)
	{
		/*
			If pr.second is nullptr here, this means that the LV2_feature
			has no "data". If this happens here, this means
			* either that this feature is static
			  (e.g. LV2_BUF_SIZE__boundedBlockLength)
			* or that the programmer forgot to use operator[] before feature
			  vector creation (This can be done in
			  Lv2Proc::initPluginSpecificFeatures or in Lv2Features::initCommon)
		*/
		m_features.push_back(LV2_Feature{(const char*)uri.data(), (void*)feature});
	}

	// create pointer vector (for lilv_plugin_instantiate)
	m_featurePointers.reserve(m_features.size() + 1);
	for (const auto& feature : m_features)
	{
		m_featurePointers.push_back(&feature);
	}
	m_featurePointers.push_back(nullptr);
}




void *&Lv2Features::operator[](const char *featName)
{
	auto itr = m_featureByUri.find(featName);
	Q_ASSERT(itr != m_featureByUri.end());
	return itr->second;
}




void Lv2Features::clear()
{
	m_features.clear();
	for (auto& [uri, feature] : m_featureByUri)
	{
		(void) uri;
		feature = nullptr;
	}
}


} // namespace lmms

#endif // LMMS_HAVE_LV2

