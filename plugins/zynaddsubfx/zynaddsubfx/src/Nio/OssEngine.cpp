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
#include <errno.h>
#include <sys/soundcard.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>
#include <signal.h>

#include "InMgr.h"

using namespace std;

/*
 * The following statemachine converts MIDI commands to USB MIDI
 * packets, derived from Linux's usbmidi.c, which was written by
 * "Clemens Ladisch". It is used to figure out when a MIDI command is
 * complete, without having to read the first byte of the next MIDI
 * command. This is useful when connecting to so-called system PIPEs
 * and FIFOs. See "man mkfifo".
 *
 * Return values:
 *    0: No command
 * Else: Command is complete
 */
static unsigned char
OssMidiParse(struct OssMidiParse &midi_parse,
    unsigned char cn, unsigned char b)
{
        unsigned char p0 = (cn << 4);

        if(b >= 0xf8) {
                midi_parse.temp_0[0] = p0 | 0x0f;
                midi_parse.temp_0[1] = b;
                midi_parse.temp_0[2] = 0;
                midi_parse.temp_0[3] = 0;
                midi_parse.temp_cmd = midi_parse.temp_0;
                return (1);

        } else if(b >= 0xf0) {
                switch (b) {
                case 0xf0:              /* system exclusive begin */
                        midi_parse.temp_1[1] = b;
                        midi_parse.state = OSSMIDI_ST_SYSEX_1;
                        break;
                case 0xf1:              /* MIDI time code */
                case 0xf3:              /* song select */
                        midi_parse.temp_1[1] = b;
                        midi_parse.state = OSSMIDI_ST_1PARAM;
                        break;
                case 0xf2:              /* song position pointer */
                        midi_parse.temp_1[1] = b;
                        midi_parse.state = OSSMIDI_ST_2PARAM_1;
                        break;
                case 0xf4:              /* unknown */
                case 0xf5:              /* unknown */
                        midi_parse.state = OSSMIDI_ST_UNKNOWN;
                        break;
                case 0xf6:              /* tune request */
                        midi_parse.temp_1[0] = p0 | 0x05;
                        midi_parse.temp_1[1] = 0xf6;
                        midi_parse.temp_1[2] = 0;
                        midi_parse.temp_1[3] = 0;
                        midi_parse.temp_cmd = midi_parse.temp_1;
                        midi_parse.state = OSSMIDI_ST_UNKNOWN;
                        return (1);

                case 0xf7:              /* system exclusive end */
                        switch (midi_parse.state) {
                        case OSSMIDI_ST_SYSEX_0:
                                midi_parse.temp_1[0] = p0 | 0x05;
                                midi_parse.temp_1[1] = 0xf7;
                                midi_parse.temp_1[2] = 0;
                                midi_parse.temp_1[3] = 0;
                                midi_parse.temp_cmd = midi_parse.temp_1;
                                midi_parse.state = OSSMIDI_ST_UNKNOWN;
                                return (1);
                        case OSSMIDI_ST_SYSEX_1:
                                midi_parse.temp_1[0] = p0 | 0x06;
                                midi_parse.temp_1[2] = 0xf7;
                                midi_parse.temp_1[3] = 0;
                                midi_parse.temp_cmd = midi_parse.temp_1;
                                midi_parse.state = OSSMIDI_ST_UNKNOWN;
                                return (1);
                        case OSSMIDI_ST_SYSEX_2:
                                midi_parse.temp_1[0] = p0 | 0x07;
                                midi_parse.temp_1[3] = 0xf7;
                                midi_parse.temp_cmd = midi_parse.temp_1;
                                midi_parse.state = OSSMIDI_ST_UNKNOWN;
                                return (1);
                        }
                        midi_parse.state = OSSMIDI_ST_UNKNOWN;
                        break;
                }
        } else if(b >= 0x80) {
                midi_parse.temp_1[1] = b;
                if((b >= 0xc0) && (b <= 0xdf)) {
                        midi_parse.state = OSSMIDI_ST_1PARAM;
                } else {
                        midi_parse.state = OSSMIDI_ST_2PARAM_1;
                }
        } else {                        /* b < 0x80 */
                switch (midi_parse.state) {
                case OSSMIDI_ST_1PARAM:
                        if(midi_parse.temp_1[1] < 0xf0) {
                                p0 |= midi_parse.temp_1[1] >> 4;
                        } else {
                                p0 |= 0x02;
                                midi_parse.state = OSSMIDI_ST_UNKNOWN;
                        }
                        midi_parse.temp_1[0] = p0;
                        midi_parse.temp_1[2] = b;
                        midi_parse.temp_1[3] = 0;
                        midi_parse.temp_cmd = midi_parse.temp_1;
                        return (1);
                case OSSMIDI_ST_2PARAM_1:
                        midi_parse.temp_1[2] = b;
                        midi_parse.state = OSSMIDI_ST_2PARAM_2;
                        break;
                case OSSMIDI_ST_2PARAM_2:
                        if(midi_parse.temp_1[1] < 0xf0) {
                                p0 |= midi_parse.temp_1[1] >> 4;
                                midi_parse.state = OSSMIDI_ST_2PARAM_1;
                        } else {
                                p0 |= 0x03;
                                midi_parse.state = OSSMIDI_ST_UNKNOWN;
                        }
                        midi_parse.temp_1[0] = p0;
                        midi_parse.temp_1[3] = b;
                        midi_parse.temp_cmd = midi_parse.temp_1;
                        return (1);
                case OSSMIDI_ST_SYSEX_0:
                        midi_parse.temp_1[1] = b;
                        midi_parse.state = OSSMIDI_ST_SYSEX_1;
                        break;
                case OSSMIDI_ST_SYSEX_1:
                        midi_parse.temp_1[2] = b;
                        midi_parse.state = OSSMIDI_ST_SYSEX_2;
                        break;
                case OSSMIDI_ST_SYSEX_2:
                        midi_parse.temp_1[0] = p0 | 0x04;
                        midi_parse.temp_1[3] = b;
                        midi_parse.temp_cmd = midi_parse.temp_1;
                        midi_parse.state = OSSMIDI_ST_SYSEX_0;
                        return (1);
                default:
                        break;
                }
        }
        return (0);
}

