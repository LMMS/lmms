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

////////////////////////////////
// ClapManager
////////////////////////////////

bool ClapManager::m_debug = false;

ClapManager::ClapManager()
{
	const char* dbgStr = getenv("LMMS_CLAP_DEBUG");
	m_debug = (dbgStr && *dbgStr);

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
	if (m_debug)
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

	if (m_debug)
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

			if (m_debug)
				qDebug() << "-" << entryPath.c_str();

			auto clapFile = ClapFile{*this, std::move(entryPath)};
			if (!clapFile.isValid())
			{
				qWarning() << "Failed to load .clap file";
				continue;
			}

			totalClapPlugins += clapFile.getPluginCount();
			for (const auto& plugin : clapFile.getPlugins())
			{
				m_plugins.push_back(plugin.getPlugin());
			}

			m_clapFiles.emplace_back(std::move(clapFile));
		}
	}

	qDebug() << "CLAP plugin SUMMARY:"
		<< m_clapFiles.size() << "of" << totalClapFiles << " .clap files"
		<< "containing" << m_plugins.size() << "of" << totalClapPlugins
		<< "CLAP plugins loaded in" << timer.elapsed() << "msecs.";
	if (m_clapFiles.size() != totalClapFiles || m_plugins.size() != totalClapPlugins)
	{
		if (m_debug)
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

/*
const clap_plugin_t* ClapManager::getPlugin(const std::string& uri)
{
	auto iter = m_clapInfoMap.find(uri);
	return iter != m_clapInfoMap.end() ? iter->second.plugin() : nullptr;
}


const clap_plugin_t* ClapManager::getPlugin(const QString& uri)
{
	return getPlugin(uri.toStdString());
}
*/

////////////////////////////////
// ClapFile
////////////////////////////////

ClapManager::ClapFile::ClapFile(const ClapManager& manager, std::filesystem::path&& filename)
	: m_parent(manager), m_filename(std::move(filename))
{
	m_filename.make_preferred();
	m_library = std::make_shared<QLibrary>();
	m_library->setFileName(QString::fromUtf8(m_filename.c_str()));
	m_library->setLoadHints(QLibrary::ResolveAllSymbolsHint | QLibrary::DeepBindHint);
	load();
}

ClapManager::ClapFile::ClapFile(ClapFile&& other) noexcept
	: m_parent(other.m_parent)
{
	m_filename = std::move(other.m_filename);
	m_library = std::exchange(other.m_library, nullptr);
	m_factory = std::exchange(other.m_factory, nullptr);
	m_pluginCount = other.m_pluginCount;
	m_valid = std::exchange(other.m_valid, false);
	m_plugins = std::move(other.m_plugins);
}

auto ClapManager::ClapFile::load() -> bool
{
	m_valid = false;
	if (!m_library)
		return false;

	// Do not allow reloading yet
	if (m_library->isLoaded())
		return false;

	if (!m_library->load())
	{
		qWarning() << m_library->errorString();
		return false;
	}

	const auto pluginEntry = reinterpret_cast<const clap_plugin_entry_t*>(m_library->resolve("clap_entry"));
	if (!pluginEntry)
	{
		qWarning() << "Unable to resolve entry point 'clap_entry' in '" << getFilename().c_str() << "'";
		m_library->unload();
		return false;
	}

	pluginEntry->init(getFilename().c_str());
	m_factory = static_cast<const clap_plugin_factory_t*>(pluginEntry->get_factory(CLAP_PLUGIN_FACTORY_ID));

	m_pluginCount = m_factory->get_plugin_count(m_factory);
	if (m_debug)
		qDebug() << "m_pluginCount:" << m_pluginCount;
	if (m_pluginCount <= 0)
		return false;

	m_plugins.clear();
	for (uint32_t i = 0; i < m_pluginCount; ++i)
	{
		const auto desc = m_factory->get_plugin_descriptor(m_factory, i);
		if (!desc)
		{
			qWarning() << "no plugin descriptor";
			continue;
		}

		if (!clap_version_is_compatible(desc->clap_version))
		{
			qWarning() << "Incompatible CLAP version: Plugin is: " << desc->clap_version.major << "."
						<< desc->clap_version.minor << "." << desc->clap_version.revision << " Host is "
						<< CLAP_VERSION.major << "." << CLAP_VERSION.minor << "." << CLAP_VERSION.revision;
			continue;
		}

		if (m_debug)
		{
			qDebug() << "name:" << desc->name;
			qDebug() << "description:" << desc->description;
		}

		auto plugin = ClapPlugin{*this, i, desc}; // Prints warning if anything goes wrong
		if (!plugin.isValid())
			continue;

		m_plugins.emplace_back(std::move(plugin));
	}

	m_valid = true;
	return true;
}

void ClapManager::ClapFile::unload()
{
	// TODO: Need to implement
	return;
}

////////////////////////////////
// ClapPlugin
////////////////////////////////

ClapManager::ClapFile::ClapPlugin::ClapPlugin(const ClapFile& parent, uint32_t index, const clap_plugin_descriptor_t* desc)
	: m_parent(parent), m_host(parent.getParent().getHost())
{
	m_valid = false;
	m_index = index;
	m_descriptor = desc;

	m_type = Plugin::PluginTypes::Undefined;
	auto features = desc->features;
	while (features && *features)
	{
		std::string_view feature = *features;
		if (m_debug)
			qDebug() << "feature:" << feature.data();
		if (feature == CLAP_PLUGIN_FEATURE_INSTRUMENT)
			m_type = Plugin::PluginTypes::Instrument;
		else if (feature == CLAP_PLUGIN_FEATURE_AUDIO_EFFECT
				|| feature == CLAP_PLUGIN_FEATURE_NOTE_EFFECT
				|| feature == "effect" /* non-standard, but used by Surge XT Effects */)
			m_type = Plugin::PluginTypes::Effect;
		else if (feature == CLAP_PLUGIN_FEATURE_ANALYZER)
			m_type = Plugin::PluginTypes::Tool;
		++features;
	}

	if (m_type != Plugin::PluginTypes::Undefined)
		m_valid = true;
	else
	{
		qWarning() << "CLAP plugin is not recognized as an instrument, effect, or tool";
	}
}

ClapManager::ClapFile::ClapPlugin::ClapPlugin(ClapPlugin&& other) noexcept
	: m_parent(other.m_parent)
{
	m_host = std::exchange(other.m_host, nullptr);
	m_index = other.m_index;
	m_descriptor = std::exchange(other.m_descriptor, nullptr);
	m_type = std::exchange(other.m_type, Plugin::PluginTypes::Undefined);
	m_valid = std::exchange(other.m_valid, false);
	m_plugin = std::exchange(other.m_plugin, nullptr);
}

auto ClapManager::ClapFile::ClapPlugin::activate() -> bool
{
	if (!isValid() || isActivated())
		return false;

	const auto factory = getParent().getFactory();
	m_plugin = ClapPluginPtr{ factory->create_plugin(factory, m_host, getDescriptor()->id) };
	if (!m_plugin)
	{
		qWarning() << "could not create the plugin with id: " << getDescriptor()->id;
		return false;
	}

	if (!m_plugin->init(m_plugin.get()))
	{
		qWarning() << "could not init the plugin with id: " << getDescriptor()->id;
		return false;
	}

	return true;
}

void ClapManager::ClapFile::ClapPlugin::deactivate()
{
	// TODO
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
