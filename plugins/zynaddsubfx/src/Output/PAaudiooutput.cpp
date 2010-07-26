/*
  ZynAddSubFX - a software synthesizer

  PAaudiooutput.cpp - Audio output for PortAudio
  Copyright (C) 2002 Nasca Octavian Paul
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

#include "PAaudiooutput.h"

Master   *PAmaster;
PaStream *stream;
REALTYPE *outl, *outr;

int PAprocess(void *inputBuffer, void *outputBuffer,
              unsigned long framesPerBuffer,
              PaTimestamp outTime, void *userData)
{
    if(framesPerBuffer != SOUND_BUFFER_SIZE) {
        fprintf(
            stderr,
            "Bug: PAudioOutput::PAprocess  SOUND_BUFFER_SIZE!=framesPerBuffer");
        fprintf(stderr, "%d %d\n", framesPerBuffer, SOUND_BUFFER_SIZE);
    }

    pthread_mutex_lock(&PAmaster->mutex);
    PAmaster->GetAudioOutSamples(SOUND_BUFFER_SIZE, SAMPLE_RATE, outl, outr);
    pthread_mutex_unlock(&PAmaster->mutex);

    float *out = (float *)outputBuffer;

    for(int i = 0; i < framesPerBuffer; i++) {
        if(i >= SOUND_BUFFER_SIZE)
            break;                      //this should never happens, except only when framesPerBuffer!>SOUND_BUFFER_SIZE
        out[i * 2]     = outl[i];
        out[i * 2 + 1] = outr[i];
    }

    return 0;
}

void PAaudiooutputinit(Master *master_)
{
    PAmaster = master_;
    outl     = new REALTYPE [SOUND_BUFFER_SIZE];
    outr     = new REALTYPE [SOUND_BUFFER_SIZE];
    Pa_Initialize();
    Pa_OpenDefaultStream(&stream,
                         0,
                         2,
                         paFloat32,
                         SAMPLE_RATE,
                         SOUND_BUFFER_SIZE,
                         0,
                         PAprocess,
                         NULL);
    Pa_StartStream(stream);
}

void PAfinish()
{
    Pa_StopStream(stream);
    delete (outl);
    delete (outr);
}

