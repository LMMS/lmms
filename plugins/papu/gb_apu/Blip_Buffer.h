
// Buffer of sound samples into which band-limited waveforms can be synthesized
// using Blip_Wave or Blip_Synth.

// Blip_Buffer 0.3.4. Copyright (C) 2003-2005 Shay Green. GNU LGPL license.

#ifndef BLIP_BUFFER_H
#define BLIP_BUFFER_H

#include "blargg_common.h"

class Blip_Reader;

// Source time unit.
typedef long blip_time_t;

// Type of sample produced. Signed 16-bit format.
typedef BOOST::int16_t blip_sample_t;

// Make buffer as large as possible (currently about 65000 samples)
const int blip_default_length = 0;

typedef unsigned long blip_resampled_time_t; // not documented

class Blip_Buffer {
public:
	// Construct an empty buffer.
	Blip_Buffer();
	~Blip_Buffer();
	
	// Set output sample rate and buffer length in milliseconds (1/1000 sec),
	// then clear buffer. If length is not specified, make as large as possible.
	// If there is insufficient memory for the buffer, sets the buffer length
	// to 0 and returns error string (or propagates exception if compiler supports it).
	blargg_err_t set_sample_rate( long samples_per_sec, int msec_length = blip_default_length );
	
	// Length of buffer, in milliseconds
	int length() const;
	
	// Current output sample rate
	long sample_rate() const;
	
	// Number of source time units per second
	void clock_rate( long );
	long clock_rate() const;
	
	// Set frequency at which high-pass filter attenuation passes -3dB
	void bass_freq( int frequency );
	
	// Remove all available samples and clear buffer to silence. If 'entire_buffer' is
	// false, just clear out any samples waiting rather than the entire buffer.
	void clear( bool entire_buffer = true );
	
	// End current time frame of specified duration and make its samples available
	// (along with any still-unread samples) for reading with read_samples(). Begin
	// a new time frame at the end of the current frame. All transitions must have
	// been added before 'time'.
	void end_frame( blip_time_t time );
	
	// Number of samples available for reading with read_samples()
	long samples_avail() const;
	
	// Read at most 'max_samples' out of buffer into 'dest', removing them from from
	// the buffer. Return number of samples actually read and removed. If stereo is
	// true, increment 'dest' one extra time after writing each sample, to allow
	// easy interleving of two channels into a stereo output buffer.
	long read_samples( blip_sample_t* dest, long max_samples, bool stereo = false );
	
	// Remove 'count' samples from those waiting to be read
	void remove_samples( long count );
	
	// Number of samples delay from synthesis to samples read out
	int output_latency() const;
	
// Beta features
	
	// Number of raw samples that can be mixed within frame of specified duration
	long count_samples( blip_time_t duration ) const;
	
	// Mix 'count' samples from 'buf' into buffer.
	void mix_samples( const blip_sample_t* buf, long count );
	
	// Count number of clocks needed until 'count' samples will be available.
	// If buffer can't even hold 'count' samples, returns number of clocks until
	// buffer is full.
	blip_time_t count_clocks( long count ) const;
	
	
	// not documented yet
	
	void remove_silence( long count );
	
	blip_resampled_time_t resampled_time( blip_time_t t ) const
	{
		return t * blip_resampled_time_t (factor_) + offset_;
	}
	
	blip_resampled_time_t clock_rate_factor( long clock_rate ) const;
	
	blip_resampled_time_t resampled_duration( int t ) const
	{
		return t * blip_resampled_time_t (factor_);
	}
	
private:
	// noncopyable
	Blip_Buffer( const Blip_Buffer& );
	Blip_Buffer& operator = ( const Blip_Buffer& );

	// Don't use the following members. They are public only for technical reasons.
	public:
		enum { sample_offset_ = 0x7F7F }; // repeated byte allows memset to clear buffer
		enum { widest_impulse_ = 24 };
		typedef BOOST::uint16_t buf_t_;
		
