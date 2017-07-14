// Nintendo Game Boy PAPU sound chip emulator

// Gb_Snd_Emu 0.1.5
#ifndef GB_APU_H
#define GB_APU_H

#include "Gb_Oscs.h"

class Gb_Apu {
public:
	
	// Set overall volume of all oscillators, where 1.0 is full volume
	void volume( double );
	
	// Set treble equalization
	void treble_eq( const blip_eq_t& );
	
	// Outputs can be assigned to a single buffer for mono output, or to three
	// buffers for stereo output (using Stereo_Buffer to do the mixing).
	
	// Assign all oscillator outputs to specified buffer(s). If buffer
	// is NULL, silences all oscillators.
	void output( Blip_Buffer* mono );
	void output( Blip_Buffer* center, Blip_Buffer* left, Blip_Buffer* right );
	
	// Assign single oscillator output to buffer(s). Valid indicies are 0 to 3,
	// which refer to Square 1, Square 2, Wave, and Noise. If buffer is NULL,
	// silences oscillator.
	enum { osc_count = 4 };
	void osc_output( int index, Blip_Buffer* mono );
	void osc_output( int index, Blip_Buffer* center, Blip_Buffer* left, Blip_Buffer* right );
	
	// Reset oscillators and internal state
	void reset();
	
	// Reads and writes at addr must satisfy start_addr <= addr <= end_addr
	enum { start_addr = 0xFF10 };
	enum { end_addr   = 0xFF3F };
	enum { register_count = end_addr - start_addr + 1 };
	
	// Write 'data' to address at specified time
	void write_register( blip_time_t, unsigned addr, int data );
	
	// Read from address at specified time
	int read_register( blip_time_t, unsigned addr );
	
	// Run all oscillators up to specified time, end current time frame, then
	// start a new frame at time 0.
	void end_frame( blip_time_t );
	
	void set_tempo( double );
	
public:
	Gb_Apu();
private:
	// noncopyable
	Gb_Apu( const Gb_Apu& );
	Gb_Apu& operator = ( const Gb_Apu& );
	
	Gb_Osc*     oscs [osc_count];
	blip_time_t   next_frame_time;
	blip_time_t   last_time;
	blip_time_t frame_period;
	double      volume_unit;
	int         frame_count;
	
	Gb_Square   square1;
	Gb_Square   square2;
	Gb_Wave     wave;
	Gb_Noise    noise;
	BOOST::uint8_t regs [register_count];
	Gb_Square::Synth square_synth; // used by squares
	Gb_Wave::Synth   other_synth;  // used by wave and noise
	
	void update_volume();
	void run_until( blip_time_t );
	void write_osc( int index, int reg, int data );
};

inline void Gb_Apu::output( Blip_Buffer* b ) { output( b, b, b ); }
	
inline void Gb_Apu::osc_output( int i, Blip_Buffer* b ) { osc_output( i, b, b, b ); }

inline void Gb_Apu::volume( double vol )
{
	volume_unit = 0.60 / osc_count / 15 /*steps*/ / 2 /*?*/ / 8 /*master vol range*/ * vol;
	update_volume();
}

#endif
