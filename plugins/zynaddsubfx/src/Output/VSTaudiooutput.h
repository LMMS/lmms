/*
  ZynAddSubFX - a software synthesizer

  VSTaudiooutput.h - Audio output for VST
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
#ifndef VST_AUDIO_OUTPUT_H
#define VST_AUDIO_OUTPUT_H

#include <pthread.h>

#include "../globals.h"
#include "../Misc/Master.h"
#include "../UI/MasterUI.h"

#include "../../../vstsdk2/source/common/audioeffectx.h"

class VSTSynth:public AudioEffectX
{
    public:
        VSTSynth(audioMasterCallback audioMaster);
        ~VSTSynth();

        virtual void process(float **inputs, float **outputs, long sampleframes);
        virtual void processReplacing(float **inputs,
                                      float **outputs,
                                      long sampleframes);
        virtual long processEvents(VstEvents *events); //this is used for Midi input
        virtual long int canDo(char *txt);
        virtual bool getVendorString(char *txt);
        virtual bool getProductString(char *txt);
        virtual void resume();

        virtual long getChunk(void **data, bool isPreset = false);
        virtual long setChunk(void *data, long size, bool isPreset = false);

        MasterUI *ui;
        int Pexitprogram;

        Master   *vmaster;
        pthread_t thr;
};

#endif

