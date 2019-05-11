/*
 * PluginFactory.cpp
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

#include "PluginFactory.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QLibrary>
#include "lmmsconfig.h"

#include "ConfigManager.h"
#include "Plugin.h"
#include "embed.h"

#ifdef LMMS_BUILD_WIN32
	QStringList nameFilters("*.dll");
#else
	QStringList nameFilters("lib*.so");
#endif

qint64 qHash(const QFileInfo& fi)
{
	return qHash(fi.absoluteFilePath());
}

std::unique_ptr<PluginFactory> PluginFactory::s_instance;

PluginFactory::PluginFactory()
{
	setupSearchPaths();
	discoverPlugins();
}

PluginFactory::~PluginFactory()
{
}

void PluginFactory::setupSearchPaths()
{
	// Adds a search path relative to the main executable if the path exists.
	auto addRelativeIfExists = [](const QString & path) {
		QDir dir(qApp->applicationDirPath());
		if (!path.isEmpty() && dir.cd(path)) {
			QDir::addSearchPath("plugins", dir.absolutePath());
		}
	};

	// We're either running LMMS installed on an Unixoid or we're running a
	// portable version like we do on Windows.
	// We want to find our plugins in both cases:
	//  (a) Installed (Unix):
	//      e.g. binary at /usr/bin/lmms - plugin dir at /usr/lib/lmms/
	//  (b) Portable:
	//      e.g. binary at "C:/Program Files/LMMS/lmms.exe"
	//           plugins at "C:/Program Files/LMMS/plugins/"

#ifndef LMMS_BUILD_WIN32
	addRelativeIfExists("../lib/lmms"); // Installed
#endif
	addRelativeIfExists("plugins"); // Portable
#ifdef PLUGIN_DIR // We may also have received a relative directory via a define
	addRelativeIfExists(PLUGIN_DIR);
#endif
	// Or via an environment variable:
	QString env_path;
	if (!(env_path = qgetenv("LMMS_PLUGIN_DIR")).isEmpty())
		QDir::addSearchPath("plugins", env_path);

	QDir::addSearchPath("plugins", ConfigManager::inst()->workingDir() + "plugins");
}

PluginFactory* PluginFactory::instance()
{
	if (s_instance == nullptr)
		s_instance.reset(new PluginFactory());

	return s_instance.get();
}

const Plugin::DescriptorList PluginFactory::descriptors() const
{
	return m_descriptors.values();
}

const Plugin::DescriptorList PluginFactory::descriptors(Plugin::PluginTypes type) const
{
	return m_descriptors.values(type);
}

const PluginFactory::PluginInfoList& PluginFactory::pluginInfos() const
{
	return m_pluginInfos;
}

const PluginFactory::PluginInfoAndKey PluginFactory::pluginSupportingExtension(const QString& ext)
{
	return m_pluginByExt.value(ext, PluginInfoAndKey());
}

const PluginFactory::PluginInfo PluginFactory::pluginInfo(const char* name) const
{
	for (const PluginInfo& info : m_pluginInfos)
	{
		if (qstrcmp(info.descriptor->name, name) == 0)
			return info;
	}
	return PluginInfo();
}

QString PluginFactory::errorString(QString pluginName) const
{
	static QString notfound = qApp->translate("PluginFactory", "Plugin not found.");
	return m_errors.value(pluginName, notfound);
}

void PluginFactory::discoverPlugins()
{
	DescriptorMap descriptors;
	PluginInfoList pluginInfos;
	m_pluginByExt.clear();

	QSet<QFileInfo> files;
	for (const QString& searchPath : QDir::searchPaths("plugins"))
	{
		files.unite(QDir(searchPath).entryInfoList(nameFilters).toSet());
	}

	// Cheap dependency handling: zynaddsubfx needs ZynAddSubFxCore. By loading
	// all libraries twice we ensure that libZynAddSubFxCore is found.
	for (const QFileInfo& file : files)
	{
		QLibrary(file.absoluteFilePath()).load();
	}

	for (const QFileInfo& file : files)
	{
		auto library = std::make_shared<QLibrary>(file.absoluteFilePath());
		if (! library->load()) {
			m_errors[file.baseName()] = library->errorString();
			qWarning("%s", library->errorString().toLocal8Bit().data());
			continue;
		}

		Plugin::Descriptor* pluginDescriptor = nullptr;
		if (library->resolve("lmms_plugin_main"))
		{
			QString descriptorName = file.baseName() + "_plugin_descriptor";
			if( descriptorName.left(3) == "lib" )
			{
				descriptorName = descriptorName.mid(3);
			}

			pluginDescriptor = reinterpret_cast<Plugin::Descriptor*>(library->resolve(descriptorName.toUtf8().constData()));
			if(pluginDescriptor == nullptr)
			{
				qWarning() << qApp->translate("PluginFactory", "LMMS plugin %1 does not have a plugin descriptor named %2!").
							  arg(file.absoluteFilePath()).arg(descriptorName);
				continue;
			}
		}

		if(pluginDescriptor)
		{
			PluginInfo info;
			info.file = file;
			info.library = library;
			info.descriptor = pluginDescriptor;
			pluginInfos << info;

			auto addSupportedFileTypes =
				[this](QString supportedFileTypes,
					const PluginInfo& info,
					const Plugin::Descriptor::SubPluginFeatures::Key* key = nullptr)
			{
				if(!supportedFileTypes.isNull())
				{
					for (const QString& ext : supportedFileTypes.split(','))
					{
						//qDebug() << "Plugin " << info.name()
						//	<< "supports" << ext;
						PluginInfoAndKey infoAndKey;
						infoAndKey.info = info;
						infoAndKey.key = key
							? *key
							: Plugin::Descriptor::SubPluginFeatures::Key();
						m_pluginByExt.insert(ext, infoAndKey);
					}
				}
			};

			if (info.descriptor->supportedFileTypes)
				addSupportedFileTypes(QString(info.descriptor->supportedFileTypes), info);

			if (info.descriptor->subPluginFeatures)
			{
				Plugin::Descriptor::SubPluginFeatures::KeyList
					subPluginKeys;
				info.descriptor->subPluginFeatures->listSubPluginKeys(
					info.descriptor,
					subPluginKeys);
				for(const Plugin::Descriptor::SubPluginFeatures::Key& key
					: subPluginKeys)
				{
					addSupportedFileTypes(key.additionalFileExtensions(), info, &key);
				}
			}

			descriptors.insert(info.descriptor->type, info.descriptor);
		}
	}

	m_pluginInfos = pluginInfos;
	m_descriptors = descriptors;
}



const QString PluginFactory::PluginInfo::name() const
{
	return descriptor ? descriptor->name : QString();
}
