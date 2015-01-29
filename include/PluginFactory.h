/*
 * PluginFactory.h
 *
 * Copyright (c) 2015 Lukas W <lukaswhl/at/gmail.com>
 *
 * This file is part of LMMS - http://lmms.io
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

#ifndef PLUGINFACTORY_H
#define PLUGINFACTORY_H

#include <QtCore/QFileInfo>
#include <QtCore/QList>

#include "Plugin.h"

class QLibrary;

class PluginFactory
{
public:
	struct PluginInfo
	{
		QFileInfo file;
		QLibrary* library;
		Plugin::Descriptor* descriptor;

		bool isNull() const {return library == 0;}
	};
	typedef QList<PluginInfo> PluginInfoList;
	typedef QMultiMap<Plugin::PluginTypes, Plugin::Descriptor*> DescriptorMap;

	PluginFactory();
	~PluginFactory();

	/// Returns the singleton instance of PluginFactory. You won't need to call
	/// this directly, use pluginFactory instead.
	static PluginFactory* instance();

	/// Returns a list of all found plugins' descriptors.
	const Plugin::DescriptorList descriptors() const;
	const Plugin::DescriptorList descriptors(Plugin::PluginTypes type) const;

	/// Returns a list of all found plugins' PluginFactory::PluginInfo objects.
	const PluginInfoList& pluginInfos() const;

	/// Returns the PluginInfo object of the plugin with the given name.
	/// If the plugin is not found, an empty PluginInfo is returned (use
	/// PluginInfo::isNull() to check this).
	const PluginInfo pluginInfo(const char* name) const;

	/// When loading a library fails during discovery, the error string is saved.
	/// It can be retrieved by calling this function.
	QString errorString(QString pluginName) const;

public slots:
	void discoverPlugins();

private:
	DescriptorMap m_descriptors;
	PluginInfoList m_pluginInfos;

	QHash<QString, QString> m_errors;

	static PluginFactory* s_instance;
};

#define pluginFactory PluginFactory::instance()

#endif // PLUGINFACTORY_H
