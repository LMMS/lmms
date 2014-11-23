/*
  ZynAddSubFX - a software synthesizer

  AudioOut.h - Audio Output superclass
  Copyright (C) 2009-2010 Mark McCurry
  Author: Mark McCurry

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

#ifndef AUDIO_OUT_H
#define AUDIO_OUT_H

#include "../Misc/Stereo.h"
#include "../globals.h"
#include "Engine.h"

class AudioOut:public virtual Engine
{
    public:
        AudioOut();
        virtual ~AudioOut();

        /**Sets the Sample Rate of this Output
         * (used for getNext()).*/
        void setSamplerate(int _samplerate);

        /**Sets the Samples required per Out of this driver
         * not a realtime opperation */
        int getSampleRate();
        void setBufferSize(int _bufferSize);

        /**Sets the Frame Size for output*/
        void bufferingSize(int nBuffering);
        int bufferingSize();

        virtual void setAudioEn(bool nval) = 0;
        virtual bool getAudioEn() const    = 0;

    protected:
        /**Get the next sample for output.
         * (has nsamples sampled at a rate of samplerate)*/
        const Stereo<float *> getNext();

        int samplerate;
        int bufferSize;
};

#endif
