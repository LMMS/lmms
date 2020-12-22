/*
 * ProjectRenderer.h - ProjectRenderer class for easily rendering projects
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef PROJECT_RENDERER_H
#define PROJECT_RENDERER_H

#include "AudioFileDevice.h"
#include "lmmsconfig.h"
#include "Mixer.h"
#include "OutputSettings.h"

#include "lmms_export.h"

class LMMS_EXPORT ProjectRenderer : public QThread
{
	Q_OBJECT
public:
	enum ExportFileFormats: int
	{
		WaveFile,
		FlacFile,
		OggFile,
		MP3File,
		NumFileFormats
	} ;

	struct FileEncodeDevice
	{
		bool isAvailable() const { return m_getDevInst != nullptr; }

		ExportFileFormats m_fileFormat;
		const char * m_description;
		const char * m_extension;
		AudioFileDeviceInstantiaton m_getDevInst;
	} ;


	ProjectRenderer( const Mixer::qualitySettings & _qs,
				const OutputSettings & _os,
				ExportFileFormats _file_format,
				const QString & _out_file );
	virtual ~ProjectRenderer();

	bool isReady() const
	{
		return m_fileDev != NULL;
	}

	static ExportFileFormats getFileFormatFromExtension(
							const QString & _ext );

	static QString getFileExtensionFromFormat( ExportFileFormats fmt );

	static const FileEncodeDevice fileEncodeDevices[];

public slots:
	void startProcessing();
	void abortProcessing();

	void updateConsoleProgress();


signals:
	void progressChanged( int );


private:
	void run() override;

	AudioFileDevice * m_fileDev;
	Mixer::qualitySettings m_qualitySettings;

	volatile int m_progress;
	volatile bool m_abort;

} ;

#endif
