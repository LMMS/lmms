/*
  ZynAddSubFX - a software synthesizer

  AdNoteTest.h - CxxTest for Synth/ADnote
  Copyright (C) 2009-2011 Mark McCurry
  Copyright (C) 2009 Harald Hvaal
  Authors: Mark McCurry, Harald Hvaal

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


#include <cxxtest/TestSuite.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include "../Misc/Util.h"
#include "../Synth/ADnote.h"
#include "../Synth/OscilGen.h"
#include "../Params/Presets.h"
#include "../DSP/FFTwrapper.h"
#include "../globals.h"
SYNTH_T *synth;

using namespace std;


#define BUF 256
class AdNoteTest:public CxxTest::TestSuite
{
    public:

        ADnote       *note;
        FFTwrapper   *fft;
        Controller   *controller;
        unsigned char testnote;
        ADnoteParameters *params;
        float freq;


        float outR[BUF], outL[BUF];

        void setUp() {
            //First the sensible settings and variables that have to be set:
            synth = new SYNTH_T;
            synth->buffersize = BUF;

            memset(outL,0,sizeof(outL));
            memset(outR,0,sizeof(outR));

            //next the bad global variables that for some reason have not been properly placed in some
            //initialization routine, but rather exist as cryptic oneliners in main.cpp:
            denormalkillbuf = new float[BUF];
            memset(denormalkillbuf, 0, sizeof(float)*BUF);

            fft = new FFTwrapper(BUF);
            //prepare the default settings
            params = new ADnoteParameters(fft);

            //sawtooth to make things a bit more interesting
            params->VoicePar[0].OscilSmp->Pcurrentbasefunc = 3;

            controller = new Controller();

            //lets go with.... 50! as a nice note
            testnote = 50;
            freq = 440.0f * powf(2.0f, (testnote - 69.0f) / 12.0f);

        }

        void tearDown() {
            delete note;
            delete controller;
            delete fft;
            delete [] denormalkillbuf;
            FFT_cleanup();
            delete synth;
            delete params;
        }

        void run_test(int a, int b, int c, int d, int e, int f, float values[4])
        {
            sprng(0);
            params->set_unison_size_index(0,a);
            params->VoicePar[0].Unison_frequency_spread = b;
            params->VoicePar[0].Unison_stereo_spread    = c;
            params->VoicePar[0].Unison_vibratto         = d;
            params->VoicePar[0].Unison_vibratto_speed   = e;
            params->VoicePar[0].Unison_invert_phase     = f;

            note = new ADnote(params, controller, freq, 120, 0, testnote, false);
            note->noteout(outL, outR);
            TS_ASSERT_DELTA(outL[80], values[0], 1e-5);
            //printf("{%f,", outL[80]);
            note->noteout(outL, outR);
            TS_ASSERT_DELTA(outR[90], values[1], 1e-5);
            //printf("%f,", outR[90]);
            note->noteout(outL, outR);
            TS_ASSERT_DELTA(outL[20], values[2], 1e-5);
            //printf("%f,", outL[20]);
            note->noteout(outL, outR);
            TS_ASSERT_DELTA(outR[200], values[3], 1e-5);
            //printf("%f},\n", outR[200]);
        }

        void testUnison() {
            sprng(0xbeef);

            float data[][4] = {
                {-0.034547,0.034349,-0.000000,0.138284},
                {0.023612,-0.093842,0.000000,-0.040384},
                {-0.015980,0.001871,-0.014463,-0.000726},
                {-0.040970,-0.000275,0.000000,-0.121016},
                {0.019250,-0.045252,0.000270,0.105372},
                {-0.086575,0.001130,-0.018921,0.001329},
                {0.009203,-0.006176,0.017344,-0.003316},
                {0.029411,-0.000248,-0.112797,-0.012883},
                {0.043657,-0.014062,-0.003374,-0.071821},
                {0.007973,0.068019,-0.038900,0.047639},
                {-0.002055,0.011170,-0.058152,-0.043493},
                {-0.005298,0.000605,-0.070932,-0.005678},
                {0.025028,-0.027742,0.020985,-0.015417},
                {0.074349,0.000640,0.080613,0.066636},
                {-0.045721,0.000279,0.009819,0.032202},
            };

            int freq_spread[15];
            int stereo_spread[15];
            int vibrato[15];
            int vibrato_speed[15];
            int inv_phase[15];
            for(int i=0; i<15; ++i)
            {
                freq_spread[i]    = prng()%0x7f;
                stereo_spread[i]  = prng()%0x7f;
                vibrato[i]        = prng()%0x7f;
                vibrato_speed[i]  = prng()%0x7f;
                inv_phase[i]      = prng()%5;
            }

            for(int i=0; i<15; ++i)
            {
                run_test(i, freq_spread[i], stereo_spread[i],
                        vibrato[i], vibrato_speed[i], inv_phase[i], data[i]);
            }
#if 0
            int sampleCount = 0;

            sampleCount += synth->buffersize;

            TS_ASSERT_DELTA(outL[255], 0.254609f, 0.0001f);

            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            TS_ASSERT_DELTA(outL[255], -0.102197f, 0.0001f);

            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            TS_ASSERT_DELTA(outL[255], -0.111422f, 0.0001f);

            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            TS_ASSERT_DELTA(outL[255], -0.021375f, 0.0001f);

            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            TS_ASSERT_DELTA(outL[255], 0.149882f, 0.0001f);
#endif
        }
};
