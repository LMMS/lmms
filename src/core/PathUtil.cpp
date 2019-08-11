#include "PathUtil.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include "ConfigManager.h"

namespace PathUtil
{
	std::vector<Base> relativeBases = { FactoryBase, SampleBase, VSTBase, SoundfontBase };

	QString baseLocation(Base base)
	{
		switch (base)
		{
			case FactoryBase   :
			{
				QDir fsd = QDir(ConfigManager::inst()->factorySamplesDir());
				return fsd.absolutePath() + "/";
			}
			case SampleBase    : return ConfigManager::inst()->userSamplesDir();
			case VSTBase       : return ConfigManager::inst()->userVstDir();
			case SoundfontBase : return ConfigManager::inst()->userSf2Dir();
			default            : return QString("");
		}
	}

	QDir baseQDir (Base base) { return QDir(baseLocation(base)); }

	QString basePrefix(Base base)
	{
		switch (base)
		{
			case FactoryBase   : return "factory:";
			case SampleBase    : return "sample:";
			case VSTBase       : return "vst:";
			case SoundfontBase : return "soundfont:";
			default            : return "";
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
		QString factory = baseLocation(FactoryBase) + input;
		QFileInfo factoryInfo(factory);
		//If we can't find a factory sample, it's probably a user sample
		return factoryInfo.exists() ? "factory:" + input : "sample:" + input;
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
