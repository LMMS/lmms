/*
 * Sample.h - Access samples with a file dialog
 *
 * Copyright (c) 2022 sakertooth <sakertooth@gmail.com>
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

#include "SampleFileDialog.h"

#include "ConfigManager.h"
#include "FileDialog.h"
#include "PathUtil.h"
#include "Sample.h"

#include <QFileDialog>

namespace lmms::gui 
{
	std::string SampleFileDialog::openSampleFile(const Sample& sample) 
	{
		gui::FileDialog ofd(nullptr, QObject::tr("Open audio file"));
		
		auto sampleFile = sample.sampleFile();
		std::string dir;

		if (!sampleFile.empty())
		{
			if (std::filesystem::path{sampleFile}.is_relative()) 
			{
				sampleFile = ConfigManager::inst()->userSamplesDir().toStdString() + sampleFile;
				if (!std::filesystem::exists(sampleFile)) 
				{
					sampleFile = ConfigManager::inst()->factorySamplesDir().toStdString() + sampleFile;
				}
			}

			dir = std::filesystem::absolute(sampleFile).string();
		}
		else 
		{
			dir = ConfigManager::inst()->userSamplesDir().toStdString();
		}

		// change dir to position of previously opened file
		ofd.setDirectory(QString::fromStdString(dir));
		ofd.setFileMode(gui::FileDialog::ExistingFiles);

		std::array<QString, 11> types = 
		{
			QObject::tr("All Audio-Files (*.wav *.ogg *.ds *.flac *.spx *.voc *.aif *.aiff *.au *.raw)"),
			QObject::tr("Wave-Files (*.wav)"),
			QObject::tr("OGG-Files (*.ogg)"),
			QObject::tr("DrumSynth-Files (*.ds)"),
			QObject::tr("FLAC-Files (*.flac)"),
			QObject::tr("SPEEX-Files (*.spx)"),
			QObject::tr("VOC-Files (*.voc)"),
			QObject::tr("AIFF-Files (*.aif *.aiff)"),
			QObject::tr("AU-Files (*.au)"),
			QObject::tr("RAW-Files (*.raw)")	
		};

		ofd.setNameFilters(QStringList{types.begin(), types.end()});

		if (!sampleFile.empty())
		{
			// select previously opened file
			ofd.selectFile(QFileInfo(QString::fromStdString(sampleFile)).fileName());
		}

		if (ofd.exec () == QDialog::Accepted)
		{
			if (ofd.selectedFiles().isEmpty())
			{
				return "";
			}

			return PathUtil::toShortestRelative(ofd.selectedFiles()[0]).toStdString();
		}

		return "";
	}
}