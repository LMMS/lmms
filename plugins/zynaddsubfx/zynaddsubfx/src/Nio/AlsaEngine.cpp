/*
    AlsaEngine.cpp

    Copyright 2009, Alan Calvert
              2010, Mark McCurry

    This file is part of ZynAddSubFX, which is free software: you can
    redistribute it and/or modify it under the terms of the GNU General
    Public License as published by the Free Software Foundation, either
    version 3 of the License, or (at your option) any later version.

    ZynAddSubFX is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ZynAddSubFX.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <cmath>

using namespace std;

#include "../Misc/Util.h"
#include "../Misc/Config.h"
#include "InMgr.h"
#include "AlsaEngine.h"

AlsaEngine::AlsaEngine()
    :AudioOut()
{
    audio.buffer = new short[synth->buffersize * 2];
    name = "ALSA";
    audio.handle = NULL;

    midi.handle  = NULL;
    midi.alsaId  = -1;
    midi.pThread = 0;
}

AlsaEngine::~AlsaEngine()
{
    Stop();
    delete[] audio.buffer;
}

void *AlsaEngine::_AudioThread(void *arg)
{
    return (static_cast<AlsaEngine *>(arg))->AudioThread();
}

void *AlsaEngine::AudioThread()
{
    set_realtime();
    return processAudio();
}

bool AlsaEngine::Start()
{
    return openAudio() && openMidi();
}

void AlsaEngine::Stop()
{
    if(getMidiEn())
        setMidiEn(false);
    if(getAudioEn())
        setAudioEn(false);
    snd_config_update_free_global();
}

void AlsaEngine::setMidiEn(bool nval)
{
    if(nval)
        openMidi();
    else
        stopMidi();
}

bool AlsaEngine::getMidiEn() const
{
    return midi.handle;
}

void AlsaEngine::setAudioEn(bool nval)
{
    if(nval)
        openAudio();
    else
        stopAudio();
}

bool AlsaEngine::getAudioEn() const
{
    return audio.handle;
}

void *AlsaEngine::_MidiThread(void *arg)
{
    return static_cast<AlsaEngine *>(arg)->MidiThread();
}


void *AlsaEngine::MidiThread(void)
{
    snd_seq_event_t *event;
    MidiEvent ev;
    set_realtime();
    while(snd_seq_event_input(midi.handle, &event) > 0) {
        //ensure ev is empty
        ev.channel = 0;
        ev.num     = 0;
        ev.value   = 0;
        ev.type    = 0;

        if(!event)
            continue;
        switch(event->type) {
            case SND_SEQ_EVENT_NOTEON:
                if(event->data.note.note) {
                    ev.type    = M_NOTE;
                    ev.channel = event->data.note.channel;
                    ev.num     = event->data.note.note;
                    ev.value   = event->data.note.velocity;
                    InMgr::getInstance().putEvent(ev);
                }
                break;

            case SND_SEQ_EVENT_NOTEOFF:
                ev.type    = M_NOTE;
                ev.channel = event->data.note.channel;
                ev.num     = event->data.note.note;
                ev.value   = 0;
                InMgr::getInstance().putEvent(ev);
                break;

            case SND_SEQ_EVENT_KEYPRESS:
                ev.type    = M_PRESSURE;
                ev.channel = event->data.note.channel;
                ev.num     = event->data.note.note;
                ev.value   = event->data.note.velocity;
                InMgr::getInstance().putEvent(ev);
                break;

            case SND_SEQ_EVENT_PITCHBEND:
                ev.type    = M_CONTROLLER;
                ev.channel = event->data.control.channel;
                ev.num     = C_pitchwheel;
                ev.value   = event->data.control.value;
                InMgr::getInstance().putEvent(ev);
                break;

            case SND_SEQ_EVENT_CONTROLLER:
                ev.type    = M_CONTROLLER;
                ev.channel = event->data.control.channel;
                ev.num     = event->data.control.param;
                ev.value   = event->data.control.value;
                InMgr::getInstance().putEvent(ev);
                break;

            case SND_SEQ_EVENT_PGMCHANGE:
                ev.type    = M_PGMCHANGE;
                ev.channel = event->data.control.channel;
                ev.num     = event->data.control.value;
                InMgr::getInstance().putEvent(ev);
                break;

            case SND_SEQ_EVENT_RESET: // reset to power-on state
                ev.type    = M_CONTROLLER;
                ev.channel = event->data.control.channel;
                ev.num     = C_resetallcontrollers;
                ev.value   = 0;
                InMgr::getInstance().putEvent(ev);
                break;

            case SND_SEQ_EVENT_PORT_SUBSCRIBED: // ports connected
                if(true)
                    cout << "Info, alsa midi port connected" << endl;
                break;

            case SND_SEQ_EVENT_PORT_UNSUBSCRIBED: // ports disconnected
                if(true)
                    cout << "Info, alsa midi port disconnected" << endl;
                break;

            case SND_SEQ_EVENT_SYSEX:   // system exclusive
            case SND_SEQ_EVENT_SENSING: // midi device still there
                break;

            default:
                if(true)
                    cout << "Info, other non-handled midi event, type: "
                         << (int)event->type << endl;
                break;
        }
        snd_seq_free_event(event);
    }
    return NULL;
}

bool AlsaEngine::openMidi()
{
    if(getMidiEn())
        return true;

    int alsaport;
    midi.handle = NULL;

    if(snd_seq_open(&midi.handle, "default", SND_SEQ_OPEN_INPUT, 0) != 0)
        return false;

    snd_seq_set_client_name(midi.handle, "ZynAddSubFX");

    alsaport = snd_seq_create_simple_port(
        midi.handle,
        "ZynAddSubFX",
        SND_SEQ_PORT_CAP_WRITE
        | SND_SEQ_PORT_CAP_SUBS_WRITE,
        SND_SEQ_PORT_TYPE_SYNTH);
    if(alsaport < 0)
        return false;

    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&midi.pThread, &attr, _MidiThread, this);
    return true;
}

void AlsaEngine::stopMidi()
{
    if(!getMidiEn())
        return;

    snd_seq_t *handle = midi.handle;
    if((NULL != midi.handle) && midi.pThread)
        pthread_cancel(midi.pThread);
    midi.handle = NULL;
    if(handle)
        snd_seq_close(handle);
}

short *AlsaEngine::interleave(const Stereo<float *> &smps)
{
    /**\todo TODO fix repeated allocation*/
    short *shortInterleaved = audio.buffer;
    memset(shortInterleaved, 0, bufferSize * 2 * sizeof(short));
    int    idx = 0; //possible off by one error here
    double scaled;
    for(int frame = 0; frame < bufferSize; ++frame) { // with a nod to libsamplerate ...
        scaled = smps.l[frame] * (8.0f * 0x10000000);
        shortInterleaved[idx++] = (short int)(lrint(scaled) >> 16);
        scaled = smps.r[frame] * (8.0f * 0x10000000);
        shortInterleaved[idx++] = (short int)(lrint(scaled) >> 16);
    }
    return shortInterleaved;
}

