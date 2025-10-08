/*
 * FileTypes.cpp
 *
 * Copyright (c) 2025 allejok96 <gmail>
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
 */

#include "FileTypes.h"

#include <QFileInfo>

#include "DataFile.h"
#include "PluginFactory.h"
#include "SampleDecoder.h"


namespace lmms::FileTypes
{

namespace
{

// Extension to FileType
static std::map<QString, FileType> s_types;

// Extension to icon name (only from plugin data)
static std::map<QString, std::string> s_icons;


void initExtensions()
{
	if (!s_types.empty()) { return; }

	// Get file extensions that can be opened or imported by a plugin
	for (const auto& desc: getPluginFactory()->descriptors())
	{
		for (const auto& fileTypeInfo: desc->supportedFileTypes)
		{
			switch (desc->type)
			{
			case Plugin::Type::Instrument:
				s_types[fileTypeInfo.ext] = FileType::InstrumentAsset;
				break;
			case Plugin::Type::ImportFilter:
				s_types[fileTypeInfo.ext] = FileType::InstrumentAsset;
				break;
			default:
				break;
			}

			if (!fileTypeInfo.iconName.empty())
			{
				s_icons[fileTypeInfo.ext] = fileTypeInfo.iconName;
			}
		}
	}

   // Get audio file extensions
   // Do this AFTER the plugins so it doesn't map AudioFileProcessor to all audio files
	for (const auto& [name, ext]: SampleDecoder::supportedAudioTypes())
	{
		s_types[QString::fromStdString(ext)] = FileType::Sample;
	}

	// Get DataFile extensions
	for (const auto& [type, ext]: DataFile::allSupportedFileTypes())
	{
		s_types[ext] = type;
	}
};

}




QString compileFilter(const std::initializer_list<FileType> fileTypes, const QString& label)
{
	initExtensions();

	bool includeAll = fileTypes.size() == 0;

	QStringList filterParts;
	for (const auto& [ext, type]: s_types)
	{
		if (includeAll || std::find(fileTypes.begin(), fileTypes.end(), type) != fileTypes.end())
		{
			filterParts.append("*." + ext);
		}
	}

	QString filterString = filterParts.join(" ");
	return label.isEmpty() ? filterString : label + " (" + filterString + ")";
}




FileType find(const QString& ext)
{
	initExtensions();

	return s_types.contains(ext) ? s_types.at(ext) : FileType::Unknown;
}




std::string iconName(const QString& ext)
{
	initExtensions();

	if (s_icons.contains(ext)) { return s_icons.at(ext); }

	switch (FileTypes::find(ext))
	{
	case FileType::Sample:
		return "sample_file";
	case FileType::Project:
	case FileType::ProjectTemplate:
	case FileType::ImportableProject:
		return "project_file";
	case FileType::InstrumentPreset:
	case FileType::InstrumentAsset:
		return "preset_file";
	case FileType::MidiClipData:
		// TODO this .xpt (exported midi notes from piano roll) not .mid
		return "midi_file";
	default:
		return "unknown_file";
	}
}


} // namespace lmms::FileTypes
