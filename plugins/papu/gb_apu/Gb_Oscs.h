
// Private oscillators used by Gb_Apu

// Gb_Snd_Emu 0.1.4. Copyright (C) 2003-2005 Shay Green. GNU LGPL license.

#ifndef GB_OSCS_H
#define GB_OSCS_H

#include "Blip_Buffer.h"

enum { gb_apu_max_vol = 7 };

struct Gb_Osc {
	Blip_Buffer* outputs [4]; // NULL, right, left, center
	Blip_Buffer* output;
	int output_select;
	
	int delay;
	int last_amp;
	int period;
	int volume;
	int global_volume;
	int frequency;
	int length;
	int new_length;
	bool enabled;
	bool length_enabled;
	
	Gb_Osc();
	
	void clock_length();
	void reset();
	virtual void run( gb_time_t begin, gb_time_t end ) = 0;
	virtual void write_register( int reg, int value );
};

struct Gb_Env : Gb_Osc {
	int env_period;
	int env_dir;
	int env_delay;
	int new_volume;
	
	Gb_Env();
	void reset();
	void clock_envelope();
	void write_register( int, int );
};

struct Gb_Square : Gb_Env {
	int phase;
	int duty;
	
	int sweep_period;
	int sweep_delay;
	int sweep_shift;
	int sweep_dir;
	int sweep_freq;
	bool has_sweep;
	
	typedef Blip_Synth<blip_good_quality,15 * gb_apu_max_vol * 2> Synth;
	const Synth* synth;
	
	Gb_Square();
	void reset();
	void run( gb_time_t, gb_time_t );
	void write_register( int, int );
	void clock_sweep();
};

struct Gb_Wave : Gb_Osc {
	int volume_shift;
	unsigned wave_pos;
	enum { wave_size = 32 };
	bool new_enabled;
	BOOST::uint8_t wave [wave_size];
	
	typedef Blip_Synth<blip_med_quality,15 * gb_apu_max_vol * 2> Synth;
	const Synth* synth;
	
	Gb_Wave();
	void reset();
	void run( gb_time_t, gb_time_t );
	void write_register( int, int );
};

struct Gb_Noise : Gb_Env {
	unsigned bits;
	int tap;
	
	typedef Blip_Synth<blip_med_quality,15 * gb_apu_max_vol * 2> Synth;
	const Synth* synth;
	
	Gb_Noise();
	void reset();
	void run( gb_time_t, gb_time_t );
	void write_register( int, int );
};

#endif

