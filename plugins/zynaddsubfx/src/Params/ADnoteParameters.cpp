/*
  ZynAddSubFX - a software synthesizer

  ADnoteParameters.C - Parameters for ADnote (ADsynth)
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "ADnoteParameters.h"

ADnoteParameters::ADnoteParameters(FFTwrapper *fft_):Presets()
{
    setpresettype("Padsyth");
    fft=fft_;

    GlobalPar.FreqEnvelope=new EnvelopeParams(0,0);
    GlobalPar.FreqEnvelope->ASRinit(64,50,64,60);
    GlobalPar.FreqLfo=new LFOParams(70,0,64,0,0,0,0,0);

    GlobalPar.AmpEnvelope=new EnvelopeParams(64,1);
    GlobalPar.AmpEnvelope->ADSRinit_dB(0,40,127,25);
    GlobalPar.AmpLfo=new LFOParams(80,0,64,0,0,0,0,1);

    GlobalPar.GlobalFilter=new FilterParams(2,94,40);
    GlobalPar.FilterEnvelope=new EnvelopeParams(0,1);
    GlobalPar.FilterEnvelope->ADSRinit_filter(64,40,64,70,60,64);
    GlobalPar.FilterLfo=new LFOParams(80,0,64,0,0,0,0,2);
    GlobalPar.Reson=new Resonance();

    for (int nvoice=0;nvoice<NUM_VOICES;nvoice++) EnableVoice(nvoice);

    defaults();
};

void ADnoteParameters::defaults()
{
    //Default Parameters
    /* Frequency Global Parameters */
    GlobalPar.PStereo=1;//stereo
    GlobalPar.PDetune=8192;//zero
    GlobalPar.PCoarseDetune=0;
    GlobalPar.PDetuneType=1;
    GlobalPar.FreqEnvelope->defaults();
    GlobalPar.FreqLfo->defaults();
    GlobalPar.PBandwidth=64;

    /* Amplitude Global Parameters */
    GlobalPar.PVolume=90;
    GlobalPar.PPanning=64;//center
    GlobalPar.PAmpVelocityScaleFunction=64;
    GlobalPar.AmpEnvelope->defaults();
    GlobalPar.AmpLfo->defaults();
    GlobalPar.PPunchStrength=0;
    GlobalPar.PPunchTime=60;
    GlobalPar.PPunchStretch=64;
    GlobalPar.PPunchVelocitySensing=72;
    GlobalPar.Hrandgrouping=0;

    /* Filter Global Parameters*/
    GlobalPar.PFilterVelocityScale=64;
    GlobalPar.PFilterVelocityScaleFunction=64;
    GlobalPar.GlobalFilter->defaults();
    GlobalPar.FilterEnvelope->defaults();
    GlobalPar.FilterLfo->defaults();
    GlobalPar.Reson->defaults();


    for (int nvoice=0;nvoice<NUM_VOICES;nvoice++) {
        defaults(nvoice);
    };
    VoicePar[0].Enabled=1;
};

/*
 * Defaults a voice
 */
