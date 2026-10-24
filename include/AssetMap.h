#ifndef LMMS_ASSET_MAP
#define LMMS_ASSET_MAP

#include <filesystem>
#include <unordered_map>
#include <bitset>

#include <QString>

#include "lmms_export.h"

namespace lmms {

class LMMS_EXPORT AssetMap
{
public:
	AssetMap();
	static AssetMap* inst()
	{
		if (!s_inst) { s_inst = new AssetMap; }
		return s_inst;
	}

	using Sha256Hash = std::bitset<256>;
	static Sha256Hash getHash(std::filesystem::path file);
	static Sha256Hash hashFromString(QString b64);
	static QString stringFromHash(Sha256Hash hash);

	void add(std::filesystem::path entry);
	std::filesystem::path get(Sha256Hash hash) const;
	std::filesystem::path get(QString b64hash) const
	{
		return b64hash.isEmpty() ? "" : get(hashFromString(b64hash));
	}

	// This is static so that the scan is "lazy" - performed only when source file is missing.
	static std::filesystem::path fallback(std::filesystem::path file, Sha256Hash hash);
	static QString fallback(QString file, QString b64hash);

private:
	static AssetMap* s_inst;
	std::unordered_map<Sha256Hash, std::filesystem::path> map = {};
};

}

#endif
