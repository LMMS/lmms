/*
 * EmbeddedPluginFactory.h
 *
 * Copyright (c) 2015 Lukas W <lukaswhl/at/gmail.com>
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

#ifndef EMBEDDED_PLUGINFACTORY_H
#define EMBEDDED_PLUGINFACTORY_H

#include <memory>

#include <QtCore/QFileInfo>
#include <QtCore/QHash>
#include <QtCore/QList>

#include "export.h"
#include "Plugin.h"

class QLibrary;

class EXPORT EmbeddedPluginFactory
{
public:
	struct PluginInfo
	{
		PluginInfo() : library(nullptr), descriptor(nullptr) {}

		const QString name() const;
		QFileInfo file;
		std::shared_ptr<QLibrary> library;
		Plugin::Descriptor* descriptor;

		bool isNull() const {return ! library;}
	};
	typedef QList<PluginInfo> PluginInfoList;
	typedef QMultiMap<Plugin::PluginType, Plugin::Descriptor*> DescriptorMap;

	EmbeddedPluginFactory();
	~EmbeddedPluginFactory();

	/// Returns the singleton instance of EmbeddedPluginFactory. You won't need to call
	/// this directly, use pluginFactory instead.
	static EmbeddedPluginFactory* instance();

	/// Returns a list of all found plugins' descriptors.
	const Plugin::DescriptorList descriptors() const;
	const Plugin::DescriptorList descriptors(Plugin::PluginType type) const;

	/// Returns a list of all found plugins' EmbeddedPluginFactory::PluginInfo objects.
	const PluginInfoList& pluginInfos() const;
	/// Returns a plugin that support the given file extension
	const PluginInfo pluginSupportingExtension(const QString& ext);

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
	QMap<QString, PluginInfo> m_pluginByExt;

	QHash<QString, QString> m_errors;

	static std::unique_ptr<EmbeddedPluginFactory> s_instance;
};

#define pluginFactory EmbeddedPluginFactory::instance()

#endif // EMBEDDED_PLUGINFACTORY_H