void ADnoteParameters::defaults(int n)
{
    int nvoice=n;
    VoicePar[nvoice].Enabled=0;
    VoicePar[nvoice].Type=0;
    VoicePar[nvoice].Pfixedfreq=0;
    VoicePar[nvoice].PfixedfreqET=0;
    VoicePar[nvoice].Presonance=1;
    VoicePar[nvoice].Pfilterbypass=0;
    VoicePar[nvoice].Pextoscil=-1;
    VoicePar[nvoice].PextFMoscil=-1;
    VoicePar[nvoice].Poscilphase=64;
    VoicePar[nvoice].PFMoscilphase=64;
    VoicePar[nvoice].PDelay=0;
    VoicePar[nvoice].PVolume=100;
    VoicePar[nvoice].PVolumeminus=0;
    VoicePar[nvoice].PPanning=64;//center
    VoicePar[nvoice].PDetune=8192;//8192=0
    VoicePar[nvoice].PCoarseDetune=0;
    VoicePar[nvoice].PDetuneType=0;
    VoicePar[nvoice].PFreqLfoEnabled=0;
    VoicePar[nvoice].PFreqEnvelopeEnabled=0;
    VoicePar[nvoice].PAmpEnvelopeEnabled=0;
    VoicePar[nvoice].PAmpLfoEnabled=0;
    VoicePar[nvoice].PAmpVelocityScaleFunction=127;
    VoicePar[nvoice].PFilterEnabled=0;
    VoicePar[nvoice].PFilterEnvelopeEnabled=0;
    VoicePar[nvoice].PFilterLfoEnabled=0;
    VoicePar[nvoice].PFMEnabled=0;

    //I use the internal oscillator (-1)
    VoicePar[nvoice].PFMVoice=-1;

    VoicePar[nvoice].PFMVolume=90;
    VoicePar[nvoice].PFMVolumeDamp=64;
    VoicePar[nvoice].PFMDetune=8192;
    VoicePar[nvoice].PFMCoarseDetune=0;
    VoicePar[nvoice].PFMDetuneType=0;
    VoicePar[nvoice].PFMFreqEnvelopeEnabled=0;
    VoicePar[nvoice].PFMAmpEnvelopeEnabled=0;
    VoicePar[nvoice].PFMVelocityScaleFunction=64;

    VoicePar[nvoice].OscilSmp->defaults();
    VoicePar[nvoice].FMSmp->defaults();

    VoicePar[nvoice].AmpEnvelope->defaults();
    VoicePar[nvoice].AmpLfo->defaults();

    VoicePar[nvoice].FreqEnvelope->defaults();
    VoicePar[nvoice].FreqLfo->defaults();

    VoicePar[nvoice].VoiceFilter->defaults();
    VoicePar[nvoice].FilterEnvelope->defaults();
    VoicePar[nvoice].FilterLfo->defaults();

    VoicePar[nvoice].FMFreqEnvelope->defaults();
    VoicePar[nvoice].FMAmpEnvelope->defaults();
};



/*
 * Init the voice parameters
 */
void ADnoteParameters::EnableVoice(int nvoice)
{
    VoicePar[nvoice].OscilSmp=new OscilGen(fft,GlobalPar.Reson);
    VoicePar[nvoice].FMSmp=new OscilGen(fft,NULL);

    VoicePar[nvoice].AmpEnvelope=new EnvelopeParams(64,1);
    VoicePar[nvoice].AmpEnvelope->ADSRinit_dB(0,100,127,100);
    VoicePar[nvoice].AmpLfo=new LFOParams(90,32,64,0,0,30,0,1);

    VoicePar[nvoice].FreqEnvelope=new EnvelopeParams(0,0);
    VoicePar[nvoice].FreqEnvelope->ASRinit(30,40,64,60);
    VoicePar[nvoice].FreqLfo=new LFOParams(50,40,0,0,0,0,0,0);

    VoicePar[nvoice].VoiceFilter=new FilterParams(2,50,60);
    VoicePar[nvoice].FilterEnvelope=new EnvelopeParams(0,0);
    VoicePar[nvoice].FilterEnvelope->ADSRinit_filter(90,70,40,70,10,40);
    VoicePar[nvoice].FilterLfo=new LFOParams(50,20,64,0,0,0,0,2);

    VoicePar[nvoice].FMFreqEnvelope=new EnvelopeParams(0,0);
    VoicePar[nvoice].FMFreqEnvelope->ASRinit(20,90,40,80);
    VoicePar[nvoice].FMAmpEnvelope=new EnvelopeParams(64,1);
    VoicePar[nvoice].FMAmpEnvelope->ADSRinit(80,90,127,100);
};

/*
 * Get the Multiplier of the fine detunes of the voices
 */
