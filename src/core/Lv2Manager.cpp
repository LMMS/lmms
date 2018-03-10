/*
 * Lv2Manager.cpp - definition of class Lv2Manager
 *						a class to manage loading and instantiation
 *						of LV2 plugins
 *
 * Copyright (c) Alexandros Theodotou @faiyadesu
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

#include <lilv/lilvmm.hpp>
#include <QtCore>

#include "Lv2Manager.h"

Lv2Manager::Lv2Manager()
{
	// TODO add host lv2 features
	m_host_features = new LV2_Feature*[3];
	m_host_features[0] = new LV2_Feature {
		"http://lv2plug.in/ns/ext/urid#unmap", nullptr };
	m_host_features[1] = new LV2_Feature {
		"http://lv2plug.in/ns/ext/worker#schedule", nullptr };
	m_host_features[2] = NULL;
	scanPlugins();
}

//Lilv::Plugin getPlugin(const Lv2PluginInfo& _pi)
//{
	//m_worl
//}

void Lv2Manager::scanPlugins()
{
	collection.clear();
	m_world = new Lilv::World();
	m_world->load_all();
	Lilv::Plugins plugins = m_world->get_all_plugins();
	LilvIter * iter =  plugins.begin();
	do {
		Lilv::Plugin* raw_plugin = new Lilv::Plugin(plugins.get(iter));
		Lv2PluginInfo * plugin = new Lv2PluginInfo( raw_plugin );
		collection.append( plugin );
	} while ((iter = plugins.next(iter)) != nullptr);
}

QVector<Lv2PluginInfo*> Lv2Manager::getPlugins()
{
	if (collection.size() == 0)
	{
		scanPlugins();
	}
	return collection;
}

//Lilv::Plugin Lv2Manager::getPlugin()
//{

//}

	//Lilv::Node name = plugin.get_name();
	//Lilv::PluginClass plugin_class = plugin.get_class();
	//if (QString::compare(plugin_class.get_label().as_string(), "Instrument") == 0)
	//{
		//m_treeWidget->addTopLevelItem( new Lv2PluginInfoItem(
		//QString(name.as_string()), QString( plugin.get_uri().as_string()) ) );
	//}

LV2_Feature* const* Lv2Manager::getHostFeatures()
{
	return const_cast<LV2_Feature* const*>(m_host_features);
}

