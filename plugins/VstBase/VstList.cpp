#include "VstList.h"

#include "VstPlugin.h"

#include <filesystem>
#include <string>
#include <vector>
#include <ranges>

#include <QString>

#include "ConfigManager.h"

namespace fs = std::filesystem;

namespace lmms
{


VstList* VstList::s_inst = nullptr;

	
VstList::VstList(bool initDefaultDir)
{
	if (initDefaultDir)
	{
		scanDirRecursive(fs::path{ConfigManager::inst()->vstDir().toStdString()});
	}
}


void VstList::scanDirRecursive(fs::path dirPath)
{
	for (const auto& entry : fs::directory_iterator{dirPath})
	{
		// skip loading already analyzed executables
		if (m_plugins.contains(entry.path())) { continue; }
		
		// ignore "hidden" files and folders. Dot and dot-dot are skipped by C++ STL, not here.
		// this doesn't go out of bounds unless the filesystem somehow has an empty string as a file name.
		if (entry.path().filename().string()[0] == '.') { continue; }

		// resolve symlinks and fetch data from the other end
		const fs::file_status& stat = entry.symlink_status();
		
		if (fs::is_directory(stat)) { scanDirRecursive(entry.path()); }
		else if (fs::is_regular_file(stat))
		{
			if (const auto& ext = entry.path().extension();
#if defined(LMMS_BUILD_WIN32)
				ext != ".dll")
#elif defined(LMMS_BUILD_LINUX)
#	if defined(LMMS_HAVE_VST_32) || defined(LMMS_HAVE_VST_64)
				ext != ".dll" && ext != ".so")
#	else
				ext != ".so")  // TODO: is this actually possible?
#	endif
#endif
			{
				continue;
			}
			
			VstPlugin plug{QString::fromStdString(entry.path().string()), true};
			if (plug.name().isEmpty()) // TODO: figure out a better way to check load fail
			{
				m_plugins.emplace(entry.path(), Metadata {entry.path(), Metadata::PluginType::NotVst});
				continue;
			}
			m_plugins.emplace(entry.path(), Metadata {
				entry.path(),
				plug.isSynth()
					? Metadata::PluginType::Instrument
					: Metadata::PluginType::Effect,
				plug.name().toStdString(),
				plug.productString().toStdString(),
				plug.vendorString().toStdString()
			});
		}
	}
}

std::vector<VstList::Metadata> VstList::instrumentPlugins()
{
	std::vector<VstList::Metadata> out;
	for (const auto& [_, data] : m_plugins
		| std::views::filter([](const auto& elem){return elem.second.type == Metadata::PluginType::Instrument;}))
	{
		out.push_back(data);
	}
	return out;
}

std::vector<VstList::Metadata> VstList::effectPlugins()
{
	std::vector<VstList::Metadata> out;
	for (const auto& [_, data] : m_plugins
		| std::views::filter([](const auto& elem){return elem.second.type == Metadata::PluginType::Effect;}))
	{
		out.push_back(data);
	}
	return out;
}

} // namespace lmms