REALTYPE ADnoteParameters::getBandwidthDetuneMultiplier()
{
    REALTYPE bw=(GlobalPar.PBandwidth-64.0)/64.0;
    bw=pow(2.0,bw*pow(fabs(bw),0.2)*5.0);

    return(bw);
};


/*
 * Kill the voice
 */
void ADnoteParameters::KillVoice(int nvoice)
{
    delete (VoicePar[nvoice].OscilSmp);
    delete (VoicePar[nvoice].FMSmp);

    delete (VoicePar[nvoice].AmpEnvelope);
    delete (VoicePar[nvoice].AmpLfo);

    delete (VoicePar[nvoice].FreqEnvelope);
    delete (VoicePar[nvoice].FreqLfo);

    delete (VoicePar[nvoice].VoiceFilter);
    delete (VoicePar[nvoice].FilterEnvelope);
    delete (VoicePar[nvoice].FilterLfo);

    delete (VoicePar[nvoice].FMFreqEnvelope);
    delete (VoicePar[nvoice].FMAmpEnvelope);
};

ADnoteParameters::~ADnoteParameters()
{
    delete(GlobalPar.FreqEnvelope);
    delete(GlobalPar.FreqLfo);
    delete(GlobalPar.AmpEnvelope);
    delete(GlobalPar.AmpLfo);
    delete(GlobalPar.GlobalFilter);
    delete(GlobalPar.FilterEnvelope);
    delete(GlobalPar.FilterLfo);
    delete(GlobalPar.Reson);

    for (int nvoice=0;nvoice<NUM_VOICES;nvoice++) {
        KillVoice(nvoice);
    };
};




