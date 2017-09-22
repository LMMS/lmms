/*
  Copyright (C) 2006 Nasca Octavian Paul
  Author: Nasca Octavian Paul
          Mark McCurry

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

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include "WavFile.h"
using namespace std;

WavFile::WavFile(string filename, int samplerate, int channels)
    :sampleswritten(0), samplerate(samplerate), channels(channels),
      file(fopen(filename.c_str(), "w"))

{
    if(file) {
        cout << "INFO: Making space for wave file header" << endl;
        //making space for the header written at destruction
        char tmp[44];
        memset(tmp, 0, 44 * sizeof(char));
        fwrite(tmp, 1, 44, file);
    }
}

WavFile::~WavFile()
{
    if(file) {
        cout << "INFO: Writing wave file header" << endl;

        unsigned int chunksize;
        rewind(file);

        fwrite("RIFF", 4, 1, file);
        chunksize = sampleswritten * 4 + 36;
        fwrite(&chunksize, 4, 1, file);

        fwrite("WAVEfmt ", 8, 1, file);
        chunksize = 16;
        fwrite(&chunksize, 4, 1, file);
        unsigned short int formattag = 1;     //uncompresed wave
        fwrite(&formattag, 2, 1, file);
        unsigned short int nchannels = channels;     //stereo
        fwrite(&nchannels, 2, 1, file);
        unsigned int samplerate_ = samplerate;         //samplerate
        fwrite(&samplerate_, 4, 1, file);
        unsigned int bytespersec = samplerate * 2 * channels;         //bytes/sec
        fwrite(&bytespersec, 4, 1, file);
        unsigned short int blockalign = 2 * channels;    //2 channels * 16 bits/8
        fwrite(&blockalign, 2, 1, file);
        unsigned short int bitspersample = 16;
        fwrite(&bitspersample, 2, 1, file);

        fwrite("data", 4, 1, file);
        chunksize = sampleswritten * blockalign;
        fwrite(&chunksize, 4, 1, file);

        fclose(file);
        file = NULL;
    }
}

bool WavFile::good() const
{
    return file;
}

void WavFile::writeStereoSamples(int nsmps, short int *smps)
{
    if(file) {
        fwrite(smps, nsmps, 4, file);
        sampleswritten += nsmps;
    }
}

void WavFile::writeMonoSamples(int nsmps, short int *smps)
{
    if(file) {
        fwrite(smps, nsmps, 2, file);
        sampleswritten += nsmps;
    }
}
