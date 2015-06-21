/*
   ZynAddSubFX - a software synthesizer

   OssMultiEngine.cpp - Multi channel audio output for Open Sound System
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

#include <cstring>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/soundcard.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>
#include <signal.h>

#include "Nio.h"
#include "../Misc/Master.h"
#include "../Misc/Part.h"
#include "../Misc/MiddleWare.h"
#include "../Misc/Util.h"

#include "OssMultiEngine.h"

extern MiddleWare *middleware;

using namespace std;

OssMultiEngine :: OssMultiEngine()
{
    /* setup variables */
    name = "OSS-MULTI";
    audioThread = 0;
    handle = -1;
    channels = 0;
    en = false;
    is32bit = false;
    buffersize = 0;

    /* compute worst case buffer size */
    maxbuffersize = NUM_MIDI_PARTS * sizeof(int) * synth->buffersize * 2;
    /* allocate buffer */
    smps.ps32 = new int[maxbuffersize / sizeof(int)];
    memset(smps.ps32, 0, maxbuffersize);
}

OssMultiEngine :: ~OssMultiEngine()
{
    Stop();
    delete [] smps.ps32;
}

    bool
OssMultiEngine :: openAudio()
{
    int snd_samplerate;
    int snd_fragment;
    int x;

    /* check if already open */
    if (handle != -1)
        return (true);

    const char *device = getenv("DSP_DEVICE");
    if(device == 0)
        device = config.cfg.LinuxOSSWaveOutDev;

    /* NOTE: PIPEs and FIFOs can block when opening them */
    handle = open(device, O_WRONLY, O_NONBLOCK);
    if (handle == -1) {
        cerr << "ERROR - I can't open the "
            << device << '.' << endl;
        return (false);
    }
    ioctl(handle, SNDCTL_DSP_RESET, 0);

    /* Figure out the correct format first */
    int snd_format16 = AFMT_S16_NE;

#ifdef AFMT_S32_NE
    int snd_format32 = AFMT_S32_NE;
    if (ioctl(handle, SNDCTL_DSP_SETFMT, &snd_format32) == 0) {
        is32bit = true;
    } else
#endif
    if (ioctl(handle, SNDCTL_DSP_SETFMT, &snd_format16) == 0) {
        is32bit = false;
    } else {
        cerr << "ERROR - Cannot set DSP format for "
            << device << '.' << endl;
        goto error;
    }
    for (x = NUM_MIDI_PARTS * 2; x >= 2; x -= 2) {
        if (ioctl(handle, SNDCTL_DSP_CHANNELS, &x) == 0)
            break;
    }
    if (x == 0) {
        cerr << "ERROR - Cannot set DSP channels for "
            << device << '.' << endl;
        goto error;
    }
    channels = x;

    snd_samplerate = synth->samplerate;

    ioctl(handle, SNDCTL_DSP_SPEED, &snd_samplerate);

    if (snd_samplerate != (int)synth->samplerate) {
        cerr << "ERROR - Cannot set samplerate for "
            << device << ". " << snd_samplerate
            << " != " << synth->samplerate << endl;
        goto error;
    }

    /* compute buffer size for 16-bit samples */
    buffersize = 2 * synth->buffersize * channels;
    if (is32bit)
        buffersize *= 2;

    for (x = 4; x < 20; x++) {
        if ((1 << x) >= buffersize)
            break;
    }

    snd_fragment = 0x20000 | x;		/* 2x buffer */

    ioctl(handle, SNDCTL_DSP_SETFRAGMENT, &snd_fragment);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&audioThread, &attr, _audioThreadCb, this);

    return (true);

error:
    close(handle);
    handle = -1;
    return (false);
}

    void
OssMultiEngine :: stopAudio()
{
    int fd = handle;

    /* check if already closed */
    if (fd == -1)
        return;
    handle = -1;

    /* close handle first, so that write() exits */
    close(fd);

    pthread_join(audioThread, 0);
    audioThread = 0;
}

    bool
OssMultiEngine :: Start()
{
    return (openAudio());
}

    void
OssMultiEngine :: Stop()
{
    stopAudio();
}

    void
OssMultiEngine :: setAudioEn(bool enable)
{
    if (enable)
        openAudio();
    else
        stopAudio();
}

bool
OssMultiEngine :: getAudioEn() const
{
    return (handle != -1);
}

    void *
OssMultiEngine :: _audioThreadCb(void *arg)
{
    return (static_cast<OssMultiEngine *>(arg))->audioThreadCb();
}

    void *
OssMultiEngine :: audioThreadCb()
{
    /*
     * In case the audio device is a PIPE/FIFO, we need to ignore
     * any PIPE signals:
     */
    signal(SIGPIPE, SIG_IGN);

    set_realtime();

    while(getAudioEn()) {
        int error;
        float l;
        float r;
        int x;
        int y;

        /* get next buffer */
        getNext();

        /* extract audio from the "channels / 2" first parts */
        for (x = 0; x != channels; x += 2) {
            Part *part = middleware->spawnMaster()->part[x / 2];

            if (is32bit) {
                for (y = 0; y != synth->buffersize; y++) {
                    l = part->partoutl[y];
                    if (l < -1.0f)
                        l = -1.0f;
                    else if (l > 1.0f)
                        l = 1.0f;
                    smps.ps32[y * channels + x] = (int)(l * 2147483647.0f);
                    r = part->partoutr[y];
                    if (r < -1.0f)
                        r = -1.0f;
                    else if (r > 1.0f)
                        r = 1.0f;
                    smps.ps32[y * channels + x + 1] = (int)(r * 2147483647.0f);
                }
            } else {
                for (y = 0; y != synth->buffersize; y++) {
                    l = part->partoutl[y];
                    if (l < -1.0f)
                        l = -1.0f;
                    else if (l > 1.0f)
                        l = 1.0f;
                    smps.ps16[y * channels + x] = (short int)(l * 32767.0f);
                    r = part->partoutr[y];
                    if (r < -1.0f)
                        r = -1.0f;
                    else if (r > 1.0f)
                        r = 1.0f;
                    smps.ps16[y * channels + x + 1] = (short int)(r * 32767.0f);
                }
            }
        }

        /* write audio buffer to DSP device */
        do {
            /* make a copy of handle, in case of OSS audio disable */
            int fd = handle;
            if (fd == -1)
                goto done;
            error = write(fd, smps.ps32, buffersize);
        } while (error == -1 && errno == EINTR);

        if(error == -1)
            goto done;
    }
done:
    pthread_exit(0);
    return (0);
}
