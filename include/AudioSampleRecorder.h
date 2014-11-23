/*
 * AudioSampleRecorder.h - device-class that implements recording
 *                         audio-buffers into RAM
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _AUDIO_SAMPLE_RECORDER_H
#define _AUDIO_SAMPLE_RECORDER_H

#include <QtCore/QList>
#include <QtCore/QPair>

#include "AudioDevice.h"

class SampleBuffer;


class AudioSampleRecorder : public AudioDevice
{
public:
	AudioSampleRecorder( const ch_cnt_t _channels, bool & _success_ful,
							Mixer* mixer );
	virtual ~AudioSampleRecorder();

	f_cnt_t framesRecorded() const;
	void createSampleBuffer( SampleBuffer** sampleBuffer );


private:
	virtual void writeBuffer( const surroundSampleFrame * _ab,
						const fpp_t _frames,
						const float _master_gain );

	typedef QList<QPair<sampleFrame *, fpp_t> > BufferList;
	BufferList m_buffers;

} ;


#endif
