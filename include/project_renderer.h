/*
 * project_renderer.h - projectRenderer-class for easily rendering projects
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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

#include "audio_file_device.h"
#include "lmmsconfig.h"


class projectRenderer : public QThread
{
	Q_OBJECT
public:
	enum ExportFileFormats
	{
		WaveFile,
		OggFile,
        Mp3File,
		NumFileFormats
	} ;

    static const char * EFF_ext[];

	enum Depths
	{
		Depth_16Bit,
		Depth_24Bit,
		Depth_32Bit,
		NumDepths
	} ;

	struct outputSettings
	{
		sample_rate_t samplerate;
		bool vbr;
		int bitrate;
		Depths depth;

		outputSettings( sample_rate_t _sr, bool _vbr, int _bitrate,
								Depths _d ) :
			samplerate( _sr ),
			vbr( _vbr ),
			bitrate( _bitrate ),
			depth( _d )
		{
		}
	} ;


	projectRenderer( const mixer::qualitySettings & _qs,
				const outputSettings & _os,
				ExportFileFormats _file_format,
				const QString & _out_file );
	virtual ~projectRenderer();

	bool isReady( void ) const
	{
		return m_fileDev != NULL;
	}

	static ExportFileFormats getFileFormatFromExtension(
							const QString & _ext );


public slots:
	void startProcessing( void );
	void abortProcessing( void );

	void updateConsoleProgress( void );


signals:
	void progressChanged( int );


private:
	virtual void run( void );

	audioFileDevice * m_fileDev;
	mixer::qualitySettings m_qualitySettings;
	mixer::qualitySettings m_oldQualitySettings;

	volatile int m_progress;
	volatile bool m_abort;

} ;


struct fileEncodeDevice
{
	projectRenderer::ExportFileFormats m_fileFormat;
	const char * m_description;
	const char * m_extension;
	audioFileDeviceInstantiaton m_getDevInst;
} ;


extern fileEncodeDevice __fileEncodeDevices[];

#endif
