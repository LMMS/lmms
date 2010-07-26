/*
  ZynAddSubFX - a software synthesizer

  VSTaudiooutput.cpp - Audio output for VST
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
#include <string.h>
#include "VSTaudiooutput.h"

//the constructor and the destructor are defined in main.cpp

void VSTSynth::process(float **inputs, float **outputs, long sampleframes)
{
    float *outl = outputs[0];
    float *outr = outputs[1];
    pthread_mutex_lock(&vmaster->mutex);
    vmaster->GetAudioOutSamples(sampleframes, (int) getSampleRate(), outl, outr);
    pthread_mutex_unlock(&vmaster->mutex);
}

void VSTSynth::processReplacing(float **inputs,
                                float **outputs,
                                long sampleframes)
{
    process(inputs, outputs, sampleframes);
}

long int VSTSynth::canDo(char *txt)
{
    if(strcmp(txt, "receiveVstEvents") == 0)
        return 1;
    if(strcmp(txt, "receiveVstMidiEvent") == 0)
        return 1;
    return -1;
}

bool VSTSynth::getVendorString(char *txt)
{
    strcpy(txt, "Nasca O. Paul");
    return true;
}

bool VSTSynth::getProductString(char *txt)
{
    strcpy(txt, "ZynAddSubFX");
    return true;
}

void VSTSynth::resume()
{
    wantEvents();
}

