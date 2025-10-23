/*
 * PluginFactory.h
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

#ifndef LMMS_PLUGIN_FACTORY_H
#define LMMS_PLUGIN_FACTORY_H

#include <memory>
#include <string>
#include <vector>

#include <QFileInfo>
#include <QList>
#include <QString>

#include "lmms_export.h"
#include "Plugin.h"

class QLibrary;  // IWYU pragma: keep

namespace lmms
{

class LMMS_EXPORT PluginFactory
{
public:
	struct PluginInfo
	{
		QString name() const;
		QFileInfo file;
		std::shared_ptr<QLibrary> library = nullptr;
		Plugin::Descriptor* descriptor = nullptr;

		bool isNull() const {return ! library;}
	};
	using PluginInfoList = QList<PluginInfo>;
	using DescriptorMap = QMultiMap<Plugin::Type, Plugin::Descriptor*>;

	PluginFactory();
	~PluginFactory() = default;

	static void setupSearchPaths();
	static QList<QRegularExpression> getExcludePatterns(const char* envVar);

	/// Returns the singleton instance of PluginFactory. You won't need to call
	/// this directly, use pluginFactory instead.
	static PluginFactory* instance();

	/// Returns a list of all found plugins' descriptors.
	Plugin::DescriptorList descriptors() const;
	Plugin::DescriptorList descriptors(Plugin::Type type) const;

	struct PluginInfoAndKey
	{
		PluginInfo info;
		Plugin::Descriptor::SubPluginFeatures::Key key;
		bool isNull() const { return info.isNull(); }
	};

	/// Returns a list of all found plugins' PluginFactory::PluginInfo objects.
	const PluginInfoList& pluginInfos() const;
	/// Returns a list of all file extensions supported by any plugin
	QStringList allSupportedExtensions();
	/// Returns a plugin that support the given file extension
	PluginInfoAndKey pluginSupportingExtension(const QString& ext);

	/// Returns the PluginInfo object of the plugin with the given name.
	/// If the plugin is not found, an empty PluginInfo is returned (use
	/// PluginInfo::isNull() to check this).
	PluginInfo pluginInfo(const char* name) const;

	/// When loading a library fails during discovery, the error string is saved.
	/// It can be retrieved by calling this function.
	QString errorString(QString pluginName) const;

public slots:
	void discoverPlugins();

private:
	DescriptorMap m_descriptors;
	PluginInfoList m_pluginInfos;

	QMap<QString, PluginInfoAndKey> m_pluginByExt;
	std::vector<std::string> m_garbage; //!< cleaned up at destruction

	QHash<QString, QString> m_errors;

	static std::unique_ptr<PluginFactory> s_instance;

	static void filterPlugins(QSet<QFileInfo>& files);
};

//Short-hand function
LMMS_EXPORT PluginFactory* getPluginFactory();


} // namespace lmms

#endif // LMMS_PLUGIN_FACTORY_H
