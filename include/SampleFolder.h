/*
 * SampleFrame.h - Representation of a stereo sample
 *
 * Copyright (c) 2024 szeli1 </at/gmail/dot/com> TODO
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
 
#ifndef LMMS_SAMPLEFOLDER_H
#define LMMS_SAMPLEFOLDER_H

#include "lmms_basics.h"

#include <filesystem>
#include <list>
#include <string>

//#include <QStrintg> // TODO

namespace lmms
{

class SampleFolder
{
public:
	//! folderPath: from where should SampleFolder load in the samples
	SampleFolder(std::string folderPath);
	~SampleFolder();

	void setTargetFolderPath(std::string folderPath);

	void updateAllFilesList();

	//! use this to save sample files
	//! this will only save a file if getSampleFileSourceName() == true and sampleFileName is not inside m_loadedFilesList
	//! sampleFileName returns the final name
	void saveUnchangedSample(std::shared_ptr<const SampleBuffer> sampleBuffer, std::string* sampleFileName);
	//! this will update the data related with a sampleFileName
	void updateSample(std::shared_ptr<const SampleBuffer> sampleBuffer, const std::string& sampleFileName);
	//! this accepts samples from all location
	//! it will decide to load
	std::shared_ptr<SampleBuffer> loadSample(const std::string& sampleFileName);

	//! appends / removes a temporary tag to a given file name if it is managed by / loaded with SampleFolder
	static void setSampleFileSourceName(std::string* sampleFileName, bool isManagedBySampleFolder);
	//! returns true if sampleFileName is managed by SampleFolder
	static bool getSampleFileSourceName(const std::string& sampleFileName, bool isOnlyNameStem = false);

	const static std::string s_sampleFileSourceName;// = "LOAD_WITH_SAMPLEFOLDER";
private:
	//! used for adding a new sample
	//! sampleInformation: string that's attached at the end of the file name for readability
	//! returns true if successful
	bool saveSample(std::shared_ptr<const SampleBuffer> sampleBuffer, const std::string& sampleFileName);
	size_t getFileNumber(const std::filesystem::path& sampleFilePath);

	std::string m_targetFolderPath;

	//! list of used file names inside m_targetFolderPath
	std::list<std::string> m_loadedFilesList;
	//! list of all file names inside m_targetFolderPath
	std::list<std::string> m_allFilesList;
};

} // namespace lmms

#endif // LMMS_SAMPLEFOLDER_H
