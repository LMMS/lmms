/*
 * audio_sample_recorder.h - device-class that implements recording
 *                           surround-audio-buffers into RAM, maybe later
 *                           also harddisk
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _AUDIO_SAMPLE_RECORDER_H
#define _AUDIO_SAMPLE_RECORDER_H


#include "qt3support.h"

#ifdef QT4

#include <QtCore/QList>
#include <QtCore/QPair>

#else

#include <qvaluelist.h>
#include <qpair.h>

#endif


#include "audio_device.h"


class sampleBuffer;


class audioSampleRecorder : public audioDevice
{
public:
	audioSampleRecorder( const sample_rate_t _sample_rate,
				const ch_cnt_t _channels, bool & _success_ful,
							mixer * _mixer );
	virtual ~audioSampleRecorder();

	f_cnt_t framesRecorded( void ) const;
	void FASTCALL createSampleBuffer( sampleBuffer * * _sample_buf );


private:
	virtual void FASTCALL writeBuffer( const surroundSampleFrame * _ab,
						const fpab_t _frames,
						const float _master_gain );

	typedef vlist<QPair<sampleFrame *, fpab_t> > bufferList;
	bufferList m_buffers;

} ;


#endif
