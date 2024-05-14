/*
 * PathUtil.cpp
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

#include "PathUtil.h"

#include <QDir>
#include <QFileInfo>

#include "ConfigManager.h"
#include "Engine.h"
#include "lmms_filesystem.h"
#include "Song.h"

namespace lmms::PathUtil
{
	namespace
	{
		constexpr auto relativeBases = std::array{ Base::ProjectDir, Base::FactorySample, Base::UserSample, Base::UserVST, Base::Preset,
			Base::UserLADSPA, Base::DefaultLADSPA, Base::UserSoundfont, Base::DefaultSoundfont, Base::UserGIG, Base::DefaultGIG,
			Base::LocalDir, Base::Internal };

		std::string oldRelativeUpgrade(std::string_view input)
		{
			auto inputStr = std::string{input};
			if (input.empty()) { return inputStr; }

			// Start by assuming that the file is a user sample
			Base assumedBase = Base::UserSample;

			// Check if it's a factory sample
			const auto factoryPath = fs::u8path(baseLocation(Base::FactorySample).value() + inputStr);
			if (std::error_code ec; fs::exists(factoryPath, ec)) { assumedBase = Base::FactorySample; }

			// Check if it's a VST
			const auto vstPath = fs::u8path(baseLocation(Base::UserVST).value() + inputStr);
			if (std::error_code ec; fs::exists(vstPath, ec)) { assumedBase = Base::UserVST; }

			// Assume we've found the correct base location, return the full path
			return std::string{basePrefix(assumedBase)} + inputStr;
		}

		//! Return the directory associated with a given base as a QDir.
		//! Will return std::nullopt if the prefix could not be resolved.
		std::optional<fs::path> baseDir(const Base base)
		{
			if (base == Base::Absolute)
			{
				return fs::current_path().root_path();
			}

			if (auto loc = baseLocation(base))
			{
				return fs::u8path(*loc);
			}

			return std::nullopt;
		}
	} // namespace

	std::optional<std::string> baseLocation(Base base)
	{
		QString loc;
		switch (base)
		{
			case Base::ProjectDir       : loc = ConfigManager::inst()->userProjectsDir(); break;
			case Base::FactorySample    :
			{
				const auto fsd = QDir{ConfigManager::inst()->factorySamplesDir()};
				loc = fsd.absolutePath();
				break;
			}
			case Base::UserSample       : loc = ConfigManager::inst()->userSamplesDir(); break;
			case Base::UserVST          : loc = ConfigManager::inst()->userVstDir(); break;
			case Base::Preset           : loc = ConfigManager::inst()->userPresetsDir(); break;
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
		return (QDir::cleanPath(loc) + "/").toStdString();
	}

	std::string_view basePrefix(const Base base)
	{
		switch (base)
		{
			case Base::ProjectDir       : return "userprojects:";
			case Base::FactorySample    : return "factorysample:";
			case Base::UserSample       : return "usersample:";
			case Base::UserVST          : return "uservst:";
			case Base::Preset           : return "preset:";
			case Base::UserLADSPA       : return "userladspa:";
			case Base::DefaultLADSPA    : return "defaultladspa:";
			case Base::UserSoundfont    : return "usersoundfont:";
			case Base::DefaultSoundfont : return "defaultsoundfont:";
			case Base::UserGIG          : return "usergig:";
			case Base::DefaultGIG       : return "defaultgig:";
			case Base::LocalDir         : return "local:";
			case Base::Internal         : return "internal:";
			default                     : return "";
		}
	}

	bool hasBase(std::string_view path, Base base)
	{
		if (base == Base::Absolute)
		{
			return baseLookup(path) == base;
		}

		auto prefix = basePrefix(base);
		return path.rfind(prefix, 0) == 0;
	}

	Base baseLookup(std::string_view path)
	{
		for (auto base : relativeBases)
		{
			const auto prefix = basePrefix(base);
			if (path.rfind(prefix, 0) == 0) { return base; }
		}
		return Base::Absolute;
	}

	std::string_view stripPrefix(std::string_view path)
	{
		path.remove_prefix(basePrefix(baseLookup(path)).length());
		return path;
	}

	std::pair<Base, std::string_view> parsePath(std::string_view path)
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

	QString cleanName(const QString& path)
	{
		return QString::fromStdString(cleanName(path.toStdString()));
	}

	std::string cleanName(std::string_view path)
	{
		auto filename = fs::u8path(path).filename().string();

		// filename.stem() would return the name up until the last '.' but
		//   Qt's QFileInfo.baseName() returns up until the *first* '.'
		return filename.substr(0, filename.find('.'));
	}

	QString toAbsolute(const QString& input, bool* error /* = nullptr*/)
	{
		if (const auto result = toAbsolute(input.toStdString()))
		{
			if (error) { *error = false; }
			return QString::fromStdString(*result);
		}

		if (error) { *error = true; }
		return QString{};
	}

	std::optional<std::string> toAbsolute(std::string_view input)
	{
		// First, check if it's Internal
		auto inputStr = std::string{input};
		if (hasBase(input, Base::Internal)) { return inputStr; }

		// Secondly, do no harm to absolute paths
		auto inputPath = fs::u8path(input);
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

	QString relativeOrAbsolute(const QString& input, const Base base)
	{
		return QString::fromStdString(relativeOrAbsolute(input.toStdString(), base));
	}

	std::string relativeOrAbsolute(std::string_view input, const Base base)
	{
		if (input.empty()) { return std::string{input}; }

		std::string absolutePath = toAbsolute(input).value_or(std::string{});
		if (base == Base::Absolute || base == Base::Internal) { return absolutePath; }

		auto bd = baseDir(base);
		if (!bd) { return absolutePath; }

		std::error_code ec;
		auto relativePath = fs::relative(absolutePath, *bd, ec).string();
		if (ec) { return absolutePath; }

		// Return the relative path if it didn't result in a path starting with ".."
		// and the baseDir was resolved properly
		return relativePath.rfind("..", 0) != std::string::npos
			? absolutePath
			: relativePath;
	}

	QString toShortestRelative(const QString& input, bool allowLocal /* = false*/)
	{
		return QString::fromStdString(toShortestRelative(input.toStdString(), allowLocal));
	}

	std::string toShortestRelative(std::string_view input, bool allowLocal /* = false*/)
	{
		auto inputStr = std::string{input};
		if (hasBase(input, Base::Internal)) { return inputStr; }

		const auto inputPath = fs::u8path(input);
		std::string absolutePath = toAbsolute(input).value();

		Base shortestBase = Base::Absolute;
		std::string shortestPath = relativeOrAbsolute(absolutePath, shortestBase);
		for (auto base : relativeBases)
		{
			// Skip local paths when searching for the shortest relative if those
			// are not allowed for that resource
			if (base == Base::LocalDir && !allowLocal) { continue; }

			std::string otherPath = relativeOrAbsolute(absolutePath, base);
			if (otherPath.length() < shortestPath.length())
			{
				shortestBase = base;
				shortestPath = otherPath;
			}
		}

		return std::string{basePrefix(shortestBase)} + relativeOrAbsolute(absolutePath, shortestBase);
	}

} // namespace lmms::PathUtil
