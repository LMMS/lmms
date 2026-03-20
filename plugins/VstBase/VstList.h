#ifndef _VST_LISTING_H
#define _VST_LISTING_H

#include <bitset>
#include <filesystem>
#include <unordered_map>
#include <vector>

#include "vstbase_export.h"

namespace lmms
{


class VSTBASE_EXPORT VstList
{
public:
	struct Metadata
	{
		using Checksum = std::bitset<128>; // md5

		std::filesystem::path path;
		Checksum file_checksum;
		// TODO add file hash to detect changes
		enum class PluginType: uint8_t { NotVst, Instrument, Effect } type = PluginType::NotVst;
		std::string name = "";
		std::string product = "";
		std::string vendor = "";
		int version = 0;
	};

	static VstList* inst()
	{
		if (!s_inst) { s_inst = new VstList; }
		return s_inst;
	}


	VstList(bool initDefaultDir = true);
	void scanDirRecursive(std::filesystem::path dirPath);

	void loadCache(std::filesystem::path cacheFilePath);
	void saveCache(std::filesystem::path cacheFilePath);

	std::vector<Metadata> instrumentPlugins();
	std::vector<Metadata> effectPlugins();

private:
	static VstList* s_inst;
	std::unordered_map<std::filesystem::path, Metadata> m_plugins;
};


} // namespace lmms

#endif
