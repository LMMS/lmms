#ifndef _PATHUTIL
#define _PATHUTIL

#include <QDir>

namespace PathUtil
{
	enum Base { AbsoluteBase, FactoryBase, SampleBase, VSTBase, SoundfontBase };

	QString baseLocation( Base base );
	QString basePrefix( Base base );
	Base baseLookup( QString path );
	QDir baseQDir ( Base base );

	QString oldRelativeUpgrade( QString input );

	QString toAbsolute( QString input );
	QString relativeOrAbsolute( QString input, Base base );
	QString toShortestRelative( QString input );

	// QString toPreferredRelative( QString, std::vector<Base> );
}

#endif
