/*
  ZynAddSubFX - a software synthesizer

  Sample.C - Object for storing information on samples
  Copyright (C) 2009-2009 Mark McCurry
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
#include <cmath>
#include "Sample.h"

/**\TODO start using pointer math here as these will be Frequency called
 * functions throughout the code*/
Sample::Sample(const Sample &smp)
    :bufferSize(smp.bufferSize)
{
    buffer = new REALTYPE[bufferSize];
    for(int i = 0; i < bufferSize; ++i)
        *(i + buffer) = *(i + smp.buffer);
}

Sample::Sample(int length, REALTYPE fill)
    :bufferSize(length)
{
    if(length < 1)
        bufferSize = 1;
    buffer = new REALTYPE[bufferSize];
    for(int i = 0; i < bufferSize; ++i)
        buffer[i] = fill;
}

Sample::Sample(int length, const REALTYPE *input)
    :bufferSize(length)
{
    if(length > 0) {
        buffer = new REALTYPE[length];
        for(int i = 0; i < length; ++i)
            *(buffer + i) = *(input + i);
    }
    else {
        buffer     = new REALTYPE[1];
        bufferSize = 1;
        *buffer    = 0;
    }
}

Sample::~Sample()
{
    delete[] buffer;
}

void Sample::clear()
{
    for(int i = 0; i < bufferSize; ++i)
        *(i + buffer) = 0;
}

void Sample::operator=(const Sample &smp)
{
    /**\todo rewrite to be less repetitive*/
    if(bufferSize == smp.bufferSize)
        for(int i = 0; i < bufferSize; ++i)
            *(i + buffer) = *(i + smp.buffer);
    else {
        delete[] buffer;
        buffer     = new REALTYPE[smp.bufferSize];
        bufferSize = smp.bufferSize;
        for(int i = 0; i < bufferSize; ++i)
            *(i + buffer) = *(i + smp.buffer);
    }
}

bool Sample::operator==(const Sample &smp) const
{
    if(this->bufferSize != smp.bufferSize)
        return false;
    for(int i = 0; i < bufferSize; ++i)
        if(this->buffer[i] != smp.buffer[i])
            return false;
    return true;
}

REALTYPE Sample::max() const
{
    REALTYPE max = -1500; //a good low considering that samples should store values -1.0 to 1.0
    for(int i = 0; i < bufferSize; ++i)
        if(buffer[i] > max)
            max = buffer[i];
    return max;
}

REALTYPE Sample::min() const
{
    REALTYPE min = 1500; //a good high considering that samples should store values -1.0 to 1.0
    for(int i = 0; i < bufferSize; ++i)
        if(buffer[i] < min)
            min = buffer[i];
    return min;
}

REALTYPE Sample::absMax() const
{
    REALTYPE max = 0;
    for(int i = 0; i < bufferSize; ++i)
        if(fabs(buffer[i]) > max)
            max = fabs(buffer[i]);
    return max;
}

