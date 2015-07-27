//Test to verify dispatch performance of rtosc is good enough

#include <ctime>
#include <cstdio>
#include <cassert>

#include <rtosc/rtosc.h>
#include <rtosc/ports.h>
using namespace rtosc;

void do_nothing(const char *data, RtData &d)
{
    (void) data;
    (void) d;
};

#define dummy(x)  {#x, NULL, NULL, do_nothing}
#define dummyT(x) {#x"::T:F", NULL, NULL, do_nothing}
#define dummyC(x) {#x"::c", NULL, NULL, do_nothing}
#define dummyI(x) {#x"::i", NULL, NULL, do_nothing}
#define dummyO(x) {#x"::s:c:i", NULL, NULL, do_nothing}


//TODO Consider performance penalty of argument specifier
//chains
//55 elements long (assuming I can count properly
Ports port_table = {
    dummy(oscil/),
    dummy(mod-oscil/),
    dummy(FreqLfo/),
    dummy(AmpLfo/),
    dummy(FilterLfo/),
    dummy(FreqEnvelope/),
    dummy(AmpEnvelope/),
    dummy(FilterEnvelope/),
    dummy(FMFreqEnvelope/),
    dummy(FMAmpEnvelope/),
    dummy(VoiceFilter/),
    dummyT(Enabled),//section
    dummyC(Unison_size),
    dummyC(Unison_frequency_spread),
    dummyC(Unison_stereo_spread),
    dummyC(Unison_vibratto),
    dummyC(Unison_vibratto_speed),
    dummyO(Unison_invert_phase),
    dummyO(Type),
    dummyC(PDelay),
    dummyT(Presonance),
    dummyC(Pextoscil),
    dummyC(PextFMoscil),
    dummyC(Poscilphase),
    dummyC(PFMoscilphase),
    dummyT(Pfilterbypass),
    dummyT(Pfixedfreq),//Freq Stuff
    dummyC(PfixedfreqET),
    dummyI(PDetune),
    dummyI(PCoarseDetune),
    dummyC(PDetuneType),
    dummyT(PFreqEnvelopeEnabled),
    dummyT(PFreqLfoEnabled),
    dummyC(PPanning),//Amplitude Stuff
    dummyC(PVolume),
    dummyT(PVolumeminus),
    dummyC(PAmpVelocityScaleFunction),
    dummyT(PAmpEnvelopeEnabled),
    dummyT(PAmpLfoEnabled),
    dummyT(PFilterEnabled),//Filter Stuff
    dummyT(PFilterEnvelopeEnabled),
    dummyT(PFilterLfoEnabled),
    dummyC(PFMEnabled),//Modulator Stuff
    dummyI(PFMVoice),
    dummyC(PFMVolume),
    dummyC(PFMVolumeDamp),
    dummyC(PFMVelocityScaleFunction),
    dummyI(PFMDetune),
    dummyI(PFMCoarseDetune),
    dummyC(PFMDetuneType),
    dummyT(PFMFreqEnvelopeEnabled),
    dummyT(PFMAmpEnvelopeEnabled),
    dummy(detunevalue),
    dummy(octave),
    dummy(coarsedetune),
};

char events[20][1024];
char loc_buffer[1024];
    
int main()
{
    rtosc_message(events[0],  1024, "PFMDetune", "i", 23);
    rtosc_message(events[1],  1024, "oscil/blam", "c", 23);
    rtosc_message(events[2],  1024, "PFilterEnabled", "T");
    rtosc_message(events[3],  1024, "PVolume", "c", 23);
    rtosc_message(events[4],  1024, "Enabled", "T");
    rtosc_message(events[5],  1024, "Unison_size", "c", 1);
    rtosc_message(events[6],  1024, "Unison_frequency_spread", "c", 2);
    rtosc_message(events[7],  1024, "Unison_stereo_spread", "c", 3);
    rtosc_message(events[8],  1024, "Unison_vibratto", "c", 4);
    rtosc_message(events[9],  1024, "Unison_vibratto_speed", "c", 5);
    rtosc_message(events[10], 1024, "Unison_invert_phase", "");
    rtosc_message(events[11], 1024, "FilterLfo/another/few/layers", "");
    rtosc_message(events[12], 1024, "FreqEnvelope/blam", "");
    rtosc_message(events[13], 1024, "PINVALID_RANDOM_STRING", "ics", 23, 23, "23");
    rtosc_message(events[14], 1024, "PFMVelocityScaleFunction", "i", 23);
    rtosc_message(events[15], 1024, "PFMDetune", "i", 230);
    rtosc_message(events[16], 1024, "Pfixedfreq", "F");
    rtosc_message(events[17], 1024, "detunevalue", "");
    rtosc_message(events[18], 1024, "PfixedfreqET", "c", 10);
    rtosc_message(events[19], 1024, "PfixedfreqET", "");
    RtData d;
    d.loc_size = 1024;
    d.obj = d.loc = loc_buffer;
    d.matches = 0;

    int repeats = 200000;
    int t_on = clock(); // timer before calling func
    for(int j=0; j<200000; ++j) {
        for(int i=0; i<20; ++i){
            port_table.dispatch(events[i], d);
        }
    }
    //printf("Matches: %d\n", d.matches);
    assert(d.matches == 3600000);
    int t_off = clock(); // timer when func returns

    double seconds = (t_off - t_on) * 1.0 / CLOCKS_PER_SEC;
    printf("RTOSC Performance: %f seconds for the test\n", seconds);
    printf("RTOSC Performace: %f ns per dispatch\n", seconds*1e9/(repeats*20.0));
}
