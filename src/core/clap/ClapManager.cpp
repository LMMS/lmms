/*
 * ClapManager.cpp - Implementation of ClapManager class
 *
 * Copyright (c) 2023 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include "ClapManager.h"

#ifdef LMMS_HAVE_CLAP

#include <algorithm>
#include <filesystem>
#include <string_view>

#include <cstdlib>
#include <cstring>

#include <QDebug>
#include <QElapsedTimer>
#include <QLibrary>

#if defined(LMMS_BUILD_LINUX) || defined(LMMS_BUILD_APPLE)
#include <QDir>
#endif

#include <clap/clap.h>

#include "ClapFile.h"
#include "Engine.h"
#include "Plugin.h"
#include "PluginIssue.h"
#include "lmmsversion.h"

namespace lmms
{

namespace
{
	inline std::string expandHomeDir(std::string_view dir)
	{
#if defined(LMMS_BUILD_LINUX) || defined(LMMS_BUILD_APPLE)
		if (!dir.empty() && dir[0] == '~')
		{
			dir.remove_prefix(1);
			return QDir::home().absolutePath().toStdString() + std::string{dir};
		}
#endif
		return std::string{dir};
	}
}

bool ClapManager::kDebug = false;

ClapManager::ClapManager()
{
	const char* dbgStr = getenv("LMMS_CLAP_DEBUG");
	kDebug = (dbgStr && *dbgStr);
}

ClapManager::~ClapManager()
{
	if (ClapManager::kDebug)
		qDebug() << "ClapManager::~ClapManager()";

	// Deactivate and destroy plugin instances first
	m_instances.clear();

	// Then deinit the .clap files and unload the shared libraries
	m_clapFiles.clear();
}

void ClapManager::initPlugins()
{
	findSearchPaths();
	if (kDebug)
		qDebug() << "Found .clap files:";
	findClapFiles(getSearchPaths());
}

void ClapManager::findSearchPaths()
{
	// FROM CLAP DOCUMENTATION:
	// CLAP plugins standard search path:
	//
	// Linux
	//   - ~/.clap
	//   - /usr/lib/clap
	//
	// Windows
	//   - %COMMONPROGRAMFILES%\CLAP
	//   - %LOCALAPPDATA%\Programs\Common\CLAP
	//
	// MacOS
	//   - /Library/Audio/Plug-Ins/CLAP
	//   - ~/Library/Audio/Plug-Ins/CLAP
	//
	// In addition to the OS-specific default locations above, a CLAP host must query the environment
	// for a CLAP_PATH variable, which is a list of directories formatted in the same manner as the host
	// OS binary search path (PATH on Unix, separated by `:` and Path on Windows, separated by ';', as
	// of this writing).
	//
	// Each directory should be recursively searched for files and/or bundles as appropriate in your OS
	// ending with the extension `.clap`.

	m_searchPaths.clear();

	// Get CLAP_PATH paths
	if (const char* clapPathTemp = getenv("CLAP_PATH"))
	{
		std::error_code ec;
		std::string_view clapPath{clapPathTemp};
		auto pos = clapPath.find(LADSPA_PATH_SEPERATOR);
		while (pos != std::string_view::npos)
		{
			std::filesystem::path path{expandHomeDir(clapPath.substr(0, pos))};
			if (std::filesystem::is_directory(path, ec))
				m_searchPaths.emplace_back(std::move(path.make_preferred()));
			clapPath = clapPath.substr(pos + 1);
			pos = clapPath.find(LADSPA_PATH_SEPERATOR);
		}
		if (!clapPath.empty())
		{
			std::filesystem::path path{expandHomeDir(clapPath)};
			if (std::filesystem::is_directory(path, ec))
				m_searchPaths.emplace_back(std::move(path.make_preferred()));
		}
	}

	// Add OS-dependent search paths
#ifdef LMMS_BUILD_LINUX
	std::filesystem::path path{expandHomeDir("~/.clap")};
	std::error_code ec;
	if (std::filesystem::is_directory(path, ec))
		m_searchPaths.emplace_back(std::move(path.make_preferred()));
	path = "/usr/lib/clap";
	if (std::filesystem::is_directory(path, ec))
		m_searchPaths.emplace_back(std::move(path.make_preferred()));
#elif defined(LMMS_BUILD_WIN32) || defined(LMMS_BUILD_WIN64)
	std::error_code ec;
	if (auto commonProgFiles = getenv("COMMONPROGRAMFILES"))
	{
		std::filesystem::path path{commonProgFiles};
		path /= "CLAP";
		if (std::filesystem::is_directory(path, ec))
			m_searchPaths.emplace_back(std::move(path.make_preferred()));
	}
	if (auto localAppData = getenv("LOCALAPPDATA"))
	{
		std::filesystem::path path{localAppData};
		path /= std::filesystem::path{"Programs/Common/CLAP"};
		if (std::filesystem::is_directory(path, ec))
			m_searchPaths.emplace_back(std::move(path.make_preferred()));
	}
#elif defined(LMMS_BUILD_APPLE)
	std::error_code ec;
	std::filesystem::path path{"/Library/Audio/Plug-Ins/CLAP"};
	if (std::filesystem::is_directory(path, ec))
		m_searchPaths.emplace_back(std::move(path.make_preferred()));
	path = expandHomeDir("~/Library/Audio/Plug-Ins/CLAP");
	if (std::filesystem::is_directory(path, ec))
		m_searchPaths.emplace_back(std::move(path.make_preferred()));
#endif

	if (kDebug)
	{
		qDebug() << "CLAP search paths:";
		for (const auto& path : m_searchPaths)
		{
			qDebug() << "-" << path.c_str();
		}
	}
}

void ClapManager::findClapFiles(const std::vector<std::filesystem::path>& searchPaths)
{
	if (!m_clapFiles.empty())
		return; // Cannot unload CLAP plugins yet

	m_instances.clear();
	m_clapFiles.clear();
	m_uriToPluginInfo.clear();

	QElapsedTimer timer;
	timer.start();

	// Search searchPaths for files with ".clap" extension
	int totalClapFiles = 0, totalClapPlugins = 0;
	for (const auto& path : searchPaths)
	{
		for (const auto& entry : std::filesystem::recursive_directory_iterator(path))
		{
			auto entryPath = entry.path();
			if (entryPath.extension() != ".clap")
				continue;

			++totalClapFiles;

			if (kDebug)
				qDebug() << "-" << entryPath.c_str();

			auto& clapFile = m_clapFiles.emplace_back(this, std::move(entryPath));
			if (!clapFile.isValid())
			{
				qWarning() << "Failed to load .clap file";
				m_clapFiles.pop_back(); // Remove/unload invalid clap file
				continue;
			}

			totalClapPlugins += clapFile.getPluginCount();
			for (const auto& plugin : clapFile.getPluginInfo())
			{
				m_pluginInfo.push_back(&plugin);
				m_uriToPluginInfo.emplace(std::string{plugin.getDescriptor()->id}, &plugin); // TODO: Does this pointer remain valid after clapFile is moved?
			}
		}
	}

	qDebug() << "CLAP plugin SUMMARY:"
		<< m_clapFiles.size() << "of" << totalClapFiles << ".clap files containing"
		<< m_pluginInfo.size() << "of" << totalClapPlugins
		<< "CLAP plugins loaded in" << timer.elapsed() << "msecs.";
	if (m_clapFiles.size() != totalClapFiles || m_pluginInfo.size() != totalClapPlugins)
	{
		if (kDebug)
		{
			qDebug() <<
				"If you don't want to see all this debug output, please set\n"
				"  environment variable \"LMMS_CLAP_DEBUG\" to empty or\n"
				"  do not set it.";
		}
		else
		{
			qDebug() <<
				"For details about not loaded plugins, please set\n"
				"  environment variable \"LMMS_CLAP_DEBUG\" to nonempty.";
		}
	}
}

void ClapManager::unload(ClapFile* file)
{
	// This method deactivates and destroys any plugins from the given file,
	// then clears any values left in the cache, then deinits and unloads
	// the .clap shared library.

	// TODO: Could be dangerous to call this method if LMMS is not shutting down
/*
	// Clear from m_pluginInfo cache
	m_pluginInfo.erase(std::remove_if(
		m_pluginInfo.begin(), m_pluginInfo.end(),
		[file](const ClapPluginInfo* pi){
			return pi->getFile() == file;
		}
	));

	// Clear from m_uriToPluginInfo cache
	m_uriToPluginInfo.erase(std::remove_if(
		m_uriToPluginInfo.begin(), m_uriToPluginInfo.end(),
		[file](const auto& mapPair){
			return mapPair->second->getFile() == file;
		}
	));

	// Destroy CLAP instances then remove from m_instances.
	// ClapInstance destructor deactivates and destroys plugin instances
	m_instances.erase(std::remove_if(
		m_instances.begin(), m_instances.end(),
		[file](ClapInstance& inst) -> bool {
			return inst.getPluginInfo().getFile() == file;
		}
	));

	// Remove CLAP file from vector.
	// ClapFile destructor handles cleanup and unloads shared library
	m_clapFiles.erase(std::remove(m_clapFiles.begin(), m_clapFiles.end(), file));
	*/
}

auto ClapManager::getPluginInfo(const std::string& uri) -> const ClapPluginInfo*
{
	const auto iter = m_uriToPluginInfo.find(uri);
	return iter != m_uriToPluginInfo.end() ? iter->second : nullptr;
}

auto ClapManager::getPluginInfo(const QString& uri) -> const ClapPluginInfo*
{
	return getPluginInfo(uri.toStdString());
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
