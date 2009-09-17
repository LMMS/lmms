/*
  ZynAddSubFX - a software synthesizer

  ADnote.h - The "additive" synthesizer
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

#ifndef AD_NOTE_H
#define AD_NOTE_H

#include "../globals.h"
#include "Envelope.h"
#include "LFO.h"
#include "../DSP/Filter.h"
#include "../Params/ADnoteParameters.h"
#include "../Params/Controller.h"

//Globals

/**FM amplitude tune*/
#define FM_AMP_MULTIPLIER 14.71280603

#define OSCIL_SMP_EXTRA_SAMPLES 5

/**The "additive" synthesizer*/
class ADnote    //ADDitive note
{
public:
    ADnote(ADnoteParameters *pars,Controller *ctl_,REALTYPE freq,REALTYPE velocity,int portamento_,int midinote_,bool besilent);//(gf)Added the besilent parameter to tell it to start silent (if true).
    ~ADnote();

    void ADlegatonote(REALTYPE freq, REALTYPE velocity, int portamento_, int midinote_, bool externcall);

    int noteout(REALTYPE *outl,REALTYPE *outr);
    void relasekey();
    int finished();


    /*ready - this is 0 if it is not ready (the parameters has to be computed)
     or other value if the parameters has been computed and if it is ready to output*/
    char ready;

private:

    void setfreq(int nvoice,REALTYPE freq);
    void setfreqFM(int nvoice,REALTYPE freq);
    void computecurrentparameters();
    void initparameters();
    void KillVoice(int nvoice);
    void KillNote();
    inline REALTYPE getvoicebasefreq(int nvoice);
    inline REALTYPE getFMvoicebasefreq(int nvoice);
    inline void ComputeVoiceOscillator_LinearInterpolation(int nvoice);
    inline void ComputeVoiceOscillator_CubicInterpolation(int nvoice);
    inline void ComputeVoiceOscillatorMorph(int nvoice);
    inline void ComputeVoiceOscillatorRingModulation(int nvoice);
    inline void ComputeVoiceOscillatorFrequencyModulation(int nvoice,int FMmode);//FMmode=0 for phase modulation, 1 for Frequency modulation
    //  inline void ComputeVoiceOscillatorFrequencyModulation(int nvoice);
    inline void ComputeVoiceOscillatorPitchModulation(int nvoice);

    inline void ComputeVoiceNoise(int nvoice);

    inline void fadein(REALTYPE *smps);


    //GLOBALS
    ADnoteParameters *partparams;
    unsigned char stereo;//if the note is stereo (allows note Panning)
    int midinote;
    REALTYPE velocity,basefreq;

    ONOFFTYPE NoteEnabled;
    Controller *ctl;

    /*****************************************************************/
    /*                    GLOBAL PARAMETERS                          */
    /*****************************************************************/

    struct ADnoteGlobal {
        /******************************************
        *     FREQUENCY GLOBAL PARAMETERS        *
        ******************************************/
        REALTYPE Detune;//cents

        Envelope *FreqEnvelope;
        LFO *FreqLfo;

        /********************************************
        *     AMPLITUDE GLOBAL PARAMETERS          *
        ********************************************/
        REALTYPE Volume;// [ 0 .. 1 ]

        REALTYPE Panning;// [ 0 .. 1 ]

        Envelope *AmpEnvelope;
        LFO *AmpLfo;

        struct {
            int Enabled;
            REALTYPE initialvalue,dt,t;
        } Punch;

        /******************************************
        *        FILTER GLOBAL PARAMETERS        *
        ******************************************/
        Filter *GlobalFilterL,*GlobalFilterR;

        REALTYPE FilterCenterPitch;//octaves
        REALTYPE FilterQ;
        REALTYPE FilterFreqTracking;

        Envelope *FilterEnvelope;

        LFO *FilterLfo;
    } NoteGlobalPar;



