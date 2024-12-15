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

SampleFolder::SampleFolder() :
	m_targetFolderPath(""),
	m_sampleFolderFiles(),
	m_exporter()
{
	m_exporter = std::make_unique<LmmsExporterSample>();
}

SampleFolder::~SampleFolder()
{
}

void SampleFolder::setTargetFolderPath(const QString& folderPath, bool shouldFilterFileName)
{
		qDebug("resetTargetFolderPath 3");
	if (shouldFilterFileName)
	{
		qDebug("resetTargetFolderPath 4");
		m_targetFolderPath = QFileInfo(folderPath).absoluteDir().path();
	}
	else
	{
		qDebug("resetTargetFolderPath 5");
		m_targetFolderPath = folderPath;
	}

		qDebug("resetTargetFolderPath 6");
	if (m_targetFolderPath.at(m_targetFolderPath.size() - 1) != "/")
	{
		qDebug("resetTargetFolderPath 7");
		m_targetFolderPath = m_targetFolderPath + "/";
	}

	qDebug("setTargetFolderPath file name: %s", m_targetFolderPath.toStdString().c_str());
	updateAllFilesList();
}

void SampleFolder::resetTargetFolderPath()
{
	qDebug("resetTargetFolderPath 1");
	setTargetFolderPath(ConfigManager::inst()->commonSampleFolderDir(), false);
}


