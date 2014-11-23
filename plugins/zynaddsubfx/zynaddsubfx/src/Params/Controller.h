/*
  ZynAddSubFX - a software synthesizer

  Controller.h - (Midi) Controllers implementation
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


#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "../globals.h"
#include "../Misc/XMLwrapper.h"

/**(Midi) Controllers implementation*/
class Controller
{
    public:
        Controller();
        ~Controller();
        void resetall();

        void add2XML(XMLwrapper *xml);
        void defaults();
        void getfromXML(XMLwrapper *xml);

        //Controllers functions
        void setpitchwheel(int value);
        void setpitchwheelbendrange(unsigned short int value);
        void setexpression(int value);
        void setpanning(int value);
        void setfiltercutoff(int value);
        void setfilterq(int value);
        void setbandwidth(int value);
        void setmodwheel(int value);
        void setfmamp(int value);
        void setvolume(int value);
        void setsustain(int value);
        /**Enable or disable portamento
         * @param value 0-127 MIDI value (greater than 64 enables)*/
        void setportamento(int value);
        void setresonancecenter(int value);
        void setresonancebw(int value);


        void setparameternumber(unsigned int type, int value); //used for RPN and NRPN's
        int getnrpn(int *parhi, int *parlo, int *valhi, int *vallo);

        /**
         * Initialize a portamento
         *
         * @param oldfreq Starting frequency of the portamento (Hz)
         * @param newfreq Ending frequency of the portamento (Hz)
         * @param legatoflag true when legato is in progress, false otherwise
         * @returns 1 if properly initialized, 0 otherwise*/
        int initportamento(float oldfreq, float newfreq, bool legatoflag);
        /**Update portamento's freqrap to next value based upon dx*/
        void updateportamento();

        // Controllers values
        struct { //Pitch Wheel
            int data;
            short int bendrange; //bendrange is in cents
            float     relfreq; //the relative frequency (default is 1.0f)
        } pitchwheel;

        struct { //Expression
            int   data;
            float relvolume;
            unsigned char receive;
        } expression;

        struct { //Panning
            int   data;
            float pan;
            unsigned char depth;
        } panning;


        struct { //Filter cutoff
            int   data;
            float relfreq;
            unsigned char depth;
        } filtercutoff;

        struct { //Filter Q
            int   data;
            float relq;
            unsigned char depth;
        } filterq;

        struct { //Bandwidth
            int   data;
            float relbw;
            unsigned char depth;
            unsigned char exponential;
        } bandwidth;

        struct { //Modulation Wheel
            int   data;
            float relmod;
            unsigned char depth;
            unsigned char exponential;
        } modwheel;

        struct { //FM amplitude
            int   data;
            float relamp;
            unsigned char receive;
        } fmamp;

        struct { //Volume
            int   data;
            float volume;
            unsigned char receive;
        } volume;

        struct { //Sustain
            int data, sustain;
            unsigned char receive;
        } sustain;

        struct { /**<Portamento*/
            //parameters
            int data;
            unsigned char portamento;
            /**Whether the portamento midi events are received or not*/
            unsigned char receive;
            /** The time that it takes for the portamento to complete
             *
             * Translates in an expontal fashion to 0 Seconds to 1.93f Seconds
             * of completion time*/
            unsigned char time;
            /**If the portamento is proportinal to the distance spanned
             *
             * 0 - constant time(default)
             * 1 - proportional*/
            unsigned char proportional;
            /**Rate of proportinal portamento*/
            unsigned char propRate;
            /**Depth of proportinal portamento*/
            unsigned char propDepth;
            /**pitchthresh is the threshold of enabling protamento*/
            unsigned char pitchthresh;
            /**enable the portamento only below(0)/above(1) the threshold*/
            unsigned char pitchthreshtype;

            /**this value represent how the portamento time is reduced
             * 0      - for down portamento
             * 1-63   - the up portamento's time is smaller than the down portamento
             * 64     - the portamento time is always the same
             * 64-126 - the down portamento's time is smaller than the up portamento
             * 127    - for upper portamento
             * 'up portamento'   means when the frequency is rising
             * (eg: the portamento is from 200Hz to 300 Hz)
             * 'down portamento' means when the frequency is lowering
             * (eg: the portamento is from 300Hz to 200 Hz)
             */
            unsigned char updowntimestretch;
            /**this value is used to compute the actual portamento
             *
             * This is a multiplyer to change the frequency of the newer
             * frequency to fit the profile of the portamento.
             * This will be linear with respect to x.*/
            float freqrap;
            /**this is used by the Part for knowing which note uses the portamento*/
            int noteusing;
            /**if a the portamento is used by a note
             * \todo see if this can be a bool*/
            int used;

            //Internal data

            /**x is from 0.0f (start portamento) to 1.0f (finished portamento)*/
            float x;
            /**dx is the increment to x when updateportamento is called*/
            float dx;
            /** this is used for computing oldfreq value from x*/
            float origfreqrap;
        } portamento;

        struct { //Resonance Center Frequency
            int   data;
            float relcenter;
            unsigned char depth;
        } resonancecenter;

        struct { //Resonance Bandwidth
            int   data;
            float relbw;
            unsigned char depth;
        } resonancebandwidth;


        /** RPN and NPRPN */
        struct { //nrpn
            int parhi, parlo;
            int valhi, vallo;
            unsigned char receive; //this is saved to disk by Master
        } NRPN;

    private:
};

#endif