void ADnoteParameters::add2XMLsection(XMLwrapper *xml,int n)
{
    int nvoice=n;
    if (nvoice>=NUM_VOICES) return;

    int oscilused=0,fmoscilused=0;//if the oscil or fmoscil are used by another voice

    for (int i=0;i<NUM_VOICES;i++) {
        if (VoicePar[i].Pextoscil==nvoice) oscilused=1;
        if (VoicePar[i].PextFMoscil==nvoice) fmoscilused=1;
    };

    xml->addparbool("enabled",VoicePar[nvoice].Enabled);
    if (((VoicePar[nvoice].Enabled==0)&&(oscilused==0)&&(fmoscilused==0))&&(xml->minimal)) return;

    xml->addpar("type",VoicePar[nvoice].Type);
    xml->addpar("delay",VoicePar[nvoice].PDelay);
    xml->addparbool("resonance",VoicePar[nvoice].Presonance);

    xml->addpar("ext_oscil",VoicePar[nvoice].Pextoscil);
    xml->addpar("ext_fm_oscil",VoicePar[nvoice].PextFMoscil);

    xml->addpar("oscil_phase",VoicePar[nvoice].Poscilphase);
    xml->addpar("oscil_fm_phase",VoicePar[nvoice].PFMoscilphase);

    xml->addparbool("filter_enabled",VoicePar[nvoice].PFilterEnabled);
    xml->addparbool("filter_bypass",VoicePar[nvoice].Pfilterbypass);

    xml->addpar("fm_enabled",VoicePar[nvoice].PFMEnabled);

    xml->beginbranch("OSCIL");
    VoicePar[nvoice].OscilSmp->add2XML(xml);
    xml->endbranch();


    xml->beginbranch("AMPLITUDE_PARAMETERS");
    xml->addpar("panning",VoicePar[nvoice].PPanning);
    xml->addpar("volume",VoicePar[nvoice].PVolume);
    xml->addparbool("volume_minus",VoicePar[nvoice].PVolumeminus);
    xml->addpar("velocity_sensing",VoicePar[nvoice].PAmpVelocityScaleFunction);

    xml->addparbool("amp_envelope_enabled",VoicePar[nvoice].PAmpEnvelopeEnabled);
    if ((VoicePar[nvoice].PAmpEnvelopeEnabled!=0)||(!xml->minimal)) {
        xml->beginbranch("AMPLITUDE_ENVELOPE");
        VoicePar[nvoice].AmpEnvelope->add2XML(xml);
        xml->endbranch();
    };
    xml->addparbool("amp_lfo_enabled",VoicePar[nvoice].PAmpLfoEnabled);
    if ((VoicePar[nvoice].PAmpLfoEnabled!=0)||(!xml->minimal)) {
        xml->beginbranch("AMPLITUDE_LFO");
        VoicePar[nvoice].AmpLfo->add2XML(xml);
        xml->endbranch();
    };
    xml->endbranch();

    xml->beginbranch("FREQUENCY_PARAMETERS");
    xml->addparbool("fixed_freq",VoicePar[nvoice].Pfixedfreq);
    xml->addpar("fixed_freq_et",VoicePar[nvoice].PfixedfreqET);
    xml->addpar("detune",VoicePar[nvoice].PDetune);
    xml->addpar("coarse_detune",VoicePar[nvoice].PCoarseDetune);
    xml->addpar("detune_type",VoicePar[nvoice].PDetuneType);

    xml->addparbool("freq_envelope_enabled",VoicePar[nvoice].PFreqEnvelopeEnabled);
    if ((VoicePar[nvoice].PFreqEnvelopeEnabled!=0)||(!xml->minimal)) {
        xml->beginbranch("FREQUENCY_ENVELOPE");
        VoicePar[nvoice].FreqEnvelope->add2XML(xml);
        xml->endbranch();
    };
    xml->addparbool("freq_lfo_enabled",VoicePar[nvoice].PFreqLfoEnabled);
    if ((VoicePar[nvoice].PFreqLfoEnabled!=0)||(!xml->minimal)) {
        xml->beginbranch("FREQUENCY_LFO");
        VoicePar[nvoice].FreqLfo->add2XML(xml);
        xml->endbranch();
    };
    xml->endbranch();


    if ((VoicePar[nvoice].PFilterEnabled!=0)||(!xml->minimal)) {
        xml->beginbranch("FILTER_PARAMETERS");
        xml->beginbranch("FILTER");
        VoicePar[nvoice].VoiceFilter->add2XML(xml);
        xml->endbranch();

        xml->addparbool("filter_envelope_enabled",VoicePar[nvoice].PFilterEnvelopeEnabled);
        if ((VoicePar[nvoice].PFilterEnvelopeEnabled!=0)||(!xml->minimal)) {
            xml->beginbranch("FILTER_ENVELOPE");
            VoicePar[nvoice].FilterEnvelope->add2XML(xml);
            xml->endbranch();
        };

        xml->addparbool("filter_lfo_enabled",VoicePar[nvoice].PFilterLfoEnabled);
        if ((VoicePar[nvoice].PFilterLfoEnabled!=0)||(!xml->minimal)) {
            xml->beginbranch("FILTER_LFO");
            VoicePar[nvoice].FilterLfo->add2XML(xml);
            xml->endbranch();
        };
        xml->endbranch();
    };

    if ((VoicePar[nvoice].PFMEnabled!=0)||(fmoscilused!=0)||(!xml->minimal)) {
        xml->beginbranch("FM_PARAMETERS");
        xml->addpar("input_voice",VoicePar[nvoice].PFMVoice);

        xml->addpar("volume",VoicePar[nvoice].PFMVolume);
        xml->addpar("volume_damp",VoicePar[nvoice].PFMVolumeDamp);
        xml->addpar("velocity_sensing",VoicePar[nvoice].PFMVelocityScaleFunction);

        xml->addparbool("amp_envelope_enabled",VoicePar[nvoice].PFMAmpEnvelopeEnabled);
        if ((VoicePar[nvoice].PFMAmpEnvelopeEnabled!=0)||(!xml->minimal)) {
            xml->beginbranch("AMPLITUDE_ENVELOPE");
            VoicePar[nvoice].FMAmpEnvelope->add2XML(xml);
            xml->endbranch();
        };
        xml->beginbranch("MODULATOR");
        xml->addpar("detune",VoicePar[nvoice].PFMDetune);
        xml->addpar("coarse_detune",VoicePar[nvoice].PFMCoarseDetune);
        xml->addpar("detune_type",VoicePar[nvoice].PFMDetuneType);

        xml->addparbool("freq_envelope_enabled",VoicePar[nvoice].PFMFreqEnvelopeEnabled);
        if ((VoicePar[nvoice].PFMFreqEnvelopeEnabled!=0)||(!xml->minimal)) {
            xml->beginbranch("FREQUENCY_ENVELOPE");
            VoicePar[nvoice].FMFreqEnvelope->add2XML(xml);
            xml->endbranch();
        };

        xml->beginbranch("OSCIL");
        VoicePar[nvoice].FMSmp->add2XML(xml);
        xml->endbranch();

        xml->endbranch();
        xml->endbranch();
    };
};


