#ifndef SINGLE_SOURCE_COMPILE

/*
 * sample_play_handle.cpp - implementation of class samplePlayHandle
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "sample_play_handle.h"
#include "audio_port.h"
#include "bb_track.h"
#include "engine.h"
#include "instrument_track.h"
#include "pattern.h"
#include "sample_buffer.h"
#include "sample_track.h"



samplePlayHandle::samplePlayHandle( const QString & _sample_file ) :
	playHandle( SamplePlayHandle ),
	m_sampleBuffer( new sampleBuffer( _sample_file ) ),
	m_doneMayReturnTrue( TRUE ),
	m_frame( 0 ),
	m_audioPort( new audioPort( "samplePlayHandle", NULL ) ),
	m_ownAudioPort( TRUE ),
	m_defaultVolumeModel( 1.0f, 0.0f, 4.0f, 0.001f/* this*/ ),
	m_volumeModel( &m_defaultVolumeModel ),
	m_track( NULL ),
	m_bbTrack( NULL )
{
}




samplePlayHandle::samplePlayHandle( sampleBuffer * _sample_buffer ) :
	playHandle( SamplePlayHandle ),
	m_sampleBuffer( sharedObject::ref( _sample_buffer ) ),
	m_doneMayReturnTrue( TRUE ),
	m_frame( 0 ),
	m_audioPort( new audioPort( "samplePlayHandle", NULL ) ),
	m_ownAudioPort( TRUE ),
	m_defaultVolumeModel( 1.0f, 0.0f, 4.0f, 0.001f/* this*/ ),
	m_volumeModel( &m_defaultVolumeModel ),
	m_track( NULL ),
	m_bbTrack( NULL )
{
}




samplePlayHandle::samplePlayHandle( sampleTCO * _tco ) :
	playHandle( SamplePlayHandle ),
	m_sampleBuffer( sharedObject::ref( _tco->getSampleBuffer() ) ),
	m_doneMayReturnTrue( TRUE ),
	m_frame( 0 ),
	m_audioPort( ( (sampleTrack *)_tco->getTrack() )->getAudioPort() ),
	m_ownAudioPort( FALSE ),
	m_defaultVolumeModel( 1.0f, 0.0f, 4.0f, 0.001f/* this*/ ),
	m_volumeModel( &m_defaultVolumeModel ),
	m_track( _tco->getTrack() ),
	m_bbTrack( NULL )
{
}




samplePlayHandle::samplePlayHandle( pattern * _pattern ) :
	playHandle( SamplePlayHandle ),
	m_sampleBuffer( sharedObject::ref( _pattern->getFrozenPattern() ) ),
	m_doneMayReturnTrue( TRUE ),
	m_frame( 0 ),
	m_audioPort( _pattern->getInstrumentTrack()->getAudioPort() ),
	m_ownAudioPort( FALSE ),
	m_defaultVolumeModel( 1.0f, 0.0f, 4.0f, 0.001f/* this*/ ),
	m_volumeModel( &m_defaultVolumeModel ),
	m_track( _pattern->getInstrumentTrack() ),
	m_bbTrack( NULL )
{
}




samplePlayHandle::~samplePlayHandle()
{
	sharedObject::unref( m_sampleBuffer );
	if( m_ownAudioPort )
	{
		delete m_audioPort;
	}
}




void samplePlayHandle::play( bool /* _try_parallelizing */ )
{
	//play( 0, _try_parallelizing );
	if( framesDone() >= totalFrames() )
	{
		return;
	}

	const fpp_t frames = engine::getMixer()->framesPerPeriod();
	if( !( m_track && m_track->muted() )
				&& !( m_bbTrack && m_bbTrack->muted() ) )
	{
		sampleFrame * buf = new sampleFrame[frames];
		stereoVolumeVector v = { { m_volumeModel->value(),
						m_volumeModel->value() } };
		m_sampleBuffer->play( buf, &m_state, frames );
		engine::getMixer()->bufferToPort( buf, frames, offset(), v,
								m_audioPort );

		delete[] buf;
	}

	m_frame += frames;
}




bool samplePlayHandle::done( void ) const
{
	return( framesDone() >= totalFrames() && m_doneMayReturnTrue == TRUE );
}




bool samplePlayHandle::isFromTrack( const track * _track ) const
{
	return( m_track == _track || m_bbTrack == _track );
}




f_cnt_t samplePlayHandle::totalFrames( void ) const
{
	return( ( m_sampleBuffer->endFrame() - m_sampleBuffer->startFrame() ) *
				( engine::getMixer()->sampleRate() /
					SAMPLE_RATES[DEFAULT_QUALITY_LEVEL] ) );
}




#endif
