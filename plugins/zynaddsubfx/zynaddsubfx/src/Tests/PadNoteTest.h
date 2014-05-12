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
#include "../Misc/Master.h"
#include "../Misc/Util.h"
#include "../Synth/PADnote.h"
#include "../Params/Presets.h"
#include "../DSP/FFTwrapper.h"
#include "../globals.h"
SYNTH_T *synth;

using namespace std;

class PadNoteTest:public CxxTest::TestSuite
{
    public:
        PADnote      *note;
        Master       *master;
        FFTwrapper   *fft;
        Controller   *controller;
        unsigned char testnote;


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
            PADnoteParameters *defaultPreset = new PADnoteParameters(fft,NULL);


            //Assert defaults
            ///TS_ASSERT(!defaultPreset->VoicePar[1].Enabled);

            XMLwrapper *wrap = new XMLwrapper();
            cout << string(SOURCE_DIR) + string("/guitar-adnote.xmz")
                 << endl;
            wrap->loadXMLfile(string(SOURCE_DIR)
                              + string("/guitar-adnote.xmz"));
            TS_ASSERT(wrap->enterbranch("MASTER"));
            TS_ASSERT(wrap->enterbranch("PART", 2));
            TS_ASSERT(wrap->enterbranch("INSTRUMENT"));
            TS_ASSERT(wrap->enterbranch("INSTRUMENT_KIT"));
            TS_ASSERT(wrap->enterbranch("INSTRUMENT_KIT_ITEM", 0));
            TS_ASSERT(wrap->enterbranch("PAD_SYNTH_PARAMETERS"));
            defaultPreset->getfromXML(wrap);


            //defaultPreset->defaults();
            defaultPreset->applyparameters(false);

            //verify xml was loaded
            ///TS_ASSERT(defaultPreset->VoicePar[1].Enabled);



            controller = new Controller();

            //lets go with.... 50! as a nice note
            testnote = 50;
            float freq = 440.0f * powf(2.0f, (testnote - 69.0f) / 12.0f);

            note = new PADnote(defaultPreset,
                              controller,
                              freq,
                              120,
                              0,
                              testnote,
                              false);

            //delete defaultPreset;
            delete wrap;
        }

        void willNoteBeRunButIsHereForLinkingReasonsHowsThisForCamelCaseEh()
        {
            master = new Master();
        }

        void tearDown() {
            delete note;
            delete controller;
            delete fft;
            delete [] outL;
            delete [] outR;
            delete [] denormalkillbuf;
            FFT_cleanup();
            delete synth;
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

            TS_ASSERT_DELTA(outL[255], 0.0660f, 0.0001f);


            note->relasekey();


            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            TS_ASSERT_DELTA(outL[255], -0.0729f, 0.0001f);

            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            TS_ASSERT_DELTA(outL[255], 0.0613f, 0.0001f);

            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            TS_ASSERT_DELTA(outL[255], 0.0378f, 0.0005f);

            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            TS_ASSERT_DELTA(outL[255], -0.0070f, 0.0001f);

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