void ADnoteParameters::add2XML(XMLwrapper *xml)
{
    xml->addparbool("stereo",GlobalPar.PStereo);

    xml->beginbranch("AMPLITUDE_PARAMETERS");
    xml->addpar("volume",GlobalPar.PVolume);
    xml->addpar("panning",GlobalPar.PPanning);
    xml->addpar("velocity_sensing",GlobalPar.PAmpVelocityScaleFunction);
    xml->addpar("punch_strength",GlobalPar.PPunchStrength);
    xml->addpar("punch_time",GlobalPar.PPunchTime);
    xml->addpar("punch_stretch",GlobalPar.PPunchStretch);
    xml->addpar("punch_velocity_sensing",GlobalPar.PPunchVelocitySensing);
    xml->addpar("harmonic_randomness_grouping",GlobalPar.Hrandgrouping);

    xml->beginbranch("AMPLITUDE_ENVELOPE");
    GlobalPar.AmpEnvelope->add2XML(xml);
    xml->endbranch();

    xml->beginbranch("AMPLITUDE_LFO");
    GlobalPar.AmpLfo->add2XML(xml);
    xml->endbranch();
    xml->endbranch();

    xml->beginbranch("FREQUENCY_PARAMETERS");
    xml->addpar("detune",GlobalPar.PDetune);

    xml->addpar("coarse_detune",GlobalPar.PCoarseDetune);
    xml->addpar("detune_type",GlobalPar.PDetuneType);

    xml->addpar("bandwidth",GlobalPar.PBandwidth);

    xml->beginbranch("FREQUENCY_ENVELOPE");
    GlobalPar.FreqEnvelope->add2XML(xml);
    xml->endbranch();

    xml->beginbranch("FREQUENCY_LFO");
    GlobalPar.FreqLfo->add2XML(xml);
    xml->endbranch();
    xml->endbranch();


    xml->beginbranch("FILTER_PARAMETERS");
    xml->addpar("velocity_sensing_amplitude",GlobalPar.PFilterVelocityScale);
    xml->addpar("velocity_sensing",GlobalPar.PFilterVelocityScaleFunction);

    xml->beginbranch("FILTER");
    GlobalPar.GlobalFilter->add2XML(xml);
    xml->endbranch();

    xml->beginbranch("FILTER_ENVELOPE");
    GlobalPar.FilterEnvelope->add2XML(xml);
    xml->endbranch();

    xml->beginbranch("FILTER_LFO");
    GlobalPar.FilterLfo->add2XML(xml);
    xml->endbranch();
    xml->endbranch();

    xml->beginbranch("RESONANCE");
    GlobalPar.Reson->add2XML(xml);
    xml->endbranch();

    for (int nvoice=0;nvoice<NUM_VOICES;nvoice++) {
        xml->beginbranch("VOICE",nvoice);
        add2XMLsection(xml,nvoice);
        xml->endbranch();
    };
};