OssEngine::OssEngine()
    :AudioOut(), audioThread(NULL), midiThread(NULL)
{
    name = "OSS";

    midi.handle  = -1;
    audio.handle = -1;

    /* allocate worst case audio buffer */
    audio.smps.ps32 = new int[synth->buffersize * 2];
    memset(audio.smps.ps32, 0, sizeof(int) * synth->buffersize * 2);
    memset(&midi.state, 0, sizeof(midi.state));
}

OssEngine::~OssEngine()
{
    Stop();
    delete [] audio.smps.ps32;
}

bool OssEngine::openAudio()
{
    int x;

    if(audio.handle != -1)
        return true;  //already open

    int snd_fragment;
    int snd_stereo     = 1; //stereo;
    int snd_samplerate = synth->samplerate;

    const char *device = getenv("DSP_DEVICE");
    if(device == NULL)
        device = config.cfg.LinuxOSSWaveOutDev;

    /* NOTE: PIPEs and FIFOs can block when opening them */
    audio.handle = open(device, O_WRONLY, O_NONBLOCK);
    if(audio.handle == -1) {
        cerr << "ERROR - I can't open the "
             << device << '.' << endl;
        return false;
    }
    ioctl(audio.handle, SNDCTL_DSP_RESET, NULL);

    /* Figure out the correct format first */

    int snd_format16 = AFMT_S16_NE;

#ifdef AFMT_S32_NE
    int snd_format32 = AFMT_S32_NE;
    if (ioctl(audio.handle, SNDCTL_DSP_SETFMT, &snd_format32) == 0) {
        audio.is32bit = true;
    } else
#endif
    if (ioctl(audio.handle, SNDCTL_DSP_SETFMT, &snd_format16) == 0) {
        audio.is32bit = false;
    } else {
        cerr << "ERROR - I cannot set DSP format for "
            << device << '.' << endl;
        goto error;
    }
    ioctl(audio.handle, SNDCTL_DSP_STEREO, &snd_stereo);
    ioctl(audio.handle, SNDCTL_DSP_SPEED, &snd_samplerate);

    if (snd_samplerate != (int)synth->samplerate) {
        cerr << "ERROR - Cannot set samplerate for "
             << device << ". " << snd_samplerate
             << " != " << synth->samplerate << endl;
        goto error;
    }

    /* compute buffer size for 16-bit stereo samples */
    audio.buffersize = 4 * synth->buffersize;
    if (audio.is32bit)
        audio.buffersize *= 2;

    for (x = 4; x < 20; x++) {
        if ((1 << x) >= audio.buffersize)
                break;
    }

    snd_fragment = 0x20000 | x;         /* 2x buffer */

    ioctl(audio.handle, SNDCTL_DSP_SETFRAGMENT, &snd_fragment);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    audioThread = new pthread_t;
    pthread_create(audioThread, &attr, _audioThreadCb, this);

    return true;

error:
    close(audio.handle);
    audio.handle = -1;
    return false;
}