bool AlsaEngine::openAudio()
{
    if(getAudioEn())
        return true;

    int rc = 0;
    /* Open PCM device for playback. */
    audio.handle = NULL;
    rc = snd_pcm_open(&audio.handle, "hw:0",
                      SND_PCM_STREAM_PLAYBACK, 0);
    if(rc < 0) {
        fprintf(stderr,
                "unable to open pcm device: %s\n",
                snd_strerror(rc));
        return false;
    }

    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca(&audio.params);

    /* Fill it in with default values. */
    snd_pcm_hw_params_any(audio.handle, audio.params);

    /* Set the desired hardware parameters. */

    /* Interleaved mode */
    snd_pcm_hw_params_set_access(audio.handle, audio.params,
                                 SND_PCM_ACCESS_RW_INTERLEAVED);

    /* Signed 16-bit little-endian format */
    snd_pcm_hw_params_set_format(audio.handle, audio.params,
                                 SND_PCM_FORMAT_S16_LE);

    /* Two channels (stereo) */
    snd_pcm_hw_params_set_channels(audio.handle, audio.params, 2);

    audio.sampleRate = synth->samplerate;
    snd_pcm_hw_params_set_rate_near(audio.handle, audio.params,
                                    &audio.sampleRate, NULL);

    audio.frames = 512;
    snd_pcm_hw_params_set_period_size_near(audio.handle,
                                           audio.params, &audio.frames, NULL);

    audio.periods = 4;
    snd_pcm_hw_params_set_periods_near(audio.handle,
                                       audio.params, &audio.periods, NULL);

    /* Write the parameters to the driver */
    rc = snd_pcm_hw_params(audio.handle, audio.params);
    if(rc < 0) {
        fprintf(stderr,
                "unable to set hw parameters: %s\n",
                snd_strerror(rc));
        return false;
    }

    /* Set buffer size (in frames). The resulting latency is given by */
    /* latency = periodsize * periods / (rate * bytes_per_frame)     */
    snd_pcm_hw_params_set_buffer_size(audio.handle,
                                      audio.params,
                                      synth->buffersize);

    //snd_pcm_hw_params_get_period_size(audio.params, &audio.frames, NULL);
    //snd_pcm_hw_params_get_period_time(audio.params, &val, NULL);


    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&audio.pThread, &attr, _AudioThread, this);
    return true;
}

void AlsaEngine::stopAudio()
{
    if(!getAudioEn())
        return;

    snd_pcm_t *handle = audio.handle;
    audio.handle = NULL;
    pthread_join(audio.pThread, NULL);
    snd_pcm_drain(handle);
    if(snd_pcm_close(handle))
        cout << "Error: in snd_pcm_close " << __LINE__ << ' ' << __FILE__
             << endl;
}

void *AlsaEngine::processAudio()
{
    while(audio.handle) {
        audio.buffer = interleave(getNext());
        snd_pcm_t *handle = audio.handle;
        int rc = snd_pcm_writei(handle, audio.buffer, synth->buffersize);
        if(rc == -EPIPE) {
            /* EPIPE means underrun */
            cerr << "underrun occurred" << endl;
            snd_pcm_prepare(handle);
        }
        else
        if(rc < 0)
            cerr << "error from writei: " << snd_strerror(rc) << endl;
    }
    return NULL;
}
