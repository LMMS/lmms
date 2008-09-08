
// Gb_Snd_Emu 0.1.4. http://www.slack.net/~ant/libs/

#include "Basic_Gb_Apu.h"

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

gb_time_t const frame_length = 70224;

Basic_Gb_Apu::Basic_Gb_Apu()
{
	time = 0;
	
	// Adjust frequency equalization to make it sound like a tiny speaker
	apu.treble_eq( -20.0 ); // lower values muffle it more
	buf.bass_freq( 461 ); // higher values simulate smaller speaker
}

Basic_Gb_Apu::~Basic_Gb_Apu()
{
}

blargg_err_t Basic_Gb_Apu::set_sample_rate( long rate )
{
	apu.output( buf.center(), buf.left(), buf.right() );
	buf.clock_rate( 4194304 );
	return buf.set_sample_rate( rate );
}

void Basic_Gb_Apu::write_register( gb_addr_t addr, int data )
{
	apu.write_register( clock(), addr, data );
}

int Basic_Gb_Apu::read_register( gb_addr_t addr )
{
	return apu.read_register( clock(), addr );
}

void Basic_Gb_Apu::end_frame()
{
	time = 0;
	bool stereo = apu.end_frame( frame_length );
	buf.end_frame( frame_length, stereo );
}

long Basic_Gb_Apu::samples_avail() const
{
	return buf.samples_avail();
}

long Basic_Gb_Apu::read_samples( sample_t* out, long count )
{
	return buf.read_samples( out, count );
}

//added by 589 --->

void Basic_Gb_Apu::reset()
{
	apu.reset();
}

void Basic_Gb_Apu::treble_eq( const blip_eq_t& eq )
{
	apu.treble_eq( eq );
}

void Basic_Gb_Apu::bass_freq( int bf )
{
	buf.bass_freq( bf );
}

// <---
