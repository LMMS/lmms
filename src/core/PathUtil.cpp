#include "PathUtil.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include "ConfigManager.h"

namespace PathUtil
{
	std::vector<Base> relativeBases = { ProjectDirBase, FactorySampleBase, UserSampleBase, UserVSTBase, PresetBase,
		DefaultLADSPABase, UserLADSPABase, DefaultSoundfontBase, UserSoundfontBase, DefaultGIGBase, UserGIGBase };

	QString baseLocation(Base base)
	{
		switch (base)
		{
			case ProjectDirBase       : return ConfigManager::inst()->userProjectsDir();
			case FactorySampleBase    :
			{
				QDir fsd = QDir(ConfigManager::inst()->factorySamplesDir());
				return fsd.absolutePath() + "/";
			}
			case UserSampleBase       : return ConfigManager::inst()->userSamplesDir();
			case UserVSTBase          : return ConfigManager::inst()->userVstDir();
			case PresetBase           : return ConfigManager::inst()->userPresetsDir();
			case DefaultLADSPABase    : return ConfigManager::inst()->userLadspaDir();
			case UserLADSPABase       : return ConfigManager::inst()->ladspaDir();
			case DefaultSoundfontBase : return ConfigManager::inst()->userSf2Dir();
			case UserSoundfontBase    : return ConfigManager::inst()->sf2Dir();
			case DefaultGIGBase       : return ConfigManager::inst()->userGigDir();
			case UserGIGBase          : return ConfigManager::inst()->gigDir();
			default                   : return QString("");
		}
	}

	QDir baseQDir (Base base) { return QDir(baseLocation(base)); }

	QString basePrefix(Base base)
	{
		switch (base)
		{
			case ProjectDirBase       : return "userprojects:";
			case FactorySampleBase    : return "factorysample:";
			case UserSampleBase       : return "usersample:";
			case UserVSTBase          : return "uservst:";
			case PresetBase           : return "preset:";
			case DefaultLADSPABase    : return "defaultladspa:";
			case UserLADSPABase       : return "userladspa:";
			case DefaultSoundfontBase : return "defaultsoundfont:";
			case UserSoundfontBase    : return "usersoundfont:";
			case DefaultGIGBase       : return "defaultgig:";
			case UserGIGBase          : return "usergig:";
			default                   : return "";
		}
	}

	Base baseLookup(QString path)
	{
		for (auto base: relativeBases)
		{
			QString prefix = basePrefix(base);
			if ( prefix == path.left(prefix.length()) ) { return base; }
		}
		return AbsoluteBase;
	}




	QString stripPrefix(QString path)
	{
		return path.remove(0, basePrefix(baseLookup(path)).length() );
	}

	QString cleanName(QString path)
	{
		return stripPrefix(QFileInfo(path).baseName());
	}




	QString oldRelativeUpgrade(QString input)
	{
		QString factory = baseLocation(FactorySampleBase) + input;
		QFileInfo factoryInfo(factory);
		//If we can't find a factory sample, it's probably a user sample
		Base base = factoryInfo.exists() ? FactorySampleBase : UserSampleBase;
		return basePrefix(base) + input;
	}




	QString toAbsolute(QString input)
	{
		//First, do no harm to absolute paths
		QFileInfo inputFileInfo = QFileInfo(input);
		if (inputFileInfo.isAbsolute()) return input;
		//Next, handle old relative paths with no prefix
		QString upgraded = input.contains(":") ? input : oldRelativeUpgrade(input);

		Base base = baseLookup(upgraded);
		return baseLocation(base) + upgraded.remove(0, basePrefix(base).length());
	}

	QString relativeOrAbsolute(QString input, Base base)
	{
		QString absolutePath = toAbsolute(input);
		QString relativePath = baseQDir(base).relativeFilePath(absolutePath);
		return relativePath.startsWith("..") ? absolutePath : relativePath;
	}

	QString toShortestRelative(QString input)
	{
		QFileInfo inputFileInfo = QFileInfo(input);
		QString absolutePath = inputFileInfo.isAbsolute() ? input : toAbsolute(input);

		Base shortestBase = AbsoluteBase;
		for (auto base: relativeBases)
		{
			QString otherPath = relativeOrAbsolute(absolutePath, base);
			if (otherPath.length() < relativeOrAbsolute(absolutePath, shortestBase).length())
			{
				shortestBase = base;
			}
		}
		return basePrefix(shortestBase) + relativeOrAbsolute(absolutePath, shortestBase);
	}

	// QString toPreferredRelative( QString, std::vector<Base> );
}
