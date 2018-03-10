/*
 * Lv2Manager.h - declaration of class Lv2Manager
 *                    a class to manage loading and instantiation
 *                    of LV2 plugins
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


#ifndef LV2_MANAGER_H
#define LV2_MANAGER_H

#include <QtCore/QString>
#include <QtCore/QVector>

#include "Lv2PluginInfo.h"

namespace Lilv {
	struct World;
}

// A singleton class to manage Lv2 plugins
class Lv2Manager
{
public:
	static Lv2Manager& getInstance()
	{
		static Lv2Manager instance;
		return instance;
	}

	QVector<Lv2PluginInfo*> getPlugins();

	//Lilv::Plugin* getPlugin();

private:
	Lv2Manager();
	Lv2Manager( Lv2Manager const& );
	void operator=( Lv2Manager const& );
	QVector<Lv2PluginInfo*> collection;
	Lilv::World* m_world;

	void scanPlugins();
};

#endif
