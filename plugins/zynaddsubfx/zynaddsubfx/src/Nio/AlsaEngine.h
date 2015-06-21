/*
  ZynAddSubFX - a software synthesizer
  AlsaEngine.h - ALSA Driver

  Copyright 2009, Alan Calvert
            2014, Mark McCurry

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

#ifndef ALSA_ENGINE_H
#define ALSA_ENGINE_H

#include <pthread.h>
#include <string>
#include <alsa/asoundlib.h>
#include <queue>

#include "AudioOut.h"
#include "MidiIn.h"
#include "OutMgr.h"
#include "../Misc/Stereo.h"

class AlsaEngine:public AudioOut, MidiIn
{
    public:
        AlsaEngine();
        ~AlsaEngine();

        bool Start();
        void Stop();

        void setAudioEn(bool nval);
        bool getAudioEn() const;
        void setMidiEn(bool nval);
        bool getMidiEn() const;

    protected:
        void *AudioThread();
        static void *_AudioThread(void *arg);
        void *MidiThread();
        static void *_MidiThread(void *arg);

    private:
        bool openMidi();
        void stopMidi();
        bool openAudio();
        void stopAudio();

        short *interleave(const Stereo<float *> &smps);

        struct {
            std::string device;
            snd_seq_t  *handle;
            int alsaId;
            pthread_t pThread;
        } midi;

        struct {
            snd_pcm_t *handle;
            snd_pcm_hw_params_t *params;
            unsigned int      sampleRate;
            snd_pcm_uframes_t frames;
            unsigned int      periods;
            short    *buffer;
            pthread_t pThread;
        } audio;

        void *processAudio();
};

#endif
