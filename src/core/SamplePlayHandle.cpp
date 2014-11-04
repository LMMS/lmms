/*
 * SamplePlayHandle.cpp - implementation of class SamplePlayHandle
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

#include "SamplePlayHandle.h"
#include "AudioPort.h"
#include "bb_track.h"
#include "engine.h"
#include "InstrumentTrack.h"
#include "Pattern.h"
#include "SampleBuffer.h"
#include "SampleTrack.h"



SamplePlayHandle::SamplePlayHandle( const QString& sampleFile ) :
	PlayHandle( TypeSamplePlayHandle ),
	m_sampleBuffer( new SampleBuffer( sampleFile ) ),
	m_doneMayReturnTrue( true ),
	m_frame( 0 ),
	m_audioPort( new AudioPort( "SamplePlayHandle", false ) ),
	m_ownAudioPort( true ),
	m_defaultVolumeModel( DefaultVolume, MinVolume, MaxVolume, 1 ),
	m_volumeModel( &m_defaultVolumeModel ),
	m_track( NULL ),
	m_bbTrack( NULL )
{
}




SamplePlayHandle::SamplePlayHandle( SampleBuffer* sampleBuffer ) :
	PlayHandle( TypeSamplePlayHandle ),
	m_sampleBuffer( sharedObject::ref( sampleBuffer ) ),
	m_doneMayReturnTrue( true ),
	m_frame( 0 ),
	m_audioPort( new AudioPort( "SamplePlayHandle", false ) ),
	m_ownAudioPort( true ),
	m_defaultVolumeModel( DefaultVolume, MinVolume, MaxVolume, 1 ),
	m_volumeModel( &m_defaultVolumeModel ),
	m_track( NULL ),
	m_bbTrack( NULL )
{
}




SamplePlayHandle::SamplePlayHandle( SampleTCO* tco ) :
	PlayHandle( TypeSamplePlayHandle ),
	m_sampleBuffer( sharedObject::ref( tco->sampleBuffer() ) ),
	m_doneMayReturnTrue( true ),
	m_frame( 0 ),
	m_audioPort( ( (SampleTrack *)tco->getTrack() )->audioPort() ),
	m_ownAudioPort( false ),
	m_defaultVolumeModel( DefaultVolume, MinVolume, MaxVolume, 1 ),
	m_volumeModel( &m_defaultVolumeModel ),
	m_track( tco->getTrack() ),
	m_bbTrack( NULL )
{
}




SamplePlayHandle::~SamplePlayHandle()
{
	sharedObject::unref( m_sampleBuffer );
	if( m_ownAudioPort )
	{
		delete m_audioPort;
	}
}




void SamplePlayHandle::play( sampleFrame * _working_buffer )
{
	//play( 0, _try_parallelizing );
	if( framesDone() >= totalFrames() )
	{
		return;
	}

	const fpp_t frames = engine::mixer()->framesPerPeriod();
	if( !( m_track && m_track->isMuted() )
				&& !( m_bbTrack && m_bbTrack->isMuted() ) )
	{
		stereoVolumeVector v =
			{ { m_volumeModel->value() / DefaultVolume,
				m_volumeModel->value() / DefaultVolume } };
		m_sampleBuffer->play( _working_buffer, &m_state, frames,
								BaseFreq );
		engine::mixer()->bufferToPort( _working_buffer, frames,
						offset(), v, m_audioPort );
	}

	m_frame += frames;
}




bool SamplePlayHandle::isFinished() const
{
	return framesDone() >= totalFrames() && m_doneMayReturnTrue == true;
}




bool SamplePlayHandle::isFromTrack( const track * _track ) const
{
	return m_track == _track || m_bbTrack == _track;
}




f_cnt_t SamplePlayHandle::totalFrames() const
{
	return ( m_sampleBuffer->endFrame() - m_sampleBuffer->startFrame() ) * ( engine::mixer()->processingSampleRate() / engine::mixer()->baseSampleRate() );
}




