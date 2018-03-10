/*
 * Lv2PluginInfo.h - Lv2 plugin metadata
 *
 * Copyright (c) 2018 Alexandros Theodotou @faiyadesu
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

#ifndef LV2_PLUGIN_H
#define LV2_PLUGIN_H

#include <QString>

namespace Lilv {
	struct Plugin;
}

// This class is a simple wrapper of Lilv Plugin
class Lv2PluginInfo
{
public:
	Lv2PluginInfo(Lilv::Plugin * plugin );
	Lv2PluginInfo(QString uri);
	virtual ~Lv2PluginInfo();

	inline QString getUri() const
	{
		return uri;
	}
	inline QString getName() const
	{
		return name;
	}
	inline QString getParentClass() const
	{
		return parentClass;
	}
	inline long getNumPorts() const
	{
		return numPorts;
	}
	inline QString getAuthorName() const
	{
		return authorName;
	}
	inline QString getAuthorEmail() const
	{
		return authorEmail;
	}
	inline QString getAuthorHomePage() const
	{
		return authorHomePage;
	}
	void debugPrint() const;

private:
	QString uri;
	QString name;
	QString parentClass;
	QString childClass;
	unsigned int numPorts;
	QString authorName;
	QString authorEmail;
	QString authorHomePage;
	Lilv::Plugin * m_raw_plugin;
};


#endif
