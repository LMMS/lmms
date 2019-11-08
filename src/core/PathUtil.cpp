#include "PathUtil.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include "ConfigManager.h"

namespace PathUtil
{
	Base relativeBases[] = { ProjectDirBase, FactorySampleBase, UserSampleBase, UserVSTBase, PresetBase,
		UserLADSPABase, DefaultLADSPABase, UserSoundfontBase, DefaultSoundfontBase, UserGIGBase, DefaultGIGBase };

	QString baseLocation(const Base & base)
	{
		QString loc = "";
		switch (base)
		{
			case ProjectDirBase       : loc = ConfigManager::inst()->userProjectsDir(); break;
			case FactorySampleBase    :
			{
				QDir fsd = QDir(ConfigManager::inst()->factorySamplesDir());
				loc = fsd.absolutePath() + "/"; break;
			}
			case UserSampleBase       : loc = ConfigManager::inst()->userSamplesDir(); break;
			case UserVSTBase          : loc = ConfigManager::inst()->userVstDir(); break;
			case PresetBase           : loc = ConfigManager::inst()->userPresetsDir(); break;
			case UserLADSPABase       : loc = ConfigManager::inst()->ladspaDir(); break;
			case DefaultLADSPABase    : loc = ConfigManager::inst()->userLadspaDir(); break;
			case UserSoundfontBase    : loc = ConfigManager::inst()->sf2Dir(); break;
			case DefaultSoundfontBase : loc = ConfigManager::inst()->userSf2Dir(); break;
			case UserGIGBase          : loc = ConfigManager::inst()->gigDir(); break;
			case DefaultGIGBase       : loc = ConfigManager::inst()->userGigDir(); break;
			default                   : return QString("");
		}
		return QDir::cleanPath(loc) + QDir::separator();
	}

	QDir baseQDir (const Base & base) { return QDir(baseLocation(base)); }

	QString basePrefix(const Base & base)
	{
		switch (base)
		{
			case ProjectDirBase       : return "userprojects:";
			case FactorySampleBase    : return "factorysample:";
			case UserSampleBase       : return "usersample:";
			case UserVSTBase          : return "uservst:";
			case PresetBase           : return "preset:";
			case UserLADSPABase       : return "userladspa:";
			case DefaultLADSPABase    : return "defaultladspa:";
			case UserSoundfontBase    : return "usersoundfont:";
			case DefaultSoundfontBase : return "defaultsoundfont:";
			case UserGIGBase          : return "usergig:";
			case DefaultGIGBase       : return "defaultgig:";
			default                   : return "";
		}
	}

	Base baseLookup(const QString & path)
	{
		for (auto base: relativeBases)
		{
			QString prefix = basePrefix(base);
			if ( path.startsWith(prefix) ) { return base; }
		}
		return AbsoluteBase;
	}




	QString stripPrefix(const QString & path)
	{
		return path.mid( basePrefix(baseLookup(path)).length() );
	}

	QString cleanName(const QString & path)
	{
		return stripPrefix(QFileInfo(path).baseName());
	}




	QString oldRelativeUpgrade(const QString & input)
	{
		if (input.isEmpty()) { return input; }
		QString factory = baseLocation(FactorySampleBase) + input;
		QFileInfo factoryInfo(factory);
		//If we can't find a factory sample, it's probably a user sample
		Base base = factoryInfo.exists() ? FactorySampleBase : UserSampleBase;
		return basePrefix(base) + input;
	}




	QString toAbsolute(const QString & input)
	{
		//First, do no harm to absolute paths
		QFileInfo inputFileInfo = QFileInfo(input);
		if (inputFileInfo.isAbsolute()) return input;
		//Next, handle old relative paths with no prefix
		QString upgraded = input.contains(":") ? input : oldRelativeUpgrade(input);

		Base base = baseLookup(upgraded);
		return baseLocation(base) + upgraded.remove(0, basePrefix(base).length());
	}

	QString relativeOrAbsolute(const QString & input, const Base & base)
	{
		if (input.isEmpty()) { return input; }
		QString absolutePath = toAbsolute(input);
		QString relativePath = baseQDir(base).relativeFilePath(absolutePath);
		return relativePath.startsWith("..") ? absolutePath : relativePath;
	}

	QString toShortestRelative(const QString & input)
	{
		QFileInfo inputFileInfo = QFileInfo(input);
		QString absolutePath = inputFileInfo.isAbsolute() ? input : toAbsolute(input);

		Base shortestBase = AbsoluteBase;
		QString shortestPath = relativeOrAbsolute(absolutePath, shortestBase);
		for (auto base: relativeBases)
		{
			QString otherPath = relativeOrAbsolute(absolutePath, base);
			if (otherPath.length() < shortestPath.length())
			{
				shortestBase = base;
				shortestPath = otherPath;
			}
		}
		return basePrefix(shortestBase) + relativeOrAbsolute(absolutePath, shortestBase);
	}

	// QString toPreferredRelative( QString, std::vector<Base> );
}
