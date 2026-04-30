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
#include "lmmsconfig.h"

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

fs::path toWinePath(const fs::path& winPath)
{
	if (const char* wineprefix_str = std::getenv("WINEPREFIX"))
	{
		return fs::path{wineprefix_str} / "drive_c" / winPath.relative_path();
	}
	else if (const char* home_str = std::getenv("HOME"))
	{
		return fs::path{home_str} / ".wine" / "drive_c" / winPath.relative_path();
	}
	return {};
}

bool isSubPath(const fs::path& sub, const fs::path& base)
{
    auto rel = std::filesystem::relative(sub, base);
    return !rel.empty() && *rel.begin() != "..";
}

namespace VstPaths
{
	const fs::path Win64 = "C:/Program Files/Steinberg/VstPlugins";
	const fs::path Linux64 = "/usr/lib64/vst";
	const fs::path Linux32 = "/usr/lib/vst";
	const fs::path Wine64 = toWinePath(Win64);
}

// enum to determine order of preference for VST dirs
// int comparison is used so this isn't an enum class
// 0 is most favorable
enum VstPreferenceType
{
	NativeUser,
	Native64,
	Native32,
	WineUser,
	Wine64,
	Wine32,
};

VstPreferenceType prefType(const lmms::VstList::Metadata& data)
{
#ifdef LMMS_BUILD_WIN32
	if (isSubPath(data.path, VstPaths::Win64)) { return Native64; }
	return NativeUser;
#else
	if      (isSubPath(data.path, VstPaths::Linux64)) { return Native64; }
	else if (isSubPath(data.path, VstPaths::Linux32)) { return Native32; }
	else if (!VstPaths::Wine64.empty() && isSubPath(data.path, VstPaths::Wine64))  { return Wine64; }
	else if (data.path.extension() == ".dll") { return WineUser; }
	return NativeUser;
#endif
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


VstList::VstList(bool initDefaultDir, bool loadNewlyFound)
{
	if (initDefaultDir)
	{
		loadCache(cacheFilePath());
		scanDefaultDirs(loadNewlyFound);
		saveCache(cacheFilePath());
	}
}


void VstList::scanDefaultDirs(bool loadNewlyFound)
{
	scanDirRecursive(ConfigManager::inst()->vstDir().toStdString(), loadNewlyFound);

	#if defined(LMMS_BUILD_WIN32)

		scanDirRecursive(VstPaths::Win64, loadNewlyFound);

	#elif defined(LMMS_BUILD_LINUX)

		scanDirRecursive(VstPaths::Linux64, loadNewlyFound);
		scanDirRecursive(VstPaths::Linux32, loadNewlyFound);

		#if defined(LMMS_HAVE_VST_32) || defined(LMMS_HAVE_VST_64)

			if (!VstPaths::Wine64.empty()) { scanDirRecursive(VstPaths::Wine64, loadNewlyFound); }
		#endif
	#endif
}


void VstList::scanDirRecursive(fs::path dirPath, bool loadNewlyFound)
{
	if (!fs::exists(dirPath)) { return; }
	for (const auto& entry : fs::directory_iterator{dirPath})
	{
		const fs::path& path = entry.path();

		// skip loading already analyzed executables
		if (m_pluginsCache.contains(path) && m_pluginsCache[path].file_checksum == checksum(path))
		{
			addPlugin(m_pluginsCache[path]);
			continue;
		}

		// ignore "hidden" files and folders. Dot and dot-dot are skipped by C++ STL, not here.
		// this doesn't go out of bounds unless the filesystem somehow has an empty string as a file name.
		if (path.filename().string()[0] == '.') { continue; }

		// resolve symlinks and fetch data from the other end
		const fs::file_status& stat = entry.status();

		if (fs::is_directory(stat)) { scanDirRecursive(path, loadNewlyFound); }
		else if (fs::is_regular_file(stat))
		{
			if (!loadNewlyFound) { continue; }
			if (const auto& ext = path.extension();
#if defined(LMMS_BUILD_WIN32)
				ext != ".dll")
#elif defined(LMMS_BUILD_LINUX)
#	if defined(LMMS_HAVE_VST_32) || defined(LMMS_HAVE_VST_64)
				ext != ".dll" &&
#	endif
				ext != ".so")
#endif
			{
				continue;
			}

			VstPlugin plug{QString::fromStdString(path.string()), true};
			if (plug.name().isEmpty()) // TODO: figure out a better way to check load fail
			{
				m_pluginsCache.emplace(path, Metadata {
					path,
					checksum(path),
					Metadata::PluginType::NotVst});
				continue;
			}
			Metadata metadata = {
				path,
				checksum(path),
				plug.isSynth()
					? Metadata::PluginType::Instrument
					: Metadata::PluginType::Effect,
				plug.uniqueID().toStdString(),
				plug.name().toStdString(),
				plug.productString().toStdString(),
				plug.vendorString().toStdString()
			};
			m_pluginsCache.insert({path, metadata});
			addPlugin(metadata);
		}
	}
}


void VstList::addPlugin(Metadata& data)
{
	if (data.type != Metadata::PluginType::NotVst
	    && (!m_plugins.contains(data.ID) || prefType(data) < prefType(m_plugins[data.ID])))
	{
		m_plugins.insert_or_assign(data.ID, data);
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


constexpr uint32_t CACHE_VER = 2; // increment this when changing the functions' output

// `saveCache` and `loadCache` implementations should be kept in sync
// All values should be dumped because cache existing for a file implies
// the dll shouldn't be queried for new information. That also means the cache has to be rejeced
// if it's from an old version entirely to fill new information.
void VstList::saveCache(fs::path cacheFilePath)
{
	std::ofstream cache{cacheFilePath, std::ios::binary};
	writeBin(cache, CACHE_VER);
	for (const auto& [_, data] : m_pluginsCache)
	{
		cache << data.path.string() << '\0';
		writeBin(cache, data.file_checksum);
		writeBin(cache, data.type);
		cache << data.ID << '\0';
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
		data.ID = buf;

		cache.getline(buf, 4096, '\0');
		data.name = buf;

		cache.getline(buf, 4096, '\0');
		data.product = buf;

		cache.getline(buf, 4096, '\0');
		data.vendor = buf;

		m_pluginsCache.insert({data.path, data});
	}
}

} // namespace lmms
