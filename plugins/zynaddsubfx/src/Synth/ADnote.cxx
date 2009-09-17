#include <cxxtest/TestSuite.h>
#include <iostream>
#include <fstream>
#include "../Misc/Master.h"
#include "../Misc/Util.h"
#include "../Synth/ADnote.h"
#include "../Params/Presets.h"
#include "../globals.h"

class AdNoteTest : public CxxTest::TestSuite
{
public:

    ADnote *note;
    Master *master;
    Controller *controller;
    unsigned char testnote;


    float *outR,*outL;

    void setUp() {

        //First the sensible settings and variables that have to be set:
        SOUND_BUFFER_SIZE = 256;

        outL=new float[SOUND_BUFFER_SIZE];
        for (int i=0;i<SOUND_BUFFER_SIZE;++i)
            *(outL+i)=0;
        outR=new float[SOUND_BUFFER_SIZE];
        for (int i=0;i<SOUND_BUFFER_SIZE;++i)
            *(outR+i)=0;

        //next the bad global variables that for some reason have not been properly placed in some
        //initialization routine, but rather exist as cryptic oneliners in main.cpp:
        denormalkillbuf= new REALTYPE[SOUND_BUFFER_SIZE];
        for (int i=0;i<SOUND_BUFFER_SIZE;i++) denormalkillbuf[i]=0;

        OscilGen::tmpsmps=new REALTYPE[OSCIL_SIZE];
        newFFTFREQS(&OscilGen::outoscilFFTfreqs,OSCIL_SIZE/2);


        //phew, glad to get thouse out of my way. took me a lot of sweat and gdb to get this far...

        //prepare the default settings
        ADnoteParameters *defaultPreset = new ADnoteParameters(new FFTwrapper(OSCIL_SIZE));
        XMLwrapper *wrap = new XMLwrapper();
        wrap->loadXMLfile("src/Tests/guitar-adnote.xmz");
        TS_ASSERT(wrap->enterbranch("MASTER"));
        TS_ASSERT(wrap->enterbranch("PART", 0));
        TS_ASSERT(wrap->enterbranch("INSTRUMENT"));
        TS_ASSERT(wrap->enterbranch("INSTRUMENT_KIT"));
        TS_ASSERT(wrap->enterbranch("INSTRUMENT_KIT_ITEM", 0));
        TS_ASSERT(wrap->enterbranch("ADD_SYNTH_PARAMETERS"));
        defaultPreset->getfromXML(wrap);
        //defaultPreset->defaults();



        controller = new Controller();

        //lets go with.... 50! as a nice note
        testnote = 50;
        REALTYPE freq = 440.0*pow(2.0,(testnote-69.0)/12.0);

        note = new ADnote(defaultPreset, controller, freq, 120, 0, testnote, false);

    }

    void willNoteBeRunButIsHereForLinkingReasonsHowsThisForCamelCaseEh()
    {
        master = new Master();
    }

    void tearDown() {
        delete note;
        deleteFFTFREQS(&OscilGen::outoscilFFTfreqs);
    }

    void testDefaults() {

        TS_ASSERT(note->ready);
        int sampleCount = 0;

//#define WRITE_OUTPUT

#ifdef WRITE_OUTPUT
        ofstream file("adnoteout", ios::out);
#endif
        note->noteout(outL, outR);
#ifdef WRITE_OUTPUT
        for (int i = 0; i < SOUND_BUFFER_SIZE; ++i) {
            file << outL[i] << std::endl;
        }
#endif
        sampleCount += SOUND_BUFFER_SIZE;

        TS_ASSERT_DELTA(outL[255], 0.1724, 0.0001);

        note->relasekey();


        note->noteout(outL, outR);
        sampleCount += SOUND_BUFFER_SIZE;
        TS_ASSERT_DELTA(outL[255], -0.1284, 0.0001);

        note->noteout(outL, outR);
        sampleCount += SOUND_BUFFER_SIZE;
        TS_ASSERT_DELTA(outL[255], -0.0206, 0.0001);

        note->noteout(outL, outR);
        sampleCount += SOUND_BUFFER_SIZE;
        TS_ASSERT_DELTA(outL[255], -0.1122, 0.0001);

        note->noteout(outL, outR);
        sampleCount += SOUND_BUFFER_SIZE;
        TS_ASSERT_DELTA(outL[255], 0.1707, 0.0001);

        while (!note->finished()) {
            note->noteout(outL, outR);
#ifdef WRITE_OUTPUT
            for (int i = 0; i < SOUND_BUFFER_SIZE; ++i) {
                file << outL[i] << std::endl;
            }
#endif
            sampleCount += SOUND_BUFFER_SIZE;
        }
#ifdef WRITE_OUTPUT
        file.close();
#endif

        TS_ASSERT_EQUALS(sampleCount, 9472);

    }
};

