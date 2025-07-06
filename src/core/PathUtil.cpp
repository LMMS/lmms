#include "PathUtil.h"

#include <QDir>
#include <QFileInfo>

#include "ConfigManager.h"
#include "Engine.h"
#include "Song.h"

namespace lmms::PathUtil
{
	constexpr auto relativeBases = std::array {
		Base::ProjectDir, Base::FactoryProjects, Base::FactorySample, Base::UserSample, Base::UserVST, Base::Preset,
		Base::FactoryPresets, Base::UserLADSPA, Base::DefaultLADSPA, Base::UserSoundfont, Base::DefaultSoundfont,
		Base::UserGIG, Base::DefaultGIG, Base::LocalDir, Base::Internal
	};

	QString baseLocation(const Base base, bool* error /* = nullptr*/)
	{
		// error is false unless something goes wrong
		if (error) { *error = false; }

		QString loc = "";
		switch (base)
		{
			case Base::ProjectDir       : loc = ConfigManager::inst()->userProjectsDir(); break;
			case Base::FactoryProjects :
			{
				QDir fpd = QDir(ConfigManager::inst()->factoryProjectsDir());
				loc = fpd.absolutePath(); break;
			}
			case Base::FactorySample    :
			{
				QDir fsd = QDir(ConfigManager::inst()->factorySamplesDir());
				loc = fsd.absolutePath(); break;
			}
			case Base::UserSample       : loc = ConfigManager::inst()->userSamplesDir(); break;
			case Base::UserVST          : loc = ConfigManager::inst()->userVstDir(); break;
			case Base::Preset           : loc = ConfigManager::inst()->userPresetsDir(); break;
			case Base::FactoryPresets    :
			{
				QDir fpd = QDir(ConfigManager::inst()->factoryPresetsDir());
				loc = fpd.absolutePath(); break;
			}
			case Base::UserLADSPA       : loc = ConfigManager::inst()->ladspaDir(); break;
			case Base::DefaultLADSPA    : loc = ConfigManager::inst()->userLadspaDir(); break;
			case Base::UserSoundfont    : loc = ConfigManager::inst()->sf2Dir(); break;
			case Base::DefaultSoundfont : loc = ConfigManager::inst()->userSf2Dir(); break;
			case Base::UserGIG          : loc = ConfigManager::inst()->gigDir(); break;
			case Base::DefaultGIG       : loc = ConfigManager::inst()->userGigDir(); break;
			case Base::LocalDir:
			{
				const Song* s = Engine::getSong();
				QString projectPath;
				if (s)
				{
					projectPath = s->projectFileName();
					loc = QFileInfo(projectPath).path();
				}
				// We resolved it properly if we had an open Song and the project
				// filename wasn't empty
				if (error) { *error = (!s || projectPath.isEmpty()); }
				break;
			}
			case Base::Internal:
				if (error) { *error = true; }
				[[fallthrough]];
			default                   : return QString("");
		}
		return QDir::cleanPath(loc) + "/";
	}

	std::optional<std::string> getBaseLocation(Base base)
	{
		bool error = false;
		const auto str = baseLocation(base, &error);
		if (error) { return std::nullopt; }
		return str.toStdString();
	}

	QDir baseQDir (const Base base, bool* error /* = nullptr*/)
	{
		if (base == Base::Absolute)
		{
			if (error) { *error = false; }
			return QDir::root();
		}
		return QDir(baseLocation(base, error));
	}

	QString basePrefixQString(const Base base)
	{
		const auto prefix = basePrefix(base);
		return QString::fromLatin1(prefix.data(), prefix.size());
	}

	std::string_view basePrefix(const Base base)
	{
		switch (base)
		{
			case Base::ProjectDir       : return "userprojects:";
			case Base::FactoryProjects  : return "factoryprojects:";
			case Base::FactorySample    : return "factorysample:";
			case Base::UserSample       : return "usersample:";
			case Base::UserVST          : return "uservst:";
			case Base::Preset           : return "preset:";
			case Base::FactoryPresets   : return "factorypreset:";
			case Base::UserLADSPA       : return "userladspa:";
			case Base::DefaultLADSPA    : return "defaultladspa:";
			case Base::UserSoundfont    : return "usersoundfont:";
			case Base::DefaultSoundfont : return "defaultsoundfont:";
			case Base::UserGIG          : return "usergig:";
			case Base::DefaultGIG       : return "defaultgig:";
			case Base::LocalDir         : return "local:";
			case Base::Internal         : return "internal:";
			default                     : return "";
		}
	}

	bool hasBase(const QString& path, Base base)
	{
		if (base == Base::Absolute)
		{
			return baseLookup(path) == base;
		}

		return path.startsWith(basePrefixQString(base), Qt::CaseInsensitive);
	}

	bool hasBase(std::string_view path, Base base)
	{
		if (base == Base::Absolute)
		{
			return baseLookup(path) == base;
		}

		auto prefix = basePrefix(base);
		return path.rfind(prefix, 0) == 0;
	}

	Base baseLookup(const QString & path)
	{
		for (auto base : relativeBases)
		{
			QString prefix = basePrefixQString(base);
			if ( path.startsWith(prefix) ) { return base; }
		}
		return Base::Absolute;
	}

