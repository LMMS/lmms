/*
 * SamplePlayHandle.h - play-handle for playing a sample
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _SAMPLE_PLAY_HANDLE_H
#define _SAMPLE_PLAY_HANDLE_H

#include "Mixer.h"
#include "SampleBuffer.h"
#include "AutomatableModel.h"

class bbTrack;
class SampleTCO;
class track;
class AudioPort;


class SamplePlayHandle : public PlayHandle
{
public:
	SamplePlayHandle( const QString& sampleFile );
	SamplePlayHandle( SampleBuffer* sampleBuffer );
	SamplePlayHandle( SampleTCO* tco );
	virtual ~SamplePlayHandle();

	virtual inline bool affinityMatters() const
	{
		return true;
	}


	virtual void play( sampleFrame * _working_buffer );
	virtual bool isFinished() const;

	virtual bool isFromTrack( const track * _track ) const;

	f_cnt_t totalFrames() const;
	inline f_cnt_t framesDone() const
	{
		return( m_frame );
	}
	void setDoneMayReturnTrue( bool _enable )
	{
		m_doneMayReturnTrue = _enable;
	}

	void setBBTrack( bbTrack * _bb_track )
	{
		m_bbTrack = _bb_track;
	}

	void setVolumeModel( FloatModel * _model )
	{
		m_volumeModel = _model;
	}


private:
	SampleBuffer * m_sampleBuffer;
	bool m_doneMayReturnTrue;

	f_cnt_t m_frame;
	SampleBuffer::handleState m_state;

	AudioPort * m_audioPort;
	const bool m_ownAudioPort;

	FloatModel m_defaultVolumeModel;
	FloatModel * m_volumeModel;
	track * m_track;

	bbTrack * m_bbTrack;

} ;


#endif
