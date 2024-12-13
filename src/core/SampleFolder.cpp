/*
 * SampleFolder.cpp - Manages sample loading and saving from a sample folder
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

#include <array>
#include <filesystem>
#include <string>

#include <QDir>
#include <QFileInfo>

#include "ConfigManager.h"
#include "LmmsExporterSample.h"
#include "SampleBuffer.h"
#include "SampleLoader.h"

namespace lmms
{

const QString SampleFolder::s_usedFolderName = "/Used";
const QString SampleFolder::s_unusedFolderName = "/Unused";

SampleFolder::SampleFolder() :
	m_targetFolderPath(""),
	m_sampleFolderFiles(),
	m_exporter()
{
}

SampleFolder::~SampleFolder()
{
}

void SampleFolder::setTargetFolderPath(const QString& folderPath)
{
	m_targetFolderPath = folderPath;
	updateAllFilesList();
}

void SampleFolder::resetTargetFolderPath()
{
	setTargetFolderPath(ConfigManager::inst()->commonSampleFolderDir());
	updateAllFilesList();
}


void SampleFolder::updateAllFilesList()
{
	if (m_targetFolderPath.size() == 0) { resetTargetFolderPath(); }
	m_sampleFolderFiles.clear();
	const std::array<QString, 3> filePaths = {QString(), s_usedFolderName, s_unusedFolderName};
	for (size_t i = 0; i < filePaths.size(); i++)
	{
		QDir currentDir(m_targetFolderPath + filePaths[i]);
		if (currentDir.exists() == true)
		{
			QStringList fileList(currentDir.entryList(QDir::Files, QDir::NoSort));
			for (const auto& file : fileList)
			{
				qDebug("updateAllFilesList file name: %s", file.toStdString().c_str());
				if (QFileInfo(file).suffix() == "flac")
				{
					// adding filenames with extensions to m_sampleFolderFiles
					m_sampleFolderFiles.push_back(SampleFile());
					m_sampleFolderFiles[m_sampleFolderFiles.size() - 1].name = file;
					m_sampleFolderFiles[m_sampleFolderFiles.size() - 1].relativeFolder = filePaths[i];
					m_sampleFolderFiles[m_sampleFolderFiles.size() - 1].isManaged = i > 0;
				}
			}
		}
	}
}

void SampleFolder::makeSampleFolderDirs(const QString& path)
{
	QDir targetDirectory(path);
	if (targetDirectory.exists(s_usedFolderName) == false)
	{
		targetDirectory.mkdir(s_usedFolderName);
	}
	if (targetDirectory.exists(s_unusedFolderName) == false)
	{
		targetDirectory.mkdir(s_unusedFolderName);
	}
}

std::shared_ptr<const SampleBuffer> SampleFolder::loadSample(const QString& sampleFileName)
{
	if (m_targetFolderPath.size() == 0) { resetTargetFolderPath(); }
	std::shared_ptr<const SampleBuffer> output = nullptr;
	QString filteredSampleFileName(QFileInfo(sampleFileName).fileName());
	ssize_t index = findFileInsideSampleFolder(filteredSampleFileName);
	if (index >= 0)
	{
		if (m_sampleFolderFiles[index].isLoaded == false)
		{
			m_sampleFolderFiles[index].buffer = gui::SampleLoader::createBufferFromFile(m_targetFolderPath + m_sampleFolderFiles[index].relativeFolder + filteredSampleFileName);
			m_sampleFolderFiles[index].isLoaded = true;
		}
		output = m_sampleFolderFiles[index].buffer;
	}
	else
	{
		output = gui::SampleLoader::createBufferFromFile(sampleFileName);
	}
	return output;
}

void SampleFolder::saveSample(std::shared_ptr<const SampleBuffer> sampleBuffer, const QString& sampleFileName, bool isManagedBySampleFolder, bool shouldGenerateUniqueName, QString* sampleFileFinalName)
{
	ssize_t index = findFileInsideSampleFolder(sampleFileName);
	if (shouldGenerateUniqueName && index >= 0)
	{
		m_sampleFolderFiles[index].isSaved = true;
	}
	else
	{
		QString exportFinalName("");
		exportSample(sampleBuffer, sampleFileName, isManagedBySampleFolder, shouldGenerateUniqueName, &exportFinalName);
		if (sampleFileFinalName != nullptr)
		{
			*sampleFileFinalName = exportFinalName;
		}
		m_sampleFolderFiles.push_back(SampleFile());
		m_sampleFolderFiles[m_sampleFolderFiles.size() - 1].name = exportFinalName;
		m_sampleFolderFiles[m_sampleFolderFiles.size() - 1].relativeFolder = isManagedBySampleFolder ? s_usedFolderName : "";
		m_sampleFolderFiles[m_sampleFolderFiles.size() - 1].isManaged = isManagedBySampleFolder;
	}
}

void SampleFolder::saveSample(const QString& sampleFileName)
{
	ssize_t index = findFileInsideSampleFolder(sampleFileName);
	if (index >= 0)
	{
		m_sampleFolderFiles[index].isSaved = true;
	}
}

void SampleFolder::updateSample(std::shared_ptr<const SampleBuffer> sampleBuffer, const QString& sampleFileName)
{
	ssize_t index = findFileInsideSampleFolder(sampleFileName);
	if (index >= 0)
	{
		exportSample(sampleBuffer, sampleFileName, m_sampleFolderFiles[index].isManaged, false, nullptr);
	}
}

void SampleFolder::exportSample(std::shared_ptr<const SampleBuffer> sampleBuffer, const QString& sampleFileName, bool isManagedBySampleFolder, bool shouldGenerateUniqueName, QString* sampleFileFinalName)
{
	if (sampleFileName.size() <= 0) { return; }
	QString finalName(sampleFileName);

	if (shouldGenerateUniqueName)
	{
		finalName = findUniqueName(QFileInfo(sampleFileName).baseName()) + ".flac";
	}

	if (sampleFileFinalName != nullptr)
	{
		*sampleFileFinalName = finalName;
	}

	// adding path to name
	finalName = m_targetFolderPath + (isManagedBySampleFolder == true ? s_usedFolderName : QString("")) + finalName;

	m_exporter->startExporting(finalName, sampleBuffer);
}
	
bool SampleFolder::isPathInsideSampleFolder(const QString& filePath)
{
	if (m_targetFolderPath.size() == 0) { resetTargetFolderPath(); }
	QString curFolderAsString(QFileInfo(m_targetFolderPath).absolutePath());
	QString curFileAsString(QFileInfo(m_targetFolderPath).absolutePath());
	/* std way:
	std::filesystem::path curFolderPath(std::filesystem::absolute(m_targetFolderPath.toStdU16String());
	std::filesystem::path curFilePath(std::filesystem::absolute(filePath.toStdU16String());
	std::u16string curFolderAsString = static_cast<std::u16string>(curFolderPath);
	std::u16string curFileAsString = static_cast<std::u16string>(curFilePath);
	*/
	bool found = curFolderAsString.size() <= curFileAsString.size();
	if (found)
	{
		for (int i = 0; i < curFolderAsString.size(); i++)
		{
			if (curFolderAsString.at(i) != curFileAsString.at(i))
			{
				found = false;
				break;
			}
		}
	}
	return found;
}

