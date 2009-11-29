/*
 * sample_record_handle.h - play-handle for recording a sample
 *
 * Copyright (c) 2008 Csaba Hruska <csaba.hruska/at/gmail.com>
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


#ifndef _SAMPLE_RECORD_HANDLE_H
#define _SAMPLE_RECORD_HANDLE_H

#include <QtCore/QList>
#include <QtCore/QPair>
#include <qobject.h>

#include "Mixer.h"
#include "sample_buffer.h"

class bbTrack;
class pattern;
class sampleTCO;
class track;


class sampleRecordHandle : public playHandle
{
public:
	sampleRecordHandle( sampleTCO * _tco );
	virtual ~sampleRecordHandle();

	virtual void play( sampleFrame * _working_buffer );
	virtual bool done() const;

	virtual bool isFromTrack( const track * _track ) const;

	f_cnt_t framesRecorded() const;
	void createSampleBuffer( sampleBuffer * * _sample_buf );


private:
	virtual void writeBuffer( const sampleFrame * _ab,
						const f_cnt_t _frames );

	typedef QList<QPair<sampleFrame *, f_cnt_t> > bufferList;
	bufferList m_buffers;
	f_cnt_t m_framesRecorded;
	midiTime m_minLength;

	track * m_track;
	bbTrack * m_bbTrack;
	sampleTCO * m_tco;

} ;


#endif
