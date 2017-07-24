// Multi-channel sound buffer interface, and basic mono and stereo buffers

// Blip_Buffer 0.4.1
#ifndef MULTI_BUFFER_H
#define MULTI_BUFFER_H

#include "blargg_common.h"
#include "Blip_Buffer.h"

// Interface to one or more Blip_Buffers mapped to one or more channels
// consisting of left, center, and right buffers.
class Multi_Buffer {
public:
	Multi_Buffer( int samples_per_frame );
	virtual ~Multi_Buffer() { }
	
	// Set the number of channels available
	virtual blargg_err_t set_channel_count( int );
	
	// Get indexed channel, from 0 to channel count - 1
	struct channel_t {
		Blip_Buffer* center;
		Blip_Buffer* left;
		Blip_Buffer* right;
	};
	enum { type_index_mask = 0xFF };
	enum { wave_type = 0x100, noise_type = 0x200, mixed_type = wave_type | noise_type };
	virtual channel_t channel( int index, int type ) = 0;
	
	// See Blip_Buffer.h
	virtual blargg_err_t set_sample_rate( long rate, int msec = blip_default_length ) = 0;
	virtual void clock_rate( long ) = 0;
	virtual void bass_freq( int ) = 0;
	virtual void clear() = 0;
	long sample_rate() const;
	
	// Length of buffer, in milliseconds
	int length() const;
	
	// See Blip_Buffer.h
	virtual void end_frame( blip_time_t ) = 0;
	
	// Number of samples per output frame (1 = mono, 2 = stereo)
	int samples_per_frame() const;
	
	// Count of changes to channel configuration. Incremented whenever
	// a change is made to any of the Blip_Buffers for any channel.
	unsigned channels_changed_count() { return channels_changed_count_; }
	
	// See Blip_Buffer.h
	virtual long read_samples( blip_sample_t*, long ) = 0;
	virtual long samples_avail() const = 0;
	
public:
	BLARGG_DISABLE_NOTHROW
protected:
	void channels_changed() { channels_changed_count_++; }
private:
	// noncopyable
	Multi_Buffer( const Multi_Buffer& );
	Multi_Buffer& operator = ( const Multi_Buffer& );
	
	unsigned channels_changed_count_;
	long sample_rate_;
	int length_;
	int const samples_per_frame_;
};

// Uses a single buffer and outputs mono samples.
class Mono_Buffer : public Multi_Buffer {
	Blip_Buffer buf;
	channel_t chan;
public:
	// Buffer used for all channels
	Blip_Buffer* center() { return &buf; }
	
public:
	Mono_Buffer();
	~Mono_Buffer();
	blargg_err_t set_sample_rate( long rate, int msec = blip_default_length );
	void clock_rate( long rate ) { buf.clock_rate( rate ); }
	void bass_freq( int freq ) { buf.bass_freq( freq ); }
	void clear() { buf.clear(); }
	long samples_avail() const { return buf.samples_avail(); }
	long read_samples( blip_sample_t* p, long s ) { return buf.read_samples( p, s ); }
	channel_t channel( int, int ) { return chan; }
	void end_frame( blip_time_t t ) { buf.end_frame( t ); }
};

// Uses three buffers (one for center) and outputs stereo sample pairs.
class Stereo_Buffer : public Multi_Buffer {
public:
	
	// Buffers used for all channels
	Blip_Buffer* center()       { return &bufs [0]; }
	Blip_Buffer* left()         { return &bufs [1]; }
	Blip_Buffer* right()        { return &bufs [2]; }
	
public:
	Stereo_Buffer();
	~Stereo_Buffer();
	blargg_err_t set_sample_rate( long, int msec = blip_default_length );
	void clock_rate( long );
	void bass_freq( int );
	void clear();
	channel_t channel( int, int ) { return chan; }
	void end_frame( blip_time_t );
	
	long samples_avail() const { return bufs [0].samples_avail() * 2; }
	long read_samples( blip_sample_t*, long );
	
private:
	enum { buf_count = 3 };
	Blip_Buffer bufs [buf_count];
	channel_t chan;
	int stereo_added;
	int was_stereo;
	
	void mix_stereo_no_center( blip_sample_t*, blargg_long );
	void mix_stereo( blip_sample_t*, blargg_long );
	void mix_mono( blip_sample_t*, blargg_long );
};

// Silent_Buffer generates no samples, useful where no sound is wanted
class Silent_Buffer : public Multi_Buffer {
	channel_t chan;
public:
	Silent_Buffer();
	blargg_err_t set_sample_rate( long rate, int msec = blip_default_length );
	void clock_rate( long ) { }
	void bass_freq( int ) { }
	void clear() { }
	channel_t channel( int, int ) { return chan; }
	void end_frame( blip_time_t ) { }
	long samples_avail() const { return 0; }
	long read_samples( blip_sample_t*, long ) { return 0; }
};


inline blargg_err_t Multi_Buffer::set_sample_rate( long rate, int msec )
{
	sample_rate_ = rate;
	length_ = msec;
	return 0;
}

inline blargg_err_t Silent_Buffer::set_sample_rate( long rate, int msec )
{
	return Multi_Buffer::set_sample_rate( rate, msec );
}

inline int Multi_Buffer::samples_per_frame() const { return samples_per_frame_; }

inline long Multi_Buffer::sample_rate() const { return sample_rate_; }

inline int Multi_Buffer::length() const { return length_; }

#endif
