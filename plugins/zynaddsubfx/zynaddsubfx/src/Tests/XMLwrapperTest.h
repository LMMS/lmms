/*
  ZynAddSubFX - a software synthesizer

  XMLwrapperTest.h - CxxTest for Misc/XMLwrapper
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
#include <cxxtest/TestSuite.h>
#include "../Misc/XMLwrapper.h"
#include <string>
#include "../globals.h"
SYNTH_T *synth;
using namespace std;

class XMLwrapperTest:public CxxTest::TestSuite
{
    public:
        void setUp() {
            xmla = new XMLwrapper;
            xmlb = new XMLwrapper;
        }


        void testAddPar() {
            xmla->addpar("my Pa*_ramet@er", 75);
            TS_ASSERT_EQUALS(xmla->getpar("my Pa*_ramet@er", 0, -200, 200), 75);
        }

        //here to verify that no leaks occur
        void testLoad() {
            string location = string(SOURCE_DIR) + string(
                "/Tests/guitar-adnote.xmz");
            xmla->loadXMLfile(location);
        }

        void testAnotherLoad()
        {
            string dat =
                "\n<?xml version=\"1.0f\" encoding=\"UTF-8\"?>\n\
<!DOCTYPE ZynAddSubFX-data>\n\
<ZynAddSubFX-data version-major=\"2\" version-minor=\"4\"\n\
version-revision=\"1\" ZynAddSubFX-author=\"Nasca Octavian Paul\">\n\
</ZynAddSubFX-data>\n";
            xmlb->putXMLdata(dat.c_str());
        }

        void tearDown() {
            delete xmla;
            delete xmlb;
        }


    private:
        XMLwrapper *xmla;
        XMLwrapper *xmlb;
};
