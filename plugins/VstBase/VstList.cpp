#include "VstList.h"

#include "VstPlugin.h"

#include <filesystem>
#include <fstream>
#include <ranges>
#include <string>
#include <vector>

#include <QString>
#include <QFile>
#include <QCryptographicHash>
#include <QStandardPaths>

#include "ConfigManager.h"

namespace fs = std::filesystem;

namespace
{

lmms::VstList::Metadata::Checksum checksum(fs::path filePath)
{
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
	QFile file{QString::fromStdString(filePath.string())};
#else
	QFile file{filePath};
#endif
	if (!file.open(QFile::ReadOnly)) { return 0; }
	QCryptographicHash hash{QCryptographicHash::Md5};
	if (!hash.addData(&file)) { return 0; }

	lmms::VstList::Metadata::Checksum out;
	memcpy(&out, hash.result().constData(), sizeof out);

	return out;
}

fs::path cacheFilePath()
{
	auto cacheDir = fs::path{QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation).toStdString()} / "lmms";
	fs::create_directories(cacheDir);
	return cacheDir / "vst-list.bin";
}

// helpers for cache I/O

template<typename T>
inline void writeBin(std::ostream& out, const T& x)
{
	out.write(reinterpret_cast<const char*>(&x), sizeof x);
}

template<typename T>
inline void readBin(std::istream& out, T& x)
{
	out.read(reinterpret_cast<char*>(&x), sizeof x);
}

} // namespace


namespace lmms
{


VstList* VstList::s_inst = nullptr;


VstList::VstList(bool initDefaultDir)
{
	if (initDefaultDir)
	{
		loadCache(cacheFilePath());
		scanDirRecursive(fs::path{ConfigManager::inst()->vstDir().toStdString()});
		saveCache(cacheFilePath());
	}
}


void VstList::scanDirRecursive(fs::path dirPath)
{
	for (const auto& entry : fs::directory_iterator{dirPath})
	{
		const fs::path& path = entry.path();

		// skip loading already analyzed executables
		if (m_plugins.contains(path) && m_plugins[path].file_checksum == checksum(path)) { continue; }

		// ignore "hidden" files and folders. Dot and dot-dot are skipped by C++ STL, not here.
		// this doesn't go out of bounds unless the filesystem somehow has an empty string as a file name.
		if (path.filename().string()[0] == '.') { continue; }

		// resolve symlinks and fetch data from the other end
		const fs::file_status& stat = entry.status();

		if (fs::is_directory(stat)) { scanDirRecursive(path); }
		else if (fs::is_regular_file(stat))
		{
			if (const auto& ext = path.extension();
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

			VstPlugin plug{QString::fromStdString(path.string()), true};
			if (plug.name().isEmpty()) // TODO: figure out a better way to check load fail
			{
				m_plugins.emplace(path, Metadata {
					path,
					checksum(path),
					Metadata::PluginType::NotVst});
				continue;
			}
			m_plugins.emplace(path, Metadata {
				path,
				checksum(path),
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


constexpr uint32_t CACHE_VER = 1; // increment this when changing the functions' output

// `saveCache` and `loadCache` implementations should be kept in sync
// All values should be dumped because cache existing for a file implies
// the dll shouldn't be queried for new information. That also means the cache has to be rejeced
// if it's from an old version entirely to fill new information.
void VstList::saveCache(fs::path cacheFilePath)
{
	std::ofstream cache{cacheFilePath, std::ios::binary};
	writeBin(cache, CACHE_VER);
	for (const auto& [_, data] : m_plugins)
	{
		cache << data.path.string() << '\0';
		writeBin(cache, data.file_checksum);
		writeBin(cache, data.type);
		cache << data.name << '\0';
		cache << data.product << '\0';
		cache << data.vendor << '\0';
	}
}

void VstList::loadCache(fs::path cacheFilePath)
{
	std::ifstream cache{cacheFilePath, std::ios::binary};
	uint32_t version;
	readBin(cache, version);
	if (version != CACHE_VER) { return; }
	while (cache.peek() != EOF)
	{
		VstList::Metadata data;
		char buf[4096];

		cache.getline(buf, 4096, '\0');
		data.path = fs::path{buf};

		readBin(cache, data.file_checksum);
		readBin(cache, data.type);

		cache.getline(buf, 4096, '\0');
		data.name = std::string{buf};

		cache.getline(buf, 4096, '\0');
		data.product = std::string{buf};

		cache.getline(buf, 4096, '\0');
		data.vendor = std::string{buf};

		// if (version >= N) { new code here; }
		// if (version >= N+1) { etc; }

		m_plugins.insert({data.path, data});
	}
}

} // namespace lmms
