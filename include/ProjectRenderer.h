/*
 * ProjectRenderer.h - ProjectRenderer class for easily rendering projects
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

#include "AudioFileDevice.h"
#include "AudioOutputContext.h"
#include "lmmsconfig.h"

class QTimer;

/*! \brief The ProjectRenderer class provides functionality to render current Song into a file. */
class ProjectRenderer : public QThread
{
	Q_OBJECT
public:
	/*! Lists all supported output file formats. */
	enum ExportFileFormats
	{
		WaveFile,		/*!< Uncompressed WAV file */
		OggFile,		/*!< Vorbis-encoded OGG file */
		Mp3File,		/*!< MP3 file encoded via LAME */
		FlacFile,		/*!< Free Lossless Audio Codec */
		NumFileFormats
	} ;

	static const char * EFF_ext[];

	/*! Lists all supported sample type depths. */
	enum Depths
	{
		Depth_16Bit,	/*!< 16 bit signed integer */
		Depth_24Bit,	/*!< 24 bit floating point */
		Depth_32Bit,	/*!< 32 bit floating point */
		NumDepths
	} ;

	/*! Settings for the output file encoder. */
	struct EncoderSettings
	{
		sample_rate_t samplerate;	/*!< Desired output sample rate */
		bool vbr;					/*!< Use variable bitrate encoding */
		int bitrate;				/*!< Desired bitrate (kbps) */
		Depths depth;				/*!< Depth of samples */

		EncoderSettings( sample_rate_t _sr, bool _vbr, int _bitrate, Depths _d ) :
			samplerate( _sr ),
			vbr( _vbr ),
			bitrate( _bitrate ),
			depth( _d )
		{
		}
	} ;

	/*! \brief Constructs a ProjectRenderer object with given settings.
	 *
	 * \param qualitySettings The desired quality settings for the AudioOutputContext
	 * \param encoderSettings The desired settings for the output file encoder
	 * \param fileFormat One of the file formats listed in the ExportFileFormats enumeration
	 * \param outFile The output file name
	 */
	ProjectRenderer( const AudioOutputContext::QualitySettings & qualitySettings,
				const EncoderSettings & encoderSettings,
				ExportFileFormats fileFormat,
				const QString & outFile );
	virtual ~ProjectRenderer();

	/*! \brief Returns whether the ProjectRenderer was initialized properly. */
	bool isReady() const
	{
		return m_fileDev != NULL;
	}

	static ExportFileFormats getFileFormatFromExtension( const QString & _ext );

	void setConsoleUpdateTimer( QTimer * t )
	{
		m_consoleUpdateTimer = t;
	}


public slots:
	/*! \brief Sets according AudioOutputContext for Mixer and starts render thread. */
	void startProcessing();
	/*! \brief Aborts the processing and cleans up resources. */
	void abortProcessing();

	/*! \brief Prints current render progress to the console. */
	void updateConsoleProgress();


signals:
	void progressChanged( int );


private slots:
	/*! \brief Finalizes the render process and restores Mixer's AudioOutputContext. */
	void finishProcessing();


private:
	virtual void run();

	AudioFileDevice * m_fileDev;
	AudioOutputContext * m_context;

	volatile int m_progress;
	volatile bool m_abort;

	QTimer * m_consoleUpdateTimer;

} ;


/*! \brief Holds information about a certain file encoder. */
struct FileEncodeDevice
{
	ProjectRenderer::ExportFileFormats m_fileFormat;
	const char * m_description;
	const char * m_extension;
	AudioFileDeviceInstantiaton m_getDevInst;
} ;


extern FileEncodeDevice __fileEncodeDevices[];

#endif

/* vim: set tw=0 noexpandtab: */