ssize_t SampleFolder::findFileInsideSampleFolder(const QString& sampleFileName)
{
	ssize_t index = -1;
	for (size_t i = 0; i < m_sampleFolderFiles.size(); i++)
	{
		if (m_sampleFolderFiles[i].name == sampleFileName)
		{
			index = i;
			break;
		}
	}
	return index;
}

QString SampleFolder::findUniqueName(const QString& sourceName) const
{
	QString output = sourceName;
	// removing number from `sourceName`
	bool isSeparatedWithWhiteSpace = false;
	size_t sourceNumberLength = SampleFolder::getNameNumberEnding(sourceName, &isSeparatedWithWhiteSpace).size();
	if (sourceNumberLength > 0)
	{
		// whitespace needs to be removed so we add + 1 to `sourceNumberLength`
		sourceNumberLength = isSeparatedWithWhiteSpace ? sourceNumberLength + 1 : sourceNumberLength;
		output.remove(output.size() - sourceNumberLength, sourceNumberLength);
	}

	size_t maxNameCounter = 0;
	bool found = false;

	for (const SampleFolder::SampleFile& it : m_sampleFolderFiles)
	{
		if (it.name.startsWith(output))
		{
			size_t nameCount = SampleFolder::getNameNumberEnding(it.name, nullptr).toInt();
			maxNameCounter = maxNameCounter < nameCount ? nameCount : maxNameCounter;
			found = true;
		}
	}

	if (found)
	{
		output = output + " " + QString::number(maxNameCounter + 1);
	}

	return output;
}

QString SampleFolder::getNameNumberEnding(const QString& name, bool* isSeparatedWithWhiteSpace)
{
	QString numberString = "";

	//! `it` will point to where the numbers start in `name`
	auto it = name.end();
	size_t digitCount = 0;
	while (it != name.begin())
	{
		it--;
		if (it->isDigit() == false)
		{
			if (isSeparatedWithWhiteSpace != nullptr)
			{
				*isSeparatedWithWhiteSpace = *it == ' ';
			}
			// the last character was not a number
			// increase `it` to account for this (and make it point to a digit)
			it++;
			break;
		}
		digitCount++;
	}

	if (digitCount > 0)
	{
		numberString.resize(digitCount);
		std::copy(it, name.end(), numberString.begin());
	}

	return numberString;
}



} // namespace lmms