void SampleFolder::updateAllFilesList()
{
	qDebug("updateAllFilesList");
	if (m_targetFolderPath.size() == 0) { resetTargetFolderPath(); }
	m_sampleFolderFiles.clear();
	const std::array<QString, 3> filePaths = {QString(""), COMMON_SAMPLE_FOLDER_USED, COMMON_SAMPLE_FOLDER_UNUSED};
	for (size_t i = 0; i < filePaths.size(); i++)
	{
		QDir currentDir(m_targetFolderPath + filePaths[i]);
		qDebug("updateAllFilesList loop folder path: %s", (m_targetFolderPath + filePaths[i]).toStdString().c_str());
		if (currentDir.exists() == true)
		{
			QStringList fileList(currentDir.entryList(QDir::Files, QDir::NoSort));
			for (const auto& file : fileList)
			{
				qDebug("updateAllFilesList file name: %s", file.toStdString().c_str());
				qDebug("updateAllFilesList folder name: %s", filePaths[i].toStdString().c_str());
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
	QDir targetDirectory(QFileInfo(path).absoluteDir());
	if (targetDirectory.exists(COMMON_SAMPLE_FOLDER_USED) == false)
	{
		targetDirectory.mkdir(COMMON_SAMPLE_FOLDER_USED);
	}
	if (targetDirectory.exists(COMMON_SAMPLE_FOLDER_UNUSED) == false)
	{
		targetDirectory.mkdir(COMMON_SAMPLE_FOLDER_UNUSED);
	}
}

std::shared_ptr<const SampleBuffer> SampleFolder::loadSample(const QString& sampleFileName, QString* sampleFileFinalName)
{
	qDebug("loadSample file name: %s", sampleFileName.toStdString().c_str());
	if (m_targetFolderPath.size() == 0) { resetTargetFolderPath(); }
	std::shared_ptr<const SampleBuffer> output = SampleBuffer::emptyBuffer();
	QString filteredSampleFileName(QFileInfo(sampleFileName).fileName());
	qDebug("loadSample filderedfile name: %s", filteredSampleFileName.toStdString().c_str());
	if (sampleFileName == filteredSampleFileName || isPathInsideSampleFolder(sampleFileName))
	{
		ssize_t index = findFileInsideSampleFolder(filteredSampleFileName);
		if (index >= 0)
		{
			if (m_sampleFolderFiles[index].isLoaded == false)
			{
				m_sampleFolderFiles[index].buffer = gui::SampleLoader::createBufferFromFile(m_targetFolderPath + m_sampleFolderFiles[index].relativeFolder + filteredSampleFileName);
				m_sampleFolderFiles[index].isLoaded = true;
			}
			output = m_sampleFolderFiles[index].buffer;

			if (sampleFileFinalName != nullptr)
			{
				*sampleFileFinalName = m_sampleFolderFiles[index].name;
			}

			qDebug("sample loading set buffer");
			if (output == SampleBuffer::emptyBuffer())
			{
				qDebug("sample loading error 3");
			}
		}
		else
		{
			qDebug("sample loading error 1");
		}
	}
	else
	{
		if (QFileInfo(sampleFileName).exists())
		{
			output = gui::SampleLoader::createBufferFromFile(sampleFileName);
			if (sampleFileFinalName != nullptr)
			{
				*sampleFileFinalName = sampleFileName;
			}
		}
		else
		{
			qDebug("sample loading error 2");
		}
	}
	return output;
}

void SampleFolder::saveSample(std::shared_ptr<const SampleBuffer> sampleBuffer, const QString& sampleFileName, bool isManagedBySampleFolder, bool shouldGenerateUniqueName, QString* sampleFileFinalName)
{
	qDebug("saveSample file name: %s", sampleFileName.toStdString().c_str());
	ssize_t index = findFileInsideSampleFolder(sampleFileName);
	if (shouldGenerateUniqueName && index >= 0)
	{
		m_sampleFolderFiles[index].isSaved = true;
	}
	else if (sampleBuffer != SampleBuffer::emptyBuffer())
	{
		QString exportFinalName("");
		exportSample(sampleBuffer, sampleFileName, isManagedBySampleFolder, shouldGenerateUniqueName, &exportFinalName);
		if (sampleFileFinalName != nullptr)
		{
			*sampleFileFinalName = exportFinalName;
		}
		m_sampleFolderFiles.push_back(SampleFile());
		m_sampleFolderFiles[m_sampleFolderFiles.size() - 1].name = exportFinalName;
		m_sampleFolderFiles[m_sampleFolderFiles.size() - 1].relativeFolder = isManagedBySampleFolder ? COMMON_SAMPLE_FOLDER_USED : "";
		m_sampleFolderFiles[m_sampleFolderFiles.size() - 1].isManaged = isManagedBySampleFolder;
		m_sampleFolderFiles[m_sampleFolderFiles.size() - 1].isLoaded = true;
		if (sampleBuffer->audioFile() == exportFinalName)
		{
			m_sampleFolderFiles[m_sampleFolderFiles.size() - 1].buffer = sampleBuffer;
		}
		else
		{
			m_sampleFolderFiles[m_sampleFolderFiles.size() - 1].buffer = std::make_shared<SampleBuffer>(SampleBuffer(sampleBuffer->data(), sampleBuffer->size(), sampleBuffer->sampleRate()));;
		}
	}
}

void SampleFolder::saveSample(const QString& sampleFileName)
{
	qDebug("saveSample 2 file name: %s", sampleFileName.toStdString().c_str());
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

void SampleFolder::sortManagedFiles()
{
	QDir curDir(QFileInfo(m_targetFolderPath).absoluteDir());
	for (size_t i = 0; i < m_sampleFolderFiles.size(); i++)
	{

		qDebug("sortManagedFiles checking: %s", m_sampleFolderFiles[i].name.toStdString().c_str());
		if (m_sampleFolderFiles[i].isLoaded && m_sampleFolderFiles[i].isManaged)
		{
			if (m_sampleFolderFiles[i].isSaved == true && m_sampleFolderFiles[i].relativeFolder == COMMON_SAMPLE_FOLDER_UNUSED)
			{
				curDir.rename(m_targetFolderPath + m_sampleFolderFiles[i].relativeFolder + m_sampleFolderFiles[i].name,
					m_targetFolderPath + COMMON_SAMPLE_FOLDER_USED + m_sampleFolderFiles[i].name);
				m_sampleFolderFiles[i].relativeFolder = COMMON_SAMPLE_FOLDER_USED;
				qDebug("sortManagedFiles move to used: %s", m_sampleFolderFiles[i].name.toStdString().c_str());
			}
			else if (m_sampleFolderFiles[i].isSaved == false && m_sampleFolderFiles[i].relativeFolder == COMMON_SAMPLE_FOLDER_USED)
			{
				curDir.rename(m_targetFolderPath + m_sampleFolderFiles[i].relativeFolder + m_sampleFolderFiles[i].name,
					m_targetFolderPath + COMMON_SAMPLE_FOLDER_UNUSED + m_sampleFolderFiles[i].name);
				m_sampleFolderFiles[i].relativeFolder = COMMON_SAMPLE_FOLDER_UNUSED;
				qDebug("sortManagedFiles move to unused: %s", m_sampleFolderFiles[i].name.toStdString().c_str());
			}
		}
	}
	resetSavedStatus();
}

void SampleFolder::resetSavedStatus()
{
	for (size_t i = 0; i < m_sampleFolderFiles.size(); i++)
	{
		m_sampleFolderFiles[i].isSaved = false;
	}
}

void SampleFolder::exportSample(std::shared_ptr<const SampleBuffer> sampleBuffer, const QString& sampleFileName, bool isManagedBySampleFolder, bool shouldGenerateUniqueName, QString* sampleFileFinalName)
{
	qDebug("exportSample file name: %s", sampleFileName.toStdString().c_str());
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

	qDebug("exportSample finalName: %s", finalName.toStdString().c_str());
	// adding path to name
	finalName = m_targetFolderPath + (isManagedBySampleFolder == true ? COMMON_SAMPLE_FOLDER_USED : QString("")) + finalName;
	qDebug("exportSample final path: %s", finalName.toStdString().c_str());

	m_exporter->startExporting(finalName, sampleBuffer);
}
	
bool SampleFolder::isPathInsideSampleFolder(const QString& filePath)
{
	if (m_targetFolderPath.size() == 0) { resetTargetFolderPath(); }
	QString curFileAsString(QFileInfo(filePath).absolutePath());
	bool found = m_targetFolderPath.size() <= curFileAsString.size();
	if (found)
	{
		for (int i = 0; i < m_targetFolderPath.size(); i++)
		{
			if (m_targetFolderPath.at(i) != curFileAsString.at(i))
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
	qDebug("findFileInsideSampleFolder sampleFileName: %s", sampleFileName.toStdString().c_str());
	ssize_t index = -1;
	for (size_t i = 0; i < m_sampleFolderFiles.size(); i++)
	{
		qDebug("findFileInsideSampleFolder compare to: %s", m_sampleFolderFiles[i].name.toStdString().c_str());
		if (m_sampleFolderFiles[i].name == sampleFileName)
		{
			qDebug("findFileInsideSampleFolder compare to: %s FOUND", m_sampleFolderFiles[i].name.toStdString().c_str());
			index = i;
			break;
		}
	}
	return index;
}

QString SampleFolder::findUniqueName(const QString& sourceName) const
{
	qDebug("findUniqueName input: %s", sourceName.toStdString().c_str());
	QString output = sourceName;
	QString outputExtension = sourceName;
	// removing number from `sourceName`
	bool isSeparatedWithWhiteSpace = false;
	size_t extensionCharCount = 0;
	size_t sourceNumberLength = SampleFolder::getNameNumberEnding(sourceName, &isSeparatedWithWhiteSpace, &extensionCharCount).size() + extensionCharCount;
	// removes the extension and the numbers from the name's end
	if (sourceNumberLength > 0)
	{
		// whitespace needs to be removed so we add + 1 to `sourceNumberLength`
		sourceNumberLength = isSeparatedWithWhiteSpace ? sourceNumberLength + 1 : sourceNumberLength;
		output.remove(output.size() - sourceNumberLength, sourceNumberLength);
	}
	if (extensionCharCount > 0)
	{
		outputExtension.remove(extensionCharCount, output.size() - extensionCharCount);
	}
	else
	{
		outputExtension.clear();
	}

	size_t maxNameCounter = 0;
	bool found = false;

	for (const SampleFolder::SampleFile& it : m_sampleFolderFiles)
	{
		if (it.name.startsWith(output))
		{
			size_t nameCount = SampleFolder::getNameNumberEnding(it.name, nullptr, nullptr).toInt();
			maxNameCounter = maxNameCounter < nameCount ? nameCount : maxNameCounter;
			found = true;
		}
	}

	if (found)
	{
		output = output + "_" + QString::number(maxNameCounter + 1) + outputExtension;
	}

	return output;
}

QString SampleFolder::getNameNumberEnding(const QString& name, bool* isSeparatedWithWhiteSpace, size_t* extensionCountOut)
{
	QString numberString = "";

	//! `endit` will point to where the extension ends
	auto endit = name.end();
	size_t extensionCount = 0;

	// getting rid of extension
	while (endit != name.begin())
	{
		endit--;
		extensionCount++;
		if (*endit == '.')
		{
			if (extensionCountOut != nullptr)
			{
				*extensionCountOut = extensionCount;
			}
			break;
		}
	}
	endit = endit != name.begin() ? endit : name.end();

	//! `it` will point to where the numbers start in `name`
	auto it = endit;
	size_t digitCount = 0;

	while (it != name.begin())
	{
		it--;

		if (it->isDigit() == false)
		{
			if (isSeparatedWithWhiteSpace != nullptr)
			{
				*isSeparatedWithWhiteSpace = *it == '_';
			}
			// the last character was not a number
			// increase `it` to account for this (and make it point to a digit)
			it++;
			break;
		}
		digitCount++;
	}

	if (digitCount > 0 && it != endit)
	{
		numberString.resize(digitCount);
		std::copy(it, endit, numberString.begin());
	}

	return numberString;
}



} // namespace lmms
