/*
  ZynAddSubFX - a software synthesizer

  MicrotonalTest.h - CxxTest for Misc/Microtonal
  Copyright (C) 2009-2012 Mark McCurry
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
#include <iostream>
#include "../Misc/Microtonal.h"
#include <cstring>
#include <string>
#include <cstdio>
#include "../globals.h"
SYNTH_T *synth;

using namespace std;

class MicrotonalTest:public CxxTest::TestSuite
{
    public:
        void setUp() {
            synth     = new SYNTH_T;
            testMicro = new Microtonal();
        }

        void tearDown() {
            delete testMicro;
            delete synth;
        }

        //Verifies that the object is initialized correctly
        void testinit() {
            TS_ASSERT_EQUALS(testMicro->Pinvertupdown, 0);
            TS_ASSERT_EQUALS(testMicro->Pinvertupdowncenter, 60);
            TS_ASSERT_EQUALS(testMicro->getoctavesize(), 12);
            TS_ASSERT_EQUALS(testMicro->Penabled, 0);
            TS_ASSERT_EQUALS(testMicro->PAnote, 69);
            TS_ASSERT_EQUALS(testMicro->PAfreq, 440.0f);
            TS_ASSERT_EQUALS(testMicro->Pscaleshift, 64);
            TS_ASSERT_EQUALS(testMicro->Pfirstkey, 0);
            TS_ASSERT_EQUALS(testMicro->Plastkey, 127);
            TS_ASSERT_EQUALS(testMicro->Pmiddlenote, 60);
            TS_ASSERT_EQUALS(testMicro->Pmapsize, 12);
            TS_ASSERT_EQUALS(testMicro->Pmappingenabled, 0);
            TS_ASSERT_EQUALS(testMicro->Pglobalfinedetune, 64);

            TS_ASSERT_EQUALS(string((const char *)testMicro->Pname), "12tET");
            TS_ASSERT_EQUALS(string(
                                 (const char *)testMicro->Pcomment),
                             "Equal Temperament 12 notes per octave");

            for(int i = 0; i < 128; ++i)
                TS_ASSERT_EQUALS(testMicro->Pmapping[i], i);

            TS_ASSERT_DELTA(testMicro->getnotefreq(19, 0), 24.4997f, 0.0001f);
        }

        //Tests saving/loading to XML
        void testXML() {
            //Gah, the XMLwrapper is a twisted maze
            testMicro->Penabled = 1;
            XMLwrapper xml;
            xml.beginbranch("Dummy"); //this should not be needed, but odd behavior
                                      //seems to exist from MICROTONAL being on the
                                      //top of the stack
            xml.beginbranch("MICROTONAL");
            testMicro->add2XML(&xml);
            xml.endbranch();
            xml.endbranch();

            char *tmp = xml.getXMLdata();
            Microtonal other;

            other.Penabled = 1;
            strcpy((char *)other.Pname, "Myname"); //will be nicer with strings

            TS_ASSERT(*testMicro != other); //sanity check

            TS_ASSERT(xml.enterbranch("Dummy"));
            TS_ASSERT(xml.enterbranch("MICROTONAL"));

            other.getfromXML(&xml);
            xml.exitbranch();
            xml.exitbranch();
            char *tmpo = xml.getXMLdata();

            TS_ASSERT(!strcmp(tmp, tmpo));
            free(tmp);
            free(tmpo);
        }

#if 0
        /**\todo Test Saving/loading from file*/

        //Test texttomapping TODO finish
        void _testTextToMapping() {
            //the mapping is from old documentation for "Intense Diatonic" scale
            const char *mapping[12] =
            {"0", "x", "1", "x", "2", "3", "x", "4", "x", "5", "x", "6"};
            //for(int i=0;i<20;++i)
            //    cout << i << ':' << testMicro->getnotefreq(i,0) << endl;
            //
            //    octave size == 7
            //    find dead notes
        }
        //Test texttotunings TODO finish
        void _testTextToTunings() {
            //the tuning is from old documentation for "Intense Diatonic" scale
            const char *tuning[7] =
            {"9/8", "5/4", "4/3", "3/2", "5/3", "15/8", "2/1"};
            const int numTunings = 7;
            //for(int i=0;i<20;++i)
            //    cout << i << ':' << testMicro->getnotefreq(i,0) << endl;
            //    go to middle key and verify the proportions
        }
        /**\TODO test loading from scl and kbm files*/
#endif

    private:
        Microtonal *testMicro;
};
