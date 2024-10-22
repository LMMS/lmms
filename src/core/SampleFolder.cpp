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

#include "SampleFolder.h"

#include "LmmsExporterSample.h"

namespace lmms
{

std::string SampleFolder::s_sampleFileSourceName = "LOAD_WITH_SAMPLEFOLDER";

SampleFolder::SampleFolder(std::filesystem::path folderPath)
{
	setTargetFolderPath(folderPath);
}

SampleFolder::~SampleFolder()
{
}

void SampleFolder::setTargetFolderPath(std::filesystem::path folderPath)
{
	m_targetFolderPath = folderPath;
	updateAllFilesList();
}

void SampleFolder::updateAllFilesList()
{
	m_allFilesList.clear();
	for (const auto& entry : std::filesystem::directory_iterator(m_targetFolderPath))
	{
		// adding filenames with extensions to m_allFilesList
		m_allFilesList.push_back(entry.path().filename());
	}
}


void SampleFolder::saveUnchangedSample(std::shared_ptr<const SampleBuffer> sampleBuffer, std::string* sampleFileName)
{
	// shouldExport = true when sampleFileName includes s_sampleFileSourceName
	bool shouldExport = getSampleFileSourceName(*sampleFileName, false);
	if (shouldExport == false) { return; }

	for (const auto& it : m_loadedFilesList)
	{
		if (it == *sampleFileName)
		{
			shouldExport = false;
			break;
		}
	}
	
	if (shouldExport)
	{
		// remove everything except the file name and extension
		*sampleFileName = std::filesystem::path(*sampleFileName).filename();		
		
		bool found = false;
		for (const auto& it : m_allFilesList)
		{
			if (it == *sampleFileName)
			{
				found = true;
				break;
			}
		}
		if (found)
		{
			// renaming
			size_t largestFileNumeber = 1;
			for (const auto& it : m_allFilesList)
			{
				if (it == *sampleFileName)
				{
					size_t curNumber = getFileNumber(std::filesystem::path(*it));
					if (largestFileNumeber < curNumber)
					{
						largestFileNumeber = curNumber;
					}
				}
			}
			sampleFileName = sampleFileName + std::to_string(largestFileNumeber + 1);
		}
		saveSample(sampleBuffer, *sampleFileName);
	}
}

void SampleFolder::updateSample(std::shared_ptr<const SampleBuffer> sampleBuffer, const std::string& sampleFileName)
{
	// TODO
}

std::shared_ptr<const SampleBuffer> SampleFolder::loadSample(const std::string& sampleFileName)
{
	if (getSampleFileSourceName(sampleFileName))
	{
		m_loadedFilesList.push_back(sampleFileName);
		std::string newName = sampleFileName;
		setSampleFileSourceName(&newName, false);
		return gui::SampleLoader::createBufferFromFile(newName);
	}
	return gui::SampleLoader::createBufferFromFile(sampleFileName);
}

void SampleFolder::setSampleFileSourceName(std::string* sampleFileName, bool isManagedBySampleFolder)
{
	std::filesystem::path sampleFilePath(sampleFileName);
	std::string nameStem = std::filesystem::path(sampleFileName).stem();
	bool isManagedBySampleFolderByName = getSampleFileSourceName(sampleFilePath.stem(), true);
	if (isManagedBySampleFolder != isManagedBySampleFolderByName)
	{
		if (isManagedBySampleFolder)
		{
			nameStem = nameStem + s_sampleFileSourceName + sampleFilePath.extension();
		}
		else
		{
			ssize_t newSize = nameStem.size() - s_sampleFileSourceName.size();
			if (newSize >= 0)
			{
				nameStem.resize(static_cast<size_t>(newSize));
			}
			nameStem = nameStem + sampleFilePath.extension();
		}

		// replace filename with the new stem + extension
		sampleFilePath.replace_filename(nameStem);
		*sampleFileName = sampleFilePath.string();
	}
}

bool SampleFolder::getSampleFileSourceName(const std::string& sampleFileName, bool isOnlyNameStem = false)
{
	if (isOnlyNameStem == false)
	{
		return sampleFileName.ends_with(s_sampleFileSourceName);
	}
	std::string nameStem = std::filesystem::path(sampleFileName).stem();
	return nameStem.ends_with(s_sampleFileSourceName);
}

bool SampleFolder::saveSample(std::shared_ptr<const SampleBuffer> sampleBuffer, const std::string& sampleFileName)
{
	// getting rid of s_sampleFileSourceName tag
	setSampleFileSourceName(&sampleFileName, false);
	
	// TODO save

	LmmsExporterSample exporter(LmmsExporterSample::ExportFileType::Audio, (m_targetFolderPath / std::filesystem::path(sampleFileName)));
	exporter.setupAudioRendering(
		const OutputSettings& outputSettings,
		LmmsExporterSample::ExportAudioFileFormat::Flac,
		256,
		sampleBuffer->data(),
		sampleBuffer->size();
	);
}

size_t SampleFolder::getFileNumber(const std::filesystem::path& sampleFilePath)
{
	size_t outout = 0;
	std::string nameStem = sampleFilePath.stem();
	std::string nameNumber = "";
	for (size_t i = nameStem.size() - 1; i >= 0; i--)
	{
		if (std::isdigit(nameStem[i]) == false) { break; }
		// could be made more efficient with copy() and resize()
		nameNumber = nameNumber + nameStem[i];
	}
	if (nameNumber.size() > 0)
	{
		output = static_cast<size_t>(std::stol(nameNumber));
	}
	return output;
}

} // namespace lmms
