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

#include <cstdio>
#include <string>
#include <sstream>
#include "../globals.h"
#include "Config.h"

//Velocity Sensing function
extern REALTYPE VelF(REALTYPE velocity, unsigned char scaling);

bool fileexists(const char *filename);

#define N_DETUNE_TYPES 4 //the number of detune types
extern REALTYPE getdetune(unsigned char type,
                          unsigned short int coarsedetune,
                          unsigned short int finedetune);

extern REALTYPE *denormalkillbuf; /**<the buffer to add noise in order to avoid denormalisation*/

extern Config config;

void invSignal(REALTYPE *sig, size_t len);

void crossover(REALTYPE &a, REALTYPE &b, REALTYPE crossover);

template<class T>
std::string stringFrom(T x)
{
    std::stringstream ss;
    ss << x;
    return ss.str();
}

template<class T>
std::string stringFrom(REALTYPE x)
{
    char buf[64];
    sprintf( buf, "%f", x );
    return buf;
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

template <class T>
T limit(T val, T min, T max)
{
    return (val < min ? min : (val > max ? max : val));
}

#endif

