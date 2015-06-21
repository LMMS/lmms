/*
   ZynAddSubFX - a software synthesizer

   OssMultiEngine.h - Multi channel audio output for Open Sound System
   Copyright (C) 2014 Hans Petter Selasky

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

#ifndef OSS_MULTI_ENGINE_H
#define OSS_MULTI_ENGINE_H

#include <sys/time.h>
#include "../globals.h"
#include "AudioOut.h"

class OssMultiEngine : public AudioOut
{
    public:
        OssMultiEngine();
        ~OssMultiEngine();

        bool Start();
        void Stop();

        void setAudioEn(bool nval);
        bool getAudioEn() const;

    protected:
        void *audioThreadCb();
        static void *_audioThreadCb(void *arg);

    private:
        pthread_t audioThread;

        /* Audio */
        bool openAudio();
        void stopAudio();

        int handle;
        int maxbuffersize;
        int buffersize;
        int channels;

        union {
            /* Samples to be sent to soundcard */
            short int *ps16;
            int *ps32;
        } smps;

        bool en;
        bool is32bit;
};

#endif
