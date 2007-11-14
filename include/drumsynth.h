// DrumSynth DS file renderer
// Copyright (c)1998-2000 Paul Kellett (mda-vst.com)
// Licensed under the MIT License. Read drumsynth.LICENSE for details
// Version 2.0 (5/10/00)
//
// Adapted for LMMS 2007  Paul Giblock
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#ifndef _DRUMSYNTH_H__
#define _DRUMSYNTH_H__

#include <stdint.h>

class DrumSynth {
    public:
        DrumSynth() {};
        int GetDSFileSamples(const char *dsfile, int16_t *&wave, int channels);

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
