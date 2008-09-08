
// Simplified Nintendo Game Boy PAPU sound chip emulator

// Gb_Snd_Emu 0.1.4. Copyright (C) 2003-2005 Shay Green. GNU LGPL license.

#ifndef BASIC_GB_APU_H
#define BASIC_GB_APU_H

#include "gb_apu/Gb_Apu.h"
#include "gb_apu/Multi_Buffer.h"

class Basic_Gb_Apu {
public:
	Basic_Gb_Apu();
	~Basic_Gb_Apu();
	
	// Set output sample rate
	blargg_err_t set_sample_rate( long rate );
	
	// Pass reads and writes in the range 0xff10-0xff3f
	void write_register( gb_addr_t, int data );
	int read_register( gb_addr_t );
	
	// End a 1/60 sound frame and add samples to buffer
	void end_frame();
	
	// Samples are generated in stereo, left first. Sample counts are always
	// a multiple of 2.
	
	// Number of samples in buffer
	long samples_avail() const;
	
	// Read at most 'count' samples out of buffer and return number actually read
	typedef blip_sample_t sample_t;
	long read_samples( sample_t* out, long count );

	//added by 589 --->
	void reset();
	void treble_eq( const blip_eq_t& eq );
	void bass_freq( int bf );
	//<---
	
private:
	Gb_Apu apu;
	Stereo_Buffer buf;
	blip_time_t time;
	
	// faked CPU timing
	blip_time_t clock() { return time += 4; }
};

#endif

