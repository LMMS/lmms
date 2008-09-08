
// Blip_Buffer 0.3.4. http://www.slack.net/~ant/libs/

#include "Multi_Buffer.h"

/* Copyright (C) 2003-2005 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details. You should have received a copy of the GNU Lesser General
Public License along with this module; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

#include BLARGG_SOURCE_BEGIN

Multi_Buffer::Multi_Buffer( int spf ) : samples_per_frame_( spf )
{
	length_ = 0;
	sample_rate_ = 0;
	channels_changed_count_ = 1;
}

blargg_err_t Multi_Buffer::set_channel_count( int )
{
	return blargg_success;
}

Mono_Buffer::Mono_Buffer() : Multi_Buffer( 1 )
{
}

Mono_Buffer::~Mono_Buffer()
{
}

blargg_err_t Mono_Buffer::set_sample_rate( long rate, int msec )
{
	BLARGG_RETURN_ERR( buf.set_sample_rate( rate, msec ) );
	return Multi_Buffer::set_sample_rate( buf.sample_rate(), buf.length() );
}

// Silent_Buffer

Silent_Buffer::Silent_Buffer() : Multi_Buffer( 1 ) // 0 channels would probably confuse
{
	chan.left   = NULL;
	chan.center = NULL;
	chan.right  = NULL;
}

// Mono_Buffer

Mono_Buffer::channel_t Mono_Buffer::channel( int index )
{
	channel_t ch;
	ch.center = &buf;
	ch.left   = &buf;
	ch.right  = &buf;
	return ch;
}

void Mono_Buffer::end_frame( blip_time_t t, bool )
{
	buf.end_frame( t );
}

// Stereo_Buffer

Stereo_Buffer::Stereo_Buffer() : Multi_Buffer( 2 )
{
	chan.center = &bufs [0];
	chan.left = &bufs [1];
	chan.right = &bufs [2];
}

Stereo_Buffer::~Stereo_Buffer()
{
}

blargg_err_t Stereo_Buffer::set_sample_rate( long rate, int msec )
{
	for ( int i = 0; i < buf_count; i++ )
		BLARGG_RETURN_ERR( bufs [i].set_sample_rate( rate, msec ) );
	return Multi_Buffer::set_sample_rate( bufs [0].sample_rate(), bufs [0].length() );
}

void Stereo_Buffer::clock_rate( long rate )
{
	for ( int i = 0; i < buf_count; i++ )
		bufs [i].clock_rate( rate );
}

void Stereo_Buffer::bass_freq( int bass )
{
	for ( unsigned i = 0; i < buf_count; i++ )
		bufs [i].bass_freq( bass );
}

void Stereo_Buffer::clear()
{
	stereo_added = false;
	was_stereo = false;
	for ( int i = 0; i < buf_count; i++ )
		bufs [i].clear();
}

void Stereo_Buffer::end_frame( blip_time_t clock_count, bool stereo )
{
	for ( unsigned i = 0; i < buf_count; i++ )
		bufs [i].end_frame( clock_count );
	
	stereo_added |= stereo;
}

long Stereo_Buffer::read_samples( blip_sample_t* out, long count )
{
	require( !(count & 1) ); // count must be even
	count = (unsigned) count / 2;
	
	long avail = bufs [0].samples_avail();
	if ( count > avail )
		count = avail;
	if ( count )
	{
		if ( stereo_added || was_stereo )
		{
			mix_stereo( out, count );
			
			bufs [0].remove_samples( count );
			bufs [1].remove_samples( count );
			bufs [2].remove_samples( count );
		}
		else
		{
			mix_mono( out, count );
			
			bufs [0].remove_samples( count );
			
			bufs [1].remove_silence( count );
			bufs [2].remove_silence( count );
		}
		
		// to do: this might miss opportunities for optimization
		if ( !bufs [0].samples_avail() ) {
			was_stereo = stereo_added;
			stereo_added = false;
		}
	}
	
	return count * 2;
}

#include BLARGG_ENABLE_OPTIMIZER

void Stereo_Buffer::mix_stereo( blip_sample_t* out, long count )
{
	Blip_Reader left; 
	Blip_Reader right; 
	Blip_Reader center;
	
	left.begin( bufs [1] );
	right.begin( bufs [2] );
	int bass = center.begin( bufs [0] );
	
	while ( count-- )
	{
		int c = center.read();
		long l = c + left.read();
		long r = c + right.read();
		center.next( bass );
		out [0] = l;
		out [1] = r;
		out += 2;
		
		if ( (BOOST::int16_t) l != l )
			out [-2] = 0x7FFF - (l >> 24);
		
		left.next( bass );
		right.next( bass );
		
		if ( (BOOST::int16_t) r != r )
			out [-1] = 0x7FFF - (r >> 24);
	}
	
	center.end( bufs [0] );
	right.end( bufs [2] );
	left.end( bufs [1] );
}

void Stereo_Buffer::mix_mono( blip_sample_t* out, long count )
{
	Blip_Reader in;
	int bass = in.begin( bufs [0] );
	
	while ( count-- )
	{
		long s = in.read();
		in.next( bass );
		out [0] = s;
		out [1] = s;
		out += 2;
		
		if ( (BOOST::int16_t) s != s ) {
			s = 0x7FFF - (s >> 24);
			out [-2] = s;
			out [-1] = s;
		}
	}
	
	in.end( bufs [0] );
}

