#include "AssetMap.h"

#include <filesystem>

#include <QByteArray>
#include <QCryptographicHash>
#include <QDebug>
#include <QFile>
#include <QString>

#include "ConfigManager.h" // To init with common userdirs
#include "PathUtil.h" // Conversion to absolute fs::string in convenience wrappers

namespace fs = std::filesystem;

namespace lmms {

AssetMap* AssetMap::s_inst = nullptr;

AssetMap::AssetMap()
{
	qDebug() << "AssetMap: Hashing user directory contents, this may take a while...";
	const ConfigManager* cfg = ConfigManager::inst();
	add(cfg->userSamplesDir().toStdString());
	qDebug() << "AssetMap: Scanning finished!";
}

AssetMap::Sha256Hash AssetMap::getHash(fs::path filePath) {
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
	QFile file{QString::fromStdString(filePath.string())};
#else
	QFile file{filePath};
#endif
	if (!file.open(QFile::ReadOnly)) { return 0; }
	QCryptographicHash hash{QCryptographicHash::Sha256};
	if (!hash.addData(&file)) { return 0; }

	AssetMap::Sha256Hash out;
	memcpy(&out, hash.result().constData(), sizeof out);

	return out;
};

QString AssetMap::QStringFromHash(AssetMap::Sha256Hash hash)
{
	return QByteArray::fromRawData(reinterpret_cast<char*>(&hash), sizeof hash).toBase64();
}

// TODO possible endianness problems; can corrupt if length is wrong
AssetMap::Sha256Hash AssetMap::hashFromQString(QString b64)
{
	return *reinterpret_cast<AssetMap::Sha256Hash*>(QByteArray::fromBase64(b64.toUtf8()).data());
}

void AssetMap::add(fs::path entry) {
	if (!fs::exists(entry)) { return; }

	const fs::file_status stat = fs::status(entry);

	if (fs::is_regular_file(stat))
	{
		const Sha256Hash file_hash = getHash(entry);
		// if (map.contains(file_hash))
		// {
		//	qWarning() << "AssetMap: Hash for" << entry.string() << "already present, file will be skipped.";
		// }
		map.emplace(file_hash, entry);
		return;
	}

	if (fs::is_directory(stat))
	{
		for (const auto& inner : fs::directory_iterator{entry})
		{
			add(inner.path());
		}
	}
}

fs::path AssetMap::get(AssetMap::Sha256Hash hash) const
{
	return map.at(hash);
}

fs::path AssetMap::fallback(fs::path file, AssetMap::Sha256Hash hash)
{
	if (fs::exists(file)) { return file; }
	try
	{
		const fs::path alt = inst()->get(hash);
		if (fs::exists(alt)) { return alt; }
	}
	catch (std::out_of_range const&) {}
	return "";
}

fs::path AssetMap::fallback(QString file, QString b64hash)
{
	const fs::path stdfile = PathUtil::toAbsolute(file).toStdString();
	if (b64hash.isEmpty()) { return std::filesystem::exists(stdfile) ? stdfile : ""; }
	return fallback(file.toStdString(), hashFromQString(b64hash));
}


}
