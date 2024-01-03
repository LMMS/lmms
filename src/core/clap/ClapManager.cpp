/*
 * ClapManager.cpp - Implementation of ClapManager class
 *
 * Copyright (c) 2024 Dalton Messmer <messmer.dalton/at/gmail.com>
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
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <string>

#include "ClapLog.h"
#include "ClapTransport.h"
#include "lmmsversion.h"

namespace lmms
{

namespace
{
	auto expandHomeDir(std::string_view dir) -> fs::path
	{
#if defined(LMMS_BUILD_LINUX) || defined(LMMS_BUILD_APPLE)
		if (!dir.empty() && dir[0] == '~')
		{
			if (auto home = std::getenv("HOME"))
			{
				const auto pos = dir.find_first_not_of(R"(/\)", 1);
				if (pos == std::string_view::npos) { return home; }
				dir.remove_prefix(pos);
				return fs::path{home} / dir;
			}
		}
#endif
		return dir;
	}
} // namespace

ClapManager::ClapManager()
{
	const char* debug = std::getenv("LMMS_CLAP_DEBUG");
	s_debugging = debug && *debug;
	ClapLog::plainLog(CLAP_LOG_DEBUG, "CLAP host debugging enabled");
}

ClapManager::~ClapManager()
{
	ClapLog::plainLog(CLAP_LOG_DEBUG, "ClapManager::~ClapManager()");

	// NOTE: All plugin instances need to be deactivated and destroyed first

	// Deinit the .clap files and unload the shared libraries
	m_files.clear();

	ClapLog::plainLog(CLAP_LOG_DEBUG, "ClapManager::~ClapManager() end");
}

void ClapManager::initPlugins()
{
	findSearchPaths();
	if (debugging())
	{
		ClapLog::plainLog("CLAP search paths:");
		for (const auto& searchPath : m_searchPaths)
		{
			std::string msg = "-" + searchPath.string();
			ClapLog::plainLog(msg);
		}
		ClapLog::plainLog("Found .clap files:");
	}
	loadClapFiles(searchPaths());
}

void ClapManager::findSearchPaths()
{
	m_searchPaths.clear();

	// Parses a string of paths, adding results to m_searchPaths
	auto parsePaths = [this](const char* pathString) {
		if (!pathString) { return; }
		auto paths = std::string_view{pathString};
		std::size_t pos = 0;
		do
		{
			if (paths.size() <= pos) { break; }
			paths.remove_prefix(pos);
			pos = paths.find(LADSPA_PATH_SEPERATOR);
			if (pos == 0) { continue; }
			auto path = expandHomeDir(paths.substr(0, pos));
			if (std::error_code ec; fs::is_directory(path, ec))
			{
				path = fs::canonical(path.make_preferred(), ec);
				if (ec) { continue; }
				m_searchPaths.emplace(std::move(path));
			}
		} while (pos++ != std::string_view::npos);
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
		m_searchPaths.emplace(std::move(path));
	}
	path = "/usr/lib/clap";
	if (fs::is_directory(path, ec))
	{
		m_searchPaths.emplace(std::move(path));
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
			m_searchPaths.emplace(std::move(path.make_preferred()));
		}
	}
	if (auto localAppData = std::getenv("LOCALAPPDATA"))
	{
		auto path = fs::path{localAppData} / "Programs/Common/CLAP";
		if (fs::is_directory(path, ec))
		{
			m_searchPaths.emplace(std::move(path.make_preferred()));
		}
	}
#elif defined(LMMS_BUILD_APPLE)
	// /Library/Audio/Plug-Ins/CLAP
	// ~/Library/Audio/Plug-Ins/CLAP
	std::error_code ec;
	auto path = fs::path{"/Library/Audio/Plug-Ins/CLAP"};
	if (fs::is_directory(path, ec))
	{
		m_searchPaths.emplace(std::move(path));
	}
	path = expandHomeDir("~/Library/Audio/Plug-Ins/CLAP");
	if (fs::is_directory(path, ec))
	{
		m_searchPaths.emplace(std::move(path));
	}
#endif
}

void ClapManager::loadClapFiles(const UniquePaths& searchPaths)
{
	if (!m_files.empty()) { return; } // Cannot unload CLAP plugins yet

	m_files.clear();
	m_uriMap.clear();
	m_pluginInfo.clear();

	const auto startTime = std::chrono::steady_clock::now();

	// Search `searchPaths` for files with ".clap" extension
	int totalClapFiles = 0;
	int totalPlugins = 0;
	for (const auto& path : searchPaths)
	{
		for (const auto& entry : fs::recursive_directory_iterator{path})
		{
			const auto& entryPath = entry.path();
			std::error_code ec;
			// NOTE: Using is_regular_file() free function workaround due to std::experimental::filesystem
			if (!fs::is_regular_file(entry, ec) || entryPath.extension() != ".clap")
			{
				continue;
			}

			++totalClapFiles;

			if (debugging())
			{
				std::string msg = "\n\n~~~CLAP FILE~~~\nfilename: ";
				msg += entryPath.string();
				ClapLog::plainLog(msg);
			}

			auto& file = m_files.emplace_back(std::make_unique<ClapFile>(entryPath));
			if (!file || !file->load())
			{
				std::string msg = "Failed to load '";
				msg += entryPath.string();
				msg += "'";
				ClapLog::globalLog(CLAP_LOG_ERROR, msg);
				m_files.pop_back(); // Remove/unload invalid clap file
				continue;
			}

			totalPlugins += file->pluginCount();
			for (auto& plugin : file->pluginInfo(ClapFile::AccessKey{}))
			{
				assert(plugin.has_value());
				const bool added = m_uriMap.emplace(std::string{plugin->descriptor().id}, *plugin).second;
				if (!added)
				{
					if (debugging())
					{
						std::string msg = "Plugin ID '";
						msg += plugin->descriptor().id;
						msg += "' in the plugin file '";
						msg += entryPath.string();
						msg += "' is identical to an ID from a previously loaded plugin file.";
						ClapLog::globalLog(CLAP_LOG_INFO, msg);
						ClapLog::globalLog(CLAP_LOG_INFO, "Skipping the duplicate plugin");
					}
					plugin.reset(); // invalidate duplicate plugin
					continue;
				}

				m_pluginInfo.push_back(&plugin.value());
			}
		}
	}

	{
		const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - startTime);
		std::string msg = "CLAP plugin SUMMARY: ";
		msg += std::to_string(m_pluginInfo.size()) + " out of " + std::to_string(totalPlugins);
		msg += " plugins in " + std::to_string(m_files.size()) + " out of " + std::to_string(totalClapFiles);
		msg += " plugin files loaded in " + std::to_string(elapsed.count()) + " msecs.";
		ClapLog::plainLog(msg);
	}

	if (debugging())
	{
		ClapLog::plainLog(
			"If you don't want to see all this debug output, please set\n"
			"  environment variable \"LMMS_CLAP_DEBUG\" to empty or\n"
			"  do not set it.");
	}
	else if (m_files.size() != totalClapFiles || m_pluginInfo.size() != totalPlugins)
	{
		ClapLog::plainLog("For details about not loaded plugins, please set\n"
			"  environment variable \"LMMS_CLAP_DEBUG\" to nonempty.");
	}
}

auto ClapManager::pluginInfo(const std::string& uri) const -> const ClapPluginInfo*
{
	const auto iter = m_uriMap.find(uri);
	return iter != m_uriMap.end() ? &iter->second : nullptr;
}

auto ClapManager::pluginInfo(const QString& uri) const -> const ClapPluginInfo*
{
	return pluginInfo(uri.toStdString());
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
