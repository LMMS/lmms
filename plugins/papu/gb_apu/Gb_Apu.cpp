
// Gb_Snd_Emu 0.1.4. http://www.slack.net/~ant/libs/

#include "Gb_Apu.h"

#include <string.h>

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

Gb_Apu::Gb_Apu()
{
	square1.synth = &square_synth;
	square2.synth = &square_synth;
	square1.has_sweep = true;
	wave.synth  = &other_synth;
	noise.synth = &other_synth;
	
	oscs [0] = &square1;
	oscs [1] = &square2;
	oscs [2] = &wave;
	oscs [3] = &noise;
	
	volume( 1.0 );
	reset();
}

Gb_Apu::~Gb_Apu()
{
}

void Gb_Apu::treble_eq( const blip_eq_t& eq )
{
	square_synth.treble_eq( eq );
	other_synth.treble_eq( eq );
}

void Gb_Apu::volume( double vol )
{
	vol *= 0.60 / osc_count;
	square_synth.volume( vol );
	other_synth.volume( vol );
}

void Gb_Apu::output( Blip_Buffer* center, Blip_Buffer* left, Blip_Buffer* right )
{
	for ( int i = 0; i < osc_count; i++ )
		osc_output( i, center, left, right );
}

void Gb_Apu::reset()
{
	next_frame_time = 0;
	last_time = 0;
	frame_count = 0;
	stereo_found = false;
	
	square1.reset();
	square2.reset();
	wave.reset();
	noise.reset();
	
	memset( regs, 0, sizeof regs );
}

void Gb_Apu::osc_output( int index, Blip_Buffer* center, Blip_Buffer* left, Blip_Buffer* right )
{
	require( (unsigned) index < osc_count );
	
	Gb_Osc& osc = *oscs [index];
	if ( center && !left && !right )
	{
		// mono
		left = center;
		right = center;
	}
	else
	{
		// must be silenced or stereo
		require( (!left && !right) || (left && right) );
	}
	osc.outputs [1] = right;
	osc.outputs [2] = left;
	osc.outputs [3] = center;
	osc.output = osc.outputs [osc.output_select];
}

void Gb_Apu::run_until( gb_time_t end_time )
{
	require( end_time >= last_time ); // end_time must not be before previous time
	if ( end_time == last_time )
		return;
	
	while ( true )
	{
		gb_time_t time = next_frame_time;
		if ( time > end_time )
			time = end_time;
		
		// run oscillators
		for ( int i = 0; i < osc_count; ++i ) {
			Gb_Osc& osc = *oscs [i];
			if ( osc.output ) {
				if ( osc.output != osc.outputs [3] )
					stereo_found = true;
				osc.run( last_time, time );
			}
		}
		last_time = time;
		
		if ( time == end_time )
			break;
		
		next_frame_time += 4194304 / 256; // 256 Hz
		
		// 256 Hz actions
		square1.clock_length();
		square2.clock_length();
		wave.clock_length();
		noise.clock_length();
		
		frame_count = (frame_count + 1) & 3;
		if ( frame_count == 0 ) {
			// 64 Hz actions
			square1.clock_envelope();
			square2.clock_envelope();
			noise.clock_envelope();
		}
		
		if ( frame_count & 1 )
			square1.clock_sweep(); // 128 Hz action
	}
}

bool Gb_Apu::end_frame( gb_time_t end_time )
{
	if ( end_time > last_time )
		run_until( end_time );
	
	assert( next_frame_time >= end_time );
	next_frame_time -= end_time;
	
	assert( last_time >= end_time );
	last_time -= end_time;
	
	bool result = stereo_found;
	stereo_found = false;
	return result;
}

void Gb_Apu::write_register( gb_time_t time, gb_addr_t addr, int data )
{
	require( (unsigned) data < 0x100 );
	
	int reg = addr - start_addr;
	if ( (unsigned) reg >= register_count )
		return;
	
	run_until( time );
	
	regs [reg] = data;
	
	if ( addr < 0xff24 )
	{
		// oscillator
		int index = reg / 5;
		oscs [index]->write_register( reg - index * 5, data ); 
	}
	// added
	else if ( addr == 0xff24 )
	{
		int global_volume = data & 7;
		int old_volume = square1.global_volume;
		if ( old_volume != global_volume )
		{
			int any_enabled = false;
			for ( int i = 0; i < osc_count; i++ )
			{
				Gb_Osc& osc = *oscs [i];
				if ( osc.enabled )
				{
					if ( osc.last_amp )
					{
						int new_amp = osc.last_amp * global_volume / osc.global_volume;
						if ( osc.output )
							square_synth.offset( time, new_amp - osc.last_amp, osc.output );
						osc.last_amp = new_amp;
					}
					any_enabled |= osc.volume;
				}
				osc.global_volume = global_volume;
			}
			
			if ( !any_enabled && square1.outputs [3] )
				square_synth.offset( time, (global_volume - old_volume) * 15 * 2, square1.outputs [3] );
		}
	}
	
	else if ( addr == 0xff25 || addr == 0xff26 )
	{
		int mask = (regs [0xff26 - start_addr] & 0x80) ? ~0 : 0;
		int flags = regs [0xff25 - start_addr] & mask;
		
		// left/right assignments
		for ( int i = 0; i < osc_count; i++ )
		{
			Gb_Osc& osc = *oscs [i];
			osc.enabled &= mask;
			int bits = flags >> i;
			Blip_Buffer* old_output = osc.output;
			osc.output_select = (bits >> 3 & 2) | (bits & 1);
			osc.output = osc.outputs [osc.output_select];
			if ( osc.output != old_output && osc.last_amp )
			{
				if ( old_output )
					square_synth.offset( time, -osc.last_amp, old_output );
				osc.last_amp = 0;
			}
		}
	}
	else if ( addr >= 0xff30 )
	{
		int index = (addr & 0x0f) * 2;
		wave.wave [index] = data >> 4;
		wave.wave [index + 1] = data & 0x0f;
	}
}

int Gb_Apu::read_register( gb_time_t time, gb_addr_t addr )
{
	// function now takes actual address, i.e. 0xFFXX
	require( start_addr <= addr && addr <= end_addr );
	
	run_until( time );
	
	int data = regs [addr - start_addr];
	
	if ( addr == 0xff26 )
	{
		data &= 0xf0;
		for ( int i = 0; i < osc_count; i++ )
		{
			const Gb_Osc& osc = *oscs [i];
			if ( osc.enabled && (osc.length || !osc.length_enabled) )
				data |= 1 << i;
		}
	}
	
	return data;
}

