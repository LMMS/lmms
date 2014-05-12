/*
  ZynAddSubFX - a software synthesizer

  Effect.h - this class is inherited by the all effects(Reverb, Echo, ..)
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

#ifndef EFFECT_H
#define EFFECT_H

#include "../Misc/Util.h"
#include "../globals.h"
#include "../Params/FilterParams.h"
#include "../Misc/Stereo.h"

class FilterParams;

/**this class is inherited by the all effects(Reverb, Echo, ..)*/
class Effect
{
    public:
        /**
         * Effect Constructor
         * @param insertion_ 1 when it is an insertion Effect
         * @param efxoutl_ Effect output buffer Left channel
         * @param efxoutr_ Effect output buffer Right channel
         * @param filterpars_ pointer to FilterParams array
         * @param Ppreset_ chosen preset
         * @return Initialized Effect object*/
        Effect(bool insertion_, float *efxoutl_, float *efxoutr_,
               FilterParams *filterpars_, unsigned char Ppreset_,
               unsigned int srate, int bufsize);
        virtual ~Effect() {}
        /**
         * Choose a preset
         * @param npreset number of chosen preset*/
        virtual void setpreset(unsigned char npreset) = 0;
        /**Change parameter npar to value
         * @param npar chosen parameter
         * @param value chosen new value*/
        virtual void changepar(int npar, unsigned char value) = 0;
        /**Get the value of parameter npar
         * @param npar chosen parameter
         * @return the value of the parameter in an unsigned char or 0 if it
         * does not exist*/
        virtual unsigned char getpar(int npar) const = 0;
        /**Output result of effect based on the given buffers
         *
         * This method should result in the effect generating its results
         * and placing them into the efxoutl and efxoutr buffers.
         * Every Effect should overide this method.
         *
         * @param smpsl Input buffer for the Left channel
         * @param smpsr Input buffer for the Right channel
         */
        void out(float *const smpsl, float *const smpsr);
        virtual void out(const Stereo<float *> &smp) = 0;
        /**Reset the state of the effect*/
        virtual void cleanup(void) {}
        virtual float getfreqresponse(float freq) { return freq; }

        unsigned char Ppreset;   /**<Currently used preset*/
        float *const  efxoutl; /**<Effect out Left Channel*/
        float *const  efxoutr; /**<Effect out Right Channel*/
        float outvolume; /**<This is the volume of effect and is public because
                          * it is needed in system effects.
                          * The out volume of such effects are always 1.0f, so
                          * this setting tells me how is the volume to the
                          * Master Output only.*/

        float volume;

        FilterParams *filterpars; /**<Parameters for filters used by Effect*/

        //Perform L/R crossover
        static void crossover(float &a, float &b, float crossover);

    protected:
        void setpanning(char Ppanning_);
        void setlrcross(char Plrcross_);

        const bool insertion;
        //panning parameters
        char  Ppanning;
        float pangainL;
        float pangainR;
        char  Plrcross; // L/R mix
        float lrcross;

        // current setup
        unsigned int samplerate;
        int buffersize;

        // alias for above terms
        float samplerate_f;
        float halfsamplerate_f;
        float buffersize_f;
        int   bufferbytes;

        inline void alias()
        {
            samplerate_f     = samplerate;
            halfsamplerate_f = samplerate_f / 2.0f;
            buffersize_f     = buffersize;
            bufferbytes      = buffersize * sizeof(float);
        }
};

#endif
