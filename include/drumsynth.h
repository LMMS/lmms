/*
 * drumsynth.h - DrumSynth DS file renderer
 *
 * Copyright (c) 1998-2000 Paul Kellett (mda-vst.com)
 * Copyright (c) 2007 Paul Giblock <drfaygo/at/gmail.com>
 * 
 * This file is part of LMMS - http://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#ifndef _DRUMSYNTH_H__
#define _DRUMSYNTH_H__

#include <stdint.h>
#include "lmms_basics.h"

class DrumSynth {
    public:
        DrumSynth() {};
        int GetDSFileSamples(const char *dsfile, int16_t *&wave, int channels, sample_rate_t Fs);

    private:
        float LoudestEnv(void);
        int   LongestEnv(void);
        void  UpdateEnv(int e, long t);
        void  GetEnv(int env, const char *sec, const char *key, const char *ini);

        float waveform(float ph, int form);
        
        int GetPrivateProfileString(const char *sec, const char *key, const char *def, char *buffer, int size, const char *file);
        int GetPrivateProfileInt(const char *sec, const char *key, int def, const char *file);
        float GetPrivateProfileFloat(const char *sec, const char *key, float def, const char *file);

};

#endif 
