/*
 * PathUtil.cpp
 *
 * Copyright (c) 2019-2022 Spekular <Spekularr@gmail.com>
 *               2024      Dalton Messmer <messmer.dalton/at/gmail.com>
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

		//! Return the directory associated with a given base as a fs::path.
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
			return baseLookup(path) == base;
		}

		auto prefix = basePrefix(base);
		return path.rfind(prefix, 0) == 0;
	}

	auto baseLookup(std::string_view path) -> Base
	{
		for (auto base : relativeBases)
		{
			const auto prefix = basePrefix(base);
			if (path.rfind(prefix, 0) == 0) { return base; }
		}
		return Base::Absolute;
	}

	auto stripPrefix(std::string_view path) -> std::string_view
	{
		path.remove_prefix(basePrefix(baseLookup(path)).length());
		return path;
	}

	auto parsePath(std::string_view path) -> std::pair<Base, std::string_view>
	{
		for (auto base : relativeBases)
		{
			auto prefix = basePrefix(base);
			if (path.rfind(prefix, 0) == 0)
			{
				path.remove_prefix(prefix.length());
				return { base, path };
			}
		}
		return { Base::Absolute, path };
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
		// First, check if it's Internal
		auto inputStr = std::string{input};
		if (hasBase(input, Base::Internal)) { return inputStr; }

		// Secondly, do no harm to absolute paths
		auto inputPath = u8path(input);
		if (inputPath.is_absolute())
		{
			return inputStr;
		}

		// Next, handle old relative paths with no prefix
		const std::string upgraded = input.find(':') != std::string_view::npos
			? inputStr
			: oldRelativeUpgrade(input);

		const Base base = baseLookup(upgraded);
		if (auto loc = baseLocation(base))
		{
			const auto noPrefix = stripPrefix(upgraded);
			return loc->append(noPrefix.data(), noPrefix.size());
		}

		return std::nullopt;
	}

	auto relativeOrAbsolute(const QString& input, const Base base) -> QString
	{
		return QString::fromStdString(relativeOrAbsolute(input.toStdString(), base));
	}

	auto relativeOrAbsolute(std::string_view input, const Base base) -> std::string
	{
		if (input.empty()) { return std::string{input}; }

		auto absolutePath = toAbsolute(input).value_or(std::string{});
		if (base == Base::Absolute || base == Base::Internal) { return absolutePath; }

		auto bd = baseDir(base);
		if (!bd) { return absolutePath; }

		std::error_code ec;
		auto relativePath = std::filesystem::relative(absolutePath, *bd, ec).u8string();
		if (ec) { return absolutePath; }

		// Return the relative path if it didn't result in a path starting with ".."
		// and the baseDir was resolved properly
		return relativePath.rfind(u8"..", 0) != std::u8string::npos
			? absolutePath
			: toStdString(relativePath);
	}

	auto toShortestRelative(const QString& input, bool allowLocal /* = false*/) -> QString
	{
		return QString::fromStdString(toShortestRelative(input.toStdString(), allowLocal));
	}

	auto toShortestRelative(std::string_view input, bool allowLocal /* = false*/) -> std::string
	{
		auto inputStr = std::string{input};
		if (hasBase(input, Base::Internal)) { return inputStr; }

		const auto inputPath = u8path(input);
		auto absolutePath = toAbsolute(input).value();

		auto shortestBase = Base::Absolute;
		auto shortestPath = relativeOrAbsolute(absolutePath, shortestBase);
		for (auto base : relativeBases)
		{
			// Skip local paths when searching for the shortest relative if those
			// are not allowed for that resource
			if (base == Base::LocalDir && !allowLocal) { continue; }

			auto otherPath = relativeOrAbsolute(absolutePath, base);
			if (otherPath.length() < shortestPath.length())
			{
				shortestBase = base;
				shortestPath = otherPath;
			}
		}

		return std::string{basePrefix(shortestBase)} + relativeOrAbsolute(absolutePath, shortestBase);
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
