/*
  Copyright (C) 2006 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

#include <stdio.h>
#include <stdlib.h>
#include "WAVaudiooutput.h"
using namespace std;

WAVaudiooutput::WAVaudiooutput()
{
    file=NULL;
    sampleswritten=0;
    samplerate=44100;
};

WAVaudiooutput::~WAVaudiooutput()
{
    close();
};

bool WAVaudiooutput::newfile(string filename,int samplerate,int channels)
{
    /**\todo Move this into the Constructor*/
    close();//inchide un posibil fisier existent
    file=fopen(filename.c_str(),"w");
    if (!file) return false;
    this->samplerate=samplerate;
    this->channels=channels;
    sampleswritten=0;
    char tmp[44];
    fwrite(tmp,1,44,file);
    return(true);
};

void WAVaudiooutput::close()
{
    if (file) {
        unsigned int chunksize;
        rewind(file);

        fwrite("RIFF",4,1,file);
        chunksize=sampleswritten*4+36;
        fwrite(&chunksize,4,1,file);

        fwrite("WAVEfmt ",8,1,file);
        chunksize=16;
        fwrite(&chunksize,4,1,file);
        unsigned short int formattag=1;//uncompresed wave
        fwrite(&formattag,2,1,file);
        unsigned short int nchannels=channels;//stereo
        fwrite(&nchannels,2,1,file);
        unsigned int samplerate_=samplerate;//samplerate
        fwrite(&samplerate_,4,1,file);
        unsigned int bytespersec=samplerate*2*channels;//bytes/sec
        fwrite(&bytespersec,4,1,file);
        unsigned short int blockalign=2*channels;//2 channels * 16 bits/8
        fwrite(&blockalign,2,1,file);
        unsigned short int bitspersample=16;
        fwrite(&bitspersample,2,1,file);

        fwrite("data",4,1,file);
        chunksize=sampleswritten*blockalign;
        fwrite(&chunksize,4,1,file);

        fclose(file);
        file=NULL;
    }
};

void WAVaudiooutput::write_stereo_samples(int nsmps,short int *smps)
{
    if (!file) return;
    fwrite(smps,nsmps,4,file);
    sampleswritten+=nsmps;
};

void WAVaudiooutput::write_mono_samples(int nsmps,short int *smps)
{
    if (!file) return;
    fwrite(smps,nsmps,2,file);
    sampleswritten+=nsmps;
};


