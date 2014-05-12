/*
  ZynAddSubFX - a software synthesizer

  AdNoteTest.h - CxxTest for Synth/OscilGen
  Copyright (C) 20011-2012 Mark McCurry
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
#include <cxxtest/TestSuite.h>
#include <string>
#include "../Synth/OscilGen.h"
#include "../globals.h"
SYNTH_T *synth;

using namespace std;

class OscilGenTest:public CxxTest::TestSuite
{
    public:
        float  freq;
        float *outR, *outL;
        FFTwrapper *fft;
        OscilGen   *oscil;

        void setUp() {
            synth = new SYNTH_T;
            //First the sensible settings and variables that have to be set:
            synth->buffersize = 256;
            synth->oscilsize  = 1024;

            outL = new float[synth->oscilsize];
            outR = new float[synth->oscilsize];
            memset(outL, 0, sizeof(float) * synth->oscilsize);
            memset(outR, 0, sizeof(float) * synth->oscilsize);

            //next the bad global variables that for some reason have not been properly placed in some
            //initialization routine, but rather exist as cryptic oneliners in main.cpp:
            denormalkillbuf = new float[synth->buffersize];
            for(int i = 0; i < synth->buffersize; ++i)
                denormalkillbuf[i] = 0;

            //prepare the default settings
            fft   = new FFTwrapper(synth->oscilsize);
            oscil = new OscilGen(fft, NULL);

            //Assert defaults [TODO]


            XMLwrapper *wrap = new XMLwrapper();
            wrap->loadXMLfile(string(SOURCE_DIR)
                              + string("/guitar-adnote.xmz"));
            TS_ASSERT(wrap->enterbranch("MASTER"));
            TS_ASSERT(wrap->enterbranch("PART", 0));
            TS_ASSERT(wrap->enterbranch("INSTRUMENT"));
            TS_ASSERT(wrap->enterbranch("INSTRUMENT_KIT"));
            TS_ASSERT(wrap->enterbranch("INSTRUMENT_KIT_ITEM", 0));
            TS_ASSERT(wrap->enterbranch("ADD_SYNTH_PARAMETERS"));
            TS_ASSERT(wrap->enterbranch("VOICE", 0));
            TS_ASSERT(wrap->enterbranch("OSCIL"));
            oscil->getfromXML(wrap);
            delete wrap;

            //verify xml was loaded [TODO]

            //lets go with.... 50! as a nice note
            const char testnote = 50;
            freq = 440.0f * powf(2.0f, (testnote - 69.0f) / 12.0f);
        }

        void tearDown() {
            delete oscil;
            delete fft;
            delete[] outL;
            delete[] outR;
            delete[] denormalkillbuf;
            FFT_cleanup();
            delete synth;
        }

        //verifies that initialization occurs
        void testInit(void)
        {
            oscil->get(outL, freq);
        }

        void testOutput(void)
        {
            oscil->get(outL, freq);
            TS_ASSERT_DELTA(outL[23], -0.044547f, 0.0001f);
            TS_ASSERT_DELTA(outL[129], -0.018169f, 0.0001f);
            TS_ASSERT_DELTA(outL[586], 0.045647f, 0.0001f);
            TS_ASSERT_DELTA(outL[1023], -0.038334f, 0.0001f);
        }

        void testSpectrum(void)
        {
            oscil->getspectrum(synth->oscilsize / 2, outR, 1);
            TS_ASSERT_DELTA(outR[0], 350.698059f, 0.0001f);
            TS_ASSERT_DELTA(outR[1], 228.889267f, 0.0001f);
            TS_ASSERT_DELTA(outR[2], 62.187931f, 0.0001f);
            TS_ASSERT_DELTA(outR[3], 22.295225f, 0.0001f);
            TS_ASSERT_DELTA(outR[4], 6.942001f, 0.0001f);
            TS_ASSERT_DELTA(outR[26], 0.015110f, 0.0001f);
            TS_ASSERT_DELTA(outR[47], 0.003425f, 0.0001f);
            TS_ASSERT_DELTA(outR[65], 0.001293f, 0.0001f);
        }

        //performance testing
        void testSpeed() {
            const int samps = 15000;

            int t_on = clock(); // timer before calling func
            for(int i = 0; i < samps; ++i)
                oscil->prepare();
            int t_off = clock(); // timer when func returns

            printf("OscilGenTest: %f seconds for %d prepares.\n",
                   (static_cast<float>(t_off - t_on)) / CLOCKS_PER_SEC, samps);

            t_on = clock(); // timer before calling func
            for(int i = 0; i < samps; ++i)
                oscil->get(outL, freq);
            t_off = clock(); // timer when func returns

            printf("OscilGenTest: %f seconds for %d gets.\n",
                   (static_cast<float>(t_off - t_on)) / CLOCKS_PER_SEC, samps);
        }
};
