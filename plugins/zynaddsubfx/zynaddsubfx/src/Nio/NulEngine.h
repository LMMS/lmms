/*
  ZynAddSubFX - a software synthesizer

  NulEngine.h - Dummy In/Out driver
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

#ifndef NUL_ENGINE_H
#define NUL_ENGINE_H

#include <sys/time.h>
#include <pthread.h>
#include "../globals.h"
#include "AudioOut.h"
#include "MidiIn.h"

class NulEngine:public AudioOut, MidiIn
{
    public:
        NulEngine();
        ~NulEngine();

        bool Start();
        void Stop();

        void setAudioEn(bool nval);
        bool getAudioEn() const;

        void setMidiEn(bool) {}
        bool getMidiEn() const {return true; }

    protected:
        void *AudioThread();
        static void *_AudioThread(void *arg);

    private:
        struct timeval playing_until;
        pthread_t     *pThread;
};

#endif