void OssEngine::stopAudio()
{
    int handle = audio.handle;
    if(handle == -1) //already closed
        return;
    audio.handle = -1;

    /* close handle first, so that write() exits */
    close(handle);

    pthread_join(*audioThread, NULL);
    delete audioThread;
    audioThread = NULL;
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

    const char *device = getenv("MIDI_DEVICE");
    if(device == NULL)
        device = config.cfg.LinuxOSSSeqInDev;

    /* NOTE: PIPEs and FIFOs can block when opening them */
    handle = open(device, O_RDONLY, O_NONBLOCK);

    if(-1 == handle)
        return false;
    midi.handle = handle;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    midiThread = new pthread_t;
    pthread_create(midiThread, &attr, _midiThreadCb, this);

    return true;
}

void OssEngine::stopMidi()
{
    int handle = midi.handle;
    if(handle == -1) //already closed
        return;

    midi.handle = -1;

    /* close handle first, so that read() exits */
    close(handle);

    pthread_join(*midiThread, NULL);
    delete midiThread;
    midiThread = NULL;
}

void *OssEngine::_audioThreadCb(void *arg)
{
    return (static_cast<OssEngine *>(arg))->audioThreadCb();
}

void *OssEngine::_midiThreadCb(void *arg)
{
    return (static_cast<OssEngine *>(arg))->midiThreadCb();
}

void *OssEngine::audioThreadCb()
{
    /*
     * In case the audio device is a PIPE/FIFO,
     * we need to ignore any PIPE signals:
     */
    signal(SIGPIPE, SIG_IGN);

    set_realtime();
    while(getAudioEn()) {
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

            if (audio.is32bit) {
                audio.smps.ps32[i * 2]     = (int) (l * 2147483647.0f);
                audio.smps.ps32[i * 2 + 1] = (int) (r * 2147483647.0f);
            } else {/* 16bit */
                audio.smps.ps16[i * 2]     = (short int) (l * 32767.0f);
                audio.smps.ps16[i * 2 + 1] = (short int) (r * 32767.0f);
            }
        }

        int error;
        do {
            /* make a copy of handle, in case of OSS audio disable */
            int handle = audio.handle;
            if(handle == -1)
                goto done;
            error = write(handle, audio.smps.ps32, audio.buffersize);
        } while (error == -1 && errno == EINTR);

        if(error == -1)
            goto done;
    }
done:
    pthread_exit(NULL);
    return NULL;
}

void *OssEngine::midiThreadCb()
{
    /*
     * In case the MIDI device is a PIPE/FIFO,
     * we need to ignore any PIPE signals:
     */
    signal(SIGPIPE, SIG_IGN);
    set_realtime();
    while(getMidiEn()) {
        unsigned char tmp;
        int error;
        do {
            /* make a copy of handle, in case of OSS MIDI disable */
            int handle = midi.handle;
            if(handle == -1)
                goto done;
            error = read(handle, &tmp, 1);
        } while (error == -1 && errno == EINTR);

        /* check that we got one byte */
        if(error != 1)
                goto done;

        /* feed MIDI byte into statemachine */
        if(OssMidiParse(midi.state, 0, tmp)) {
            /* we got a complete MIDI command */
            midiProcess(midi.state.temp_cmd[1],
                midi.state.temp_cmd[2],
                midi.state.temp_cmd[3]);
        }
    }
done:
    pthread_exit(NULL);
    return NULL;
}
