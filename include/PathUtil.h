#ifndef _PATHUTIL
#define _PATHUTIL

#include <QDebug>

namespace PathUtil
{
	enum Base { AbsoluteBase, FactoryBase, SampleBase, VSTBase, SoundfontBase };
	std::vector<Base> relativeBases = { FactoryBase, SampleBase, VSTBase, SoundfontBase };
	bool initialized = false;

	QString baseLocation( Base base )
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

	QString basePrefix( Base base )
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

	Base baseLookup( QString path )
	{
		for (auto base: relativeBases)
		{
			QString prefix = basePrefix( base );
			if ( prefix == path.left(prefix.length()) ) return base;
		}
		return AbsoluteBase;
	}

	QDir baseQDir ( Base base ) { return QDir( baseLocation(base) ); }

	void initializeSearchPaths()
	{
		if (!initialized)
		{
			for (auto base: relativeBases) QDir::addSearchPath( basePrefix(base), baseLocation(base) );
			initialized = true;
		}
	}

	QString oldRelativeUpgrade( QString input )
	{
		initializeSearchPaths();
		QString factory = baseLocation(FactoryBase) + input;
		QFileInfo factoryInfo(factory);
		//If we can't find a factory sample, it's probably a user sample
		return factoryInfo.exists() ? "factory:" + input : "sample:" + input;
	}

	QString toAbsolute( QString input )
	{
		initializeSearchPaths();
		//First, do no harm to absolute paths
		QFileInfo inputFileInfo = QFileInfo(input);
		if ( inputFileInfo.isAbsolute() ) return input;
		//Next, handle old relative paths with no prefix
		QString upgraded = input.contains(":") ? input : oldRelativeUpgrade( input );

		Base base = baseLookup(upgraded);
		return baseLocation(base) + upgraded.remove(0, basePrefix(base).length() );
	}

	QString relativeOrAbsolute( QString input, Base base )
	{
		initializeSearchPaths();
		QString absolutePath = toAbsolute(input);
		QString relativePath = baseQDir(base).relativeFilePath( absolutePath );
		return relativePath.startsWith("..") ? absolutePath : relativePath;
	}

	QString toShortestRelative( QString input )
	{
		initializeSearchPaths();
		QFileInfo inputFileInfo = QFileInfo( input );
		QString absolutePath = inputFileInfo.isAbsolute() ? input : toAbsolute(input);

		Base shortestBase = AbsoluteBase;
		for (auto base: relativeBases){
			QString otherPath = relativeOrAbsolute( absolutePath, base );
			if ( otherPath.length() < relativeOrAbsolute( absolutePath, shortestBase ).length() )
			{ shortestBase = base; }
		}
		return basePrefix(shortestBase) + relativeOrAbsolute( absolutePath, shortestBase );
	}

	// QString PathUtil::toPreferredRelative( QString, std::vector<Base> );
}

#endif