	Base baseLookup(std::string_view path)
	{
		for (auto base : relativeBases)
		{
			const auto prefix = basePrefix(base);
			if (path.rfind(prefix, 0) == 0) { return base; }
		}
		return Base::Absolute;
	}

	QString stripPrefix(const QString & path)
	{
		return path.mid( basePrefix(baseLookup(path)).length() );
	}

	std::string_view stripPrefix(std::string_view path)
	{
		path.remove_prefix(basePrefix(baseLookup(path)).length());
		return path;
	}

	std::pair<Base, std::string_view> parsePath(std::string_view path)
	{
		for (auto base : relativeBases)
		{
			auto prefix = basePrefix(base);
			if (path.rfind(prefix, 0) == 0)
			{
				path.remove_prefix(prefix.length());
				return { base, path };
			}
		}
		return { Base::Absolute, path };
	}

	QString cleanName(const QString& path)
	{
		return stripPrefix(QFileInfo(path).completeBaseName());
	}

	QString oldRelativeUpgrade(const QString & input)
	{
		if (input.isEmpty()) { return input; }

		//Start by assuming that the file is a user sample
		Base assumedBase = Base::UserSample;

		//Check if it's a factory sample
		QString factoryPath = baseLocation(Base::FactorySample) + input;
		QFileInfo factoryInfo(factoryPath);
		if (factoryInfo.exists()) { assumedBase = Base::FactorySample; }

		//Check if it's a VST
		QString vstPath = baseLocation(Base::UserVST) + input;
		QFileInfo vstInfo(vstPath);
		if (vstInfo.exists()) { assumedBase = Base::UserVST; }

		//Assume we've found the correct base location, return the full path
		return basePrefixQString(assumedBase) + input;
	}

	QString toAbsolute(const QString & input, bool* error /* = nullptr*/)
	{
		// First, check if it's Internal
		if (hasBase(input, Base::Internal)) { return input; }

		// Secondly, do no harm to absolute paths
		QFileInfo inputFileInfo = QFileInfo(input);
		if (inputFileInfo.isAbsolute())
		{
			if (error) { *error = false; }
			return input;
		}

		// Next, handle old relative paths with no prefix
		QString upgraded = input.contains(":") ? input : oldRelativeUpgrade(input);

		Base base = baseLookup(upgraded);
		return baseLocation(base, error) + upgraded.remove(0, basePrefix(base).length());
	}

	std::optional<std::string> toAbsolute(std::string_view input)
	{
		bool error = false;
		const auto str = toAbsolute(QString::fromUtf8(input.data(), input.size()), &error);
		if (error) { return std::nullopt; }
		return str.toStdString();
	}

	QString relativeOrAbsolute(const QString & input, const Base base)
	{
		if (input.isEmpty()) { return input; }
		QString absolutePath = toAbsolute(input);
		if (base == Base::Absolute || base == Base::Internal) { return absolutePath; }
		bool error;
		QString relativePath = baseQDir(base, &error).relativeFilePath(absolutePath);
		// Return the relative path if it didn't result in a path starting with ..
		// and the baseQDir was resolved properly
		return (relativePath.startsWith("..") || error)
			? absolutePath
			: relativePath;
	}

	QString toShortestRelative(const QString & input, bool allowLocal /* = false*/)
	{
		if (hasBase(input, Base::Internal)) { return input; }

		QFileInfo inputFileInfo = QFileInfo(input);
		QString absolutePath = inputFileInfo.isAbsolute() ? input : toAbsolute(input);

		Base shortestBase = Base::Absolute;
		QString shortestPath = relativeOrAbsolute(absolutePath, shortestBase);
		for (auto base: relativeBases)
		{
			// Skip local paths when searching for the shortest relative if those
			// are not allowed for that resource
			if (base == Base::LocalDir && !allowLocal) { continue; }

			QString otherPath = relativeOrAbsolute(absolutePath, base);
			if (otherPath.length() < shortestPath.length())
			{
				shortestBase = base;
				shortestPath = otherPath;
			}
		}
		return basePrefixQString(shortestBase) + relativeOrAbsolute(absolutePath, shortestBase);
	}

	std::string toShortestRelative(std::string_view input, bool allowLocal)
	{
		if (hasBase(input, Base::Internal)) { return std::string{input}; }

		const auto qstr = QString::fromUtf8(input.data(), input.size());
		QFileInfo inputFileInfo = QFileInfo(qstr);
		QString absolutePath = inputFileInfo.isAbsolute() ? qstr : toAbsolute(qstr);

		Base shortestBase = Base::Absolute;
		QString shortestPath = relativeOrAbsolute(absolutePath, shortestBase);
		for (auto base : relativeBases)
		{
			// Skip local paths when searching for the shortest relative if those
			// are not allowed for that resource
			if (base == Base::LocalDir && !allowLocal) { continue; }

			QString otherPath = relativeOrAbsolute(absolutePath, base);
			if (otherPath.length() < shortestPath.length())
			{
				shortestBase = base;
				shortestPath = otherPath;
			}
		}

		return (basePrefixQString(shortestBase) + relativeOrAbsolute(absolutePath, shortestBase)).toStdString();
	}

} // namespace lmms::PathUtil
