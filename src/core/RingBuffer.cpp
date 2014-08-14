/*
 * RingBuffer.cpp - an effective, thread-safe and flexible implementation of a ringbuffer for LMMS
 *
 * Copyright (c) 2014 Vesa Kivimäki
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "RingBuffer.h"
#include "engine.h"
#include "Mixer.h"
#include <string.h>
#include "MixHelpers.h"

 
RingBuffer::RingBuffer( f_cnt_t size ) : 
	m_fpp( engine::mixer()->framesPerPeriod() ),
	m_samplerate( engine::mixer()->processingSampleRate() ),
	m_size( size + m_fpp )
{
	m_buffer = new sampleFrame[ m_size ];
	m_position = 0;
}


RingBuffer::RingBuffer( float size ) : 
	m_fpp( engine::mixer()->framesPerPeriod() ),
	m_samplerate( engine::mixer()->processingSampleRate() )
{
	m_size = msToFrames( size ) + m_fpp;
	m_buffer = new sampleFrame[ m_size ];
	memset( m_buffer, 0, m_size * sizeof( sampleFrame ) );
	m_position = 0;
	setSamplerateAware( true );
}


RingBuffer::~RingBuffer()
{
	delete m_buffer;
}


void RingBuffer::reset()
{
	lock();

	memset( m_buffer, 0, m_size * sizeof( sampleFrame ) );
	m_position = 0;

	unlock();
}


void RingBuffer::changeSize( f_cnt_t size )
{
	lock();

	delete m_buffer;
	m_size = size + m_fpp;
	m_buffer = new sampleFrame[ m_size ];
	memset( m_buffer, 0, m_size * sizeof( sampleFrame ) );
	m_position = 0;
	
	unlock();
}


void RingBuffer::changeSize( float size )
{
	changeSize( msToFrames( size ) );
}


void RingBuffer::setSamplerateAware( bool b )
{
	if( b )
	{
		connect( engine::mixer(), SIGNAL( sampleRateChanged() ), this, SLOT( updateSampleRate() ), Qt::UniqueConnection );
	}
	else
	{
		disconnect( engine::mixer(), SIGNAL( sampleRateChanged() ), this, SLOT( updateSampleRate() ) );
	}
}


void RingBuffer::advance()
{
	lock();
		
	m_position = ( m_position + m_fpp ) % m_size;
	
	unlock();
}


void RingBuffer::movePosition( f_cnt_t amount )
{
	m_position = ( m_position + amount ) % m_size;
}


void RingBuffer::movePosition( float amount )
{
	movePosition( msToFrames( amount ) );
}


void RingBuffer::pop( sampleFrame * dst )
{
	lock();
	
	if( m_position + m_fpp <= m_size ) // we won't go over the edge so we can just memcpy here
	{
		memcpy( dst, m_buffer + ( m_position * sizeof( sampleFrame ) ), m_fpp * sizeof( sampleFrame ) );
		memset( m_buffer + ( m_position * sizeof( sampleFrame ) ), 0, m_fpp * sizeof( sampleFrame ) );
	}
	else
	{
		f_cnt_t first = m_size - m_position;
		f_cnt_t second = m_fpp - first;
		
		memcpy( dst, m_buffer + ( m_position * sizeof( sampleFrame ) ), first * sizeof( sampleFrame ) );
		memset( m_buffer + ( m_position * sizeof( sampleFrame ) ), 0, first * sizeof( sampleFrame ) );
		
		memcpy( dst + ( first * sizeof( sampleFrame ) ), m_buffer, second * sizeof( sampleFrame ) );
		memset( m_buffer, 0, second * sizeof( sampleFrame ) );
	}
	
	m_position = ( m_position + m_fpp ) % m_size;
	
	unlock();
}


void RingBuffer::read( sampleFrame * dst, f_cnt_t offset )
{
	lock();
	
	f_cnt_t pos = ( m_position + offset ) % m_size;
	if( pos < 0 ) { pos += m_size; }
	
	if( pos + m_fpp <= m_size ) // we won't go over the edge so we can just memcpy here
	{
		memcpy( dst, m_buffer + ( pos * sizeof( sampleFrame ) ), m_fpp * sizeof( sampleFrame ) );
	}
	else
	{
		f_cnt_t first = m_size - pos;
		f_cnt_t second = m_fpp - first;
		
		memcpy( dst, m_buffer + ( pos * sizeof( sampleFrame ) ), first * sizeof( sampleFrame ) );
		
		memcpy( dst + ( first * sizeof( sampleFrame ) ), m_buffer, second * sizeof( sampleFrame ) );
	}
	
	unlock();
}


void RingBuffer::read( sampleFrame * dst, float offset )
{
	read( dst, msToFrames( offset ) );
}


void RingBuffer::read( sampleFrame * dst, f_cnt_t offset, f_cnt_t length )
{
	lock();
	
	f_cnt_t pos = ( m_position + offset ) % m_size;
	if( pos < 0 ) { pos += m_size; }
	
	if( pos + length <= m_size ) // we won't go over the edge so we can just memcpy here
	{
		memcpy( dst, m_buffer + ( pos * sizeof( sampleFrame ) ), length * sizeof( sampleFrame ) );
	}
	else
	{
		f_cnt_t first = m_size - pos;
		f_cnt_t second = length - first;
		
		memcpy( dst, m_buffer + ( pos * sizeof( sampleFrame ) ), first * sizeof( sampleFrame ) );
		
		memcpy( dst + ( first * sizeof( sampleFrame ) ), m_buffer, second * sizeof( sampleFrame ) );
	}
	
	unlock();
}


void RingBuffer::read( sampleFrame * dst, float offset, f_cnt_t length )
{
	read( dst, msToFrames( offset ), length );
}


void RingBuffer::write( sampleFrame * src, f_cnt_t offset, f_cnt_t length )
{
	lock();
	
	const f_cnt_t pos = ( m_position + offset ) % m_size;
	if( length == 0 ) { length = m_fpp; }
	
	if( pos + length <= m_size ) // we won't go over the edge so we can just memcpy here
	{
		memcpy( m_buffer + ( pos * sizeof( sampleFrame ) ), src, length * sizeof( sampleFrame ) );
	}
	else
	{
		f_cnt_t first = m_size - pos;
		f_cnt_t second = length - first;

		memcpy( m_buffer + ( pos * sizeof( sampleFrame ) ), src, first * sizeof( sampleFrame ) );
		
		memcpy( m_buffer, src + ( first * sizeof( sampleFrame ) ), second * sizeof( sampleFrame ) );
	}
	
	unlock();
}


void RingBuffer::write( sampleFrame * src, float offset, f_cnt_t length )
{
	write( src, msToFrames( offset ), length );
}


void RingBuffer::writeAdding( sampleFrame * src, f_cnt_t offset, f_cnt_t length )
{
	lock();
	
	const f_cnt_t pos = ( m_position + offset ) % m_size;
	if( length == 0 ) { length = m_fpp; }
	
	if( pos + length <= m_size ) // we won't go over the edge so we can just memcpy here
	{
		MixHelpers::add( m_buffer + ( pos * sizeof( sampleFrame ) ), src, length );
	}
	else
	{
		f_cnt_t first = m_size - pos;
		f_cnt_t second = length - first;

		MixHelpers::add( m_buffer + ( pos * sizeof( sampleFrame ) ), src, first );
		
		MixHelpers::add( m_buffer, src + ( first * sizeof( sampleFrame ) ), second );
	}
	
	unlock();
}


void RingBuffer::writeAdding( sampleFrame * src, float offset, f_cnt_t length )
{
	writeAdding( src, msToFrames( offset ), length );
}


void RingBuffer::writeAddingMultiplied( sampleFrame * src, f_cnt_t offset, f_cnt_t length, float level )
{
	lock();
	
	const f_cnt_t pos = ( m_position + offset ) % m_size;
	if( length == 0 ) { length = m_fpp; }
	
	if( pos + length <= m_size ) // we won't go over the edge so we can just memcpy here
	{
		MixHelpers::addMultiplied( m_buffer + ( pos * sizeof( sampleFrame ) ), src, level, length );
	}
	else
	{
		f_cnt_t first = m_size - pos;
		f_cnt_t second = length - first;

		MixHelpers::addMultiplied( m_buffer + ( pos * sizeof( sampleFrame ) ), src, level, first );
		
		MixHelpers::addMultiplied( m_buffer, src + ( first * sizeof( sampleFrame ) ), level, second );
	}
	
	unlock();
}


void RingBuffer::writeAddingMultiplied( sampleFrame * src, float offset, f_cnt_t length, float level )
{
	writeAddingMultiplied( src, msToFrames( offset ), length, level );
}


void RingBuffer::updateSamplerate()
{
	lock();
	
	float newsize = static_cast<float>( ( m_size - m_fpp ) * engine::mixer()->processingSampleRate() ) / m_samplerate;
	m_size = static_cast<f_cnt_t>( ceilf( newsize ) ) + m_fpp;
	m_samplerate = engine::mixer()->processingSampleRate();
	delete m_buffer;
	m_buffer = new sampleFrame[ m_size ];
	memset( m_buffer, 0, m_size * sizeof( sampleFrame ) );
	m_position = 0;
	
	unlock();
}


