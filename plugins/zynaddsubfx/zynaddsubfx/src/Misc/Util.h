/*
  ZynAddSubFX - a software synthesizer

  Util.h - Miscellaneous functions
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <sstream>
#include <stdint.h>
#include "Config.h"
#include "../globals.h"

//Velocity Sensing function
extern float VelF(float velocity, unsigned char scaling);

bool fileexists(const char *filename);

#define N_DETUNE_TYPES 4 //the number of detune types
extern float getdetune(unsigned char type,
                       unsigned short int coarsedetune,
                       unsigned short int finedetune);

/**Try to set current thread to realtime priority program priority
 * \todo see if the right pid is being sent
 * \todo see if this is having desired effect, if not then look at
 * pthread_attr_t*/
void set_realtime();

/**Os independent sleep in microsecond*/
void os_sleep(long length);

std::string legalizeFilename(std::string filename);

extern float *denormalkillbuf; /**<the buffer to add noise in order to avoid denormalisation*/

extern class Config config;

void invSignal(float *sig, size_t len);

template<class T>
std::string stringFrom(T x)
{
    std::stringstream ss;
    ss << x;
    return ss.str();
}

template<class T>
T stringTo(const char *x)
{
    std::string str = x != NULL ? x : "0"; //should work for the basic float/int
    std::stringstream ss(str);
    T ans;
    ss >> ans;
    return ans;
}

template<class T>
T limit(T val, T min, T max)
{
    return val < min ? min : (val > max ? max : val);
}

//Random number generator

typedef uint32_t prng_t;
extern prng_t prng_state;

// Portable Pseudo-Random Number Generator
inline prng_t prng_r(prng_t &p)
{
    return p = p * 1103515245 + 12345;
}

inline prng_t prng(void)
{
    return prng_r(prng_state) & 0x7fffffff;
}

inline void sprng(prng_t p)
{
    prng_state = p;
}

/*
 * The random generator (0.0f..1.0f)
 */
# define INT32_MAX      (2147483647)
#define RND (prng() / (INT32_MAX * 1.0f))

//Linear Interpolation
float interpolate(const float *data, size_t len, float pos);

//Linear circular interpolation
float cinterpolate(const float *data, size_t len, float pos);

#endif
