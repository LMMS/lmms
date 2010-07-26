/*
  ZynAddSubFX - a software synthesizer

  Sample.cpp - Object for storing information on samples
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
#include <cstring>//for memcpy/memset
#include <iostream>
#include "Sample.h"

using namespace std;

#warning TODO Think about renaming Sample to Frame
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

/**
 * Linear point estimation
 * @param ya Y of point a
 * @param yb Y of point b
 * @param xt X of test point
 * @param xa X of point a
 * @param xb X of point b
 * @return estimated Y of test point
 */
inline float linearEstimate(float ya, float yb, float xt, int xa = 0, int xb = 1)
{
    if(xa == xb)
        return ya;

    return (yb-ya) * (xt-xa)/(xb-xa) + ya;
}

void Sample::resize(unsigned int nsize)
{
    if(bufferSize == nsize)
        return;
    else {//resampling occurs here
        float ratio = (nsize * 1.0) / (bufferSize * 1.0);

        int    nBufferSize = nsize;
        float *nBuffer     = new float[nBufferSize];

        //take care of edge cases
        *nBuffer = *buffer;
        *(nBuffer+nBufferSize-1) = *(buffer+bufferSize-1);

        //addition is done to avoid 0 edge case
        for(int i = 1; i < nBufferSize - 1; ++i)
        {
            float left  = floor(i/ratio);
            float right = ceil((i+1)/ratio);
            float test  = i/ratio;
            if(left > bufferSize - 1)
                left = bufferSize - 1;
            if(right > bufferSize - 1)
                right = bufferSize - 1;
            if(left > test)
                test = left;
            nBuffer[i] = linearEstimate(buffer[(int)left],
                                        buffer[(int)right],
                                        test, (int)left, (int)right);
        }

        //put the new data in
        delete[] buffer;
        buffer     = nBuffer;
        bufferSize = nBufferSize;
    }
}

void Sample::append(const Sample &smp)
{
    int nbufferSize = bufferSize + smp.bufferSize;
    float *nbuffer  = new float[nbufferSize];

    memcpy(nbuffer, buffer, bufferSize * sizeof(float));
    memcpy(nbuffer + bufferSize, smp.buffer, smp.bufferSize * sizeof(float));
    delete buffer;

    buffer     = nbuffer;
    bufferSize = nbufferSize;
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

