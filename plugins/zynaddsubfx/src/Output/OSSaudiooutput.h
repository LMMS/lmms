/*
  ZynAddSubFX - a software synthesizer

  OSSaudiooutput.h - Audio output for Open Sound System
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

#ifndef OSS_AUDIO_OUTPUT_H
#define OSS_AUDIO_OUTPUT_H

#include <sys/time.h>
#include "../globals.h"

class OSSaudiooutput
{
    public:
        OSSaudiooutput();
        ~OSSaudiooutput();

        //the out is [-1.0 .. 1.0]
        /* smp_left[] and smp_right[] has the size of SOUND_BUFFER_SIZE */
        void OSSout(REALTYPE *smp_left, REALTYPE *smp_right);
    private:
        int snd_handle;
        int snd_fragment;
        int snd_stereo;
        int snd_format;
        int snd_samplerate;
        struct timeval playing_until;

        short int *smps; //Samples to be sent to soundcard
};

#endif

