#include "PathUtil.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include "ConfigManager.h"
#include "Engine.h"
#include "Song.h"

namespace PathUtil
{
	Base relativeBases[] = { Base::ProjectDir, Base::FactorySample, Base::UserSample, Base::UserVST, Base::Preset,
		Base::UserLADSPA, Base::DefaultLADSPA, Base::UserSoundfont, Base::DefaultSoundfont, Base::UserGIG, Base::DefaultGIG,
		Base::LocalDir };

	QString baseLocation(const Base base)
	{
		QString loc = "";
		switch (base)
		{
			case Base::ProjectDir       : loc = ConfigManager::inst()->userProjectsDir(); break;
			case Base::FactorySample    :
			{
				QDir fsd = QDir(ConfigManager::inst()->factorySamplesDir());
				loc = fsd.absolutePath(); break;
			}
			case Base::UserSample       : loc = ConfigManager::inst()->userSamplesDir(); break;
			case Base::UserVST          : loc = ConfigManager::inst()->userVstDir(); break;
			case Base::Preset           : loc = ConfigManager::inst()->userPresetsDir(); break;
			case Base::UserLADSPA       : loc = ConfigManager::inst()->ladspaDir(); break;
			case Base::DefaultLADSPA    : loc = ConfigManager::inst()->userLadspaDir(); break;
			case Base::UserSoundfont    : loc = ConfigManager::inst()->sf2Dir(); break;
			case Base::DefaultSoundfont : loc = ConfigManager::inst()->userSf2Dir(); break;
			case Base::UserGIG          : loc = ConfigManager::inst()->gigDir(); break;
			case Base::DefaultGIG       : loc = ConfigManager::inst()->userGigDir(); break;
			case Base::LocalDir:
			{
				Song* s = Engine::getSong();
				if (s)
				{
					QString projectPath = s->projectFileName();
					loc = QFileInfo(projectPath).path();
				}
				else
				{
					// Keeps the base prefix so the method calling this
					// can handle the situation.
					return basePrefix(Base::LocalDir);
				}
				break;
			}
			default                   : return QString("");
		}
		return QDir::cleanPath(loc) + "/";
	}

	QDir baseQDir (const Base base)
	{
		if (base == Base::Absolute) { return QDir::root(); }
		return QDir(baseLocation(base));
	}

	QString basePrefix(const Base base)
	{
		switch (base)
		{
			case Base::ProjectDir       : return QStringLiteral("userprojects:");
			case Base::FactorySample    : return QStringLiteral("factorysample:");
			case Base::UserSample       : return QStringLiteral("usersample:");
			case Base::UserVST          : return QStringLiteral("uservst:");
			case Base::Preset           : return QStringLiteral("preset:");
			case Base::UserLADSPA       : return QStringLiteral("userladspa:");
			case Base::DefaultLADSPA    : return QStringLiteral("defaultladspa:");
			case Base::UserSoundfont    : return QStringLiteral("usersoundfont:");
			case Base::DefaultSoundfont : return QStringLiteral("defaultsoundfont:");
			case Base::UserGIG          : return QStringLiteral("usergig:");
			case Base::DefaultGIG       : return QStringLiteral("defaultgig:");
			case Base::LocalDir         : return QStringLiteral("local:");
			default                     : return QStringLiteral("");
		}
	}

	Base baseLookup(const QString & path)
	{
		for (auto base: relativeBases)
		{
			QString prefix = basePrefix(base);
			if ( path.startsWith(prefix) ) { return base; }
		}
		return Base::Absolute;
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
		return basePrefix(assumedBase) + input;
	}




	QString toAbsolute(const QString & input)
	{
		//First, do no harm to absolute paths
		QFileInfo inputFileInfo = QFileInfo(input);
		if (inputFileInfo.isAbsolute()) { return input; }
		//Next, handle old relative paths with no prefix
		QString upgraded = input.contains(":") ? input : oldRelativeUpgrade(input);

		Base base = baseLookup(upgraded);
		return baseLocation(base) + upgraded.remove(0, basePrefix(base).length());
	}

	QString relativeOrAbsolute(const QString & input, const Base base)
	{
		if (input.isEmpty()) { return input; }
		QString absolutePath = toAbsolute(input);
		if (base == Base::Absolute) { return absolutePath; }
		QString relativePath = baseQDir(base).relativeFilePath(absolutePath);
		return relativePath.startsWith("..") ? absolutePath : relativePath;
	}

	QString toShortestRelative(const QString & input)
	{
		QFileInfo inputFileInfo = QFileInfo(input);
		QString absolutePath = inputFileInfo.isAbsolute() ? input : toAbsolute(input);

		Base shortestBase = Base::Absolute;
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
}