		unsigned long factor_;
		blip_resampled_time_t offset_;
		buf_t_* buffer_;
		unsigned buffer_size_;
	private:
		long reader_accum;
		int bass_shift;
		long samples_per_sec;
		long clocks_per_sec;
		int bass_freq_;
		int length_;
		
		enum { accum_fract = 15 }; // less than 16 to give extra sample range
		
		friend class Blip_Reader;
};

// Low-pass equalization parameters (see notes.txt)
class blip_eq_t {
public:
	blip_eq_t( double treble = 0 );
	blip_eq_t( double treble, long cutoff, long sample_rate );
private:
	double treble;
	long cutoff;
	long sample_rate;
	friend class Blip_Impulse_;
};

// not documented yet (see Multi_Buffer.cpp for an example of use)
class Blip_Reader {
	const Blip_Buffer::buf_t_* buf;
	long accum;
	#ifdef __MWERKS__
	void operator = ( struct foobar ); // helps optimizer
	#endif
public:
	// avoid anything which might cause optimizer to put object in memory
	
	int begin( Blip_Buffer& blip_buf ) {
		buf = blip_buf.buffer_;
		accum = blip_buf.reader_accum;
		return blip_buf.bass_shift;
	}
	
	int read() const {
		return accum >> Blip_Buffer::accum_fract;
	}
	
	void next( int bass_shift = 9 ) {
		accum -= accum >> bass_shift;
		accum += ((long) *buf++ - Blip_Buffer::sample_offset_) << Blip_Buffer::accum_fract;
	}
	
	void end( Blip_Buffer& blip_buf ) {
		blip_buf.reader_accum = accum;
	}
};



// End of public interface
	
#ifndef BLIP_BUFFER_ACCURACY
	#define BLIP_BUFFER_ACCURACY 16
#endif

const int blip_res_bits_ = 5;

typedef BOOST::uint32_t blip_pair_t_;

class Blip_Impulse_ {
	typedef BOOST::uint16_t imp_t;
	
	blip_eq_t eq;
	double  volume_unit_;
	imp_t*  impulses;
	imp_t*  impulse;
	int     width;
	int     fine_bits;
	int     res;
	bool    generate;
	
	void fine_volume_unit();
	void scale_impulse( int unit, imp_t* ) const;
public:
	Blip_Buffer*    buf;
	BOOST::uint32_t offset;
	
	void init( blip_pair_t_* impulses, int width, int res, int fine_bits = 0 );
	void volume_unit( double );
	void treble_eq( const blip_eq_t& );
};

inline blip_eq_t::blip_eq_t( double t ) :
		treble( t ), cutoff( 0 ), sample_rate( 44100 ) {
}

inline blip_eq_t::blip_eq_t( double t, long c, long sr ) :
		treble( t ), cutoff( c ), sample_rate( sr ) {
}

inline int Blip_Buffer::length() const {
	return length_;
}

inline long Blip_Buffer::samples_avail() const {
	return long (offset_ >> BLIP_BUFFER_ACCURACY);
}

inline long Blip_Buffer::sample_rate() const {
	return samples_per_sec;
}

inline void Blip_Buffer::end_frame( blip_time_t t ) {
	offset_ += t * factor_;
	assert(( "Blip_Buffer::end_frame(): Frame went past end of buffer",
			samples_avail() <= (long) buffer_size_ ));
}

inline void Blip_Buffer::remove_silence( long count ) {
	assert(( "Blip_Buffer::remove_silence(): Tried to remove more samples than available",
			count <= samples_avail() ));
	offset_ -= blip_resampled_time_t (count) << BLIP_BUFFER_ACCURACY;
}

inline int Blip_Buffer::output_latency() const {
	return widest_impulse_ / 2;
}

inline long Blip_Buffer::clock_rate() const {
	return clocks_per_sec;
}

inline void Blip_Buffer::clock_rate( long cps )
{
	clocks_per_sec = cps;
	factor_ = clock_rate_factor( cps );
}

#include "Blip_Synth.h"

#endif