void ADnoteParameters::getfromXML(XMLwrapper *xml)
{
    GlobalPar.PStereo=xml->getparbool("stereo",GlobalPar.PStereo);

    if (xml->enterbranch("AMPLITUDE_PARAMETERS")) {
        GlobalPar.PVolume=xml->getpar127("volume",GlobalPar.PVolume);
        GlobalPar.PPanning=xml->getpar127("panning",GlobalPar.PPanning);
        GlobalPar.PAmpVelocityScaleFunction=xml->getpar127("velocity_sensing",GlobalPar.PAmpVelocityScaleFunction);

        GlobalPar.PPunchStrength=xml->getpar127("punch_strength",GlobalPar.PPunchStrength);
        GlobalPar.PPunchTime=xml->getpar127("punch_time",GlobalPar.PPunchTime);
        GlobalPar.PPunchStretch=xml->getpar127("punch_stretch",GlobalPar.PPunchStretch);
        GlobalPar.PPunchVelocitySensing=xml->getpar127("punch_velocity_sensing",GlobalPar.PPunchVelocitySensing);
        GlobalPar.Hrandgrouping=xml->getpar127("harmonic_randomness_grouping",GlobalPar.Hrandgrouping);

        if (xml->enterbranch("AMPLITUDE_ENVELOPE")) {
            GlobalPar.AmpEnvelope->getfromXML(xml);
            xml->exitbranch();
        };

        if (xml->enterbranch("AMPLITUDE_LFO")) {
            GlobalPar.AmpLfo->getfromXML(xml);
            xml->exitbranch();
        };

        xml->exitbranch();
    };

    if (xml->enterbranch("FREQUENCY_PARAMETERS")) {
        GlobalPar.PDetune=xml->getpar("detune",GlobalPar.PDetune,0,16383);
        GlobalPar.PCoarseDetune=xml->getpar("coarse_detune",GlobalPar.PCoarseDetune,0,16383);
        GlobalPar.PDetuneType=xml->getpar127("detune_type",GlobalPar.PDetuneType);

        GlobalPar.PBandwidth=xml->getpar127("bandwidth",GlobalPar.PBandwidth);

        xml->enterbranch("FREQUENCY_ENVELOPE");
        GlobalPar.FreqEnvelope->getfromXML(xml);
        xml->exitbranch();

        xml->enterbranch("FREQUENCY_LFO");
        GlobalPar.FreqLfo->getfromXML(xml);
        xml->exitbranch();

        xml->exitbranch();
    };


    if (xml->enterbranch("FILTER_PARAMETERS")) {
        GlobalPar.PFilterVelocityScale=xml->getpar127("velocity_sensing_amplitude",GlobalPar.PFilterVelocityScale);
        GlobalPar.PFilterVelocityScaleFunction=xml->getpar127("velocity_sensing",GlobalPar.PFilterVelocityScaleFunction);

        xml->enterbranch("FILTER");
        GlobalPar.GlobalFilter->getfromXML(xml);
        xml->exitbranch();

        xml->enterbranch("FILTER_ENVELOPE");
        GlobalPar.FilterEnvelope->getfromXML(xml);
        xml->exitbranch();

        xml->enterbranch("FILTER_LFO");
        GlobalPar.FilterLfo->getfromXML(xml);
        xml->exitbranch();
        xml->exitbranch();
    };

    if (xml->enterbranch("RESONANCE")) {
        GlobalPar.Reson->getfromXML(xml);
        xml->exitbranch();
    };

    for (int nvoice=0;nvoice<NUM_VOICES;nvoice++) {
        VoicePar[nvoice].Enabled=0;
        if (xml->enterbranch("VOICE",nvoice)==0) continue;
        getfromXMLsection(xml,nvoice);
        xml->exitbranch();
    };


};

