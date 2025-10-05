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


namespace lmms
{

namespace FileTypes
{

namespace
{

// Static map of supported extensions and their FileType, only updated on first call
std::map<QString, FileType> extensionMap()
{
	static std::map<QString, FileType> s_extensions;

	if (!s_extensions.empty()) { return s_extensions; }

	// Get file extensions that can be opened or imported by a plugin
	for (const auto& desc: getPluginFactory()->descriptors())
	{
		for (const auto& ext: desc->supportedFileTypes)
		{
			switch (desc->type)
			{
			case Plugin::Type::Instrument:
				s_extensions[ext] = FileType::InstrumentAsset;
				break;
			case Plugin::Type::ImportFilter:
				s_extensions[ext] = FileType::InstrumentAsset;
				break;
			default:
				break;
			}
		}
	}

	// Get audio file extensions
	// Do this AFTER the plugins so it doesn't map AudioFileProcessor to all audio files
	for (const auto& [name, ext]: SampleDecoder::supportedAudioTypes())
	{
		s_extensions[QString::fromStdString(ext)] = FileType::Sample;
	}

	// Get DataFile extensions
	for (const auto& [type, ext]: DataFile::allSupportedFileTypes())
	{
		s_extensions[ext] = type;
	}

	return s_extensions;
};

}




QString compileFilter(const std::initializer_list<FileType> fileTypes, const QString& label)
{
	bool includeAll = fileTypes.size() == 0;

	QStringList filterParts;
	for (const auto& [ext, type]: extensionMap())
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
	return extensionMap().contains(ext) ? extensionMap().at(ext) : FileType::Unknown;
}




std::string iconName(const QString& ext)
{
	// TODO icon for .pat files

	if (ext == "sf2" || ext == "sf3")
	{
		return "soundfont_file";
	}
	else if (ext == "dll" || ext == "so")
	{
		return "vst_plugin_file";
	}
	else if (ext == "mid" || ext == "midi" || ext == "rmi")
	{
		return "midi_file";
	}
	else
	{
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
		default:
			return "unknown_file";
		}
	}
}




bool matchPath(const std::initializer_list<FileType> types, const QString& path)
{
	auto type = FileTypes::find(QFileInfo(path).suffix().toLower());
	auto it = std::find(types.begin(), types.end(), type);
	return it != types.end();
}


} // namespace FileTypes


} // namespace lmms
