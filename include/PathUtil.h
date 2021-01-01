#ifndef PATHUTIL_H
#define PATHUTIL_H

#include "lmms_export.h"

#include <QDir>

namespace PathUtil
{
	enum class Base { Absolute, ProjectDir, FactorySample, UserSample, UserVST, Preset,
		UserLADSPA, DefaultLADSPA, UserSoundfont, DefaultSoundfont, UserGIG, DefaultGIG };

	//! Return the directory associated with a given base as a QString
	QString LMMS_EXPORT baseLocation(const Base base);
	//! Return the directory associated with a given base as a QDir
	QDir LMMS_EXPORT baseQDir (const Base base);
	//! Return the prefix used to denote this base in path strings
	QString LMMS_EXPORT basePrefix(const Base base);
	//! Check the prefix of a path and return the base it corresponds to
	//! Defaults to Base::Absolute
	Base LMMS_EXPORT baseLookup(const QString & path);

	//! Remove the prefix from a path, iff there is one
	QString LMMS_EXPORT stripPrefix(const QString & path);
	//! Get the filename for a path, handling prefixed paths correctly
	QString LMMS_EXPORT cleanName(const QString & path);

	//! Upgrade prefix-less relative paths to the new format
	QString LMMS_EXPORT oldRelativeUpgrade(const QString & input);

	//! Make this path absolute
	QString LMMS_EXPORT toAbsolute(const QString & input);
	//! Make this path relative to a given base, return an absolute path if that fails
	QString LMMS_EXPORT relativeOrAbsolute(const QString & input, const Base base);
	//! Make this path relative to any base, choosing the shortest if there are
	//! multiple options. Defaults to an absolute path if all bases fail.
	QString LMMS_EXPORT toShortestRelative(const QString & input);

}

#endif
