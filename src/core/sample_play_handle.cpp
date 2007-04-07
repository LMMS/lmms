#ifndef SINGLE_SOURCE_COMPILE

/*
 * sample_play_handle.cpp - implementation of class samplePlayHandle
 *
 * Copyright (c) 2005-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include "bb_track.h"
#include "instrument_track.h"
#include "pattern.h"
#include "sample_buffer.h"
#include "sample_track.h"
#include "buffer_allocator.h"
#include "audio_port.h"



samplePlayHandle::samplePlayHandle( const QString & _sample_file,
							engine * _engine ) :
	playHandle( SamplePlayHandle ),
	engineObject( _engine ),
	m_sampleBuffer( new sampleBuffer( eng(), _sample_file ) ),
	m_doneMayReturnTrue( TRUE ),
	m_frame( 0 ),
	m_audioPort( new audioPort( "samplePlayHandle", eng() ) ),
	m_ownAudioPort( TRUE ),
	m_volume( 1.0f ),
	m_track( NULL ),
	m_bbTrack( NULL )
{
}




samplePlayHandle::samplePlayHandle( sampleBuffer * _sample_buffer ) :
	playHandle( SamplePlayHandle ),
	engineObject( _sample_buffer->eng() ),
	m_sampleBuffer( sharedObject::ref( _sample_buffer ) ),
	m_doneMayReturnTrue( TRUE ),
	m_frame( 0 ),
	m_audioPort( new audioPort( "samplePlayHandle", eng() ) ),
	m_ownAudioPort( TRUE ),
	m_volume( 1.0f ),
	m_track( NULL ),
	m_bbTrack( NULL )
{
}




samplePlayHandle::samplePlayHandle( sampleTCO * _tco ) :
	playHandle( SamplePlayHandle ),
	engineObject( _tco->eng() ),
	m_sampleBuffer( sharedObject::ref( _tco->getSampleBuffer() ) ),
	m_doneMayReturnTrue( TRUE ),
	m_frame( 0 ),
	m_audioPort( ( (sampleTrack *)_tco->getTrack() )->getAudioPort() ),
	m_ownAudioPort( FALSE ),
	m_volume( 1.0f ),
	m_track( _tco->getTrack() ),
	m_bbTrack( NULL )
{
}




samplePlayHandle::samplePlayHandle( pattern * _pattern ) :
	playHandle( SamplePlayHandle ),
	engineObject( _pattern->eng() ),
	m_sampleBuffer( sharedObject::ref( _pattern->getFrozenPattern() ) ),
	m_doneMayReturnTrue( TRUE ),
	m_frame( 0 ),
	m_audioPort( _pattern->getInstrumentTrack()->getAudioPort() ),
	m_ownAudioPort( FALSE ),
	m_volume( 1.0f ),
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




void samplePlayHandle::play( bool _try_parallelizing )
{
	play( 0, _try_parallelizing );
}




void samplePlayHandle::play( const fpab_t _frame_base, bool )
{
	if( framesDone() >= totalFrames() )
	{
		return;
	}

	const fpab_t frames = eng()->getMixer()->framesPerAudioBuffer()
								- _frame_base;
	if( !( m_track && m_track->muted() )
				&& !( m_bbTrack && m_bbTrack->muted() ) )
	{
		sampleFrame * buf = bufferAllocator::alloc<sampleFrame>(
								frames );
		volumeVector v = { { m_volume, m_volume
#ifndef DISABLE_SURROUND
						, m_volume, m_volume
#endif
				} } ;
		m_sampleBuffer->play( buf, &m_state, frames );
		eng()->getMixer()->bufferToPort( buf, frames, _frame_base, v,
								m_audioPort );

		bufferAllocator::free( buf );
	}

	m_frame += frames;
}




bool samplePlayHandle::done( void ) const
{
	return( framesDone() >= totalFrames() && m_doneMayReturnTrue == TRUE );
}




f_cnt_t samplePlayHandle::totalFrames( void ) const
{
	return( m_sampleBuffer->endFrame() - m_sampleBuffer->startFrame() );
}




void samplePlayHandle::setVolume( float _new_volume )
{
	if( _new_volume <= MAX_VOLUME )
	{
		m_volume = _new_volume / 100.0f;
	}
}




#include "sample_play_handle.moc"


#endif
