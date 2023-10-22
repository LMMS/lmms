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
#include "ClapTransport.h"
#include "Engine.h"
#include "Song.h"
#include "Plugin.h"
#include "PluginIssue.h"
#include "lmmsversion.h"

namespace lmms
{

namespace
{
	auto expandHomeDir(std::string_view dir) -> std::filesystem::path
	{
#if defined(LMMS_BUILD_LINUX) || defined(LMMS_BUILD_APPLE)
		if (!dir.empty() && dir[0] == '~')
		{
			dir.remove_prefix(1);
			return QDir::home().absolutePath().toStdString().append(dir.data(), dir.size());
		}
#endif
		return std::filesystem::path{dir};
	}
}

bool ClapManager::s_debug = false;

ClapManager::ClapManager()
{
	const char* dbgStr = std::getenv("LMMS_CLAP_DEBUG");
	s_debug = (dbgStr && *dbgStr);
	if (s_debug) { qDebug() << "CLAP host debugging enabled"; }
}

ClapManager::~ClapManager()
{
	qDebug() << "ClapManager::~ClapManager";
	// Deactivate and destroy plugin instances first
	//m_instances.clear();

	// Then deinit the .clap files and unload the shared libraries
	m_files.clear();
	qDebug() << "ClapManager::~ClapManager end";
}

void ClapManager::initPlugins()
{
	findSearchPaths();
	if (debugging())
	{
		qDebug() << "CLAP search paths:";
		for (const auto& searchPath : m_searchPaths)
		{
			qDebug() << "-" << searchPath.c_str();
		}
		qDebug() << "Found .clap files:";
	}
	loadClapFiles(getSearchPaths());
}

void ClapManager::findSearchPaths()
{
	namespace fs = std::filesystem;
	m_searchPaths.clear();

	// Parses a string of paths, adding results to m_searchPaths
	auto parsePaths = [this](const char* pathString) {
		if (!pathString) { return; }
		std::error_code ec;
		auto paths = std::string_view{pathString};
		auto pos = paths.find(LADSPA_PATH_SEPERATOR);
		while (pos != std::string_view::npos)
		{
			auto path = expandHomeDir(paths.substr(0, pos));
			if (fs::is_directory(path, ec))
			{
				m_searchPaths.emplace_back(std::move(path.make_preferred()));
			}
			paths = paths.substr(pos + 1);
			pos = paths.find(LADSPA_PATH_SEPERATOR);
		}
		if (!paths.empty())
		{
			auto path = expandHomeDir(paths);
			if (fs::is_directory(path, ec))
			{
				m_searchPaths.emplace_back(std::move(path.make_preferred()));
			}
		}
	};

	// Use LMMS_CLAP_PATH to override all of CLAP's default search paths
	if (auto paths = std::getenv("LMMS_CLAP_PATH"))
	{
		parsePaths(paths);
		return;
	}

	// Get CLAP_PATH paths
	parsePaths(std::getenv("CLAP_PATH"));

	// Add OS-dependent search paths
#ifdef LMMS_BUILD_LINUX
	// ~/.clap
	// /usr/lib/clap
	std::error_code ec;
	auto path = expandHomeDir("~/.clap");
	if (fs::is_directory(path, ec))
	{
		m_searchPaths.emplace_back(std::move(path.make_preferred()));
	}
	path = "/usr/lib/clap";
	if (fs::is_directory(path, ec))
	{
		m_searchPaths.emplace_back(std::move(path.make_preferred()));
	}
#elif defined(LMMS_BUILD_WIN32) || defined(LMMS_BUILD_WIN64)
	// %COMMONPROGRAMFILES%\CLAP
	// %LOCALAPPDATA%\Programs\Common\CLAP
	std::error_code ec;
	if (auto commonProgFiles = std::getenv("COMMONPROGRAMFILES"))
	{
		auto path = fs::path{commonProgFiles} / "CLAP";
		if (fs::is_directory(path, ec))
		{
			m_searchPaths.emplace_back(std::move(path.make_preferred()));
		}
	}
	if (auto localAppData = std::getenv("LOCALAPPDATA"))
	{
		auto path = fs::path{localAppData} / "Programs/Common/CLAP";
		if (fs::is_directory(path, ec))
		{
			m_searchPaths.emplace_back(std::move(path.make_preferred()));
		}
	}
#elif defined(LMMS_BUILD_APPLE)
	// /Library/Audio/Plug-Ins/CLAP
	// ~/Library/Audio/Plug-Ins/CLAP
	std::error_code ec;
	auto path = fs::path{"/Library/Audio/Plug-Ins/CLAP"};
	if (fs::is_directory(path, ec))
	{
		m_searchPaths.emplace_back(std::move(path.make_preferred()));
	}
	path = expandHomeDir("~/Library/Audio/Plug-Ins/CLAP");
	if (fs::is_directory(path, ec))
	{
		m_searchPaths.emplace_back(std::move(path.make_preferred()));
	}
#endif
}

void ClapManager::loadClapFiles(const std::vector<std::filesystem::path>& searchPaths)
{
	if (!m_files.empty()) { return; } // Cannot unload CLAP plugins yet

	//m_instances.clear();
	m_files.clear();
	m_uriToPluginInfo.clear();
	m_pluginInfo.clear();

	QElapsedTimer timer; // TODO: Use <chrono> library
	timer.start();

	// Search searchPaths for files with ".clap" extension
	int totalClapFiles = 0;
	int totalClapPlugins = 0;
	for (const auto& path : searchPaths)
	{
		for (const auto& entry : std::filesystem::recursive_directory_iterator{path})
		{
			const auto& entryPath = entry.path();
			std::error_code ec;
			if (!entry.is_regular_file(ec) || entryPath.extension() != ".clap")
			{
				continue;
			}

			++totalClapFiles;

			if (debugging()) { qDebug() << "\n\n~~~CLAP FILE~~~\nfilename:" << entryPath.c_str(); }

			auto& clapFile = m_files.emplace_back(entryPath);
			clapFile.load();
			if (!clapFile.isValid())
			{
				qWarning() << "Failed to load .clap file";
				m_files.pop_back(); // Remove/unload invalid clap file
				continue;
			}

			bool purgeNeeded = false;
			totalClapPlugins += clapFile.pluginCount();
			for (const auto& plugin : clapFile.pluginInfo())
			{
				auto [_, added] = m_uriToPluginInfo.emplace(std::string{plugin->descriptor()->id}, std::weak_ptr{plugin});
				if (!added)
				{
					if (debugging())
					{
						qDebug().nospace() << "The CLAP plugin ID '" << plugin->descriptor()->id
							<< "' in the plugin file '" << entry.path().c_str() << "' is identical to an ID"
							<< " in a previously loaded plugin file. Skipping the duplicate CLAP plugin.";
					}
					plugin->invalidate();
					purgeNeeded = true;
					continue;
				}
				m_pluginInfo.push_back(plugin);
			}

			if (purgeNeeded) { clapFile.purgeInvalidPlugins(); }
		}
	}

	qDebug() << "CLAP plugin SUMMARY:"
		<< m_files.size() << "of" << totalClapFiles << "files and"
		<< m_pluginInfo.size() << "of" << totalClapPlugins
		<< "plugins loaded in" << timer.elapsed() << "msecs.";
	if (m_files.size() != totalClapFiles || m_pluginInfo.size() != totalClapPlugins)
	{
		if (debugging())
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

auto ClapManager::clapGuiApi() -> const char*
{
#if defined(LMMS_BUILD_LINUX)
	return CLAP_WINDOW_API_X11;
#elif defined(LMMS_BUILD_WIN32) || defined(LMMS_BUILD_WIN64)
	return CLAP_WINDOW_API_WIN32;
#elif defined(LMMS_BUILD_APPLE)
	return CLAP_WINDOW_API_COCOA;
#else
	// Unsupported platform
	return nullptr;
#endif
}

auto ClapManager::pluginInfo(const std::string& uri) -> std::weak_ptr<const ClapPluginInfo>
{
	const auto iter = m_uriToPluginInfo.find(uri);
	return iter != m_uriToPluginInfo.end() ? iter->second : std::weak_ptr<const ClapPluginInfo>{};
}

auto ClapManager::pluginInfo(const QString& uri) -> std::weak_ptr<const ClapPluginInfo>
{
	return pluginInfo(uri.toStdString());
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
