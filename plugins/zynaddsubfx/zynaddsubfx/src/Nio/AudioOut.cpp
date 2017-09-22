/*
  ZynAddSubFX - a software synthesizer

  AudioOut.h - Audio Output superclass
  Copyright (C) 2009-2010 Mark McCurry
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

#include <iostream>
#include <cstring>
#include "SafeQueue.h"

using namespace std;

#include "OutMgr.h"
#include "../Misc/Master.h"
#include "AudioOut.h"

AudioOut::AudioOut()
    :samplerate(synth->samplerate), bufferSize(synth->buffersize)
{}

AudioOut::~AudioOut()
{}

void AudioOut::setSamplerate(int _samplerate)
{
    samplerate = _samplerate;
}

int AudioOut::getSampleRate()
{
    return samplerate;
}

void AudioOut::setBufferSize(int _bufferSize)
{
    bufferSize = _bufferSize;
}

const Stereo<float *> AudioOut::getNext()
{
    return OutMgr::getInstance().tick(bufferSize);
}
