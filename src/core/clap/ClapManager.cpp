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
			return QDir::home().absolutePath().toStdString() + std::string{dir};
		}
#endif
		return std::filesystem::path{dir};
	}
}

clap_event_transport ClapManager::s_transport = {};
bool ClapManager::s_debug = false;

ClapManager::ClapManager()
{
	updateTransport(); // TODO: Is Song available at this point?

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
	if (debugging()) { qDebug() << "Found .clap files:"; }
	loadClapFiles(getSearchPaths());

	// TEMPORARY: Testing purposes
	/*
	for (const auto& file : files())
	{
		for (auto& plugin : file.pluginInfo())
		{
			qDebug() << plugin->getDescriptor()->name;
			auto& test = m_instances.emplace_back(std::make_shared<ClapInstance>(plugin.get()));
			test->pluginLoad();
		}
	}
	*/
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

	namespace fs = std::filesystem;
	m_searchPaths.clear();

	// Get CLAP_PATH paths
	if (const char* clapPathTemp = std::getenv("CLAP_PATH"))
	{
		std::error_code ec;
		auto clapPath = std::string_view{clapPathTemp};
		auto pos = clapPath.find(LADSPA_PATH_SEPERATOR);
		while (pos != std::string_view::npos)
		{
			auto path = expandHomeDir(clapPath.substr(0, pos));
			if (fs::is_directory(path, ec))
			{
				m_searchPaths.emplace_back(std::move(path.make_preferred()));
			}
			clapPath = clapPath.substr(pos + 1);
			pos = clapPath.find(LADSPA_PATH_SEPERATOR);
		}
		if (!clapPath.empty())
		{
			auto path = expandHomeDir(clapPath);
			if (fs::is_directory(path, ec))
			{
				m_searchPaths.emplace_back(std::move(path.make_preferred()));
			}
		}
	}

	// Add OS-dependent search paths
#ifdef LMMS_BUILD_LINUX
	auto path = expandHomeDir("~/.clap");
	std::error_code ec;
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
	std::error_code ec;
	if (auto commonProgFiles = std::getenv("COMMONPROGRAMFILES"))
	{
		auto path = fs::path{commonProgFiles};
		path /= "CLAP";
		if (fs::is_directory(path, ec))
		{
			m_searchPaths.emplace_back(std::move(path.make_preferred()));
		}
	}
	if (auto localAppData = std::getenv("LOCALAPPDATA"))
	{
		auto path = fs::path{localAppData};
		path /= fs::path{"Programs/Common/CLAP"};
		if (fs::is_directory(path, ec))
		{
			m_searchPaths.emplace_back(std::move(path.make_preferred()));
		}
	}
#elif defined(LMMS_BUILD_APPLE)
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

	if (debugging())
	{
		qDebug() << "CLAP search paths:";
		for (const auto& path : m_searchPaths)
		{
			qDebug() << "-" << path.c_str();
		}
	}
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
			auto entryPath = entry.path();
			std::error_code ec;
			if (!entry.is_regular_file(ec) || entryPath.extension() != ".clap")
			{
				continue;
			}

			++totalClapFiles;

			if (debugging()) { qDebug() << "\n\n~~~CLAP FILE~~~\nfilename:" << entryPath.c_str(); }

			auto& clapFile = m_files.emplace_back(this, std::move(entryPath));
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
				auto [ignore, added] = m_uriToPluginInfo.emplace(std::string{plugin->descriptor()->id}, std::weak_ptr{plugin});
				if (!added)
				{
					qWarning().nospace() << "The CLAP plugin ID '" << plugin->descriptor()->id
						<< "' in the plugin file '" << entry.path().c_str() << "' is identical to an ID"
						<< " in a previously loaded plugin file. Skipping the duplicate CLAP plugin.";
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
		<< m_files.size() << "of" << totalClapFiles << ".clap files containing"
		<< m_pluginInfo.size() << "of" << totalClapPlugins
		<< "CLAP plugins loaded in" << timer.elapsed() << "msecs.";
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

void ClapManager::updateTransport()
{
	s_transport = {};
	s_transport.header.size = sizeof(clap_event_transport);
	s_transport.header.type = CLAP_EVENT_TRANSPORT;

	const Song* song = Engine::getSong();
	if (!song) { return; }

	s_transport.flags = 0;

	setPlaying(song->isPlaying());
	setRecording(song->isRecording());
	//setLooping(song->isLooping()); // TODO

	setTimePosition(song->getMilliseconds());

	// TODO: Find a way to get the absolute beat within the song in order to support beats timeline
	s_transport.flags &= ~static_cast<std::uint32_t>(CLAP_TRANSPORT_HAS_BEATS_TIMELINE);

	setTempo(song->getTempo());
	setTimeSignature(song->getTimeSigModel().getNumerator(), song->getTimeSigModel().getDenominator());
}

void ClapManager::setPlaying(bool isPlaying)
{
	s_transport.flags |= isPlaying ? CLAP_TRANSPORT_IS_PLAYING : 0;
}

void ClapManager::setRecording(bool isRecording)
{
	s_transport.flags |= isRecording ? CLAP_TRANSPORT_IS_RECORDING : 0;
}

void ClapManager::setLooping(bool isLooping)
{
	s_transport.flags |= isLooping ? CLAP_TRANSPORT_IS_LOOP_ACTIVE : 0;
}

void ClapManager::setTimePosition(int elapsedMilliseconds)
{
	s_transport.flags |= static_cast<std::uint32_t>(CLAP_TRANSPORT_HAS_SECONDS_TIMELINE);
	s_transport.song_pos_seconds = std::lround(CLAP_SECTIME_FACTOR * (elapsedMilliseconds / 1000.0));
}

void ClapManager::setTempo(bpm_t tempo)
{
	s_transport.flags |= static_cast<std::uint32_t>(CLAP_TRANSPORT_HAS_TEMPO);
	s_transport.tempo  = static_cast<double>(tempo);
}

void ClapManager::setTimeSignature(int num, int denom)
{
	s_transport.flags |= static_cast<std::uint32_t>(CLAP_TRANSPORT_HAS_TIME_SIGNATURE);
	s_transport.tsig_num = static_cast<std::uint16_t>(num);
	s_transport.tsig_denom = static_cast<std::uint16_t>(denom);
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
