#ifndef _VST_LISTING_H
#define _VST_LISTING_H

#include <filesystem>
#include <unordered_map>
#include <vector>

#include <QObject>

#include "vstbase_export.h"

namespace lmms
{


class VSTBASE_EXPORT VstList : public QObject
{
	Q_OBJECT
public:
	struct Metadata
	{
		std::filesystem::path path;
		// TODO add file hash to detect changes
		enum class PluginType { NotVst, Instrument, Effect } type = PluginType::NotVst;
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

	std::vector<Metadata> instrumentPlugins();
	std::vector<Metadata> effectPlugins();

private:
	static VstList* s_inst;
	std::unordered_map<std::filesystem::path, Metadata> m_plugins;
};


} // namespace lmms

#endif
