
// Blip_Buffer 0.3.4. http://www.slack.net/~ant/libs/

#include "Blip_Buffer.h"

#include <string.h>
#include <stddef.h>
#include <math.h>

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

Blip_Buffer::Blip_Buffer()
{
	samples_per_sec = 44100;
	buffer_ = NULL;
	
	// try to cause assertion failure if buffer is used before these are set
	clocks_per_sec = 0;
	factor_ = ~0ul;
	offset_ = 0;
	buffer_size_ = 0;
	length_ = 0;
	
	bass_freq_ = 16;
}

void Blip_Buffer::clear( bool entire_buffer )
{
	long count = (entire_buffer ? buffer_size_ : samples_avail());
	offset_ = 0;
	reader_accum = 0;
	if ( buffer_ )
		memset( buffer_, sample_offset_ & 0xFF, (count + widest_impulse_) * sizeof (buf_t_) );
}

blargg_err_t Blip_Buffer::set_sample_rate( long new_rate, int msec )
{
	unsigned new_size = (ULONG_MAX >> BLIP_BUFFER_ACCURACY) + 1 - widest_impulse_ - 64;
	if ( msec != blip_default_length )
	{
		size_t s = (new_rate * (msec + 1) + 999) / 1000;
		if ( s < new_size )
			new_size = s;
		else
			require( false ); // requested buffer length exceeds limit
	}
	
	if ( buffer_size_ != new_size )
	{
		delete [] buffer_;
		buffer_ = NULL; // allow for exception in allocation below
		buffer_size_ = 0;
		offset_ = 0;
		
		int const count_clocks_extra = 2;
		buffer_ = BLARGG_NEW buf_t_ [new_size + widest_impulse_ + count_clocks_extra];
		BLARGG_CHECK_ALLOC( buffer_ );
	}
	
	buffer_size_ = new_size;
	length_ = new_size * 1000 / new_rate - 1;
	if ( msec )
		assert( length_ == msec ); // ensure length is same as that passed in
	
	samples_per_sec = new_rate;
	if ( clocks_per_sec )
		clock_rate( clocks_per_sec ); // recalculate factor
	
	bass_freq( bass_freq_ ); // recalculate shift
	
	clear();
	
	return blargg_success;
}

blip_resampled_time_t Blip_Buffer::clock_rate_factor( long clock_rate ) const
{
	blip_resampled_time_t factor = (unsigned long) floor(
			(double) samples_per_sec / clock_rate * (1L << BLIP_BUFFER_ACCURACY) + 0.5 );
	require( factor > 0 ); // clock_rate/sample_rate ratio is too large
	return factor;
}

Blip_Buffer::~Blip_Buffer()
{
	delete [] buffer_;
}

void Blip_Buffer::bass_freq( int freq )
{
	bass_freq_ = freq;
	if ( freq == 0 )
	{
		bass_shift = 31; // 32 or greater invokes undefined behavior elsewhere
		return;
	}
	bass_shift = 1 + (int) floor( 1.442695041 * log( 0.124 * samples_per_sec / freq ) );
	if ( bass_shift < 0 )
		bass_shift = 0;
	if ( bass_shift > 24 )
		bass_shift = 24;
}

long Blip_Buffer::count_samples( blip_time_t t ) const
{
	return (resampled_time( t ) >> BLIP_BUFFER_ACCURACY) - (offset_ >> BLIP_BUFFER_ACCURACY);
}

blip_time_t Blip_Buffer::count_clocks( long count ) const
{
	if ( count > buffer_size_ )
		count = buffer_size_;
	
	return ((count << BLIP_BUFFER_ACCURACY) - offset_ + (factor_ - 1)) / factor_;
}

void Blip_Impulse_::init( blip_pair_t_* imps, int w, int r, int fb )
{
	fine_bits = fb;
	width = w;
	impulses = (imp_t*) imps;
	generate = true;
	volume_unit_ = -1.0;
	res = r;
	buf = NULL;
	
	impulse = &impulses [width * res * 2 * (fine_bits ? 2 : 1)];
	offset = 0;
}

const int impulse_bits = 15;
const long impulse_amp = 1L << impulse_bits;
const long impulse_offset = impulse_amp / 2;

