/*
  ZynAddSubFX - a software synthesizer

  JackMultiEngine.h - Channeled Audio output JACK
  Copyright (C) 2012-2012 Mark McCurry
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
#ifndef JACK_MULTI_ENGINE
#define JACK_MULTI_ENGINE

#include "AudioOut.h"

class JackMultiEngine:public AudioOut
{
    public:
        JackMultiEngine(void);
        ~JackMultiEngine(void);

        void setAudioEn(bool nval);
        bool getAudioEn() const;

        bool Start(void);
        void Stop(void);

    private:
        static int _processCallback(unsigned nframes, void *arg);
        int processAudio(unsigned nframes);

        struct jack_multi *impl;
};

#endif
