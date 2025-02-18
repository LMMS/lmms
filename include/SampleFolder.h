/*
 * SampleFolder.h - Manages sample loading and saving from a sample folder
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

#include <memory>
#include <vector>

#include <QString>

namespace lmms
{

class LmmsExporterSample;
class SampleBuffer;

class SampleFolder
{
public:
	//! folderPath: from where should SampleFolder load in the samples
	SampleFolder();
	~SampleFolder();

	//! folderPath: the path to the sample folder
	//! filterFileName: should get rid of file names inside the path
	void setTargetFolderPath(const QString& folderPath, bool shouldFilterFileName);
	//! sets folder path to ConfigManager's default SampleFolder path
	void resetTargetFolderPath();

	//! scans all files in `m_targetFolderPath` and makes an array out of them
	void updateAllFilesList();

	//! makes new dirs (`ConfigManager::COMMON_SAMPLE_FOLDER_USED`
	//! and `ConfigManager::COMMON_SAMPLE_FOLDER_UNUSED`) inside a given path
	static void makeSampleFolderDirs(const QString& path);

	//! retruns loaded sample or SampleBuffer::emptyBuffer if not found
	//! sampleFileName: could be a name contained in the sample folder, or a path to a sample anywhere
	//! sampleFileFinalName: if the sample was found, retruns a name that should be used to refer to that sample, else it will not be modified
	std::shared_ptr<const SampleBuffer> loadSample(const QString& sampleFileName, QString* sampleFileFinalName);

	//! sampleFileName: the exact file name (not path) that is stored inside the sample folder, or that needs to be saved as a new file
	//! isManagedBySampleFolder: if true, then the sample will be inside `ConfigManager::COMMON_SAMPLE_FOLDER_USED`
	//! or `ConfigManager::COMMON_SAMPLE_FOLDER_UNUSED`
	//! shouldGenerateUniqueName: forces saving the sample into a new file, appends a unique number behind `sampleFileName` and exports it as a new file
	//! sampleFileFinalName: returns the final file name, could be nullptr
	void saveSample(std::shared_ptr<const SampleBuffer> sampleBuffer, const QString& sampleFileName, bool isManagedBySampleFolder, bool shouldGenerateUniqueName, QString* sampleFileFinalName);
	//! sampleFileName: the exact file name (not path) that is stored inside the sample folder
	void saveSample(const QString& sampleFileName);
	
	// exports a sample, only used for replacing old versions of a sample file
	void updateSample(std::shared_ptr<const SampleBuffer> sampleBuffer, const QString& sampleFileName);

	struct SampleFile
	{
		SampleFile() :
			name(""),
			relativeFolder(""),
			isManaged(false),
			isLoaded(false),
			isSaved(false),
			buffer()
		{}
		//! the Sample's name, doesn't contain path, includes extension
		QString name;
		//! where
		QString relativeFolder;
		//! if it is inside `ConfigManager::COMMON_SAMPLE_FOLDER_USED`
		//! or `ConfigManager::COMMON_SAMPLE_FOLDER_UNUSED`
		bool isManaged;
		//! if it was loaded by the project (since the last save)
		//! used for tracking whether or not a sample file should be moved to the
		//! `ConfigManager::COMMON_SAMPLE_FOLDER_USED` or `ConfigManager::COMMON_SAMPLE_FOLDER_UNUSED`
		bool isLoaded;
		bool isSaved;
		//! the sample's audio buffer (nullptr if wasLoaded == false)
		std::shared_ptr<const SampleBuffer> buffer;
	};

	//! moves the files inside `ConfigManager::COMMON_SAMPLE_FOLDER_USED`
	//! and `ConfigManager::COMMON_SAMPLE_FOLDER_UNUSED` to their correct places
	//! resets `SampleFile::isLoaded`, `SampleFile::isSaved` and unloads `SampleFile::buffer` if the sample isn't used
	void sortManagedFiles();
	void resetSavedStatus();

private:

	void exportSample(std::shared_ptr<const SampleBuffer> sampleBuffer, const QString& sampleFileName, bool isManagedBySampleFolder, bool shouldGenerateUniqueName, QString* sampleFileFinalName);
	
	bool isPathInsideSampleFolder(const QString& filePath);
	//! returns -1 if not found, returns index if found
	//! sampleFileName: the exact file name (not path) that is stored inside the sample folder
	ssize_t findFileInsideSampleFolder(const QString& sampleFileName);

	//! sourceName: file name (not path) with extension
	//! isSeparatedWithWhiteSpace: returns if the source name's last character before the numbers was '_'
	//! extensionCountOut: returns the number of characters that are part of the extension
	QString findUniqueName(const QString& sourceName) const;
	static QString getNameNumberEnding(const QString& name, bool* isSeparatedWithWhiteSpace, size_t* extensionCountOut);

	//! absolute path to the target sample folder
	QString m_targetFolderPath;

	//! list of all file info inside m_targetFolderPath
	std::vector<SampleFile> m_sampleFolderFiles;
	std::unique_ptr<LmmsExporterSample> m_exporter;
};

} // namespace lmms

#endif // LMMS_SAMPLEFOLDER_H
