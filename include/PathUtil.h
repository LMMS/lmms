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

#include <QString>
#include <optional>
#include <string>
#include <string_view>

#include "lmms_basics.h"
#include "lmms_export.h"

namespace lmms::PathUtil
{
	enum class Base { Absolute, ProjectDir, FactorySample, UserSample, UserVST, Preset,
		UserLADSPA, DefaultLADSPA, UserSoundfont, DefaultSoundfont, UserGIG, DefaultGIG,
		LocalDir, Internal };

	//! Return the directory associated with a given base.
	//! Will return std::nullopt if the prefix could not be resolved.
	LMMS_EXPORT auto baseLocation(Base base) -> std::optional<u8string>;

	//! Return the prefix used to denote this base in path strings
	LMMS_EXPORT auto basePrefix(Base base) -> u8string_view;

	//! Return whether `path` uses the `base` prefix
	LMMS_EXPORT auto hasBase(u8string_view path, Base base) -> bool;

	//! Check the prefix of a path and return the base it corresponds to.
	//! Defaults to Base::Absolute
	LMMS_EXPORT auto baseLookup(u8string_view path) -> Base;

	//! Remove the prefix from a path, iff there is one
	LMMS_EXPORT auto stripPrefix(u8string_view path) -> u8string_view;

	//! Return the results of baseLookup() and stripPrefix()
	LMMS_EXPORT auto parsePath(u8string_view path) -> std::pair<Base, u8string_view>;

	//! Get the filename for a path, handling prefixed paths correctly
	LMMS_EXPORT auto cleanName(const QString& path) -> QString;

	//! Get the filename for a path, handling prefixed paths correctly
	LMMS_EXPORT auto cleanName(u8string_view path) -> std::string;

	//! Make this path absolute. If a pointer to boolean is given
	//! it will indicate whether the path was converted successfully
	LMMS_EXPORT auto toAbsolute(const QString& input, bool* error = nullptr) -> QString;

	//! Make this path absolute. Returns std::nullopt upon failure
	LMMS_EXPORT auto toAbsolute(u8string_view input) -> std::optional<u8string>;

	//! Make this path relative to a given base, return an absolute path if that fails
	LMMS_EXPORT auto relativeOrAbsolute(const QString& input, const Base base) -> QString;

	//! Make this path relative to a given base, return an absolute path if that fails.
	LMMS_EXPORT auto relativeOrAbsolute(u8string_view input, const Base base) -> u8string;

	//! Make this path relative to any base, choosing the shortest if there are
	//! multiple options. allowLocal defines whether local paths should be considered.
	//! Defaults to an absolute path if all bases fail.
	LMMS_EXPORT auto toShortestRelative(const QString& input, bool allowLocal = false) -> QString;

	//! Make this path relative to any base, choosing the shortest if there are
	//! multiple options. allowLocal defines whether local paths should be considered.
	//! Defaults to an absolute path if all bases fail.
	LMMS_EXPORT auto toShortestRelative(u8string_view input, bool allowLocal = false) -> u8string;
} // namespace lmms::PathUtil

#endif // LMMS_PATHUTIL_H
