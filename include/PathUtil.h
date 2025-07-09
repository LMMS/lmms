/*
 * PathUtil.h
 *
 * Copyright (c) 2019-2022 Spekular <Spekularr@gmail.com>
 * Copyright (c) 2025      Dalton Messmer <messmer.dalton/at/gmail.com>
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
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "lmms_export.h"

//! Every std::string and std::string_view used by PathUtil is UTF-8 encoded
namespace lmms::PathUtil
{
	/**
	 * Each of the following represents a different base directory. A base directory
	 * has a prefix string (see `basePrefix`) which, when combined with a path relative
	 * to that base directory, forms a portable and flexible file path representation
	 * appropriate for storing in project files.
	 */
	enum class Base
	{
		//! Just a full filepath, no prefix
		Absolute,

		ProjectDir,
		FactoryProjects,
		FactorySample,
		UserSample,
		UserVST,
		Preset,
		FactoryPresets,
		UserLADSPA,
		DefaultLADSPA,
		UserSoundfont,
		DefaultSoundfont,
		UserGIG,
		DefaultGIG,

		//! The directory of the current project file
		LocalDir,

		//! A virtual path within LMMS or a plugin
		Internal
	};

	/**
	 * @returns The directory associated with the given base,
	 *          or std::nullopt if the prefix could not be resolved
	 */
	LMMS_EXPORT auto baseLocation(Base base) -> std::optional<std::string>;

	//! @returns The prefix used to denote this base in path strings
	LMMS_EXPORT auto basePrefix(Base base) -> std::string_view;

	//! @returns Whether `path` uses the `base` prefix
	LMMS_EXPORT auto hasBase(std::string_view path, Base base) -> bool;

	/**
	 * @brief Parses a path by checking if it contains any known base prefix
	 *
	 * @param path A prefixed or absolute path
	 * @returns The base prefix + the path with the prefix stripped off.
	 *          If the base prefix cannot be determined, assumes the path is absolute.
	 */
	LMMS_EXPORT auto parsePath(std::string_view path) -> std::pair<Base, std::string_view>;

	//! @returns The extensionless base name (stem) of the given path
	LMMS_EXPORT auto cleanName(const QString& path) -> QString;

	//! @returns The extensionless base name (stem) of the given path
	LMMS_EXPORT auto cleanName(std::string_view path) -> std::string;

	/**
	 * @brief Converts a path to an absolute path
	 *
	 * @param input A prefixed path, absolute path, or a (legacy) non-prefixed relative path
	 * @param error For optional error reporting
	 * @returns An absolute path if successful, else an empty string
	 */
	LMMS_EXPORT auto toAbsolute(const QString& input, bool* error = nullptr) -> QString;

	/**
	 * @brief Converts a path to an absolute path
	 *
	 * @param input A prefixed path, absolute path, or a (legacy) non-prefixed relative path
	 * @returns An absolute path if successful, else std::nullopt
	 */
	LMMS_EXPORT auto toAbsolute(std::string_view input) -> std::optional<std::string>;

	/**
	 * @brief Makes this path relative to any base, choosing the shortest path if there
	 *        are multiple options.
	 *
	 * @param input Any path
	 * @param allowLocal Whether local paths (the project file's directory) should be considered
	 * @returns The shortest relative path if successful. If all bases fail, returns an absolute path.
	 *          And if the absolute path conversion fails, returns the input unchanged.
	 */
	LMMS_EXPORT auto toShortestRelative(const QString& input, bool allowLocal = false) -> QString;

	/**
	 * @brief Makes this path relative to any base, choosing the shortest path if there
	 *        are multiple options.
	 *
	 * @param input Any path
	 * @param allowLocal Whether local paths (the project file's directory) should be considered
	 * @returns The shortest relative path if successful. If all bases fail, returns an absolute path.
	 *          And if the absolute path conversion fails, returns the input unchanged.
	 */
	LMMS_EXPORT auto toShortestRelative(std::string_view input, bool allowLocal = false) -> std::string;

	//! Converts std::u8string_view to std::string
	LMMS_EXPORT auto toStdString(std::u8string_view input) -> std::string;

	//! Converts std::u8string_view to std::string_view
	LMMS_EXPORT auto toStdStringView(std::u8string_view input) -> std::string_view;

	//! Converts QString to std::filesystem::path
	LMMS_EXPORT auto stringToPath(const QString& path) -> std::filesystem::path;

	//! Converts std::filesystem::path to a UTF-8 encoded std::string
	LMMS_EXPORT auto pathToString(const std::filesystem::path& path) -> std::string;

	//! Warning-free replacement for the deprecated std::filesystem::u8path function
	LMMS_EXPORT auto u8path(std::string_view path) -> std::filesystem::path;
} // namespace lmms::PathUtil

#endif // LMMS_PATHUTIL_H
