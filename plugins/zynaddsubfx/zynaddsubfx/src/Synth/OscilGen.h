/*
  ZynAddSubFX - a software synthesizer

  OscilGen.h - Waveform generator for ADnote
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

#ifndef OSCIL_GEN_H
#define OSCIL_GEN_H

#include "../globals.h"
#include "../Misc/XMLwrapper.h"
#include "../DSP/FFTwrapper.h"
#include "../Params/Presets.h"
#include "Resonance.h"

class OscilGen:public Presets
{
    public:
        OscilGen(FFTwrapper *fft_, Resonance *res_);
        ~OscilGen();

        /**computes the full spectrum of oscil from harmonics,phases and basefunc*/
        void prepare();

        /**do the antialiasing(cut off higher freqs.),apply randomness and do a IFFT*/
        //returns where should I start getting samples, used in block type randomness
        short get(float *smps, float freqHz, int resonance = 0);
        //if freqHz is smaller than 0, return the "un-randomized" sample for UI

        void getbasefunction(float *smps);

        //called by UI
        void getspectrum(int n, float *spc, int what); //what=0 pt. oscil,1 pt. basefunc
        void getcurrentbasefunction(float *smps);
        /**convert oscil to base function*/
        void useasbase();

        void add2XML(XMLwrapper *xml);
        void defaults();
        void getfromXML(XMLwrapper *xml);

        void convert2sine();

        //Parameters

        /**
         * The hmag and hphase starts counting from 0, so the first harmonic(1) has the index 0,
         * 2-nd harmonic has index 1, ..the 128 harminic has index 127
         */
        unsigned char Phmag[MAX_AD_HARMONICS], Phphase[MAX_AD_HARMONICS]; //the MIDI parameters for mag. and phases


        /**The Type of magnitude:
         *   0 - Linear
         *   1 - dB scale (-40)
         *   2 - dB scale (-60)
         *   3 - dB scale (-80)
         *   4 - dB scale (-100)*/
        unsigned char Phmagtype;

        unsigned char Pcurrentbasefunc; //The base function used - 0=sin, 1=...
        unsigned char Pbasefuncpar; //the parameter of the base function

        unsigned char Pbasefuncmodulation; //what modulation is applied to the basefunc
        unsigned char Pbasefuncmodulationpar1, Pbasefuncmodulationpar2,
                      Pbasefuncmodulationpar3; //the parameter of the base function modulation

        /*the Randomness:
          64=no randomness
          63..0 - block type randomness - 0 is maximum
          65..127 - each harmonic randomness - 127 is maximum*/
        unsigned char Prand;
        unsigned char Pwaveshaping, Pwaveshapingfunction;
        unsigned char Pfiltertype, Pfilterpar1, Pfilterpar2;
        unsigned char Pfilterbeforews;
        unsigned char Psatype, Psapar; //spectrum adjust

        unsigned char Pamprandpower, Pamprandtype; //amplitude randomness
        int Pharmonicshift; //how the harmonics are shifted
        int Pharmonicshiftfirst; //if the harmonic shift is done before waveshaping and filter

        unsigned char Padaptiveharmonics; //the adaptive harmonics status (off=0,on=1,etc..)
        unsigned char Padaptiveharmonicsbasefreq; //the base frequency of the adaptive harmonic (30..3000Hz)
        unsigned char Padaptiveharmonicspower; //the strength of the effect (0=off,100=full)
        unsigned char Padaptiveharmonicspar; //the parameters in 2,3,4.. modes of adaptive harmonics

        unsigned char Pmodulation; //what modulation is applied to the oscil
        unsigned char Pmodulationpar1, Pmodulationpar2, Pmodulationpar3; //the parameter of the parameters


        //makes a new random seed for Amplitude Randomness
        //this should be called every note on event
        void newrandseed(unsigned int randseed);

        bool ADvsPAD; //if it is used by ADsynth or by PADsynth

    private:
        //This array stores some termporary data and it has OSCIL_SIZE elements
        float *tmpsmps;
        fft_t *outoscilFFTfreqs;

        float hmag[MAX_AD_HARMONICS], hphase[MAX_AD_HARMONICS]; //the magnituides and the phases of the sine/nonsine harmonics
//    private:
        FFTwrapper *fft;
        //computes the basefunction and make the FFT; newbasefunc<0  = same basefunc
        void changebasefunction();
        //Waveshaping
        void waveshape();

        //Filter the oscillator accotding to Pfiltertype and Pfilterpar
        void oscilfilter();

        //Adjust the spectrum
        void spectrumadjust();

        //Shift the harmonics
        void shiftharmonics();

        //Do the oscil modulation stuff
        void modulation();

        //Check system for needed updates
        bool needPrepare(void);

        //Do the adaptive harmonic stuff
        void adaptiveharmonic(fft_t *f, float freq);

        //Do the adaptive harmonic postprocessing (2n+1,2xS,2xA,etc..)
        //this function is called even for the user interface
        //this can be called for the sine and components, and for the spectrum
        //(that's why the sine and cosine components should be processed with a separate call)
        void adaptiveharmonicpostprocess(fft_t *f, int size);

        //Internal Data
        unsigned char oldbasefunc, oldbasepar, oldhmagtype,
                      oldwaveshapingfunction, oldwaveshaping;
        int oldfilterpars, oldsapars, oldbasefuncmodulation,
            oldbasefuncmodulationpar1, oldbasefuncmodulationpar2,
            oldbasefuncmodulationpar3, oldharmonicshift;
        int oldmodulation, oldmodulationpar1, oldmodulationpar2,
            oldmodulationpar3;


        fft_t *basefuncFFTfreqs; //Base Function Frequencies
        fft_t *oscilFFTfreqs; //Oscillator Frequencies - this is different than the hamonics set-up by the user, it may contains time-domain data if the antialiasing is turned off
        int    oscilprepared;   //1 if the oscil is prepared, 0 if it is not prepared and is need to call ::prepare() before ::get()

        Resonance *res;

        unsigned int randseed;
};

typedef float (*filter_func)(unsigned int, float, float);
filter_func getFilter(unsigned char func);
typedef float (*base_func)(float, float);
base_func getBaseFunction(unsigned char func);


#endif
