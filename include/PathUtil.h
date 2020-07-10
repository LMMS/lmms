#ifndef PATHUTIL_H
#define PATHUTIL_H

#include "lmms_export.h"

#include <QDir>

namespace PathUtil
{
	enum class Base { Absolute, ProjectDir, FactorySample, UserSample, UserVST, Preset,
		UserLADSPA, DefaultLADSPA, UserSoundfont, DefaultSoundfont, UserGIG, DefaultGIG };

	QString LMMS_EXPORT baseLocation(const Base base);
	QDir LMMS_EXPORT baseQDir (const Base base);
	QString LMMS_EXPORT basePrefix(const Base base);
	Base LMMS_EXPORT baseLookup(const QString & path);

	QString LMMS_EXPORT stripPrefix(const QString & path);
	QString LMMS_EXPORT cleanName(const QString & path);

	QString LMMS_EXPORT oldRelativeUpgrade(const QString & input);

	QString LMMS_EXPORT toAbsolute(const QString & input);
	QString LMMS_EXPORT relativeOrAbsolute(const QString & input, const Base base);
	QString LMMS_EXPORT toShortestRelative(const QString & input);

}

#endif
