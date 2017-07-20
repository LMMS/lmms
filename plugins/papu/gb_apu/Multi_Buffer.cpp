// Blip_Buffer 0.4.1. http://www.slack.net/~ant/

#include "Multi_Buffer.h"

/* Copyright (C) 2003-2006 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

#include "blargg_source.h"

#ifdef BLARGG_ENABLE_OPTIMIZER
	#include BLARGG_ENABLE_OPTIMIZER
#endif

Multi_Buffer::Multi_Buffer( int spf ) : samples_per_frame_( spf )
{
	length_ = 0;
	sample_rate_ = 0;
	channels_changed_count_ = 1;
}

blargg_err_t Multi_Buffer::set_channel_count( int ) { return 0; }

// Silent_Buffer

Silent_Buffer::Silent_Buffer() : Multi_Buffer( 1 ) // 0 channels would probably confuse
{
	// TODO: better to use empty Blip_Buffer so caller never has to check for NULL?
	chan.left   = 0;
	chan.center = 0;
	chan.right  = 0;
}

// Mono_Buffer

Mono_Buffer::Mono_Buffer() : Multi_Buffer( 1 )
{
	chan.center = &buf;
	chan.left   = &buf;
	chan.right  = &buf;
}

Mono_Buffer::~Mono_Buffer() { }

blargg_err_t Mono_Buffer::set_sample_rate( long rate, int msec )
{
	RETURN_ERR( buf.set_sample_rate( rate, msec ) );
	return Multi_Buffer::set_sample_rate( buf.sample_rate(), buf.length() );
}

// Stereo_Buffer

Stereo_Buffer::Stereo_Buffer() : Multi_Buffer( 2 )
{
	chan.center = &bufs [0];
	chan.left = &bufs [1];
	chan.right = &bufs [2];
}

Stereo_Buffer::~Stereo_Buffer() { }

blargg_err_t Stereo_Buffer::set_sample_rate( long rate, int msec )
{
	for ( int i = 0; i < buf_count; i++ )
		RETURN_ERR( bufs [i].set_sample_rate( rate, msec ) );
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
	stereo_added = 0;
	was_stereo   = false;
	for ( int i = 0; i < buf_count; i++ )
		bufs [i].clear();
}

void Stereo_Buffer::end_frame( blip_time_t clock_count )
{
	stereo_added = 0;
	for ( unsigned i = 0; i < buf_count; i++ )
	{
		stereo_added |= bufs [i].clear_modified() << i;
		bufs [i].end_frame( clock_count );
	}
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
		int bufs_used = stereo_added | was_stereo;
		//debug_printf( "%X\n", bufs_used );
		if ( bufs_used <= 1 )
		{
			mix_mono( out, count );
			bufs [0].remove_samples( count );
			bufs [1].remove_silence( count );
			bufs [2].remove_silence( count );
		}
		else if ( bufs_used & 1 )
		{
			mix_stereo( out, count );
			bufs [0].remove_samples( count );
			bufs [1].remove_samples( count );
			bufs [2].remove_samples( count );
		}
		else
		{
			mix_stereo_no_center( out, count );
			bufs [0].remove_silence( count );
			bufs [1].remove_samples( count );
			bufs [2].remove_samples( count );
		}
		
		// to do: this might miss opportunities for optimization
		if ( !bufs [0].samples_avail() )
		{
			was_stereo   = stereo_added;
			stereo_added = 0;
		}
	}
	
	return count * 2;
}

void Stereo_Buffer::mix_stereo( blip_sample_t* out_, blargg_long count )
{
	blip_sample_t* BLIP_RESTRICT out = out_;
	int const bass = BLIP_READER_BASS( bufs [1] );
	BLIP_READER_BEGIN( left, bufs [1] );
	BLIP_READER_BEGIN( right, bufs [2] );
	BLIP_READER_BEGIN( center, bufs [0] );
	
	for ( ; count; --count )
	{
		int c = BLIP_READER_READ( center );
		blargg_long l = c + BLIP_READER_READ( left );
		blargg_long r = c + BLIP_READER_READ( right );
		if ( (BOOST::int16_t) l != l )
			l = 0x7FFF - (l >> 24);
		
		BLIP_READER_NEXT( center, bass );
		if ( (BOOST::int16_t) r != r )
			r = 0x7FFF - (r >> 24);
		
		BLIP_READER_NEXT( left, bass );
		BLIP_READER_NEXT( right, bass );
		
		out [0] = l;
		out [1] = r;
		out += 2;
	}
	
	BLIP_READER_END( center, bufs [0] );
	BLIP_READER_END( right, bufs [2] );
	BLIP_READER_END( left, bufs [1] );
}

void Stereo_Buffer::mix_stereo_no_center( blip_sample_t* out_, blargg_long count )
{
	blip_sample_t* BLIP_RESTRICT out = out_;
	int const bass = BLIP_READER_BASS( bufs [1] );
	BLIP_READER_BEGIN( left, bufs [1] );
	BLIP_READER_BEGIN( right, bufs [2] );
	
	for ( ; count; --count )
	{
		blargg_long l = BLIP_READER_READ( left );
		if ( (BOOST::int16_t) l != l )
			l = 0x7FFF - (l >> 24);
		
		blargg_long r = BLIP_READER_READ( right );
		if ( (BOOST::int16_t) r != r )
			r = 0x7FFF - (r >> 24);
		
		BLIP_READER_NEXT( left, bass );
		BLIP_READER_NEXT( right, bass );
		
		out [0] = l;
		out [1] = r;
		out += 2;
	}
	
	BLIP_READER_END( right, bufs [2] );
	BLIP_READER_END( left, bufs [1] );
}

void Stereo_Buffer::mix_mono( blip_sample_t* out_, blargg_long count )
{
	blip_sample_t* BLIP_RESTRICT out = out_;
	int const bass = BLIP_READER_BASS( bufs [0] );
	BLIP_READER_BEGIN( center, bufs [0] );
	
	for ( ; count; --count )
	{
		blargg_long s = BLIP_READER_READ( center );
		if ( (BOOST::int16_t) s != s )
			s = 0x7FFF - (s >> 24);
		
		BLIP_READER_NEXT( center, bass );
		out [0] = s;
		out [1] = s;
		out += 2;
	}
	
	BLIP_READER_END( center, bufs [0] );
}