void Blip_Impulse_::scale_impulse( int unit, imp_t* imp_in ) const
{
	long offset = ((long) unit << impulse_bits) - impulse_offset * unit +
			(1 << (impulse_bits - 1));
	imp_t* imp = imp_in;
	imp_t* fimp = impulse;
	for ( int n = res / 2 + 1; n--; )
	{
		int error = unit;
		for ( int nn = width; nn--; )
		{
			long a = ((long) *fimp++ * unit + offset) >> impulse_bits;
			error -= a - unit;
			*imp++ = (imp_t) a;
		}
		
		// add error to middle
		imp [-width / 2 - 1] += (imp_t) error;
	}
	
	if ( res > 2 )
	{
		// second half is mirror-image
		const imp_t* rev = imp - width - 1;
		for ( int nn = (res / 2 - 1) * width - 1; nn--; )
			*imp++ = *--rev;
		*imp++ = (imp_t) unit;
	}
	
	// copy to odd offset
	*imp++ = (imp_t) unit;
	memcpy( imp, imp_in, (res * width - 1) * sizeof *imp );
	
	/*
	for ( int i = 0; i < res; i++ )
	{
		for ( int j = 0; j < width; j++ )
			printf( "%6d,", imp_in [i * width + j] - 0x8000 );
		printf( "\n" );
	}*/
}

const int max_res = 1 << blip_res_bits_;

void Blip_Impulse_::fine_volume_unit()
{
	// to do: find way of merging in-place without temporary buffer
	
	imp_t temp [max_res * 2 * Blip_Buffer::widest_impulse_];
	scale_impulse( (offset & 0xffff) << fine_bits, temp );
	imp_t* imp2 = impulses + res * 2 * width;
	scale_impulse( offset & 0xffff, imp2 );
	
	// merge impulses
	imp_t* imp = impulses;
	imp_t* src2 = temp;
	for ( int n = res / 2 * 2 * width; n--; )
	{
		*imp++ = *imp2++;
		*imp++ = *imp2++;
		*imp++ = *src2++;
		*imp++ = *src2++;
	}
}

void Blip_Impulse_::volume_unit( double new_unit )
{
	if ( new_unit == volume_unit_ )
		return;
	
	if ( generate )
		treble_eq( blip_eq_t( -8.87, 8800, 44100 ) );
	
	volume_unit_ = new_unit;
	
	offset = 0x10001 * (unsigned long) floor( volume_unit_ * 0x10000 + 0.5 );
	
	if ( fine_bits )
		fine_volume_unit();
	else
		scale_impulse( offset & 0xffff, impulses );
}

static const double pi = 3.1415926535897932384626433832795029L;

void Blip_Impulse_::treble_eq( const blip_eq_t& new_eq )
{
	if ( !generate && new_eq.treble == eq.treble && new_eq.cutoff == eq.cutoff &&
			new_eq.sample_rate == eq.sample_rate )
		return; // already calculated with same parameters
	
	generate = false;
	eq = new_eq;
	
	double treble = pow( 10.0, 1.0 / 20 * eq.treble ); // dB (-6dB = 0.50)
	if ( treble < 0.000005 )
		treble = 0.000005;
	
	const double treble_freq = 22050.0; // treble level at 22 kHz harmonic
	const double sample_rate = eq.sample_rate;
	const double pt = treble_freq * 2 / sample_rate;
	double cutoff = eq.cutoff * 2 / sample_rate;
	if ( cutoff >= pt * 0.95 || cutoff >= 0.95 )
	{
		cutoff = 0.5;
		treble = 1.0;
	}
	
	// DSF Synthesis (See T. Stilson & J. Smith (1996),
	// Alias-free digital synthesis of classic analog waveforms)
	
	// reduce adjacent impulse interference by using small part of wide impulse
	const double n_harm = 4096;
	const double rolloff = pow( treble, 1.0 / (n_harm * pt - n_harm * cutoff) );
	const double rescale = 1.0 / pow( rolloff, n_harm * cutoff );
	
	const double pow_a_n = rescale * pow( rolloff, n_harm );
	const double pow_a_nc = rescale * pow( rolloff, n_harm * cutoff );
	
	double total = 0.0;
	const double to_angle = pi / 2 / n_harm / max_res;
	
	float buf [max_res * (Blip_Buffer::widest_impulse_ - 2) / 2];
	const int size = max_res * (width - 2) / 2;
	for ( int i = size; i--; )
	{
		double angle = (i * 2 + 1) * to_angle;
		
		// equivalent
		//double y =     dsf( angle, n_harm * cutoff, 1.0 );
		//y -= rescale * dsf( angle, n_harm * cutoff, rolloff );
		//y += rescale * dsf( angle, n_harm,          rolloff );
		
		const double cos_angle = cos( angle );
		const double cos_nc_angle = cos( n_harm * cutoff * angle );
		const double cos_nc1_angle = cos( (n_harm * cutoff - 1.0) * angle );
		
		double b = 2.0 - 2.0 * cos_angle;
		double a = 1.0 - cos_angle - cos_nc_angle + cos_nc1_angle;
		
		double d = 1.0 + rolloff * (rolloff - 2.0 * cos_angle);
		double c = pow_a_n * rolloff * cos( (n_harm - 1.0) * angle ) -
				pow_a_n * cos( n_harm * angle ) -
				pow_a_nc * rolloff * cos_nc1_angle +
				pow_a_nc * cos_nc_angle;
		
		// optimization of a / b + c / d
		double y = (a * d + c * b) / (b * d);
		
		// fixed window which affects wider impulses more
		if ( width > 12 )
		{
			double window = cos( n_harm / 1.25 / Blip_Buffer::widest_impulse_ * angle );
			y *= window * window;
		}
		
		total += (float) y;
		buf [i] = (float) y;
	}
	
	// integrate runs of length 'max_res'
	double factor = impulse_amp * 0.5 / total; // 0.5 accounts for other mirrored half
	imp_t* imp = impulse;
	const int step = max_res / res;
	int offset = res > 1 ? max_res : max_res / 2;
	for ( int n = res / 2 + 1; n--; offset -= step )
	{
		for ( int w = -width / 2; w < width / 2; w++ )
		{
			double sum = 0;
			for ( int i = max_res; i--; )
			{
				int index = w * max_res + offset + i;
				if ( index < 0 )
					index = -index - 1;
				if ( index < size )
					sum += buf [index];
			}
			*imp++ = (imp_t) floor( sum * factor + (impulse_offset + 0.5) );
		}
	}
	
	// rescale
	double unit = volume_unit_;
	if ( unit >= 0 )
	{
		volume_unit_ = -1;
		volume_unit( unit );
	}
}

