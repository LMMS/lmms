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
#include <algorithm>
#include "Config.h"
#include "../globals.h"

using std::min;
using std::max;

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
std::string to_s(T x)
{
    return stringFrom(x);
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

template<class T>
bool inRange(T val, T min, T max)
{
    return val >= min && val <= max;
}

template<class T>
T array_max(const T *data, size_t len)
{
    T max = 0;

    for(unsigned i = 0; i < len; ++i)
        if(max < data[i])
            max = data[i];
    return max;
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
#ifndef INT32_MAX
#define INT32_MAX      (2147483647)
#endif
#define RND (prng() / (INT32_MAX * 1.0f))

//Linear Interpolation
float interpolate(const float *data, size_t len, float pos);

//Linear circular interpolation
float cinterpolate(const float *data, size_t len, float pos);

/**
 * Port macros - these produce easy and regular port definitions for common
 * types
 */

///trims a path in recursions
const char *message_snip(const char *m);

template<class T>
static inline void nullify(T &t) {delete t; t = NULL; }
template<class T>
static inline void arrayNullify(T &t) {delete [] t; t = NULL; }
#define rParamZyn(name, ...) \
  {STRINGIFY(name) "::i",  rProp(parameter) rMap(min, 0) rMap(max, 127) DOC(__VA_ARGS__), NULL, rParamICb(name)}

///floating point parameter - with lookup code
#define PARAMF(type, var, name, scale, _min, _max, desc) \
{#name"::f", ":parameter\0:documentation\0=" desc "\0", 0, \
    [](const char *m, rtosc::RtData &d) { \
        if(rtosc_narguments(m)==0) {\
            d.reply(d.loc, "f", ((type*)d.obj)->var); \
        } else if(rtosc_narguments(m)==1 && rtosc_type(m,0)=='f') {\
            ((type*)d.obj)->var = limit<float>(rtosc_argument(m,0).f,_min,_max); \
            d.broadcast(d.loc, "f", ((type*)d.obj)->var);}}}

///character parameter - with lookup code
#define PARAMC(type, var, name, desc) \
{#name"::c", ":parameter\0:old-param\0:documentation\0=" desc"\0", 0, \
    [](const char *m, rtosc::RtData &d) { \
        if(rtosc_narguments(m)==0) {\
            d.reply(d.loc, "c", ((type*)d.obj)->var); \
        } else if(rtosc_narguments(m)==1 && rtosc_type(m,0)=='c') {\
            ((type*)d.obj)->var = limit<char>(rtosc_argument(m,0).i,0,127); \
            d.broadcast(d.loc, "c", ((type*)d.obj)->var);}}}

///Recur - perform a simple recursion
#define RECUR(type, cast, name, var, desc) \
{#name"/", ":recursion\0:documentation\0=" desc"\0", &cast::ports, [](const char *m, rtosc::RtData &d){\
    d.obj = &(((type*)d.obj)->var); \
    cast::ports.dispatch(message_snip(m), d);}}

///Recurs - perform a ranged recursion
#define RECURS(type, cast, name, var, length, desc) \
{#name "#" #length "/", ":recursion\0:documentation\0=" desc"\0", &cast::ports, \
    [](const char *m, rtosc::RtData &d){ \
        const char *mm = m; \
        while(!isdigit(*mm))++mm; \
        d.obj = &(((type*)d.obj)->var)[atoi(mm)]; \
        cast::ports.dispatch(message_snip(m), d);}}

///Recur - perform a simple recursion (on pointer member)
#define RECURP(type, cast, name, var, desc) \
{#name"/", ":recursion\0:documentation\0=" desc"\0", &cast::ports, [](const char *m, rtosc::RtData &d){\
    d.obj = (((type*)d.obj)->var); \
    if(d.obj) cast::ports.dispatch(message_snip(m), d);}}

///Recurs - perform a ranged recursion (on pointer array member)
#define RECURSP(type, cast, name, var, length, desc) \
{#name "#" #length "/", ":recursion\0:documentation\0=" desc"\0", &cast::ports, \
    [](const char *m, rtosc::RtData &d){ \
        const char *mm = m; \
        while(!isdigit(*mm))++mm; \
        d.obj = (((type*)d.obj)->var)[atoi(mm)]; \
        cast::ports.dispatch(message_snip(m), d);}}

#define rSelf(type) \
{"self", rProp(internal) rMap(class, type) rDoc("port metadata"), 0, \
    [](const char *, rtosc::RtData &d){ \
        d.reply(d.loc, "b", sizeof(d.obj), &d.obj);}}\

#define rPaste() \
{"paste:b", rProp(internal) rDoc("paste port"), 0, \
    [](const char *m, rtosc::RtData &d){ \
        printf("rPaste...\n"); \
        rObject &paste = **(rObject **)rtosc_argument(m,0).b.data; \
        rObject &o = *(rObject*)d.obj;\
        o.paste(paste);}}

#endif
