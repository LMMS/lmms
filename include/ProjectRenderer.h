/*
 * ProjectRenderer.h - ProjectRenderer class for easily rendering projects
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#ifndef _PROJECT_RENDERER_H
#define _PROJECT_RENDERER_H

#include "AudioFileDevice.h"
#include "lmmsconfig.h"


class ProjectRenderer : public QThread
{
	Q_OBJECT
public:
	enum ExportFileFormats
	{
		WaveFile,
		OggFile,
		NumFileFormats
	} ;

	enum Depths
	{
		Depth_16Bit,
		Depth_32Bit,
		NumDepths
	} ;

	struct OutputSettings
	{
		sample_rate_t samplerate;
		bool vbr;
		int bitrate;
		Depths depth;

		OutputSettings( sample_rate_t _sr, bool _vbr, int _bitrate,
								Depths _d ) :
			samplerate( _sr ),
			vbr( _vbr ),
			bitrate( _bitrate ),
			depth( _d )
		{
		}
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


public slots:
	void startProcessing();
	void abortProcessing();

	void updateConsoleProgress();


signals:
	void progressChanged( int );


private:
	virtual void run();

	AudioFileDevice * m_fileDev;
	Mixer::qualitySettings m_qualitySettings;
	Mixer::qualitySettings m_oldQualitySettings;

	volatile int m_progress;
	volatile bool m_abort;

} ;


struct FileEncodeDevice
{
	ProjectRenderer::ExportFileFormats m_fileFormat;
	const char * m_description;
	const char * m_extension;
	AudioFileDeviceInstantiaton m_getDevInst;
} ;


extern FileEncodeDevice __fileEncodeDevices[];

#endif
