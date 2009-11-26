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
        /**Constructor.
         * @param pars Note Parameters
         * @param ctl_ Pointer to system Controller
         * @param freq Base frequency for note
         * @param velocity Velocity of note
         * @param portamento_ 1 if the note has portamento
         * @param midinote_ The midi number of the note
         * @param besilent Start silent note if true*/
        ADnote(ADnoteParameters *pars, Controller *ctl_, REALTYPE freq,
               REALTYPE velocity, int portamento_, int midinote_,
               bool besilent);
        /**Destructor*/
        ~ADnote();

        /**Alters the playing note for legato effect*/
        void ADlegatonote(REALTYPE freq, REALTYPE velocity, int portamento_,
                          int midinote_, bool externcall);

        /**Compute ADnote Samples.
         * @return 0 if note is finished*/
        int noteout(REALTYPE *outl, REALTYPE *outr);

        /**Release the key for the note and start release portion of envelopes.*/
        void relasekey();
        /**Return if note is finished.
         * @return finished=1 unfinished=0*/
        int finished() const;


        /**Nonzero when ready for output(the parameters has been computed)
         * zero when parameters need to be computed.*/
        char ready;

    private:

        /**Changes the frequency of an oscillator.
         * @param nvoice voice to run computations on
         * @param in_freq new frequency*/
        void setfreq(int nvoice, REALTYPE in_freq);
        /**Set the frequency of the modulator oscillator*/
        void setfreqFM(int nvoice, REALTYPE in_freq);
        /**Computes relative frequency for unison and unison's vibratto.
         * Note: Must be called before setfreq* functions.*/
        void compute_unison_freq_rap(int nvoice);
        /**Compute parameters for next tick*/
        void computecurrentparameters();
        /**Initializes All Parameters*/
        void initparameters();
        /**Deallocate/Cleanup given voice*/
        void KillVoice(int nvoice);
        /**Deallocate Note resources and voice resources*/
        void KillNote();
        /**Get the Voice's base frequency*/
        inline REALTYPE getvoicebasefreq(int nvoice) const;
        /**Get modulator's base frequency*/
        inline REALTYPE getFMvoicebasefreq(int nvoice) const;
        /**Compute the Oscillator's samples.
         * Affects tmpwave_unison and updates oscposhi/oscposlo*/
        inline void ComputeVoiceOscillator_LinearInterpolation(int nvoice);
        /**Compute the Oscillator's samples.
         * Affects tmpwave_unison and updates oscposhi/oscposlo
         * @todo remove this declaration if it is commented out*/
        inline void ComputeVoiceOscillator_CubicInterpolation(int nvoice);
        /**Computes the Oscillator samples with morphing.
         * updates tmpwave_unison*/
        inline void ComputeVoiceOscillatorMorph(int nvoice);
        /**Computes the Ring Modulated Oscillator.*/
        inline void ComputeVoiceOscillatorRingModulation(int nvoice);
        /**Computes the Frequency Modulated Oscillator.
         * @param FMmode modulation type 0=Phase 1=Frequency*/
        inline void ComputeVoiceOscillatorFrequencyModulation(int nvoice,
                                                              int FMmode);       //FMmode=0 for phase modulation, 1 for Frequency modulation
        //  inline void ComputeVoiceOscillatorFrequencyModulation(int nvoice);
        /**TODO*/
        inline void ComputeVoiceOscillatorPitchModulation(int nvoice);

        /**Generate Noise Samples for Voice*/
        inline void ComputeVoiceNoise(int nvoice);

        /**Fadein in a way that removes clicks but keep sound "punchy"*/
        inline void fadein(REALTYPE *smps) const;


        //GLOBALS
        ADnoteParameters *partparams;
        unsigned char     stereo; //if the note is stereo (allows note Panning)
        int      midinote;
        REALTYPE velocity, basefreq;

        ONOFFTYPE   NoteEnabled;
        Controller *ctl;

        /*****************************************************************/
        /*                    GLOBAL PARAMETERS                          */
        /*****************************************************************/

        struct ADnoteGlobal {
            /******************************************
            *     FREQUENCY GLOBAL PARAMETERS        *
            ******************************************/
            REALTYPE  Detune; //cents

            Envelope *FreqEnvelope;
            LFO      *FreqLfo;

            /********************************************
            *     AMPLITUDE GLOBAL PARAMETERS          *
            ********************************************/
            REALTYPE  Volume; // [ 0 .. 1 ]

            REALTYPE  Panning; // [ 0 .. 1 ]

            Envelope *AmpEnvelope;
            LFO      *AmpLfo;

            struct {
                int      Enabled;
                REALTYPE initialvalue, dt, t;
            } Punch;

            /******************************************
            *        FILTER GLOBAL PARAMETERS        *
            ******************************************/
            Filter   *GlobalFilterL, *GlobalFilterR;

            REALTYPE  FilterCenterPitch; //octaves
            REALTYPE  FilterQ;
            REALTYPE  FilterFreqTracking;

            Envelope *FilterEnvelope;

            LFO      *FilterLfo;
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
            int fixedfreq; //if the frequency is fixed to 440 Hz
            int fixedfreqET; //if the "fixed" frequency varies according to the note (ET)

            // cents = basefreq*VoiceDetune
            REALTYPE  Detune, FineDetune;

            Envelope *FreqEnvelope;
            LFO      *FreqLfo;


            /***************************
            *   AMPLITUDE PARAMETERS   *
            ***************************/

            /* Panning 0.0=left, 0.5 - center, 1.0 = right */
            REALTYPE  Panning;
            REALTYPE  Volume; // [-1.0 .. 1.0]

            Envelope *AmpEnvelope;
            LFO      *AmpLfo;

            /*************************
            *   FILTER PARAMETERS    *
            *************************/

            Filter   *VoiceFilterL;
            Filter   *VoiceFilterR;

            REALTYPE  FilterCenterPitch; /* Filter center Pitch*/
            REALTYPE  FilterFreqTracking;

            Envelope *FilterEnvelope;
            LFO      *FilterLfo;


            /****************************
            *   MODULLATOR PARAMETERS   *
            ****************************/

            FMTYPE FMEnabled;

            int    FMVoice;

            // Voice Output used by other voices if use this as modullator
            REALTYPE *VoiceOut;

            /* Wave of the Voice */
            REALTYPE *FMSmp;

            REALTYPE  FMVolume;
            REALTYPE  FMDetune; //in cents

            Envelope *FMFreqEnvelope;
            Envelope *FMAmpEnvelope;
        } NoteVoicePar[NUM_VOICES];


        /********************************************************/
        /*    INTERNAL VALUES OF THE NOTE AND OF THE VOICES     */
        /********************************************************/

        //time from the start of the note
        REALTYPE time;

        //the size of unison for a single voice
        int unison_size[NUM_VOICES];

        //the stereo spread of the unison subvoices (0.0=mono,1.0=max)
        REALTYPE unison_stereo_spread[NUM_VOICES];

        //fractional part (skip)
        REALTYPE *oscposlo[NUM_VOICES], *oscfreqlo[NUM_VOICES];

        //integer part (skip)
        int *oscposhi[NUM_VOICES], *oscfreqhi[NUM_VOICES];

        //fractional part (skip) of the Modullator
        REALTYPE *oscposloFM[NUM_VOICES], *oscfreqloFM[NUM_VOICES];

        //the unison base_value
        REALTYPE *unison_base_freq_rap[NUM_VOICES];

        //how the unison subvoice's frequency is changed (1.0 for no change)
        REALTYPE *unison_freq_rap[NUM_VOICES];

        //which subvoice has phase inverted
        bool *unison_invert_phase[NUM_VOICES];

        //unison vibratto
        struct {
            REALTYPE  amplitude; //amplitude which be added to unison_freq_rap
            REALTYPE *step; //value which increments the position
            REALTYPE *position; //between -1.0 and 1.0
        } unison_vibratto[NUM_VOICES];


        //integer part (skip) of the Modullator
        unsigned int *oscposhiFM[NUM_VOICES], *oscfreqhiFM[NUM_VOICES];

        //used to compute and interpolate the amplitudes of voices and modullators
        REALTYPE oldamplitude[NUM_VOICES],
                 newamplitude[NUM_VOICES],
                 FMoldamplitude[NUM_VOICES],
                 FMnewamplitude[NUM_VOICES];

        //used by Frequency Modulation (for integration)
        REALTYPE *FMoldsmp[NUM_VOICES];

        //temporary buffer
        REALTYPE *tmpwavel;
        REALTYPE *tmpwaver;
        int max_unison;
        REALTYPE **tmpwave_unison;

        //Filter bypass samples
        REALTYPE *bypassl, *bypassr;

        //interpolate the amplitudes
        REALTYPE globaloldamplitude, globalnewamplitude;

        //1 - if it is the fitst tick (used to fade in the sound)
        char firsttick[NUM_VOICES];

        //1 if the note has portamento
        int portamento;

        //how the fine detunes are made bigger or smaller
        REALTYPE bandwidthDetuneMultiplier;

        // Legato vars
        struct {
            bool      silent;
            REALTYPE  lastfreq;
            LegatoMsg msg;
            int decounter;
            struct { // Fade In/Out vars
                int      length;
                REALTYPE m, step;
            } fade;
            struct { // Note parameters
                REALTYPE freq, vel;
                int      portamento, midinote;
            } param;
        } Legato;
};

#endif

