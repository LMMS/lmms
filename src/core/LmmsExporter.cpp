/*
 * LmmsExporter.cpp - exporting files (currently only audio files), TODO rename class when things change
 *
 * Copyright (c) 2024 szeli1 <TODO/at/gmail/dot.com>
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

#include <QFile>

#include "LmmsExporter.h"

#include "AudioFileWave.h"
#include "AudioFileFlac.h"
#include "AudioFileOgg.h"
#include "AudioFileMP3.h"

namespace lmms
{

const std::array<LmmsExporter::FileEncodeDevice, 5> LmmsExporter::s_fileEncodeDevices
{
	FileEncodeDevice{LmmsExporter::ExportAudioFileFormat::Wave,
		QT_TRANSLATE_NOOP("LmmsExporter", "WAV (*.wav)"),
					".wav", &AudioFileWave::getInst },
	FileEncodeDevice{LmmsExporter::ExportAudioFileFormat::Flac,
		QT_TRANSLATE_NOOP("LmmsExporter", "FLAC (*.flac)"),
		".flac",
		&AudioFileFlac::getInst
	},
	FileEncodeDevice{LmmsExporter::ExportAudioFileFormat::Ogg,
		QT_TRANSLATE_NOOP("LmmsExporter", "OGG (*.ogg)"),
					".ogg",
#ifdef LMMS_HAVE_OGGVORBIS
					&AudioFileOgg::getInst
#else
					nullptr
#endif
	},
	FileEncodeDevice{LmmsExporter::ExportAudioFileFormat::MP3,
		QT_TRANSLATE_NOOP("LmmsExporter", "MP3 (*.mp3)"),
					".mp3",
#ifdef LMMS_HAVE_MP3LAME
					&AudioFileMP3::getInst
#else
					nullptr
#endif
	},
	// Insert your own file-encoder infos here.
	// Maybe one day the user can add own encoders inside the program.

	FileEncodeDevice{LmmsExporter::ExportAudioFileFormat::Count, nullptr, nullptr, nullptr}
};

LmmsExporter::LmmsExporter(const ExportFileType fileType,
			const QString& outputLocationAndName) :
	m_exportFileType(fileType),
	m_outputFile(outputLocationAndName),
	m_abort(false),
	m_getBufferFunction(nullptr),
	m_endFunction(nullptr),
	m_getBufferData(nullptr),
	m_fileDev(nullptr),
	m_buffer(0)
{
	m_thread = nullptr;
}

LmmsExporter::~LmmsExporter()
{
	// close thread
	stopExporting();

	if (m_fileDev != nullptr)
	{
		delete m_fileDev;
	}
}

void LmmsExporter::setupAudioRendering(
		const OutputSettings& outputSettings,
		ExportAudioFileFormat fileFormat,
		const fpp_t defaultFrameCount,
		SampleFrame* exportBuffer,
		const fpp_t exportBufferFrameCount)
{
	setupAudioRenderingInternal(outputSettings, fileFormat, defaultFrameCount);
	
	processThisBuffer(exportBuffer, exportBufferFrameCount);
}

void LmmsExporter::setupAudioRendering(
		const OutputSettings& outputSettings,
		ExportAudioFileFormat fileFormat,
		const fpp_t defaultFrameCount,
		BufferFn getBufferFunction,
		EndFn endFunction,
		void* getBufferData)
{
	m_getBufferFunction = getBufferFunction;
	m_endFunction = endFunction;
	m_getBufferData = getBufferData;

	setupAudioRenderingInternal(outputSettings, fileFormat, defaultFrameCount);
}

bool LmmsExporter::canExportAutioFile() const
{
	return m_fileDev != nullptr && (m_buffer.size() > 0 || m_getBufferFunction != nullptr);
}



LmmsExporter::ExportAudioFileFormat LmmsExporter::getAudioFileFormatFromFileName(const QString& fileName)
{
	// TODO test
	QString extension = "";
	for (size_t i = fileName.size(); i >= 0; i--)
	{
		extension = extension + fileName.at(i);
		if (fileName.at(i) == ".")
		{
			break;
		}
	}
	return getAudioFileFormatFromExtension(extension);
}

LmmsExporter::ExportAudioFileFormat LmmsExporter::getAudioFileFormatFromExtension(const QString& extenisonString)
{
	int idx = 0;
	while (s_fileEncodeDevices[idx].m_fileFormat != ExportAudioFileFormat::Count)
	{
		if (QString(s_fileEncodeDevices[idx].m_extension) == extenisonString)
		{
			return s_fileEncodeDevices[idx].m_fileFormat;
		}
		idx++;
	}
	return ExportAudioFileFormat::Count;
}

QString LmmsExporter::getAudioFileExtensionFromFormat(ExportAudioFileFormat fmt)
{
	return s_fileEncodeDevices[static_cast<std::size_t>(fmt)].m_extension;
}



void LmmsExporter::startExporting()
{
	if (m_thread.get() != nullptr) { return; }
	m_abort = false;

	switch (m_exportFileType)
	{
		case ExportFileType::Audio:
			if (canExportAutioFile())
			{
				m_thread = std::make_unique<std::thread>(processExportingAudioFile, this);
			}
			else if (m_endFunction != nullptr)
			{
				m_endFunction(m_getBufferData);
			}
			break;
	}
}

void LmmsExporter::stopExporting()
{
	// close thread
	if (m_thread.get() != nullptr)
	{
		m_abort = true;
		m_thread->join();
		// deleting unique_ptr
		m_thread = nullptr;
	}
}



void LmmsExporter::processExportingAudioFile(LmmsExporter* thisExporter)
{
	while (!thisExporter->m_abort)
	{
		// if a function pointer was provided
		// use that to fill m_buffer
		if (thisExporter->m_getBufferFunction != nullptr)
		{
			thisExporter->processNextBuffer();
		}
		
		// if thisExporter->m_buffer wasn't filled by
		// processNextBuffer() or processThisBuffer() before
		if (thisExporter->m_buffer.size() <= 0)
		{
			break;
		}

		thisExporter->m_fileDev->processThisBuffer(thisExporter->m_buffer.data(), thisExporter->m_buffer.size());
		
		// if no function pointer was provided
		if (thisExporter->m_getBufferFunction == nullptr)
		{
			// clear buffer to end while loop
			thisExporter->m_buffer.clear();
			break;
		}
	}

	if (thisExporter->m_endFunction != nullptr)
	{
		thisExporter->m_endFunction(thisExporter->m_getBufferData);
	}

	// If the user aborted export-process, the file has to be deleted.
	if(thisExporter->m_abort)
	{
		const QString file = thisExporter->m_fileDev->outputFile();
		QFile(file).remove();
	}
}



bool LmmsExporter::processNextBuffer()
{
	m_getBufferFunction(&m_buffer, m_getBufferData);
	if (m_buffer.size() <= 0) { return true; }

	return false;
}

bool LmmsExporter::processThisBuffer(SampleFrame* frameBuffer, const fpp_t frameCount)
{
	if (frameCount <= 0) { m_buffer.clear(); return true; }
	if (m_buffer.size() != frameCount)
	{
		m_buffer.resize(frameCount);
	}

	memcpy(m_buffer.data(), frameBuffer, m_buffer.size() * sizeof(SampleFrame));
	return false;
}

void LmmsExporter::setupAudioRenderingInternal(
	const OutputSettings& outputSettings,
	ExportAudioFileFormat fileFormat,
	const fpp_t defaultFrameCount)
{
	AudioFileDeviceInstantiaton audioEncoderFactory = s_fileEncodeDevices[static_cast<std::size_t>(fileFormat)].m_getDevInst;

	if (audioEncoderFactory)
	{
		bool successful = false;

		m_fileDev = audioEncoderFactory(outputSettings, successful, m_outputFile, defaultFrameCount);
		if(!successful)
		{
			delete m_fileDev;
			m_fileDev = nullptr;
		}
	}
}

} // namespace lmms

