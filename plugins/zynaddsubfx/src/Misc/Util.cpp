/*
  ZynAddSubFX - a software synthesizer

  Util.C - Miscellaneous functions
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

#include "Util.h"
#include <math.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int SAMPLE_RATE = 44100;
int SOUND_BUFFER_SIZE = 256;
int OSCIL_SIZE  = 1024;

Config    config;
REALTYPE *denormalkillbuf;


/*
 * Transform the velocity according the scaling parameter (velocity sensing)
 */
REALTYPE VelF(REALTYPE velocity, unsigned char scaling)
{
    REALTYPE x;
    x = pow(VELOCITY_MAX_SCALE, (64.0 - scaling) / 64.0);
    if((scaling == 127) || (velocity > 0.99))
        return 1.0;
    else
        return pow(velocity, x);
}

/*
 * Get the detune in cents
 */
REALTYPE getdetune(unsigned char type,
                   unsigned short int coarsedetune,
                   unsigned short int finedetune)
{
    REALTYPE det = 0.0, octdet = 0.0, cdet = 0.0, findet = 0.0;
    //Get Octave
    int octave   = coarsedetune / 1024;
    if(octave >= 8)
        octave -= 16;
    octdet = octave * 1200.0;

    //Coarse and fine detune
    int cdetune = coarsedetune % 1024;
    if(cdetune > 512)
        cdetune -= 1024;

    int fdetune = finedetune - 8192;

    switch(type) {
//	case 1: is used for the default (see below)
    case 2:
        cdet   = fabs(cdetune * 10.0);
        findet = fabs(fdetune / 8192.0) * 10.0;
        break;
    case 3:
        cdet   = fabs(cdetune * 100);
        findet = pow(10, fabs(fdetune / 8192.0) * 3.0) / 10.0 - 0.1;
        break;
    case 4:
        cdet   = fabs(cdetune * 701.95500087); //perfect fifth
        findet = (pow(2, fabs(fdetune / 8192.0) * 12.0) - 1.0) / 4095 * 1200;
        break;
    //case ...: need to update N_DETUNE_TYPES, if you'll add more
    default:
        cdet   = fabs(cdetune * 50.0);
        findet = fabs(fdetune / 8192.0) * 35.0; //almost like "Paul's Sound Designer 2"
        break;
    }
    if(finedetune < 8192)
        findet = -findet;
    if(cdetune < 0)
        cdet = -cdet;

    det = octdet + cdet + findet;
    return det;
}


bool fileexists(const char *filename)
{
    struct stat tmp;
    int result = stat(filename, &tmp);
    if(result >= 0)
        return true;

    return false;
}

