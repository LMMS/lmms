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

#include <memory>
#include <vector>

#include <QString>

namespace lmms
{

/* SampleFolder/
TODO
*/
class SampleBuffer;

class SampleFolder
{
public:
	//! folderPath: from where should SampleFolder load in the samples
	SampleFolder();
	~SampleFolder();

	void setTargetFolderPath(const QString& folderPath);
	//! sets folder path to ConfigManager's default SampleFolder path
	void resetTargetFolderPath();

	void updateAllFilesList();

	// retruns loaded sample or nullptr if not found
	// TODO: decide if sample is in the sample folder
	std::shared_ptr<const SampleBuffer> loadSample(const QString& sampleFileName);

	//! saves sample inside TODO
	void saveSample(std::shared_ptr<const SampleBuffer> sampleBuffer, const QString& sampleFileName, bool isManagedBySampleFodler, bool shouldGenerateUniqueName, QString* sampleFileFinalName);
	
	// exports a sample, only used for replacing old versions of same sample
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
		//! the Sample's name
		QString name;
		QString relativeFolder;
		//! if it is inside s_usedFolderName or s_unusedFolderName
		bool isManaged;
		//! if it was loaded by the project (since the last save)
		//! used for tracking whether or not a sample file should be moved to the
		//! `s_usedFolderName` or `s_unusedFolderName` folder
		bool isLoaded;
		bool isSaved;
		//! the sample's audio buffer (nullptr if wasLoaded == false)
		std::shared_ptr<const SampleBuffer> buffer;
	};

	//! moves the files inside s_usedFolderName and s_unusedFolderName to their correct places
	//! resets `SampleFile::isLoaded`, `SampleFile::isSaved` and unloads `SampleFile::buffer` if the sample isn't used
	void sortManagedFiles();

private:

	void exportSample(std::shared_ptr<const SampleBuffer> sampleBuffer, const QString& sampleFileName, bool isManagedBySampleFodler, bool shouldGenerateUniqueName, QString* sampleFileFinalName);
	
	bool isPathInsideSampleFolder(const QString& filePath);
	// returns -1 if not found, returns index if found
	ssize_t findFileInsideSampleFolder(const QString& sampleFileName);

	QString findUniqueName(const QString& sourceName) const;
	static QString getNameNumberEnding(const QString& name, bool* isSeparatedWithWhiteSpace);

	static const QString s_usedFolderName;
	static const QString s_unusedFolderName;

	QString m_targetFolderPath;

	//! list of all file info inside m_targetFolderPath
	std::vector<SampleFile> m_sampleFolderFiles;
};

} // namespace lmms

#endif // LMMS_SAMPLEFOLDER_H
