/*
  ZynAddSubFX - a software synthesizer

  FilterParams.h - Parameters for filter
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

#ifndef FILTER_PARAMS_H
#define FILTER_PARAMS_H

#include "../globals.h"
#include "../Misc/XMLwrapper.h"
#include "PresetsArray.h"

class FilterParams:public PresetsArray
{
    public:
        FilterParams(unsigned char Ptype_,
                     unsigned char Pfreq,
                     unsigned char Pq_);
        ~FilterParams();

        void add2XML(XMLwrapper *xml);
        void add2XMLsection(XMLwrapper *xml, int n);
        void defaults();
        void getfromXML(XMLwrapper *xml);
        void getfromXMLsection(XMLwrapper *xml, int n);


        void getfromFilterParams(FilterParams *pars);

        float getfreq();
        float getq();
        float getfreqtracking(float notefreq);
        float getgain();

        unsigned char Pcategory; //Filter category (Analog/Formant/StVar)
        unsigned char Ptype; // Filter type  (for analog lpf,hpf,bpf..)
        unsigned char Pfreq; // Frequency (64-central frequency)
        unsigned char Pq; // Q parameters (resonance or bandwidth)
        unsigned char Pstages; //filter stages+1
        unsigned char Pfreqtrack; //how the filter frequency is changing according the note frequency
        unsigned char Pgain; //filter's output gain

        //Formant filter parameters
        unsigned char Pnumformants; //how many formants are used
        unsigned char Pformantslowness; //how slow varies the formants
        unsigned char Pvowelclearness; //how vowels are kept clean (how much try to avoid "mixed" vowels)
        unsigned char Pcenterfreq, Poctavesfreq; //the center frequency of the res. func., and the number of octaves

        struct {
            struct {
                unsigned char freq, amp, q; //frequency,amplitude,Q
            } formants[FF_MAX_FORMANTS];
        } Pvowels[FF_MAX_VOWELS];


        unsigned char Psequencesize; //how many vowels are in the sequence
        unsigned char Psequencestretch; //how the sequence is stretched (how the input from filter envelopes/LFOs/etc. is "stretched")
        unsigned char Psequencereversed; //if the input from filter envelopes/LFOs/etc. is reversed(negated)
        struct {
            unsigned char nvowel; //the vowel from the position
        } Psequence[FF_MAX_SEQUENCE];

        float getcenterfreq();
        float getoctavesfreq();
        float getfreqpos(float freq);
        float getfreqx(float x);

        void formantfilterH(int nvowel, int nfreqs, float *freqs); //used by UI

        float getformantfreq(unsigned char freq);
        float getformantamp(unsigned char amp);
        float getformantq(unsigned char q);

        bool changed;

    private:
        void defaults(int n);

        //stored default parameters
        unsigned char Dtype;
        unsigned char Dfreq;
        unsigned char Dq;
};

#endif
