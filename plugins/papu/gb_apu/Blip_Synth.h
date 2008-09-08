
// Blip_Synth and Blip_Wave are waveform transition synthesizers for adding
// waveforms to a Blip_Buffer.

// Blip_Buffer 0.3.4. Copyright (C) 2003-2005 Shay Green. GNU LGPL license.

#ifndef BLIP_SYNTH_H
#define BLIP_SYNTH_H

#ifndef BLIP_BUFFER_H
	#include "Blip_Buffer.h"
#endif

// Quality level. Higher levels are slower, and worse in a few cases.
// Use blip_good_quality as a starting point.
const int blip_low_quality = 1;
const int blip_med_quality = 2;
const int blip_good_quality = 3;
const int blip_high_quality = 4;

// Blip_Synth is a transition waveform synthesizer which adds band-limited
// offsets (transitions) into a Blip_Buffer. For a simpler interface, use
// Blip_Wave (below).
//
// Range specifies the greatest expected offset that will occur. For a
// waveform that goes between +amp and -amp, range should be amp * 2 (half
// that if it only goes between +amp and 0). When range is large, a higher
// accuracy scheme is used; to force this even when range is small, pass
// the negative of range (i.e. -range).
template<int quality,int range>
class Blip_Synth {
	BOOST_STATIC_ASSERT( 1 <= quality && quality <= 5 );
	BOOST_STATIC_ASSERT( -32768 <= range && range <= 32767 );
	enum {
		abs_range = (range < 0) ? -range : range,
		fine_mode = (range > 512 || range < 0),
		width = (quality < 5 ? quality * 4 : Blip_Buffer::widest_impulse_),
		res = 1 << blip_res_bits_,
		impulse_size = width / 2 * (fine_mode + 1),
		base_impulses_size = width / 2 * (res / 2 + 1),
		fine_bits = (fine_mode ? (abs_range <= 64 ? 2 : abs_range <= 128 ? 3 :
			abs_range <= 256 ? 4 : abs_range <= 512 ? 5 : abs_range <= 1024 ? 6 :
			abs_range <= 2048 ? 7 : 8) : 0)
	};
	blip_pair_t_  impulses [impulse_size * res * 2 + base_impulses_size];
	Blip_Impulse_ impulse;
	void init() { impulse.init( impulses, width, res, fine_bits ); }
public:
	Blip_Synth()                            { init(); }
	Blip_Synth( double volume )             { init(); this->volume( volume ); }
	
	// Configure low-pass filter (see notes.txt). Not optimized for real-time control
	void treble_eq( const blip_eq_t& eq )   { impulse.treble_eq( eq ); }
	
	// Set volume of a transition at amplitude 'range' by setting volume_unit
	// to v / range
	void volume( double v )                 { impulse.volume_unit( v * (1.0 / abs_range) ); }
	
	// Set base volume unit of transitions, where 1.0 is a full swing between the
	// positive and negative extremes. Not optimized for real-time control.
	void volume_unit( double unit )         { impulse.volume_unit( unit ); }
	
	// Default Blip_Buffer used for output when none is specified for a given call
	Blip_Buffer* output() const             { return impulse.buf; }
	void output( Blip_Buffer* b )           { impulse.buf = b; }
	
	// Add an amplitude offset (transition) with a magnitude of delta * volume_unit
	// into the specified buffer (default buffer if none specified) at the
	// specified source time. Delta can be positive or negative. To increase
	// performance by inlining code at the call site, use offset_inline().
	void offset( blip_time_t, int delta, Blip_Buffer* ) const;
	
	void offset_resampled( blip_resampled_time_t, int delta, Blip_Buffer* ) const;
	void offset_resampled( blip_resampled_time_t t, int o ) const {
		offset_resampled( t, o, impulse.buf );
	}
	void offset( blip_time_t t, int delta ) const {
		offset( t, delta, impulse.buf );
	}
	void offset_inline( blip_time_t time, int delta, Blip_Buffer* buf ) const {
		offset_resampled( time * buf->factor_ + buf->offset_, delta, buf );
	}
	void offset_inline( blip_time_t time, int delta ) const {
		offset_inline( time, delta, impulse.buf );
	}
};

