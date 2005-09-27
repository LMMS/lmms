/*
 * sample_play_handle.cpp - implementation of class samplePlayHandle
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "qt3support.h"
#ifndef QT4
#include <qpair.h>
#endif

#include "sample_play_handle.h"
#include "sample_buffer.h"
#include "buffer_allocator.h"


samplePlayHandle::samplePlayHandle( const QString & _sample_file ) :
	playHandle(),
	m_sampleBuffer( new sampleBuffer( _sample_file ) ),
	m_ownSampleBuffer( TRUE ),
	m_doneMayReturnTrue( TRUE ),
	m_frame( 0 )
{
}


samplePlayHandle::samplePlayHandle( sampleBuffer * _sample_buffer ) :
	playHandle(),
	m_sampleBuffer( _sample_buffer ),
	m_ownSampleBuffer( FALSE ),
	m_doneMayReturnTrue( TRUE ),
	m_frame( 0 )
{
}



samplePlayHandle::~samplePlayHandle()
{
	if( m_ownSampleBuffer == TRUE )
	{
		delete m_sampleBuffer;
	}
}




void samplePlayHandle::play( void )
{
	if( framesDone() >= totalFrames() )
	{
		return;
	}

	sampleFrame * buf = bufferAllocator::alloc<sampleFrame>(
					mixer::inst()->framesPerAudioBuffer() *
							DEFAULT_CHANNELS );
	volumeVector v = { 1.0f, 1.0f
#ifndef DISABLE_SURROUND
					, 1.0f, 1.0f
#endif
			} ;
	m_sampleBuffer->play( buf, m_frame );
	mixer::inst()->addBuffer( buf, mixer::inst()->framesPerAudioBuffer(),
									0, v );

	bufferAllocator::free( buf );

	m_frame += mixer::inst()->framesPerAudioBuffer();
}




bool samplePlayHandle::done( void ) const
{
	return( framesDone() >= totalFrames() && m_doneMayReturnTrue == TRUE );
}




Uint32 samplePlayHandle::totalFrames( void ) const
{
	return( m_sampleBuffer->endFrame() - m_sampleBuffer->startFrame() );
}

