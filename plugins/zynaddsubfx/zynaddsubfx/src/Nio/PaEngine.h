/*
  ZynAddSubFX - a software synthesizer

  PAaudiooutput.h - Audio output for PortAudio
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
#ifndef PA_ENGINE_H
#define PA_ENGINE_H

#include <portaudio.h>

#include "../globals.h"
#include "AudioOut.h"

class PaEngine:public AudioOut
{
    public:
        PaEngine();
        ~PaEngine();

        bool Start();
        void Stop();

        void setAudioEn(bool nval);
        bool getAudioEn() const;

    protected:
        static int PAprocess(const void *inputBuffer,
                             void *outputBuffer,
                             unsigned long framesPerBuffer,
                             const PaStreamCallbackTimeInfo *outTime,
                             PaStreamCallbackFlags flags,
                             void *userData);
        int process(float *out, unsigned long framesPerBuffer);
    private:
        PaStream *stream;
};


void PAfinish();

#endif
