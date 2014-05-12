/*
  ZynAddSubFX - a software synthesizer

  Microtonal.h - Tuning settings and microtonal capabilities
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

#ifndef MICROTONAL_H
#define MICROTONAL_H

#include "../globals.h"
#include "XMLwrapper.h"

#define MAX_OCTAVE_SIZE 128
#define MICROTONAL_MAX_NAME_LEN 120

#include <stdio.h>


/**Tuning settings and microtonal capabilities*/
class Microtonal
{
    public:
        /**Constructor*/
        Microtonal();
        /**Destructor*/
        ~Microtonal();
        void defaults();
        /**Calculates the frequency for a given note
         */
        float getnotefreq(int note, int keyshift) const;


        //Parameters
        /**if the keys are inversed (the pitch is lower to keys from the right direction)*/
        unsigned char Pinvertupdown;

        /**the central key of the inversion*/
        unsigned char Pinvertupdowncenter;

        /**0 for 12 key temperate scale, 1 for microtonal*/
        unsigned char Penabled;

        /**the note of "A" key*/
        unsigned char PAnote;

        /**the frequency of the "A" note*/
        float PAfreq;

        /**if the scale is "tuned" to a note, you can tune to other note*/
        unsigned char Pscaleshift;

        //first and last key (to retune)
        unsigned char Pfirstkey;
        unsigned char Plastkey;

        /**The middle note where scale degree 0 is mapped to*/
        unsigned char Pmiddlenote;

        /**Map size*/
        unsigned char Pmapsize;

        /**Mapping ON/OFF*/
        unsigned char Pmappingenabled;
        /**Mapping (keys)*/
        short int Pmapping[128];

        /**Fine detune to be applied to all notes*/
        unsigned char Pglobalfinedetune;

        // Functions
        /** Return the current octave size*/
        unsigned char getoctavesize() const;
        /**Convert tunning to string*/
        void tuningtoline(int n, char *line, int maxn);
        /**load the tunnings from a .scl file*/
        int loadscl(const char *filename);
        /**load the mapping from .kbm file*/
        int loadkbm(const char *filename);
        /**Load text into the internal tunings
         *
         *\todo better description*/
        int texttotunings(const char *text);
        /**Load text into the internal mappings
         *
         *\todo better description*/
        void texttomapping(const char *text);

        /**Name of Microtonal tuning*/
        unsigned char *Pname;
        /**Comment about the tuning*/
        unsigned char *Pcomment;

        void add2XML(XMLwrapper *xml) const;
        void getfromXML(XMLwrapper *xml);
        int saveXML(const char *filename) const;
        int loadXML(const char *filename);

        //simple operators primarily for debug
        bool operator==(const Microtonal &micro) const;
        bool operator!=(const Microtonal &micro) const;

    private:
        int linetotunings(unsigned int nline, const char *line);
        int loadline(FILE *file, char *line); //loads a line from the text file, while ignoring the lines beggining with "!"
        unsigned char octavesize;
        struct {
            unsigned char type; //1 for cents or 2 for division

            // the real tuning (eg. +1.05946f for one halftone)
            // or 2.0f for one octave
            float tuning;

            //the real tunning is x1/x2
            unsigned int x1, x2;
        } octave[MAX_OCTAVE_SIZE], tmpoctave[MAX_OCTAVE_SIZE];
};

#endif
