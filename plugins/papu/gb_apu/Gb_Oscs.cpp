
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

const int trigger = 0x80;

// Gb_Osc

Gb_Osc::Gb_Osc()
{
	output = NULL;
	outputs [0] = NULL;
	outputs [1] = NULL;
	outputs [2] = NULL;
	outputs [3] = NULL;
}

void Gb_Osc::reset()
{
	delay = 0;
	last_amp = 0;
	period = 2048;
	volume = 0;
	global_volume = 7; // added
	frequency = 0;
	length = 0;
	enabled = false;
	length_enabled = false;
	output_select = 3;
	output = outputs [output_select];
}

void Gb_Osc::clock_length()
{
	if ( length_enabled && length )
		--length;
}

void Gb_Osc::write_register( int reg, int value )
{
	if ( reg == 4 )
		length_enabled = value & 0x40;
}

// Gb_Env

void Gb_Env::reset()
{
	env_period = 0;
	env_dir = 0;
	env_delay = 0;
	new_volume = 0;
	Gb_Osc::reset();
}

Gb_Env::Gb_Env()
{
}

void Gb_Env::clock_envelope()
{
	if ( env_delay && !--env_delay )
	{
		env_delay = env_period;
		if ( env_dir )
		{
			if ( volume < 15 )
				++volume;
		}
		else if ( volume > 0 )
		{
			--volume;
		}
	}
}

void Gb_Env::write_register( int reg, int value )
{
	if ( reg == 2 ) {
		env_period = value & 7;
		env_dir = value & 8;
		volume = new_volume = value >> 4;
	}
	else if ( reg == 4 && (value & trigger) ) {
		env_delay = env_period;
		volume = new_volume;
		enabled = true;
	}
	Gb_Osc::write_register( reg, value );
}

// Gb_Square

void Gb_Square::reset()
{
	phase = 1;
	duty = 1;
	
	sweep_period = 0;
	sweep_delay = 0;
	sweep_shift = 0;
	sweep_dir = 0;
	sweep_freq = 0;
	
	new_length = 0;
	
	Gb_Env::reset();
}

Gb_Square::Gb_Square()
{
	has_sweep = false;
}

void Gb_Square::clock_sweep()
{
	if ( sweep_period && sweep_delay && !--sweep_delay )
	{
		sweep_delay = sweep_period;
		frequency = sweep_freq;
		
		period = (2048 - frequency) * 4;
		
		int offset = sweep_freq >> sweep_shift;
		if ( sweep_dir )
			offset = -offset;
		sweep_freq += offset;
		
		if ( sweep_freq < 0 )
		{
			sweep_freq = 0;
		}
		else if ( sweep_freq >= 2048 )
		{
			sweep_delay = 0;
			sweep_freq = 2048; // stop sound output
		}
	}
}

void Gb_Square::write_register( int reg, int value )
{
	static unsigned char const duty_table [4] = { 1, 2, 4, 6 };
	
	switch ( reg )
	{
	case 0:
		sweep_period = (value >> 4) & 7; // changed
		sweep_shift = value & 7;
		sweep_dir = value & 0x08;
		break;
	
	case 1:
		new_length = length = 64 - (value & 0x3f);
		duty = duty_table [value >> 6];
		break;
	
	case 3:
		frequency = (frequency & ~0xFF) + value;
		length = new_length;
		break;
	
	case 4:
		frequency = (value & 7) * 0x100 + (frequency & 0xFF);
		length = new_length;
		if ( value & trigger )
		{
			sweep_freq = frequency;
			if ( has_sweep && sweep_period && sweep_shift )
			{
				sweep_delay = 1;
				clock_sweep();
			}
		}
		break;
	}
	
	period = (2048 - frequency) * 4;
	
	Gb_Env::write_register( reg, value );
}

void Gb_Square::run( gb_time_t time, gb_time_t end_time )
{
	// to do: when frequency goes above 20000 Hz output should actually be 1/2 volume
	// rather than 0
	
	if ( !enabled || (!length && length_enabled) || !volume || sweep_freq == 2048 ||
			!frequency || period < 27 )
	{
		if ( last_amp )
		{
			synth->offset( time, -last_amp, output );
			last_amp = 0;
		}
		delay = 0;
	}
	else
	{
		int amp = (phase < duty) ? volume : -volume;
		amp *= global_volume;
		if ( amp != last_amp )
		{
			synth->offset( time, amp - last_amp, output );
			last_amp = amp;
		}
		
		time += delay;
		if ( time < end_time )
		{
			Blip_Buffer* const output = this->output;
			const int duty = this->duty;
			int phase = this->phase;
			amp *= 2;
			do
			{
				phase = (phase + 1) & 7;
				if ( phase == 0 || phase == duty )
				{
					amp = -amp;
					synth->offset_inline( time, amp, output );
				}
				time += period;
			}
			while ( time < end_time );
			
			this->phase = phase;
			last_amp = amp >> 1;
		}
		delay = time - end_time;
	}
}


// Gb_Wave

