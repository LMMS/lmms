/*
  ZynAddSubFX - a software synthesizer

  WaveShapeSmps.cpp - Sample Distortion
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

#include "WaveShapeSmps.h"
#include <cmath>

void waveShapeSmps(int n,
                   float *smps,
                   unsigned char type,
                   unsigned char drive)
{
    int   i;
    float ws = drive / 127.0f;
    float tmpv;

    switch(type) {
        case 1:
            ws = powf(10, ws * ws * 3.0f) - 1.0f + 0.001f; //Arctangent
            for(i = 0; i < n; ++i)
                smps[i] = atanf(smps[i] * ws) / atanf(ws);
            break;
        case 2:
            ws = ws * ws * 32.0f + 0.0001f; //Asymmetric
            if(ws < 1.0f)
                tmpv = sinf(ws) + 0.1f;
            else
                tmpv = 1.1f;
            for(i = 0; i < n; ++i)
                smps[i] = sinf(smps[i] * (0.1f + ws - ws * smps[i])) / tmpv;
            ;
            break;
        case 3:
            ws = ws * ws * ws * 20.0f + 0.0001f; //Pow
            for(i = 0; i < n; ++i) {
                smps[i] *= ws;
                if(fabs(smps[i]) < 1.0f) {
                    smps[i] = (smps[i] - powf(smps[i], 3.0f)) * 3.0f;
                    if(ws < 1.0f)
                        smps[i] /= ws;
                }
                else
                    smps[i] = 0.0f;
            }
            break;
        case 4:
            ws = ws * ws * ws * 32.0f + 0.0001f; //Sine
            if(ws < 1.57f)
                tmpv = sinf(ws);
            else
                tmpv = 1.0f;
            for(i = 0; i < n; ++i)
                smps[i] = sinf(smps[i] * ws) / tmpv;
            break;
        case 5:
            ws = ws * ws + 0.000001f; //Quantisize
            for(i = 0; i < n; ++i)
                smps[i] = floor(smps[i] / ws + 0.5f) * ws;
            break;
        case 6:
            ws = ws * ws * ws * 32 + 0.0001f; //Zigzag
            if(ws < 1.0f)
                tmpv = sinf(ws);
            else
                tmpv = 1.0f;
            for(i = 0; i < n; ++i)
                smps[i] = asinf(sinf(smps[i] * ws)) / tmpv;
            break;
        case 7:
            ws = powf(2.0f, -ws * ws * 8.0f); //Limiter
            for(i = 0; i < n; ++i) {
                float tmp = smps[i];
                if(fabs(tmp) > ws) {
                    if(tmp >= 0.0f)
                        smps[i] = 1.0f;
                    else
                        smps[i] = -1.0f;
                }
                else
                    smps[i] /= ws;
            }
            break;
        case 8:
            ws = powf(2.0f, -ws * ws * 8.0f); //Upper Limiter
            for(i = 0; i < n; ++i) {
                float tmp = smps[i];
                if(tmp > ws)
                    smps[i] = ws;
                smps[i] *= 2.0f;
            }
            break;
        case 9:
            ws = powf(2.0f, -ws * ws * 8.0f); //Lower Limiter
            for(i = 0; i < n; ++i) {
                float tmp = smps[i];
                if(tmp < -ws)
                    smps[i] = -ws;
                smps[i] *= 2.0f;
            }
            break;
        case 10:
            ws = (powf(2.0f, ws * 6.0f) - 1.0f) / powf(2.0f, 6.0f); //Inverse Limiter
            for(i = 0; i < n; ++i) {
                float tmp = smps[i];
                if(fabs(tmp) > ws) {
                    if(tmp >= 0.0f)
                        smps[i] = tmp - ws;
                    else
                        smps[i] = tmp + ws;
                }
                else
                    smps[i] = 0;
            }
            break;
        case 11:
            ws = powf(5, ws * ws * 1.0f) - 1.0f; //Clip
            for(i = 0; i < n; ++i)
                smps[i] = smps[i]
                          * (ws + 0.5f) * 0.9999f - floor(
                    0.5f + smps[i] * (ws + 0.5f) * 0.9999f);
            break;
        case 12:
            ws = ws * ws * ws * 30 + 0.001f; //Asym2
            if(ws < 0.3f)
                tmpv = ws;
            else
                tmpv = 1.0f;
            for(i = 0; i < n; ++i) {
                float tmp = smps[i] * ws;
                if((tmp > -2.0f) && (tmp < 1.0f))
                    smps[i] = tmp * (1.0f - tmp) * (tmp + 2.0f) / tmpv;
                else
                    smps[i] = 0.0f;
            }
            break;
        case 13:
            ws = ws * ws * ws * 32.0f + 0.0001f; //Pow2
            if(ws < 1.0f)
                tmpv = ws * (1 + ws) / 2.0f;
            else
                tmpv = 1.0f;
            for(i = 0; i < n; ++i) {
                float tmp = smps[i] * ws;
                if((tmp > -1.0f) && (tmp < 1.618034f))
                    smps[i] = tmp * (1.0f - tmp) / tmpv;
                else
                if(tmp > 0.0f)
                    smps[i] = -1.0f;
                else
                    smps[i] = -2.0f;
            }
            break;
        case 14:
            ws = powf(ws, 5.0f) * 80.0f + 0.0001f; //sigmoid
            if(ws > 10.0f)
                tmpv = 0.5f;
            else
                tmpv = 0.5f - 1.0f / (expf(ws) + 1.0f);
            for(i = 0; i < n; ++i) {
                float tmp = smps[i] * ws;
                if(tmp < -10.0f)
                    tmp = -10.0f;
                else
                if(tmp > 10.0f)
                    tmp = 10.0f;
                tmp     = 0.5f - 1.0f / (expf(tmp) + 1.0f);
                smps[i] = tmp / tmpv;
            }
            break;
    }
}
