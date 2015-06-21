/*
  ZynAddSubFX - a software synthesizer

  PadNoteTest.h - CxxTest for Synth/PADnote
  Copyright (C) 20012 zco
  Author: zco

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


//Based Upon AdNoteTest.h and SubNoteTest.h
#include <cxxtest/TestSuite.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#define private public
#include "../Misc/Master.h"
#include "../Misc/Util.h"
#include "../Misc/Allocator.h"
#include "../Synth/PADnote.h"
#include "../Synth/OscilGen.h"
#include "../Params/PADnoteParameters.h"
#include "../Params/Presets.h"
#include "../DSP/FFTwrapper.h"
#include "../globals.h"
SYNTH_T *synth;

using namespace std;
#ifndef SOURCE_DIR
#define SOURCE_DIR "BE QUIET COMPILER"
#endif

class PadNoteTest:public CxxTest::TestSuite
{
    public:
        PADnote      *note;
        PADnoteParameters *pars;
        Master       *master;
        FFTwrapper   *fft;
        Controller   *controller;
        unsigned char testnote;
        Allocator     memory;


        float *outR, *outL;

        void setUp() {
            synth = new SYNTH_T;
            //First the sensible settings and variables that have to be set:
            synth->buffersize = 256;

            outL = new float[synth->buffersize];
            for(int i = 0; i < synth->buffersize; ++i)
                *(outL + i) = 0;
            outR = new float[synth->buffersize];
            for(int i = 0; i < synth->buffersize; ++i)
                *(outR + i) = 0;

            //next the bad global variables that for some reason have not been properly placed in some
            //initialization routine, but rather exist as cryptic oneliners in main.cpp:
            denormalkillbuf = new float[synth->buffersize];
            for(int i = 0; i < synth->buffersize; ++i)
                denormalkillbuf[i] = 0;

            //phew, glad to get thouse out of my way. took me a lot of sweat and gdb to get this far...

            fft = new FFTwrapper(synth->oscilsize);
            //prepare the default settings
            pars = new PADnoteParameters(fft);


            //Assert defaults
            ///TS_ASSERT(!defaultPreset->VoicePar[1].Enabled);

            XMLwrapper wrap;
            cout << string(SOURCE_DIR) + string("/guitar-adnote.xmz")
                 << endl;
            wrap.loadXMLfile(string(SOURCE_DIR)
                              + string("/guitar-adnote.xmz"));
            TS_ASSERT(wrap.enterbranch("MASTER"));
            TS_ASSERT(wrap.enterbranch("PART", 2));
            TS_ASSERT(wrap.enterbranch("INSTRUMENT"));
            TS_ASSERT(wrap.enterbranch("INSTRUMENT_KIT"));
            TS_ASSERT(wrap.enterbranch("INSTRUMENT_KIT_ITEM", 0));
            TS_ASSERT(wrap.enterbranch("PAD_SYNTH_PARAMETERS"));
            pars->getfromXML(&wrap);


            //defaultPreset->defaults();
            pars->applyparameters();

            //verify xml was loaded
            ///TS_ASSERT(defaultPreset->VoicePar[1].Enabled);



            controller = new Controller();

            //lets go with.... 50! as a nice note
            testnote = 50;
            float freq = 440.0f * powf(2.0f, (testnote - 69.0f) / 12.0f);
            SynthParams pars_{memory, *controller, freq, 120, 0, testnote, false};

            note = new PADnote(pars, pars_);
        }

        void tearDown() {
            delete note;
            delete controller;
            delete fft;
            delete [] outL;
            delete [] outR;
            delete [] denormalkillbuf;
            delete pars;
            FFT_cleanup();
            delete synth;

            note = NULL;
            controller = NULL;
            fft = NULL;
            outL = NULL;
            outR = NULL;
            denormalkillbuf = NULL;
            pars = NULL;
            synth = NULL;
        }


        void testDefaults() {
            int sampleCount = 0;


//#define WRITE_OUTPUT

#ifdef WRITE_OUTPUT
            ofstream file("padnoteout", ios::out);
#endif
            note->noteout(outL, outR);

#ifdef WRITE_OUTPUT
            for(int i = 0; i < synth->buffersize; ++i)
                file << outL[i] << std::endl;

#endif
            sampleCount += synth->buffersize;

            TS_ASSERT_DELTA(outL[255], 0.0660f, 0.0005f);


            note->relasekey();


            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            TS_ASSERT_DELTA(outL[255], -0.0729f, 0.0005f);

            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            TS_ASSERT_DELTA(outL[255], 0.060818f, 0.0005f);

            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            TS_ASSERT_DELTA(outL[255], 0.036895f, 0.0005f);

            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            TS_ASSERT_DELTA(outL[255], -0.006623f, 0.0001f);

            while(!note->finished()) {
                note->noteout(outL, outR);

#ifdef WRITE_OUTPUT
                for(int i = 0; i < synth->buffersize; ++i)
                    file << outL[i] << std::endl;

#endif
                sampleCount += synth->buffersize;
            }
#ifdef WRITE_OUTPUT
            file.close();
#endif

            TS_ASSERT_EQUALS(sampleCount, 2304);
        }

        void testInitialization() {
            TS_ASSERT_EQUALS(pars->Pmode, 0);

            TS_ASSERT_EQUALS(pars->PVolume, 90);
            TS_ASSERT(pars->oscilgen);
            TS_ASSERT(pars->resonance);

            TS_ASSERT_DELTA(note->NoteGlobalPar.Volume, 2.597527f, 0.001f);
            TS_ASSERT_DELTA(note->NoteGlobalPar.Panning, 0.500000f, 0.01f);


            for(int i=0; i<7; ++i)
                TS_ASSERT(pars->sample[i].smp);
            for(int i=7; i<PAD_MAX_SAMPLES; ++i)
                TS_ASSERT(!pars->sample[i].smp);

            TS_ASSERT_DELTA(pars->sample[0].smp[0],  -0.057407f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[1],  -0.050704f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[2],  -0.076559f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[3],  -0.069974f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[4],  -0.053268f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[5],  -0.025702f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[6],  -0.021064f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[7],   0.002593f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[8],   0.049286f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[9],   0.031929f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[10],  0.044527f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[11],  0.040447f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[12],  0.022108f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[13],  0.005787f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[14], -0.008430f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[15], -0.009642f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[16], -0.018427f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[17], -0.052831f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[18], -0.058690f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[19], -0.090954f, 0.0005f);


            //Verify Harmonic Input
            float harmonics[synth->oscilsize];
            memset(harmonics, 0, sizeof(float) * synth->oscilsize);

            pars->oscilgen->get(harmonics, 440.0f, false);

            TS_ASSERT_DELTA(harmonics[0] ,0.683947, 0.0005f);
            TS_ASSERT_DELTA(harmonics[1] ,0.128246, 0.0005f);
            TS_ASSERT_DELTA(harmonics[2] ,0.003238, 0.0005f);
            TS_ASSERT_DELTA(harmonics[3] ,0.280945, 0.0005f);
            TS_ASSERT_DELTA(harmonics[4] ,0.263548, 0.0005f);
            TS_ASSERT_DELTA(harmonics[5] ,0.357070, 0.0005f);
            TS_ASSERT_DELTA(harmonics[6] ,0.096287, 0.0005f);
            TS_ASSERT_DELTA(harmonics[7] ,0.128685, 0.0005f);
            TS_ASSERT_DELTA(harmonics[8] ,0.003238, 0.0005f);
            TS_ASSERT_DELTA(harmonics[9] ,0.149376, 0.0005f);
            TS_ASSERT_DELTA(harmonics[10],0.063892, 0.0005f);
            TS_ASSERT_DELTA(harmonics[11],0.296716, 0.0005f);
            TS_ASSERT_DELTA(harmonics[12],0.051057, 0.0005f);
            TS_ASSERT_DELTA(harmonics[13],0.066310, 0.0005f);
            TS_ASSERT_DELTA(harmonics[14],0.004006, 0.0005f);
            TS_ASSERT_DELTA(harmonics[15],0.038662, 0.0005f);

            float sum = 0;
            for(int i=0; i<synth->oscilsize/2; ++i)
                sum += harmonics[i];
            TS_ASSERT_DELTA(sum, 5.863001, 0.0005f);

            TS_ASSERT_DELTA(pars->getNhr(0), 0.000000, 0.0005f);
            TS_ASSERT_DELTA(pars->getNhr(1), 1.000000, 0.0005f);
            TS_ASSERT_DELTA(pars->getNhr(2), 2.000000, 0.0005f);
            TS_ASSERT_DELTA(pars->getNhr(3), 3.000000, 0.0005f);
            TS_ASSERT_DELTA(pars->getNhr(4), 4.000000, 0.0005f);
            TS_ASSERT_DELTA(pars->getNhr(5), 5.000000, 0.0005f);
            TS_ASSERT_DELTA(pars->getNhr(6), 6.000000, 0.0005f);
            TS_ASSERT_DELTA(pars->getNhr(7), 7.000000, 0.0005f);
            TS_ASSERT_DELTA(pars->getNhr(8), 8.000000, 0.0005f);
            TS_ASSERT_DELTA(pars->getNhr(9), 9.000000, 0.0005f);

        }

#define OUTPUT_PROFILE
#ifdef OUTPUT_PROFILE
        void testSpeed() {
            const int samps = 15000;

            int t_on = clock(); // timer before calling func
            for(int i = 0; i < samps; ++i)
                note->noteout(outL, outR);
            int t_off = clock(); // timer when func returns

            printf("PadNoteTest: %f seconds for %d Samples to be generated.\n",
                   (static_cast<float>(t_off - t_on)) / CLOCKS_PER_SEC, samps);
        }
#endif
};
