/*
  ZynAddSubFX - a software synthesizer

  OSSaudiooutput.C - Audio output for Open Sound System
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

#include "OssEngine.h"
#include "../Misc/Util.h"
#include "../globals.h"

#include <cstring>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>

#include "InMgr.h"

using namespace std;

OssEngine::OssEngine()
    :AudioOut(), engThread(NULL)
{
    name = "OSS";

    midi.handle  = -1;
    audio.handle = -1;

    audio.smps = new short[synth->buffersize * 2];
    memset(audio.smps, 0, synth->bufferbytes);
}

OssEngine::~OssEngine()
{
    Stop();
    delete [] audio.smps;
}

bool OssEngine::openAudio()
{
    if(audio.handle != -1)
        return true;  //already open

    int snd_bitsize    = 16;
    int snd_fragment   = 0x00080009; //fragment size (?);
    int snd_stereo     = 1; //stereo;
    int snd_format     = AFMT_S16_LE;
    int snd_samplerate = synth->samplerate;

    const char *device = config.cfg.LinuxOSSWaveOutDev;
    if(getenv("DSP_DEVICE"))
        device = getenv("DSP_DEVICE");

    audio.handle = open(device, O_WRONLY, 0);
    if(audio.handle == -1) {
        cerr << "ERROR - I can't open the "
             << device << '.' << endl;
        return false;
    }
    ioctl(audio.handle, SNDCTL_DSP_RESET, NULL);
    ioctl(audio.handle, SNDCTL_DSP_SETFMT, &snd_format);
    ioctl(audio.handle, SNDCTL_DSP_STEREO, &snd_stereo);
    ioctl(audio.handle, SNDCTL_DSP_SPEED, &snd_samplerate);
    ioctl(audio.handle, SNDCTL_DSP_SAMPLESIZE, &snd_bitsize);
    ioctl(audio.handle, SNDCTL_DSP_SETFRAGMENT, &snd_fragment);

    if(!getMidiEn()) {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        engThread = new pthread_t;
        pthread_create(engThread, &attr, _thread, this);
    }

    return true;
}

void OssEngine::stopAudio()
{
    int handle = audio.handle;
    if(handle == -1) //already closed
        return;
    audio.handle = -1;

    if(!getMidiEn() && engThread)
        pthread_join(*engThread, NULL);
    delete engThread;
    engThread = NULL;

    close(handle);
}

bool OssEngine::Start()
{
    bool good = true;

    if(!openAudio()) {
        cerr << "Failed to open OSS audio" << endl;
        good = false;
    }

    if(!openMidi()) {
        cerr << "Failed to open OSS midi" << endl;
        good = false;
    }

    return good;
}

void OssEngine::Stop()
{
    stopAudio();
    stopMidi();
}

void OssEngine::setMidiEn(bool nval)
{
    if(nval)
        openMidi();
    else
        stopMidi();
}

bool OssEngine::getMidiEn() const
{
    return midi.handle != -1;
}

void OssEngine::setAudioEn(bool nval)
{
    if(nval)
        openAudio();
    else
        stopAudio();
}

bool OssEngine::getAudioEn() const
{
    return audio.handle != -1;
}

bool OssEngine::openMidi()
{
    int handle = midi.handle;
    if(handle != -1)
        return true;  //already open

    handle = open(config.cfg.LinuxOSSSeqInDev, O_RDONLY, 0);

    if(-1 == handle)
        return false;
    midi.handle = handle;

    if(!getAudioEn()) {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        engThread = new pthread_t;
        pthread_create(engThread, &attr, _thread, this);
    }

    return true;
}

void OssEngine::stopMidi()
{
    int handle = midi.handle;
    if(handle == -1) //already closed
        return;

    midi.handle = -1;

    if(!getAudioEn() && engThread) {
        pthread_join(*engThread, NULL);
        delete engThread;
        engThread = NULL;
    }

    close(handle);
}

void *OssEngine::_thread(void *arg)
{
    return (static_cast<OssEngine *>(arg))->thread();
}

void *OssEngine::thread()
{
    unsigned char tmp[4] = {0, 0, 0, 0};
    set_realtime();
    while(getAudioEn() || getMidiEn()) {
        if(getAudioEn()) {
            const Stereo<float *> smps = getNext();

            float l, r;
            for(int i = 0; i < synth->buffersize; ++i) {
                l = smps.l[i];
                r = smps.r[i];

                if(l < -1.0f)
                    l = -1.0f;
                else
                if(l > 1.0f)
                    l = 1.0f;
                if(r < -1.0f)
                    r = -1.0f;
                else
                if(r > 1.0f)
                    r = 1.0f;

                audio.smps[i * 2]     = (short int) (l * 32767.0f);
                audio.smps[i * 2 + 1] = (short int) (r * 32767.0f);
            }
            int handle = audio.handle;
            if(handle != -1)
                write(handle, audio.smps, synth->buffersize * 4);  // *2 because is 16 bit, again * 2 because is stereo
            else
                break;
        }

        //Collect up to 30 midi events
        for(int k = 0; k < 30 && getMidiEn(); ++k) {
            static char escaped;

            memset(tmp, 0, 4);

            if(escaped) {
                tmp[0]  = escaped;
                escaped = 0;
            }
            else {
                getMidi(tmp);
                if(!(tmp[0] & 0x80))
                    continue;
            }
            getMidi(tmp + 1);
            if(tmp[1] & 0x80) {
                escaped = tmp[1];
                tmp[1]  = 0;
            }
            else {
                getMidi(tmp + 2);
                if(tmp[2] & 0x80) {
                    escaped = tmp[2];
                    tmp[2]  = 0;
                }
                else {
                    getMidi(tmp + 3);
                    if(tmp[3] & 0x80) {
                        escaped = tmp[3];
                        tmp[3]  = 0;
                    }
                }
            }
            midiProcess(tmp[0], tmp[1], tmp[2]);
        }
    }
    pthread_exit(NULL);
    return NULL;
}

void OssEngine::getMidi(unsigned char *midiPtr)
{
    read(midi.handle, midiPtr, 1);
}
