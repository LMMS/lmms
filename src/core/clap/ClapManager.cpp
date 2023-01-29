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

	m_host.host_data = this;
	m_host.clap_version = CLAP_VERSION;
	m_host.name = "LMMS";
	m_host.version = LMMS_VERSION;
	m_host.vendor = nullptr;
	m_host.url = "https://github.com/LMMS/lmms";
	m_host.get_extension = ClapManager::hostGetExtension;
	m_host.request_callback = ClapManager::hostRequestCallback;
	m_host.request_process = ClapManager::hostRequestProcess;
	m_host.request_restart = ClapManager::hostRequestRestart;

	// TODO
}

ClapManager::~ClapManager()
{
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

	m_clapFiles.clear();
	m_plugins.clear();

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

			auto clapFile = ClapFile{this, std::move(entryPath)};
			if (!clapFile.isValid())
			{
				qWarning() << "Failed to load .clap file";
				continue;
			}

			totalClapPlugins += clapFile.getPluginCount();
			for (const auto& plugin : clapFile.getPlugins())
			{
				m_plugins.push_back(plugin.getPlugin());
				m_uriToPlugin.emplace(std::string{plugin.getDescriptor()->id}, &plugin); // TODO: Does this pointer remain valid after clapFile is moved?
			}

			m_clapFiles.emplace_back(std::move(clapFile));
		}
	}

	qDebug() << "CLAP plugin SUMMARY:"
		<< m_clapFiles.size() << "of" << totalClapFiles << ".clap files containing"
		<< m_plugins.size() << "of" << totalClapPlugins
		<< "CLAP plugins loaded in" << timer.elapsed() << "msecs.";
	if (m_clapFiles.size() != totalClapFiles || m_plugins.size() != totalClapPlugins)
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

// clap_host functions

const void* ClapManager::hostGetExtension(const clap_host* host, const char* extension_id)
{
	//qDebug() << "hostGetExtension called; extension='" << extension_id << "'";

	/*
	checkForMainThread();

	auto h = fromHost(host);

	if (!strcmp(extension, CLAP_EXT_GUI))
		return &h->_hostGui;
	if (!strcmp(extension, CLAP_EXT_LOG))
		return &h->_hostLog;
	if (!strcmp(extension, CLAP_EXT_THREAD_CHECK))
		return &h->_hostThreadCheck;
	if (!strcmp(extension, CLAP_EXT_THREAD_POOL))
		return &h->_hostThreadPool;
	if (!strcmp(extension, CLAP_EXT_TIMER_SUPPORT))
		return &h->_hostTimerSupport;
	if (!strcmp(extension, CLAP_EXT_POSIX_FD_SUPPORT))
		return &h->_hostPosixFdSupport;
	if (!strcmp(extension, CLAP_EXT_PARAMS))
		return &h->_hostParams;
	if (!strcmp(extension, CLAP_EXT_QUICK_CONTROLS))
		return &h->_hostQuickControls;
	if (!strcmp(extension, CLAP_EXT_STATE))
		return &h->_hostState;
	*/
	return nullptr;
}

void ClapManager::hostRequestCallback(const clap_host* host)
{
	//auto h = fromHost(host);
	//h->_scheduleMainThreadCallback = true;
	//qDebug() << "hostRequestCallback called";
}

void ClapManager::hostRequestProcess(const clap_host* host)
{
	//auto h = fromHost(host);
	//h->_scheduleProcess = true;
	//qDebug() << "hostRequestProcess called";
}

void ClapManager::hostRequestRestart(const clap_host* host)
{
	//auto h = fromHost(host);
	//h->_scheduleRestart = true;
	//qDebug() << "hostRequestRestart called";
}

const ClapPlugin* ClapManager::getPlugin(const std::string& uri)
{
	const auto iter = m_uriToPlugin.find(uri);
	return iter != m_uriToPlugin.end() ? iter->second : nullptr;
}

const ClapPlugin* ClapManager::getPlugin(const QString& uri)
{
	return getPlugin(uri.toStdString());
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
