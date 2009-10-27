/*
  ZynAddSubFX - a software synthesizer

  Recorder.C - Records sound to a file
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

#include <sys/stat.h>
#include "Recorder.h"

Recorder::Recorder()
{
    recordbuf_16bit = new short int [SOUND_BUFFER_SIZE * 2];
    status = 0;
    notetrigger     = 0;
    for(int i = 0; i < SOUND_BUFFER_SIZE * 2; i++)
        recordbuf_16bit[i] = 0;
}

Recorder::~Recorder()
{
    if(recording() == 1)
        stop();
    delete [] recordbuf_16bit;
}

int Recorder::preparefile(std::string filename_, int overwrite)
{
    if(!overwrite) {
        struct stat fileinfo;
        int statr;
        statr = stat(filename_.c_str(), &fileinfo);
        if(statr == 0)   //file exists
            return 1;
    }

    if(!wav.newfile(filename_, SAMPLE_RATE, 2))
        return 2;

    status = 1; //ready

    return 0;
}

void Recorder::start()
{
    notetrigger = 0;
    status      = 2; //recording
}

void Recorder::stop()
{
    wav.close();
    status = 0;
}

void Recorder::pause()
{
    status = 0;
}

int Recorder::recording()
{
    if((status == 2) && (notetrigger != 0))
        return 1;
    else
        return 0;
}

void Recorder::recordbuffer(REALTYPE *outl, REALTYPE *outr)
{
    int tmp;
    if(status != 2)
        return;
    for(int i = 0; i < SOUND_BUFFER_SIZE; i++) {
        tmp = (int)(outl[i] * 32767.0);
        if(tmp < -32768)
            tmp = -32768;
        if(tmp > 32767)
            tmp = 32767;
        recordbuf_16bit[i * 2] = tmp;

        tmp = (int)(outr[i] * 32767.0);
        if(tmp < -32768)
            tmp = -32768;
        if(tmp > 32767)
            tmp = 32767;
        recordbuf_16bit[i * 2 + 1] = tmp;
    }
    wav.write_stereo_samples(SOUND_BUFFER_SIZE, recordbuf_16bit);
}

void Recorder::triggernow()
{
    if(status == 2)
        notetrigger = 1;
}

