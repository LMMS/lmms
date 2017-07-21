/*
  ZynAddSubFX - a software synthesizer

  EchoTest.h - CxxTest for Effect/Echo
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
#include <cmath>
#include <cstdlib>
#include <iostream>
#include "../Effects/Echo.h"
#include "../globals.h"
SYNTH_T *synth;

using namespace std;

class EchoTest:public CxxTest::TestSuite
{
    public:
        void setUp() {
            synth = new SYNTH_T;
            outL  = new float[synth->buffersize];
            for(int i = 0; i < synth->buffersize; ++i)
                outL[i] = 0.0f;
            outR = new float[synth->buffersize];
            for(int i = 0; i < synth->buffersize; ++i)
                outR[i] = 0.0f;
            input = new Stereo<float *>(new float[synth->buffersize],
                                        new float[synth->buffersize]);
            for(int i = 0; i < synth->buffersize; ++i)
                input->l[i] = input->r[i] = 0.0f;
            testFX = new Echo(true, outL, outR, 44100, 256);
        }

        void tearDown() {
            delete[] input->r;
            delete[] input->l;
            delete input;
            delete[] outL;
            delete[] outR;
            delete testFX;
            delete synth;
        }


        void testInit() {
            //Make sure that the output will be zero at start
            //(given a zero input)
            testFX->out(*input);
            for(int i = 0; i < synth->buffersize; ++i) {
                TS_ASSERT_DELTA(outL[i], 0.0f, 0.0001f);
                TS_ASSERT_DELTA(outR[i], 0.0f, 0.0001f);
            }
        }

        void testClear() {
            char DELAY = 2;
            testFX->changepar(DELAY, 127);

            //flood with high input
            for(int i = 0; i < synth->buffersize; ++i)
                input->r[i] = input->l[i] = 1.0f;

            for(int i = 0; i < 500; ++i)
                testFX->out(*input);
            for(int i = 0; i < synth->buffersize; ++i) {
                TS_ASSERT_DIFFERS(outL[i], 0.0f);
                TS_ASSERT_DIFFERS(outR[i], 0.0f)
            }
            //After making sure the internal buffer has a nonzero value
            //cleanup
            //Then get the next output, which should be zereoed out if DELAY
            //is large enough
            testFX->cleanup();
            testFX->out(*input);
            for(int i = 0; i < synth->buffersize; ++i) {
                TS_ASSERT_DELTA(outL[i], 0.0f, 0.0001f);
                TS_ASSERT_DELTA(outR[i], 0.0f, 0.0001f);
            }
        }
        //Insures that the proper decay occurs with high feedback
        void testDecaywFb() {
            //flood with high input
            for(int i = 0; i < synth->buffersize; ++i)
                input->r[i] = input->l[i] = 1.0f;
            char FEEDBACK = 5;
            testFX->changepar(FEEDBACK, 127);
            for(int i = 0; i < 100; ++i)
                testFX->out(*input);
            for(int i = 0; i < synth->buffersize; ++i) {
                TS_ASSERT_DIFFERS(outL[i], 0.0f);
                TS_ASSERT_DIFFERS(outR[i], 0.0f)
            }
            float amp = abs(outL[0] + outR[0]) / 2;
            //reset input to zero
            for(int i = 0; i < synth->buffersize; ++i)
                input->r[i] = input->l[i] = 0.0f;

            //give the echo time to fade based upon zero input and high feedback
            for(int i = 0; i < 50; ++i)
                testFX->out(*input);
            TS_ASSERT_LESS_THAN_EQUALS(abs(outL[0] + outR[0]) / 2, amp);
        }


    private:
        Stereo<float *> *input;
        float *outR, *outL;
        Echo  *testFX;
};
