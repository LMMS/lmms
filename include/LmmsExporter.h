/*
 * LmmsExporter.h - exporting files (currently only audio files), TODO rename class when things change
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

#include <thread>
#include <vector>

#include "AudioFileDevice.h"
#include "SampleFrame.h"

namespace lmms
{

//class AudioFileDevice;

class LmmsExporter
{
public:
	using BufferFn = std::function<void(std::vector<SampleFrame>*, void*)>;
	using EndFn = std::function<void(void*)>;

	enum class ExportAudioFileFormat : int
	{
		Wave,
		Flac,
		Ogg,
		MP3,
		Count
	};
	// in the future more types can be added
	enum class ExportFileType
	{
		Audio
	};

	constexpr static auto NumFileFormats = static_cast<std::size_t>(ExportAudioFileFormat::Count);

	struct FileEncodeDevice
	{
		bool isAvailable() const { return m_getDevInst != nullptr; }

		ExportAudioFileFormat m_fileFormat;
		const char* m_description;
		const char* m_extension;
		AudioFileDeviceInstantiaton m_getDevInst;
	};

	LmmsExporter(const ExportFileType fileType,
				const QString& outputLocationAndName);
	~LmmsExporter();

	void setupAufioFile(
			const OutputSettings& outputSettings,
			ExportAudioFileFormat fileFormat,
			const fpp_t defaultFrameCount,
			SampleFrame* exportBuffer,
			const fpp_t exportBufferFrameCount);
	void setupAufioFile(
			const OutputSettings& outputSettings,
			ExportAudioFileFormat fileFormat,
			const fpp_t defaultFrameCount,
			BufferFn getBufferFunction,
			EndFn endFunction,
			void* getBufferData);

	static ExportAudioFileFormat getAudioFileFormatFromFileName(const QString& fileName);
	static ExportAudioFileFormat getAudioFileFormatFromExtension(const QString& extenisonString);
	static QString getAudioFileExtensionFromFormat(ExportAudioFileFormat fmt);

	static const std::array<FileEncodeDevice, 5> s_fileEncodeDevices;

	void startExporting();
	void stopExporting();

private:
	static void processExportingAudioFile(LmmsExporter* thisExporter);
	bool canExportAutioFile() const;
	
	// audio exporting
	bool processNextBuffer();
	bool processThisBuffer(SampleFrame* frameBuffer, const fpp_t frameCount);
	void setupAudioFileInternal(
		const OutputSettings& outputSettings,
		ExportAudioFileFormat fileFormat,
		const fpp_t defaultFrameCount);



	ExportFileType m_exportFileType;
	QString m_outputFile;

	volatile bool m_abort;

	// called if not nullptr
	// while run()
	// if returned buffer.size() <= 0 then break; end
	BufferFn m_getBufferFunction;
	// called at the end of run(), can be nullptr
	EndFn m_endFunction;
	void* m_getBufferData;
	
	AudioFileDevice* m_fileDev;
	std::vector<SampleFrame> m_buffer;
	std::unique_ptr<std::thread> m_thread;
};

}

