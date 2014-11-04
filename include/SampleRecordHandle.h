/*
 * SampleRecordHandle.h - play-handle for recording a sample
 *
 * Copyright (c) 2008 Csaba Hruska <csaba.hruska/at/gmail.com>
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


#ifndef _SAMPLE_RECORD_HANDLE_H
#define _SAMPLE_RECORD_HANDLE_H

#include <QtCore/QList>
#include <QtCore/QPair>

#include "Mixer.h"
#include "SampleBuffer.h"

class bbTrack;
class SampleTCO;
class track;


class SampleRecordHandle : public PlayHandle
{
public:
	SampleRecordHandle( SampleTCO* tco );
	virtual ~SampleRecordHandle();

	virtual void play( sampleFrame * _working_buffer );
	virtual bool isFinished() const;

	virtual bool isFromTrack( const track * _track ) const;

	f_cnt_t framesRecorded() const;
	void createSampleBuffer( SampleBuffer * * _sample_buf );


private:
	virtual void writeBuffer( const sampleFrame * _ab,
						const f_cnt_t _frames );

	typedef QList<QPair<sampleFrame *, f_cnt_t> > bufferList;
	bufferList m_buffers;
	f_cnt_t m_framesRecorded;
	MidiTime m_minLength;

	track * m_track;
	bbTrack * m_bbTrack;
	SampleTCO * m_tco;

} ;


#endif
