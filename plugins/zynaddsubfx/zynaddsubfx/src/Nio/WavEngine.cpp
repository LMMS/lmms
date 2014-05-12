/*
  Copyright (C) 2006 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

#include "WavEngine.h"
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include "../Misc/WavFile.h"
#include "../Misc/Util.h"

using namespace std;

WavEngine::WavEngine()
    :AudioOut(), file(NULL), buffer(synth->samplerate * 4), pThread(NULL)
{
    sem_init(&work, PTHREAD_PROCESS_PRIVATE, 0);
}

WavEngine::~WavEngine()
{
    Stop();
    sem_destroy(&work);
    destroyFile();
}

bool WavEngine::openAudio()
{
    return file && file->good();
}

bool WavEngine::Start()
{
    if(pThread)
        return true;
    pThread = new pthread_t;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(pThread, &attr, _AudioThread, this);

    return true;
}

void WavEngine::Stop()
{
    if(!pThread)
        return;

    pthread_t *tmp = pThread;
    pThread = NULL;

    sem_post(&work);
    pthread_join(*tmp, NULL);
    delete pThread;
}

void WavEngine::push(Stereo<float *> smps, size_t len)
{
    if(!pThread)
        return;


    //copy the input [overflow when needed]
    for(size_t i = 0; i < len; ++i) {
        buffer.push(*smps.l++);
        buffer.push(*smps.r++);
    }
    sem_post(&work);
}

void WavEngine::newFile(WavFile *_file)
{
    //ensure system is clean
    destroyFile();
    file = _file;

    //check state
    if(!file->good())
        cerr
        << "ERROR: WavEngine handed bad file output WavEngine::newFile()"
        << endl;
}

void WavEngine::destroyFile()
{
    if(file)
        delete file;
    file = NULL;
}

void *WavEngine::_AudioThread(void *arg)
{
    return (static_cast<WavEngine *>(arg))->AudioThread();
}

void *WavEngine::AudioThread()
{
    short *recordbuf_16bit = new short[2 * synth->buffersize];

    while(!sem_wait(&work) && pThread) {
        for(int i = 0; i < synth->buffersize; ++i) {
            float left = 0.0f, right = 0.0f;
            buffer.pop(left);
            buffer.pop(right);
            recordbuf_16bit[2 * i] = limit((int)(left * 32767.0f),
                                           -32768,
                                           32767);
            recordbuf_16bit[2 * i + 1] = limit((int)(right * 32767.0f),
                                               -32768,
                                               32767);
        }
        file->writeStereoSamples(synth->buffersize, recordbuf_16bit);
    }

    delete[] recordbuf_16bit;

    return NULL;
}
