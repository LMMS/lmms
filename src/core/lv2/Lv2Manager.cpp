/*
 * Lv2Manager.cpp - Implementation of Lv2Manager class
 *
 * Copyright (c) 2018-2019 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#include "Lv2Manager.h"

#ifdef LMMS_HAVE_LV2

#include <QDebug>
#include <QDir>
#include <QLibrary>
#include <lv2.h>

#include <lilv/lilv.h>

#include "ConfigManager.h"
#include "Plugin.h"
#include "PluginFactory.h"
#include "Lv2ControlBase.h"
#include "PluginIssue.h"




Lv2Manager::Lv2Manager()
{
	m_world = lilv_world_new();
	lilv_world_load_all(m_world);
}




Lv2Manager::~Lv2Manager()
{
}




const LilvPlugin *Lv2Manager::getPlugin(const std::string &uri)
{
	auto itr = m_lv2InfoMap.find(uri);
	return itr == m_lv2InfoMap.end() ? nullptr : itr->second.plugin();
}




const LilvPlugin *Lv2Manager::getPlugin(const QString uri)
{
	return getPlugin(std::string(uri.toUtf8()));
}




void Lv2Manager::initPlugins()
{
	const LilvPlugins* plugins = lilv_world_get_all_plugins(m_world);

	LILV_FOREACH(plugins, itr, plugins)
	{
		const LilvPlugin* curPlug = lilv_plugins_get(plugins, itr);

		std::vector<PluginIssue> issues;
		Plugin::PluginTypes type = Lv2ControlBase::check(curPlug, issues, true);
		Lv2Info info(curPlug, type, issues.empty());

		m_lv2InfoMap[lilv_node_as_uri(lilv_plugin_get_uri(curPlug))]
			= std::move(info);
	}
}




// unused + untested yet
bool Lv2Manager::isSubclassOf(const LilvPluginClass* clvss, const char* uriStr)
{
	const LilvPluginClasses* allClasses = lilv_world_get_plugin_classes(m_world);
	const LilvPluginClass* root = lilv_world_get_plugin_class(m_world);
	const LilvPluginClass* gen = lilv_plugin_classes_get_by_uri(allClasses,
					uri(uriStr).get());

	// lv2:Generator is what can be generating an LMMS instrument track
	// lv2:Instrument is lv:Generator with MIDI/piano input
	// => LMMS "Instrument" corresponds to lv2:Generator
	auto clssEq = [](const LilvPluginClass* pc1,
		const LilvPluginClass* pc2) -> bool
	{
		return lilv_node_equals(
			lilv_plugin_class_get_uri(pc1),
			lilv_plugin_class_get_uri(pc2));
	};
	bool isGen = false;
	for (;
		clssEq(clvss, root) && (isGen = clssEq(clvss, gen));
		clvss = lilv_plugin_classes_get_by_uri(allClasses,
			lilv_plugin_class_get_parent_uri(clvss))
	) ;
	return isGen;
}




#endif // LMMS_HAVE_LV2