    /***********************************************************/
    /*                    VOICE PARAMETERS                     */
    /***********************************************************/
    struct ADnoteVoice {
        /* If the voice is enabled */
        ONOFFTYPE Enabled;

        /* Voice Type (sound/noise)*/
        int noisetype;

        /* Filter Bypass */
        int filterbypass;

        /* Delay (ticks) */
        int DelayTicks;

        /* Waveform of the Voice */
        REALTYPE *OscilSmp;

        /************************************
        *     FREQUENCY PARAMETERS          *
        ************************************/
        int fixedfreq;//if the frequency is fixed to 440 Hz
        int fixedfreqET;//if the "fixed" frequency varies according to the note (ET)

        // cents = basefreq*VoiceDetune
        REALTYPE Detune,FineDetune;

        Envelope *FreqEnvelope;
        LFO *FreqLfo;


        /***************************
        *   AMPLITUDE PARAMETERS   *
        ***************************/

        /* Panning 0.0=left, 0.5 - center, 1.0 = right */
        REALTYPE Panning;
        REALTYPE Volume;// [-1.0 .. 1.0]

        Envelope *AmpEnvelope;
        LFO *AmpLfo;

        /*************************
        *   FILTER PARAMETERS    *
        *************************/

        Filter *VoiceFilter;

        REALTYPE FilterCenterPitch;/* Filter center Pitch*/
        REALTYPE FilterFreqTracking;

        Envelope *FilterEnvelope;
        LFO *FilterLfo;


        /****************************
        *   MODULLATOR PARAMETERS   *
        ****************************/

        FMTYPE FMEnabled;

        int FMVoice;

        // Voice Output used by other voices if use this as modullator
        REALTYPE *VoiceOut;

        /* Wave of the Voice */
        REALTYPE *FMSmp;

        REALTYPE FMVolume;
        REALTYPE FMDetune; //in cents

        Envelope *FMFreqEnvelope;
        Envelope *FMAmpEnvelope;
    } NoteVoicePar[NUM_VOICES];


    /********************************************************/
    /*    INTERNAL VALUES OF THE NOTE AND OF THE VOICES     */
    /********************************************************/

    //time from the start of the note
    REALTYPE time;

    //fractional part (skip)
    REALTYPE oscposlo[NUM_VOICES],oscfreqlo[NUM_VOICES];

    //integer part (skip)
    int oscposhi[NUM_VOICES],oscfreqhi[NUM_VOICES];

    //fractional part (skip) of the Modullator
    REALTYPE oscposloFM[NUM_VOICES],oscfreqloFM[NUM_VOICES];

    //integer part (skip) of the Modullator
    unsigned short int oscposhiFM[NUM_VOICES],oscfreqhiFM[NUM_VOICES];

    //used to compute and interpolate the amplitudes of voices and modullators
    REALTYPE oldamplitude[NUM_VOICES],
    newamplitude[NUM_VOICES],
    FMoldamplitude[NUM_VOICES],
    FMnewamplitude[NUM_VOICES];

    //used by Frequency Modulation (for integration)
    REALTYPE FMoldsmp[NUM_VOICES];

    //temporary buffer
    REALTYPE *tmpwave;

    //Filter bypass samples
    REALTYPE *bypassl,*bypassr;

    //interpolate the amplitudes
    REALTYPE globaloldamplitude,globalnewamplitude;

    //1 - if it is the fitst tick (used to fade in the sound)
    char firsttick[NUM_VOICES];

    //1 if the note has portamento
    int portamento;

    //how the fine detunes are made bigger or smaller
    REALTYPE bandwidthDetuneMultiplier;

    // Legato vars
    struct {
        bool silent;
        REALTYPE lastfreq;
        LegatoMsg msg;
        int decounter;
        struct { // Fade In/Out vars
            int length;
            REALTYPE m, step;
        } fade;
        struct { // Note parameters
            REALTYPE freq, vel;
            int portamento, midinote;
        } param;
    } Legato;
};

#endif




