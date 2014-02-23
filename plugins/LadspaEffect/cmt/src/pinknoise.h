/* pinknoise.h

   pink noise generating class using the Voss-McCartney algorithm, as
   described at www.firstpr.com.au/dsp/pink-noise/

   (c) 2002 Nathaniel Virgo

   Computer Music Toolkit - a library of LADSPA plugins. Copyright (C)
   2000-2002 Richard W.E. Furse. The author may be contacted at
   richard@muse.demon.co.uk.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public Licence as
   published by the Free Software Foundation; either version 2 of the
   Licence, or (at your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA. */

#ifndef _PINKNOISE_H
#define _PINKNOISE_H

#include <stdlib.h>

typedef unsigned int CounterType;
typedef float DataValue;

const int n_generators = 8*sizeof(CounterType);

class PinkNoise {
 private:
    
    CounterType counter;
    DataValue * generators;
    DataValue last_value;

 public:
    
    PinkNoise() {
	generators = new DataValue[n_generators];
	reset();
    }

    ~PinkNoise() {delete [] generators;};

    void reset() {
	counter = 0;
	last_value = 0;
	for (int i=0; i<n_generators; ++i) {
	    generators[i] = 2*(rand()/DataValue(RAND_MAX))-1;
	    last_value += generators[i];
	}
    }

    inline DataValue getUnscaledValue() {
	if (counter != 0) {
	    // set index to number of trailing zeros in counter.
	    // hangs if counter==0, hence the slightly inefficient
	    // test above.
	    CounterType n = counter;
	    int index = 0;
	    while ( (n & 1) == 0 ) {
		n >>= 1;
		index++;
		// this loop means that the plugins cannot be labelled as
		// capable of hard real-time performance.
	    }

	    last_value -= generators[index];
	    generators[index] = 2*(rand()/DataValue(RAND_MAX))-1;
	    last_value += generators[index];
	}

	counter++;
	
	return last_value;
    }

    inline DataValue getValue() {
	return getUnscaledValue()/n_generators;
    }

    inline DataValue getLastValue() {
	return last_value/n_generators;
    }

    inline DataValue getValue2() {
	// adding some white noise gets rid of some nulls in the frequency spectrum
	// but makes the signal spikier, so possibly not so good for control signals.
	return (getUnscaledValue() + rand()/DataValue(RAND_MAX*0.5)-1)/(n_generators+1);
    }

};

#endif









