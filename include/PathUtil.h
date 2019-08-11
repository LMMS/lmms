#ifndef PATHUTIL_H
#define PATHUTIL_H

#include "lmms_export.h"

#include <QDir>

namespace PathUtil
{
	enum Base { AbsoluteBase, FactoryBase, SampleBase, VSTBase, SoundfontBase };

	QString LMMS_EXPORT baseLocation(Base base);
	QDir LMMS_EXPORT baseQDir (Base base);
	QString LMMS_EXPORT basePrefix(Base base);
	Base LMMS_EXPORT baseLookup(QString path);

	QString LMMS_EXPORT stripPrefix(QString path);
	QString LMMS_EXPORT cleanName(QString path);

	QString LMMS_EXPORT oldRelativeUpgrade(QString input);

	QString LMMS_EXPORT toAbsolute(QString input);
	QString LMMS_EXPORT relativeOrAbsolute(QString input, Base base);
	QString LMMS_EXPORT toShortestRelative(QString input);

	// QString LMMS_EXPORT toPreferredRelative(QString, std::vector<Base>);
}

#endif
