/*
 * PathUtil.cpp
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

#include "PathUtil.h"

#include <QDir>
#include <QFileInfo>
#include <cassert>
#include <filesystem>
#include <stdexcept>

#include "ConfigManager.h"
#include "Engine.h"
#include "Song.h"

namespace lmms::PathUtil
{
	namespace
	{
		constexpr auto relativeBases = std::array {
			Base::ProjectDir, Base::FactoryProjects, Base::FactorySample, Base::UserSample, Base::UserVST,
			Base::Preset, Base::FactoryPresets, Base::UserLADSPA, Base::DefaultLADSPA, Base::UserSoundfont,
			Base::DefaultSoundfont, Base::UserGIG, Base::DefaultGIG, Base::LocalDir, Base::Internal
		};

		//! Adds a prefix to a old non-prefixed relative path
		auto oldRelativeUpgrade(std::string_view input) -> std::string
		{
			auto inputStr = std::string{input};
			if (input.empty()) { return inputStr; }

			// Start by assuming that the file is a user sample
			auto assumedBase = Base::UserSample;

			// Check if it's a factory sample
			if (auto factorySampleDir = baseLocation(Base::FactorySample))
			{
				const auto potentialPath = u8path(*factorySampleDir + inputStr);
				if (std::error_code ec; std::filesystem::exists(potentialPath, ec))
				{
					assumedBase = Base::FactorySample;
				}
			}

			// Check if it's a VST
			if (auto userVstDir = baseLocation(Base::UserVST))
			{
				const auto potentialPath = u8path(*userVstDir + inputStr);
				if (std::error_code ec; std::filesystem::exists(potentialPath, ec))
				{
					assumedBase = Base::UserVST;
				}
			}

			// Assume we've found the correct base location, return the full path
			return std::string{basePrefix(assumedBase)} + inputStr;
		}

		//! Return the directory associated with a given base as a std::filesystem::path.
		//! Will return std::nullopt if the prefix could not be resolved.
		auto baseDir(Base base) -> std::optional<std::filesystem::path>
		{
			if (base == Base::Absolute)
			{
				return std::filesystem::current_path().root_path();
			}

			if (auto loc = baseLocation(base))
			{
				return u8path(*loc);
			}

			return std::nullopt;
		}
	} // namespace

	auto baseLocation(Base base) -> std::optional<std::string>
	{
		QString loc;
		switch (base)
		{
			case Base::ProjectDir       : loc = ConfigManager::inst()->userProjectsDir(); break;
			case Base::FactoryProjects  :
			{
				const auto fpd = QDir{ConfigManager::inst()->factoryProjectsDir()};
				loc = fpd.absolutePath();
				break;
			}
			case Base::FactorySample    :
			{
				const auto fsd = QDir{ConfigManager::inst()->factorySamplesDir()};
				loc = fsd.absolutePath();
				break;
			}
			case Base::UserSample       : loc = ConfigManager::inst()->userSamplesDir(); break;
			case Base::UserVST          : loc = ConfigManager::inst()->userVstDir(); break;
			case Base::Preset           : loc = ConfigManager::inst()->userPresetsDir(); break;
			case Base::FactoryPresets   :
			{
				const auto fpd = QDir{ConfigManager::inst()->factoryPresetsDir()};
				loc = fpd.absolutePath();
				break;
			}
			case Base::UserLADSPA       : loc = ConfigManager::inst()->ladspaDir(); break;
			case Base::DefaultLADSPA    : loc = ConfigManager::inst()->userLadspaDir(); break;
			case Base::UserSoundfont    : loc = ConfigManager::inst()->sf2Dir(); break;
			case Base::DefaultSoundfont : loc = ConfigManager::inst()->userSf2Dir(); break;
			case Base::UserGIG          : loc = ConfigManager::inst()->gigDir(); break;
			case Base::DefaultGIG       : loc = ConfigManager::inst()->userGigDir(); break;
			case Base::LocalDir:
			{
				const Song* s = Engine::getSong();
				if (!s) { return std::nullopt; }

				QString projectPath = s->projectFileName();
				if (projectPath.isEmpty()) { return std::nullopt; }

				// We resolved it properly if we had an open Song and the project
				// filename wasn't empty
				loc = QFileInfo{projectPath}.path();
				break;
			}
			case Base::Internal:
				return std::nullopt;
			default:
				return std::string{};
		}
		return (QDir::cleanPath(loc) + '/').toStdString();
	}

	auto basePrefix(Base base) -> std::string_view
	{
		switch (base)
		{
			case Base::Absolute         : return "";
			case Base::ProjectDir       : return "userprojects:";
			case Base::FactoryProjects  : return "factoryprojects:";
			case Base::FactorySample    : return "factorysample:";
			case Base::UserSample       : return "usersample:";
			case Base::UserVST          : return "uservst:";
			case Base::Preset           : return "preset:";
			case Base::FactoryPresets   : return "factorypreset:";
			case Base::UserLADSPA       : return "userladspa:";
			case Base::DefaultLADSPA    : return "defaultladspa:";
			case Base::UserSoundfont    : return "usersoundfont:";
			case Base::DefaultSoundfont : return "defaultsoundfont:";
			case Base::UserGIG          : return "usergig:";
			case Base::DefaultGIG       : return "defaultgig:";
			case Base::LocalDir         : return "local:";
			case Base::Internal         : return "internal:";
			default                     : throw std::invalid_argument{"PathUtil::basePrefix: Invalid base"};
		}
	}

	auto hasBase(std::string_view path, Base base) -> bool
	{
		if (base == Base::Absolute)
		{
			// Assume it's absolute if it's not a recognized relative path
			return parsePath(path).first == Base::Absolute;
		}

		auto prefix = basePrefix(base);
		return path.rfind(prefix, 0) == 0;
	}

	auto parsePath(std::string_view path) -> std::pair<Base, std::string_view>
	{
		if (path.empty())
		{
			// avoid loop
			return {Base::Absolute, path};
		}

		for (auto base : relativeBases)
		{
			auto prefix = basePrefix(base);
			if (path.rfind(prefix, 0) == 0)
			{
				path.remove_prefix(prefix.length());
				return {base, path};
			}
		}

		// None of our recognized base prefixes are present, so assume it's absolute
		return {Base::Absolute, path};
	}

	auto cleanName(const QString& path) -> QString
	{
		return QString::fromStdString(cleanName(path.toStdString()));
	}

	auto cleanName(std::string_view path) -> std::string
	{
		auto stem = pathToString(u8path(path).stem());

		// If the path has no base name and is all extension (for example, "/path/.ext"),
		// the stem will be the entire filename and start with "." or "..".
		if (!stem.empty() && stem[0] == '.') { return std::string{}; }

		return stem;
	}

	auto toAbsolute(const QString& input, bool* error /* = nullptr*/) -> QString
	{
		if (const auto result = toAbsolute(input.toStdString()))
		{
			if (error) { *error = false; }
			return QString::fromStdString(*result);
		}

		if (error) { *error = true; }
		return QString{};
	}

	auto toAbsolute(std::string_view input) -> std::optional<std::string>
	{
		// 1) Check if empty
		if (input.empty()) { return std::string{}; }

		// 2) Check if it's an internal path since they can never be made absolute
		if (hasBase(input, Base::Internal)) { return std::nullopt; }

		// 3) Return absolute path if it is already absolute
		auto inputStr = std::string{input};
		if (u8path(input).is_absolute())
		{
			return inputStr;
		}

		// 4) Handle old relative paths which have no prefix
		if (input.find(':') == std::string_view::npos)
		{
			// Upgrade to prefixed relative path
			inputStr = oldRelativeUpgrade(input);
		}

		// 5) Get the base prefix
		auto [base, noPrefix] = parsePath(inputStr);
		if (base == Base::Absolute)
		{
			// Unknown base prefix - cannot convert to absolute path
			return std::nullopt;
		}

		// 6) Try to convert relative path to absolute path
		if (auto loc = baseLocation(base))
		{
			// Append relative path to create absolute path
			loc->append(noPrefix.data(), noPrefix.size());

			if (u8path(*loc).is_absolute())
			{
				// Successful conversion
				return *loc;
			}
		}

		// Failed to convert to absolute path
		return std::nullopt;
	}

	auto toShortestRelative(const QString& input, bool allowLocal /* = false*/) -> QString
	{
		return QString::fromStdString(toShortestRelative(input.toStdString(), allowLocal));
	}

	auto toShortestRelative(std::string_view input, bool allowLocal /* = false*/) -> std::string
	{
		// First, check if it's an internal path since they can never be modified
		auto inputStr = std::string{input};
		if (hasBase(input, Base::Internal)) { return inputStr; }

		auto absolutePath = toAbsolute(input);
		if (!absolutePath)
		{
			// There's no hope of finding a shortest relative path
			// if the absolute path cannot be found.
			// TODO: Return std::nullopt?
			return inputStr;
		}

		auto makeRelativeTo = [](std::string_view absolutePath, Base base) -> std::optional<std::string> {
			assert(base != Base::Absolute);

			// No path can be made internal if it isn't already
			if (base == Base::Internal) { return std::nullopt; }

			const auto bd = baseDir(base);
			if (!bd) { return std::nullopt; }

			std::error_code ec;
			const auto relativePath = std::filesystem::relative(absolutePath, *bd, ec).u8string();
			if (ec) { return std::nullopt; }

			// Check if the path starts with ".." which would indicate `absolutePath`
			// is not contained within the base directory
			if (relativePath.rfind(u8"..", 0) != std::u8string::npos)
			{
				return std::nullopt;
			}

			// The relative path resolved properly
			return toStdString(relativePath);
		};

		auto shortestBase = Base::Absolute;
		auto shortestPath = *absolutePath;

		for (auto base : relativeBases)
		{
			// Skip local paths when searching for the shortest relative if those
			// are not allowed for that resource
			if (base == Base::LocalDir && !allowLocal) { continue; }

			if (auto relativePath = makeRelativeTo(*absolutePath, base))
			{
				if (relativePath->length() < shortestPath.length())
				{
					shortestBase = base;
					shortestPath = *relativePath;
				}
			}
		}

		// TODO: Return std::nullopt if a shortest relative path was not found?
		return shortestBase == Base::Absolute
			? *absolutePath
			: std::string{basePrefix(shortestBase)} + shortestPath;
	}

	auto toStdString(std::u8string_view input) -> std::string
	{
		return !input.empty()
			? std::string{reinterpret_cast<const char*>(input.data()), input.size()}
			: std::string{};
	}

	auto toStdStringView(std::u8string_view input) -> std::string_view
	{
		return !input.empty()
			? std::string_view{reinterpret_cast<const char*>(input.data()), input.size()}
			: std::string_view{};
	}

	auto stringToPath(const QString& path) -> std::filesystem::path
	{
#ifdef _WIN32
		return path.toStdWString();
#else
		return path.toStdString();
#endif
	}

	auto pathToString(const std::filesystem::path& path) -> std::string
	{
#ifdef _WIN32
		const auto utf8String = path.u8string();
		return {reinterpret_cast<const char*>(utf8String.c_str()), utf8String.size()};
#else
		// Assume UTF-8 encoding on non-Windows
		return path.string();
#endif
	}

	auto u8path(std::string_view path) -> std::filesystem::path
	{
		// Disable decrecation warnings temporarily
#if defined(__GNUC__) || defined(__clang__)
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable: 4996)
#endif

		return std::filesystem::u8path(path);

		// Restore deprecation warnings
#if defined(__GNUC__) || defined(__clang__)
#	pragma GCC diagnostic pop
#elif defined(__MSC_VER)
#	pragma warning(pop)
#endif
	}

} // namespace lmms::PathUtil
