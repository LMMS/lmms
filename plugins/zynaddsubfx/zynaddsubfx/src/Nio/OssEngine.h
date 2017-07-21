/*
  ZynAddSubFX - a software synthesizer

  OSSaudiooutput.h - Audio output for Open Sound System
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

#ifndef OSS_ENGINE_H
#define OSS_ENGINE_H

#include <sys/time.h>
#include "../globals.h"
#include "AudioOut.h"
#include "MidiIn.h"

class OssEngine:public AudioOut, MidiIn
{
    public:
        OssEngine();
        ~OssEngine();

        bool Start();
        void Stop();

        void setAudioEn(bool nval);
        bool getAudioEn() const;

        void setMidiEn(bool nval);
        bool getMidiEn() const;


    protected:
        void *thread();
        static void *_thread(void *arg);

    private:
        pthread_t *engThread;

        //Audio
        bool openAudio();
        void stopAudio();

        struct audio {
            int handle;
            short int *smps; //Samples to be sent to soundcard
            bool en;
        } audio;

        //Midi
        bool openMidi();
        void stopMidi();
        void getMidi(unsigned char *midiPtr);

        struct midi {
            int  handle;
            bool en;
            bool run;
        } midi;
};

#endif