// Blip_Wave is a synthesizer for adding a *single* waveform to a Blip_Buffer.
// A wave is built from a series of delays and new amplitudes. This provides a
// simpler interface than Blip_Synth, nothing more.
template<int quality,int range>
class Blip_Wave {
	Blip_Synth<quality,range> synth;
	blip_time_t time_;
	int last_amp;
	void init() { time_ = 0; last_amp = 0; }
public:
	// Start wave at time 0 and amplitude 0
	Blip_Wave()                         { init(); }
	Blip_Wave( double volume )          { init(); this->volume( volume ); }
	
	// See Blip_Synth for description
	void volume( double v )             { synth.volume( v ); }
	void volume_unit( double v )        { synth.volume_unit( v ); }
	void treble_eq( const blip_eq_t& eq){ synth.treble_eq( eq ); }
	Blip_Buffer* output() const         { return synth.output(); }
	void output( Blip_Buffer* b )       { synth.output( b ); if ( !b ) time_ = last_amp = 0; }
	
	// Current time in frame
	blip_time_t time() const            { return time_; }
	void time( blip_time_t t )          { time_ = t; }
	
	// Current amplitude of wave
	int amplitude() const               { return last_amp; }
	void amplitude( int );
	
	// Move forward by 't' time units
	void delay( blip_time_t t )         { time_ += t; }
	
	// End time frame of specified duration. Localize time to new frame.
	// If wave hadn't been run to end of frame, start it at beginning of new frame.
	void end_frame( blip_time_t duration )
	{
		time_ -= duration;
		if ( time_ < 0 )
			time_ = 0;
	}
};

// End of public interface
	
template<int quality,int range>
void Blip_Wave<quality,range>::amplitude( int amp ) {
	int delta = amp - last_amp;
	last_amp = amp;
	synth.offset_inline( time_, delta );
}

template<int quality,int range>
inline void Blip_Synth<quality,range>::offset_resampled( blip_resampled_time_t time,
		int delta, Blip_Buffer* blip_buf ) const
{
	typedef blip_pair_t_ pair_t;
	
	unsigned sample_index = (time >> BLIP_BUFFER_ACCURACY) & ~1;
	assert(( "Blip_Synth/Blip_wave: Went past end of buffer",
			sample_index < blip_buf->buffer_size_ ));
	enum { const_offset = Blip_Buffer::widest_impulse_ / 2 - width / 2 };
	pair_t* buf = (pair_t*) &blip_buf->buffer_ [const_offset + sample_index];
	
	enum { shift = BLIP_BUFFER_ACCURACY - blip_res_bits_ };
	enum { mask = res * 2 - 1 };
	const pair_t* imp = &impulses [((time >> shift) & mask) * impulse_size];
	
	pair_t offset = impulse.offset * delta;
	
	if ( !fine_bits )
	{
		// normal mode
		for ( int n = width / 4; n; --n )
		{
			pair_t t0 = buf [0] - offset;
			pair_t t1 = buf [1] - offset;
			
			t0 += imp [0] * delta;
			t1 += imp [1] * delta;
			imp += 2;
			
			buf [0] = t0;
			buf [1] = t1;
			buf += 2;
		}
	}
	else
	{
		// fine mode
		enum { sub_range = 1 << fine_bits };
		delta += sub_range / 2;
		int delta2 = (delta & (sub_range - 1)) - sub_range / 2;
		delta >>= fine_bits;
		
		for ( int n = width / 4; n; --n )
		{
			pair_t t0 = buf [0] - offset;
			pair_t t1 = buf [1] - offset;
			
			t0 += imp [0] * delta2;
			t0 += imp [1] * delta;
			
			t1 += imp [2] * delta2;
			t1 += imp [3] * delta;
			
			imp += 4;
			
			buf [0] = t0;
			buf [1] = t1;
			buf += 2;
		}
	}
}

template<int quality,int range>
void Blip_Synth<quality,range>::offset( blip_time_t time, int delta, Blip_Buffer* buf ) const {
	offset_resampled( time * buf->factor_ + buf->offset_, delta, buf );
}

#endif