void Blip_Buffer::remove_samples( long count )
{
	require( buffer_ ); // sample rate must have been set
	
	if ( !count ) // optimization
		return;
	
	remove_silence( count );
	
	// Allows synthesis slightly past time passed to end_frame(), as long as it's
	// not more than an output sample.
	// to do: kind of hacky, could add run_until() which keeps track of extra synthesis
	int const copy_extra = 1;
	
	// copy remaining samples to beginning and clear old samples
	long remain = samples_avail() + widest_impulse_ + copy_extra;
	if ( count >= remain )
		memmove( buffer_, buffer_ + count, remain * sizeof (buf_t_) );
	else
		memcpy(  buffer_, buffer_ + count, remain * sizeof (buf_t_) );
	memset( buffer_ + remain, sample_offset_ & 0xFF, count * sizeof (buf_t_) );
}

#include BLARGG_ENABLE_OPTIMIZER

long Blip_Buffer::read_samples( blip_sample_t* out, long max_samples, bool stereo )
{
	require( buffer_ ); // sample rate must have been set
	
	long count = samples_avail();
	if ( count > max_samples )
		count = max_samples;
	
	if ( !count )
		return 0; // optimization
	
	int sample_offset_ = this->sample_offset_;
	int bass_shift = this->bass_shift;
	buf_t_* buf = buffer_;
	long accum = reader_accum;
	
	if ( !stereo )
	{
		for ( long n = count; n--; )
		{
			long s = accum >> accum_fract;
			accum -= accum >> bass_shift;
			accum += (long (*buf++) - sample_offset_) << accum_fract;
			*out++ = (blip_sample_t) s;
			
			// clamp sample
			if ( (BOOST::int16_t) s != s )
				out [-1] = blip_sample_t (0x7FFF - (s >> 24));
		}
	}
	else
	{
		for ( long n = count; n--; )
		{
			long s = accum >> accum_fract;
			accum -= accum >> bass_shift;
			accum += (long (*buf++) - sample_offset_) << accum_fract;
			*out = (blip_sample_t) s;
			out += 2;
			
			// clamp sample
			if ( (BOOST::int16_t) s != s )
				out [-2] = blip_sample_t (0x7FFF - (s >> 24));
		}
	}
	
	reader_accum = accum;
	
	remove_samples( count );
	
	return count;
}

void Blip_Buffer::mix_samples( const blip_sample_t* in, long count )
{
	buf_t_* buf = &buffer_ [(offset_ >> BLIP_BUFFER_ACCURACY) + (widest_impulse_ / 2 - 1)];
	
	int prev = 0;
	while ( count-- )
	{
		int s = *in++;
		*buf += s - prev;
		prev = s;
		++buf;
	}
	*buf -= *--in;
}

