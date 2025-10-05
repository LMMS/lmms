/*
 * FileTypes.h
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

#ifndef LMMS_FILE_TYPES_H
#define LMMS_FILE_TYPES_H

#include <QString>


namespace lmms
{

class DataFile;

enum class FileType
{
	Unknown,
	ImportableProject, //!< any file that can be imported by some import filter
	InstrumentAsset, //!< any file that can be loaded by some instrument plugin
	InstrumentPreset, //!< DataFile of Instrument
	MidiClipData, //!< DataFile of MidiClip
	Project,
	ProjectTemplate,
	Sample, //! any audio file supported by SampleDecoder
};


namespace FileTypes
{
	//! Create a filter string to be used in file dialogs
	QString compileFilter(const std::initializer_list<FileType> fileTypes = {}, const QString& label = {});

	//! Return a FileType for the given file extension
	FileType find(const QString& ext);

	//! Return an icon name for the given file extension
	std::string iconName(const QString& ext);

	//! True if any of the file types matches the path
	bool matchPath(const std::initializer_list<FileType> fileTypes, const QString& path);

} // namespace FileTypes

} // namespace lmms

#endif // LMMS_FILE_TYPES_H
