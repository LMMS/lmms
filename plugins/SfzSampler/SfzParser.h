/*
 * SfzParser.h - Helper class for parsing .sfz files into objects which can be used by the SFZ player
 *
 * Copyright (c) 2026 Keratin
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

#ifndef LMMS_SFZ_PARSER_H
#define LMMS_SFZ_PARSER_H

#include "SfzRegion.h"
#include "SfzControlsConfig.h"
#include <QString>
#include <QDir>
#include <vector>

namespace lmms
{

class SfzParser
{
public:
	//! Loads the .sfz file at `filePath`, parses it for all the sfz headers and opcode assignments, and populates the `outputRegions` vector
	//! with new SfzRegion objects based on the configurations in the .sfz file. Additionally, it sets up `controlsConfig` with misc info about
	//! switch keys and midi CC's, which is useful for the GUI.
	static bool parseSfzFile(const QString& filePath, std::vector<SfzRegion>& outputRegions, SfzControlsConfig& controlsConfig);

private:
	//! Helper function to parse any #include or #define statements from a string, recursively so that the included files have their includes and defines handled too
	//! The defineMap parameter should be left blank, since it's only there to internally keep track of what $keywords are defined to be what as the recursion goes down each path
	static QString recursiveHandleIncludeAndDefineStatements(const QDir& parentDirectory, QString fileContents, std::map<QString, QString> defineMap = {});

	//! All the possible header types in an sfz file
	enum class Header
	{
		Global,
		Group,
		Region,
		Control,
		Curve, // TODO Not implemented yet
		None
	};
};



} // namespace lmms


#endif // LMMS_SFZ_PARSER_H