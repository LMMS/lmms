/*
 * SpaManager.cpp - Implementation of SpaManager class
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

#include "SpaManager.h"

#ifdef LMMS_HAVE_SPA

#include <QDebug>
#include <QDir>
#include <QLibrary>
#include <spa/audio.h>
#include <spa/spa.h>

#include "ConfigManager.h"
#include "Plugin.h"
#include "PluginFactory.h"

SpaManager::SpaManager()
{
	// TODO: make common func? (LADSPA, SPA, LV2, ...)

	// Make sure plugin search paths are set up
	PluginFactory::setupSearchPaths();

	// TODO: "LADSPA"
	QStringList spaDirectories =
		QString(getenv("SPA_PATH")).split(LADSPA_PATH_SEPERATOR);
	spaDirectories += ConfigManager::inst()->spaDir().split(',');

	spaDirectories.push_back("plugins:spa");
	/*
		// nothing official yet:
	#ifndef LMMS_BUILD_WIN32
		spaDirectories.push_back( qApp->applicationDirPath() + '/' +
	LIB_DIR + "spa" ); spaDirectories.push_back( "/usr/lib/spa" );
		spaDirectories.push_back( "/usr/lib64/spa" );
		spaDirectories.push_back( "/usr/local/lib/spa" );
		spaDirectories.push_back( "/usr/local/lib64/spa" );
		spaDirectories.push_back( "/Library/Audio/Plug-Ins/SPA" );
	#endif*/

	auto addSinglePlugin = [this](const QString &absolutePath) {
		SpaInfo info;
		info.m_lib = new QLibrary(absolutePath);
		spa::descriptor_loader_t spaDescriptorLoader;

		if (info.m_lib->load())
		{
			spaDescriptorLoader =
				reinterpret_cast<spa::descriptor_loader_t>(
					info.m_lib->resolve(
						spa::descriptor_name));

			if (spaDescriptorLoader)
			{
				info.m_descriptor = (*spaDescriptorLoader)(
					0 /* = plugin number, TODO */);
				if (info.m_descriptor)
				{
					info.m_type = computePluginType(
						info.m_descriptor);
					if (info.m_type !=
						Plugin::PluginTypes::Undefined)
					{
						qDebug()
							<< "SpaManager: Adding "
							<< spa::unique_name(
								*info.m_descriptor)
								.c_str();
						info.m_path = absolutePath;
						m_spaInfoMap[spa::unique_name(
							*info.m_descriptor)] =
							std::move(info);
					}
				}
			}
			else
			{
				qWarning() << info.m_lib->errorString();
			}
		}
		else
		{
			qWarning() << info.m_lib->errorString();
		}
	};

	for (QStringList::iterator it = spaDirectories.begin();
		it != spaDirectories.end(); ++it)
	{
		QDir directory((*it));
		QFileInfoList list = directory.entryInfoList();
		for (QFileInfoList::iterator file = list.begin();
			file != list.end(); ++file)
		{
			const QFileInfo &f = *file;
			if (f.isFile() &&
				f.fileName().right(3).toLower() ==
#ifdef LMMS_BUILD_WIN32
					"dll"
#else
					".so"
#endif
			)
			{
				addSinglePlugin(f.absoluteFilePath());
			}
		}
	}
}

SpaManager::~SpaManager()
{
	for (std::pair<const std::string, SpaInfo> &pr : m_spaInfoMap)
	{
		pr.second.cleanup();
	}
}

Plugin::PluginTypes SpaManager::computePluginType(spa::descriptor *desc)
{
	spa::plugin *plug = desc->instantiate();

	struct TypeChecker final : public virtual spa::audio::visitor
	{
		std::size_t m_inCount = 0, m_outCount = 0;
		void visit(spa::audio::in &) override { ++m_inCount; }
		void visit(spa::audio::out &) override { ++m_outCount; }
		void visit(spa::audio::stereo::in &) override { ++++m_inCount; }
		void visit(spa::audio::stereo::out &) override
		{
			++++m_outCount;
		}
	} tyc;

	for (const spa::simple_str &portname : desc->port_names())
	{
		try
		{
			plug->port(portname.data()).accept(tyc);
		}
		catch (spa::port_not_found &)
		{
			return Plugin::PluginTypes::Undefined;
		}
	}

	delete plug;

	Plugin::PluginTypes res;
	if (tyc.m_inCount > 2 || tyc.m_outCount > 2)
	{
		res = Plugin::PluginTypes::Undefined;
	} // TODO: enable mono effects?
	else if (tyc.m_inCount == 2 && tyc.m_outCount == 2)
	{
		res = Plugin::PluginTypes::Effect;
	}
	else if (tyc.m_inCount == 0 && tyc.m_outCount == 2)
	{
		res = Plugin::PluginTypes::Instrument;
	}
	else
	{
		res = Plugin::PluginTypes::Other;
	}

	qDebug() << "Plugin type of " << spa::unique_name(*desc).c_str() << ":";
	qDebug() << (res == Plugin::PluginTypes::Undefined
			? "  undefined"
			: res == Plugin::PluginTypes::Effect
				? "  effect"
				: res == Plugin::PluginTypes::Instrument
					? "  instrument"
					: "  other");

	return res;
}

spa::descriptor *SpaManager::getDescriptor(const std::string &uniqueName)
{
	auto itr = m_spaInfoMap.find(uniqueName);
	return itr == m_spaInfoMap.end() ? nullptr : itr->second.m_descriptor;
}

spa::descriptor *SpaManager::getDescriptor(const QString uniqueName)
{
	return getDescriptor(std::string(uniqueName.toUtf8()));
}

void SpaManager::SpaInfo::cleanup()
{
	//m_descriptor->delete_self();
	delete m_descriptor;
	delete m_lib;
}

#endif // LMMS_HAVE_SPA
