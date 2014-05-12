/*
  ZynAddSubFX - a software synthesizer

  ADnoteParameters.h - Parameters for ADnote (ADsynth)
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

#ifndef AD_NOTE_PARAMETERS_H
#define AD_NOTE_PARAMETERS_H


#include "../globals.h"
#include "../Misc/Util.h"
#include "PresetsArray.h"

class EnvelopeParams;
class LFOParams;
class FilterParams;
class Resonance;
class OscilGen;
class FFTwrapper;

enum FMTYPE {
    NONE, MORPH, RING_MOD, PHASE_MOD, FREQ_MOD, PITCH_MOD
};
extern int ADnote_unison_sizes[];

/*****************************************************************/
/*                    GLOBAL PARAMETERS                          */
/*****************************************************************/

struct ADnoteGlobalParam {
    ADnoteGlobalParam();
    ~ADnoteGlobalParam();
    void defaults();
    void add2XML(XMLwrapper *xml);
    void getfromXML(XMLwrapper *xml);
    /* The instrument type  - MONO/STEREO
    If the mode is MONO, the panning of voices are not used
    Stereo=1, Mono=0. */

    unsigned char PStereo;


    /******************************************
    *     FREQUENCY GLOBAL PARAMETERS        *
    ******************************************/
    unsigned short int PDetune; //fine detune
    unsigned short int PCoarseDetune; //coarse detune+octave
    unsigned char      PDetuneType; //detune type

    unsigned char PBandwidth;      //how much the relative fine detunes of the voices are changed

    EnvelopeParams *FreqEnvelope;    //Frequency Envelope

    LFOParams *FreqLfo; //Frequency LFO

    /********************************************
    *     AMPLITUDE GLOBAL PARAMETERS          *
    ********************************************/

    /* Panning -  0 - random
              1 - left
             64 - center
            127 - right */
    unsigned char PPanning;

    unsigned char PVolume;

    unsigned char PAmpVelocityScaleFunction;

    EnvelopeParams *AmpEnvelope;

    LFOParams *AmpLfo;

    unsigned char PPunchStrength, PPunchTime, PPunchStretch,
                  PPunchVelocitySensing;

    /******************************************
    *        FILTER GLOBAL PARAMETERS        *
    ******************************************/
    FilterParams *GlobalFilter;

    // filter velocity sensing
    unsigned char PFilterVelocityScale;

    // filter velocity sensing
    unsigned char PFilterVelocityScaleFunction;

    EnvelopeParams *FilterEnvelope;

    LFOParams *FilterLfo;

    // RESONANCE
    Resonance *Reson;

    //how the randomness is applied to the harmonics on more voices using the same oscillator
    unsigned char Hrandgrouping;
};



/***********************************************************/
/*                    VOICE PARAMETERS                     */
/***********************************************************/
struct ADnoteVoiceParam {
    void getfromXML(XMLwrapper *xml, unsigned nvoice);
    void add2XML(XMLwrapper *xml, bool fmoscilused);
    void defaults();
    void enable(FFTwrapper *fft, Resonance *Reson);
    void kill();
    /** If the voice is enabled */
    unsigned char Enabled;

    /** How many subvoices are used in this voice */
    unsigned char Unison_size;

    /** How subvoices are spread */
    unsigned char Unison_frequency_spread;

    /** Stereo spread of the subvoices*/
    unsigned char Unison_stereo_spread;

    /** Vibratto of the subvoices (which makes the unison more "natural")*/
    unsigned char Unison_vibratto;

    /** Medium speed of the vibratto of the subvoices*/
    unsigned char Unison_vibratto_speed;

    /** Unison invert phase */
    unsigned char Unison_invert_phase; //0=none,1=random,2=50%,3=33%,4=25%

    /** Type of the voice (0=Sound,1=Noise)*/
    unsigned char Type;

    /** Voice Delay */
    unsigned char PDelay;

    /** If the resonance is enabled for this voice */
    unsigned char Presonance;

    // What external oscil should I use, -1 for internal OscilSmp&FMSmp
    short int Pextoscil, PextFMoscil;
    // it is not allowed that the externoscil,externFMoscil => current voice

    // oscillator phases
    unsigned char Poscilphase, PFMoscilphase;

    // filter bypass
    unsigned char Pfilterbypass;

