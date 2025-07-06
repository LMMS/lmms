/*
 * PathUtil.h
 *
 * Copyright (c) 2019-2022 Spekular <Spekularr@gmail.com>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#ifndef LMMS_PATHUTIL_H
#define LMMS_PATHUTIL_H

#include "lmms_export.h"

#include <QDir>
#include <optional>
#include <string>
#include <string_view>

namespace lmms::PathUtil
{
	enum class Base { Absolute, ProjectDir, FactoryProjects, FactorySample, UserSample, UserVST, Preset,
		FactoryPresets, UserLADSPA, DefaultLADSPA, UserSoundfont, DefaultSoundfont, UserGIG, DefaultGIG,
		LocalDir, Internal };

	//! Return the directory associated with a given base as a QString
	//! Optionally, if a pointer to boolean is given the method will
	//! use it to indicate whether the prefix could be resolved properly
	//! or not.
	QString LMMS_EXPORT baseLocation(const Base base, bool* error = nullptr);

	//! Return the directory associated with a given base as a std::string.
	//! Will return std::nullopt if the prefix could not be resolved.
	std::optional<std::string> LMMS_EXPORT getBaseLocation(Base base);

	//! Return the directory associated with a given base as a QDir.
	//! Optional pointer to boolean to indicate if the prefix could
	//! be resolved properly.
	QDir LMMS_EXPORT baseQDir (const Base base, bool* error = nullptr);

	//! Return the prefix used to denote this base in path strings
	QString LMMS_EXPORT basePrefixQString(const Base base);

	//! Return the prefix used to denote this base in path strings
	std::string_view LMMS_EXPORT basePrefix(const Base base);

	//! Return whether the path uses the `base` prefix
	bool LMMS_EXPORT hasBase(const QString& path, Base base);

	//! Return whether the path uses the `base` prefix
	bool LMMS_EXPORT hasBase(std::string_view path, Base base);

	//! Check the prefix of a path and return the base it corresponds to
	//! Defaults to Base::Absolute
	Base LMMS_EXPORT baseLookup(const QString & path);

	//! Check the prefix of a path and return the base it corresponds to
	//! Defaults to Base::Absolute
	Base LMMS_EXPORT baseLookup(std::string_view path);

	//! Remove the prefix from a path, iff there is one
	QString LMMS_EXPORT stripPrefix(const QString & path);

	//! Remove the prefix from a path, iff there is one
	std::string_view LMMS_EXPORT stripPrefix(std::string_view path);

	//! Return the results of baseLookup() and stripPrefix()
	std::pair<Base, std::string_view> LMMS_EXPORT parsePath(std::string_view path);

	//! Get the filename for a path, handling prefixed paths correctly
	QString LMMS_EXPORT cleanName(const QString & path);

	//! Upgrade prefix-less relative paths to the new format
	QString LMMS_EXPORT oldRelativeUpgrade(const QString & input);

	//! Make this path absolute. If a pointer to boolean is given
	//! it will indicate whether the path was converted successfully
	QString LMMS_EXPORT toAbsolute(const QString & input, bool* error = nullptr);

	//! Make this path absolute. Returns std::nullopt upon failure.
	std::optional<std::string> LMMS_EXPORT toAbsolute(std::string_view input);

	//! Make this path relative to a given base, return an absolute path if that fails
	QString LMMS_EXPORT relativeOrAbsolute(const QString & input, const Base base);

	//! Make this path relative to any base, choosing the shortest if there are
	//! multiple options. allowLocal defines whether local paths should be considered.
	//! Defaults to an absolute path if all bases fail.
	QString LMMS_EXPORT toShortestRelative(const QString & input, bool allowLocal = false);

	//! Make this path relative to any base, choosing the shortest if there are
	//! multiple options. allowLocal defines whether local paths should be considered.
	//! Defaults to an absolute path if all bases fail.
	std::string LMMS_EXPORT toShortestRelative(std::string_view input, bool allowLocal = false);
} // namespace lmms::PathUtil

#endif // LMMS_PATHUTIL_H
