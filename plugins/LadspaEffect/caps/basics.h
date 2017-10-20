/*
	basics.h
	
	Copyright 2004-9 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	common constants, typedefs, utility functions 
	and simplified LADSPA #defines.

*/
/*
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA or point your web browser to http://www.gnu.org.
*/

#ifndef _BASICS_H_
#define _BASICS_H_

#define _GNU_SOURCE 1
#define _USE_GNU 1

/* unlocking some standard math calls. */
#define __USE_ISOC99 1
#define __USE_ISOC9X 1
#define _ISOC99_SOURCE 1
#define _ISOC9X_SOURCE 1

#include <stdlib.h>
#include <string.h>

#include <math.h>

#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include <ladspa.h> 	 

/* reducing LADSPA_DEFINES_WITH_LOTS_OF_CHARACTERS_REALLY verbosity */
#define BOUNDED (LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE)
#define INTEGER LADSPA_HINT_INTEGER
#define LOG LADSPA_HINT_LOGARITHMIC
#define TOGGLE LADSPA_HINT_TOGGLED

#define DEFAULT_0 LADSPA_HINT_DEFAULT_0
#define DEFAULT_1 LADSPA_HINT_DEFAULT_1
#define DEFAULT_100 LADSPA_HINT_DEFAULT_100
#define DEFAULT_440 LADSPA_HINT_DEFAULT_440
#define DEFAULT_MIN LADSPA_HINT_DEFAULT_MINIMUM
#define DEFAULT_LOW LADSPA_HINT_DEFAULT_LOW
#define DEFAULT_MID LADSPA_HINT_DEFAULT_MIDDLE
#define DEFAULT_HIGH LADSPA_HINT_DEFAULT_HIGH
#define DEFAULT_MAX LADSPA_HINT_DEFAULT_MAXIMUM

#define INPUT LADSPA_PORT_INPUT
#define OUTPUT LADSPA_PORT_OUTPUT
#define AUDIO LADSPA_PORT_AUDIO
#define CONTROL LADSPA_PORT_CONTROL

#define HARD_RT LADSPA_PROPERTY_HARD_RT_CAPABLE

#define TEN_TO_THE_SIXTH 1000000

#define MIN_GAIN .000001 /* -120 dB */

/* smallest non-denormal 32 bit IEEE float is 1.18�10-38 */
#define NOISE_FLOOR .00000000000005 /* -266 dB */

typedef int8_t			int8;
typedef uint8_t			uint8;
typedef int16_t			int16;
typedef uint16_t		uint16;
typedef int32_t			int32;
typedef uint32_t		uint32;
typedef int64_t			int64;
typedef uint64_t		uint64;

typedef struct {
	const char * name;
	LADSPA_PortDescriptor descriptor;
	LADSPA_PortRangeHint range;
} PortInfo;

typedef LADSPA_Data sample_t;
typedef unsigned long ulong;

/* flavours for sample store functions run() and run_adding() */
typedef void (*sample_func_t) (sample_t *, int, sample_t, sample_t);

inline void
store_func (sample_t * s, int i, sample_t x, sample_t gain)
{
	s[i] = x;
}

inline void
adding_func (sample_t * s, int i, sample_t x, sample_t gain)
{
	s[i] += gain * x;
}

#ifndef max

template <class X, class Y>
X min (X x, Y y)
{
	return x < y ? x : (X) y;
}

template <class X, class Y>
X max (X x, Y y)
{
	return x > y ? x : (X) y;
}

#endif /* ! max */

template <class T>
T clamp (T value, T lower, T upper)
{
	if (value < lower) return lower;
	if (value > upper) return upper;
	return value;
}

static inline float
frandom()
{
	return (float) rand() / (float) RAND_MAX;
}

/* NB: also true if 0  */
inline bool 
is_denormal (float & f)
{
	int32 i = *((int32 *) &f);
	return ((i & 0x7f800000) == 0);
}

/* todo: not sure if this double version is correct, actually ... */
inline bool 
is_denormal (double & f)
{
	int64 i = *((int64 *) &f);
	return ((i & 0x7fe0000000000000ll) == 0);
}

#ifdef __i386__
	#define TRAP asm ("int $3;")
#else
	#define TRAP
#endif

/* //////////////////////////////////////////////////////////////////////// */

#define CAPS "C* "

class Plugin {
	public:
		double fs; /* sample rate */
		double adding_gain; /* for run_adding() */

		int first_run; /* 1st block after activate(), do no parameter smoothing */
		sample_t normal; /* renormal constant */

		sample_t ** ports;
		LADSPA_PortRangeHint * ranges; /* for getport() below */

	public:
		/* get port value, mapping inf or nan to 0 */
		inline sample_t getport_unclamped (int i)
			{
				sample_t v = *ports[i];
				return (isinf (v) || isnan(v)) ? 0 : v;
			}

		/* get port value and clamp to port range */
		inline sample_t getport (int i)
			{
				LADSPA_PortRangeHint & r = ranges[i];
				sample_t v = getport_unclamped (i);
				return clamp (v, r.LowerBound, r.UpperBound);
			}
};

#endif /* _BASICS_H_ */