    /** Voice oscillator */
    OscilGen *OscilSmp;

    /**********************************
    *     FREQUENCY PARAMETERS        *
    **********************************/

    /** If the base frequency is fixed to 440 Hz*/
    unsigned char Pfixedfreq;

    /* Equal temperate (this is used only if the Pfixedfreq is enabled)
       If this parameter is 0, the frequency is fixed (to 440 Hz);
       if this parameter is 64, 1 MIDI halftone -> 1 frequency halftone */
    unsigned char PfixedfreqET;

    /** Fine detune */
    unsigned short int PDetune;

    /** Coarse detune + octave */
    unsigned short int PCoarseDetune;

    /** Detune type */
    unsigned char PDetuneType;

    /* Frequency Envelope */
    unsigned char   PFreqEnvelopeEnabled;
    EnvelopeParams *FreqEnvelope;

    /* Frequency LFO */
    unsigned char PFreqLfoEnabled;
    LFOParams    *FreqLfo;


    /***************************
    *   AMPLITUDE PARAMETERS   *
    ***************************/

    /* Panning       0 - random
             1 - left
                64 - center
               127 - right
       The Panning is ignored if the instrument is mono */
    unsigned char PPanning;

    /* Voice Volume */
    unsigned char PVolume;

    /* If the Volume negative */
    unsigned char PVolumeminus;

    /* Velocity sensing */
    unsigned char PAmpVelocityScaleFunction;

    /* Amplitude Envelope */
    unsigned char   PAmpEnvelopeEnabled;
    EnvelopeParams *AmpEnvelope;

    /* Amplitude LFO */
    unsigned char PAmpLfoEnabled;
    LFOParams    *AmpLfo;



    /*************************
    *   FILTER PARAMETERS    *
    *************************/

    /* Voice Filter */
    unsigned char PFilterEnabled;
    FilterParams *VoiceFilter;

    /* Filter Envelope */
    unsigned char   PFilterEnvelopeEnabled;
    EnvelopeParams *FilterEnvelope;

    /* LFO Envelope */
    unsigned char PFilterLfoEnabled;
    LFOParams    *FilterLfo;

    /****************************
    *   MODULLATOR PARAMETERS   *
    ****************************/

    /* Modullator Parameters (0=off,1=Morph,2=RM,3=PM,4=FM.. */
    unsigned char PFMEnabled;

    /* Voice that I use as modullator instead of FMSmp.
       It is -1 if I use FMSmp(default).
       It maynot be equal or bigger than current voice */
    short int PFMVoice;

    /* Modullator oscillator */
    OscilGen *FMSmp;

    /* Modullator Volume */
    unsigned char PFMVolume;

    /* Modullator damping at higher frequencies */
    unsigned char PFMVolumeDamp;

    /* Modullator Velocity Sensing */
    unsigned char PFMVelocityScaleFunction;

    /* Fine Detune of the Modullator*/
    unsigned short int PFMDetune;

    /* Coarse Detune of the Modullator */
    unsigned short int PFMCoarseDetune;

    /* The detune type */
    unsigned char PFMDetuneType;

    /* Frequency Envelope of the Modullator */
    unsigned char   PFMFreqEnvelopeEnabled;
    EnvelopeParams *FMFreqEnvelope;

    /* Frequency Envelope of the Modullator */
    unsigned char   PFMAmpEnvelopeEnabled;
    EnvelopeParams *FMAmpEnvelope;
};

class ADnoteParameters:public PresetsArray
{
    public:
        ADnoteParameters(FFTwrapper *fft_);
        ~ADnoteParameters();

        ADnoteGlobalParam GlobalPar;
        ADnoteVoiceParam  VoicePar[NUM_VOICES];

        void defaults();
        void add2XML(XMLwrapper *xml);
        void getfromXML(XMLwrapper *xml);

        float getBandwidthDetuneMultiplier();
        float getUnisonFrequencySpreadCents(int nvoice);
        int get_unison_size_index(int nvoice);
        void set_unison_size_index(int nvoice, int index);
    private:
        void defaults(int n); //n is the nvoice

        void EnableVoice(int nvoice);
        void KillVoice(int nvoice);
        FFTwrapper *fft;

        void add2XMLsection(XMLwrapper *xml, int n);
        void getfromXMLsection(XMLwrapper *xml, int n);
};

#endif