void ADnoteParameters::getfromXMLsection(XMLwrapper *xml,int n)
{
    int nvoice=n;
    if (nvoice>=NUM_VOICES) return;

    VoicePar[nvoice].Enabled=xml->getparbool("enabled",0);

    VoicePar[nvoice].Type=xml->getpar127("type",VoicePar[nvoice].Type);
    VoicePar[nvoice].PDelay=xml->getpar127("delay",VoicePar[nvoice].PDelay);
    VoicePar[nvoice].Presonance=xml->getparbool("resonance",VoicePar[nvoice].Presonance);

    VoicePar[nvoice].Pextoscil=xml->getpar("ext_oscil",-1,-1,nvoice-1);
    VoicePar[nvoice].PextFMoscil=xml->getpar("ext_fm_oscil",-1,-1,nvoice-1);

    VoicePar[nvoice].Poscilphase=xml->getpar127("oscil_phase",VoicePar[nvoice].Poscilphase);
    VoicePar[nvoice].PFMoscilphase=xml->getpar127("oscil_fm_phase",VoicePar[nvoice].PFMoscilphase);

    VoicePar[nvoice].PFilterEnabled=xml->getparbool("filter_enabled",VoicePar[nvoice].PFilterEnabled);
    VoicePar[nvoice].Pfilterbypass=xml->getparbool("filter_bypass",VoicePar[nvoice].Pfilterbypass);

    VoicePar[nvoice].PFMEnabled=xml->getpar127("fm_enabled",VoicePar[nvoice].PFMEnabled);

    if (xml->enterbranch("OSCIL")) {
        VoicePar[nvoice].OscilSmp->getfromXML(xml);
        xml->exitbranch();
    };


    if (xml->enterbranch("AMPLITUDE_PARAMETERS")) {
        VoicePar[nvoice].PPanning=xml->getpar127("panning",VoicePar[nvoice].PPanning);
        VoicePar[nvoice].PVolume=xml->getpar127("volume",VoicePar[nvoice].PVolume);
        VoicePar[nvoice].PVolumeminus=xml->getparbool("volume_minus",VoicePar[nvoice].PVolumeminus);
        VoicePar[nvoice].PAmpVelocityScaleFunction=xml->getpar127("velocity_sensing",VoicePar[nvoice].PAmpVelocityScaleFunction);

        VoicePar[nvoice].PAmpEnvelopeEnabled=xml->getparbool("amp_envelope_enabled",VoicePar[nvoice].PAmpEnvelopeEnabled);
        if (xml->enterbranch("AMPLITUDE_ENVELOPE")) {
            VoicePar[nvoice].AmpEnvelope->getfromXML(xml);
            xml->exitbranch();
        };

        VoicePar[nvoice].PAmpLfoEnabled=xml->getparbool("amp_lfo_enabled",VoicePar[nvoice].PAmpLfoEnabled);
        if (xml->enterbranch("AMPLITUDE_LFO")) {
            VoicePar[nvoice].AmpLfo->getfromXML(xml);
            xml->exitbranch();
        };
        xml->exitbranch();
    };

    if (xml->enterbranch("FREQUENCY_PARAMETERS")) {
        VoicePar[nvoice].Pfixedfreq=xml->getparbool("fixed_freq",VoicePar[nvoice].Pfixedfreq);
        VoicePar[nvoice].PfixedfreqET=xml->getpar127("fixed_freq_et",VoicePar[nvoice].PfixedfreqET);


        VoicePar[nvoice].PDetune=xml->getpar("detune",VoicePar[nvoice].PDetune,0,16383);

        VoicePar[nvoice].PCoarseDetune=xml->getpar("coarse_detune",VoicePar[nvoice].PCoarseDetune,0,16383);
        VoicePar[nvoice].PDetuneType=xml->getpar127("detune_type",VoicePar[nvoice].PDetuneType);

        VoicePar[nvoice].PFreqEnvelopeEnabled=xml->getparbool("freq_envelope_enabled",VoicePar[nvoice].PFreqEnvelopeEnabled);
        if (xml->enterbranch("FREQUENCY_ENVELOPE")) {
            VoicePar[nvoice].FreqEnvelope->getfromXML(xml);
            xml->exitbranch();
        };

        VoicePar[nvoice].PFreqLfoEnabled=xml->getparbool("freq_lfo_enabled",VoicePar[nvoice].PFreqLfoEnabled);
        if (xml->enterbranch("FREQUENCY_LFO")) {
            VoicePar[nvoice].FreqLfo->getfromXML(xml);
            xml->exitbranch();
        };
        xml->exitbranch();
    };

    if (xml->enterbranch("FILTER_PARAMETERS")) {
        if (xml->enterbranch("FILTER")) {
            VoicePar[nvoice].VoiceFilter->getfromXML(xml);
            xml->exitbranch();
        };

        VoicePar[nvoice].PFilterEnvelopeEnabled=xml->getparbool("filter_envelope_enabled",VoicePar[nvoice].PFilterEnvelopeEnabled);
        if (xml->enterbranch("FILTER_ENVELOPE")) {
            VoicePar[nvoice].FilterEnvelope->getfromXML(xml);
            xml->exitbranch();
        };

        VoicePar[nvoice].PFilterLfoEnabled=xml->getparbool("filter_lfo_enabled",VoicePar[nvoice].PFilterLfoEnabled);
        if (xml->enterbranch("FILTER_LFO")) {
            VoicePar[nvoice].FilterLfo->getfromXML(xml);
            xml->exitbranch();
        };
        xml->exitbranch();
    };

    if (xml->enterbranch("FM_PARAMETERS")) {
        VoicePar[nvoice].PFMVoice=xml->getpar("input_voice",VoicePar[nvoice].PFMVoice,-1,nvoice-1);

        VoicePar[nvoice].PFMVolume=xml->getpar127("volume",VoicePar[nvoice].PFMVolume);
        VoicePar[nvoice].PFMVolumeDamp=xml->getpar127("volume_damp",VoicePar[nvoice].PFMVolumeDamp);
        VoicePar[nvoice].PFMVelocityScaleFunction=xml->getpar127("velocity_sensing",VoicePar[nvoice].PFMVelocityScaleFunction);

        VoicePar[nvoice].PFMAmpEnvelopeEnabled=xml->getparbool("amp_envelope_enabled",VoicePar[nvoice].PFMAmpEnvelopeEnabled);
        if (xml->enterbranch("AMPLITUDE_ENVELOPE")) {
            VoicePar[nvoice].FMAmpEnvelope->getfromXML(xml);
            xml->exitbranch();
        };

        if (xml->enterbranch("MODULATOR")) {
            VoicePar[nvoice].PFMDetune=xml->getpar("detune",VoicePar[nvoice].PFMDetune,0,16383);
            VoicePar[nvoice].PFMCoarseDetune=xml->getpar("coarse_detune",VoicePar[nvoice].PFMCoarseDetune,0,16383);
            VoicePar[nvoice].PFMDetuneType=xml->getpar127("detune_type",VoicePar[nvoice].PFMDetuneType);

            VoicePar[nvoice].PFMFreqEnvelopeEnabled=xml->getparbool("freq_envelope_enabled",VoicePar[nvoice].PFMFreqEnvelopeEnabled);
            if (xml->enterbranch("FREQUENCY_ENVELOPE")) {
                VoicePar[nvoice].FMFreqEnvelope->getfromXML(xml);
                xml->exitbranch();
            };

            if (xml->enterbranch("OSCIL")) {
                VoicePar[nvoice].FMSmp->getfromXML(xml);
                xml->exitbranch();
            };

            xml->exitbranch();
        };
        xml->exitbranch();
    };
};