void Gb_Wave::reset()
{
	volume_shift = 0;
	wave_pos = 0;
	new_length = 0;
	memset( wave, 0, sizeof wave );
	Gb_Osc::reset();
}

Gb_Wave::Gb_Wave() {
}

void Gb_Wave::write_register( int reg, int value )
{
	switch ( reg )
	{
	case 0:
		new_enabled = value & 0x80;
		enabled &= new_enabled;
		break;
	
	case 1:
		new_length = length = 256 - value;
		break;
	
	case 2:
		volume = ((value >> 5) & 3);
		volume_shift = (volume - 1) & 7; // silence = 7
		break;
	
	case 3:
		frequency = (frequency & ~0xFF) + value;
		break;
	
	case 4:
		frequency = (value & 7) * 0x100 + (frequency & 0xFF);
		if ( new_enabled && (value & trigger) )
		{
			wave_pos = 0;
			length = new_length;
			enabled = true;
		}
		break;
	}
	
	period = (2048 - frequency) * 2;
	
	Gb_Osc::write_register( reg, value );
}

void Gb_Wave::run( gb_time_t time, gb_time_t end_time )
{
	// to do: when frequency goes above 20000 Hz output should actually be 1/2 volume
	// rather than 0
	if ( !enabled || (!length && length_enabled) || !volume || !frequency || period < 7 )
	{
		if ( last_amp ) {
			synth->offset( time, -last_amp, output );
			last_amp = 0;
		}
		delay = 0;
	}
	else
	{
		int const vol_factor = global_volume * 2;
		
		// wave data or shift may have changed
		int diff = (wave [wave_pos] >> volume_shift) * vol_factor - last_amp;
		if ( diff )
		{
			last_amp += diff;
			synth->offset( time, diff, output );
		}
		
		time += delay;
		if ( time < end_time )
		{
			int const volume_shift = this->volume_shift;
		 	int wave_pos = this->wave_pos;
		 	
			do
			{
				wave_pos = unsigned (wave_pos + 1) % wave_size;
				int amp = (wave [wave_pos] >> volume_shift) * vol_factor;
				int delta = amp - last_amp;
				if ( delta )
				{
					last_amp = amp;
					synth->offset_inline( time, delta, output );
				}
				time += period;
			}
			while ( time < end_time );
			
			this->wave_pos = wave_pos;
		}
		delay = time - end_time;
	}
}


// Gb_Noise

void Gb_Noise::reset()
{
	bits = 1;
	tap = 14;
	Gb_Env::reset();
}

Gb_Noise::Gb_Noise() {
}

void Gb_Noise::write_register( int reg, int value )
{
	if ( reg == 1 ) {
		new_length = length = 64 - (value & 0x3f);
	}
	else if ( reg == 2 ) {
		// based on VBA code, noise is the only exception to the envelope code
		// while the volume level here is applied when the channel is enabled,
		// current volume is only affected by writes to this register if volume
		// is zero and direction is up... (definitely needs verification)
		int temp = volume;
		Gb_Env::write_register( reg, value );
		if ( ( value & 0xF8 ) != 0 ) volume = temp;
		return;
	}
	else if ( reg == 3 ) {
		tap = 14 - (value & 8);
		// noise formula and frequency tested against Metroid 2 and Zelda LA
		int divisor = (value & 7) * 16;
		if ( !divisor )
			divisor = 8;
		period = divisor << (value >> 4);
	}
	else if ( reg == 4 && value & trigger ) {
		bits = ~0u;
		length = new_length;
	}
	
	Gb_Env::write_register( reg, value );
}

#include BLARGG_ENABLE_OPTIMIZER

void Gb_Noise::run( gb_time_t time, gb_time_t end_time )
{
	if ( !enabled || (!length && length_enabled) || !volume ) {
		if ( last_amp ) {
			synth->offset( time, -last_amp, output );
			last_amp = 0;
		}
		delay = 0;
	}
	else
	{
		int amp = bits & 1 ? -volume : volume;
		amp *= global_volume;
		if ( amp != last_amp ) {
			synth->offset( time, amp - last_amp, output );
			last_amp = amp;
		}
		
		time += delay;
		if ( time < end_time )
		{
			Blip_Buffer* const output = this->output;
			// keep parallel resampled time to eliminate multiplication in the loop
			const blip_resampled_time_t resampled_period =
					output->resampled_duration( period );
			blip_resampled_time_t resampled_time = output->resampled_time( time );
			const unsigned mask = ~(1u << tap);
			unsigned bits = this->bits;
			amp *= 2;
			
			do {
				unsigned feedback = bits;
				bits >>= 1;
				feedback = 1 & (feedback ^ bits);
				time += period;
				bits = (feedback << tap) | (bits & mask);
				// feedback just happens to be true only when the level needs to change
				// (the previous and current bits are different)
				if ( feedback ) {
					amp = -amp;
					synth->offset_resampled( resampled_time, amp, output );
				}
				resampled_time += resampled_period;
			}
			while ( time < end_time );
			
			this->bits = bits;
			last_amp = amp >> 1;
		}
		delay = time - end_time;
	}
}

