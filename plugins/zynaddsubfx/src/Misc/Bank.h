/*
  ZynAddSubFX - a software synthesizer

  Bank.C - Instrument Bank
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

#ifndef BANK_H
#define BANK_H

#include "../globals.h"
#include "XMLwrapper.h"
#include "Part.h"

#define BANK_SIZE 160

/**
 * The max. number of banks that are used
 */
#define MAX_NUM_BANKS 400

/**The instrument Bank
 * \todo add in strings to replace char* */
class Bank
{
    public:
        /**Constructor*/
        Bank();
        ~Bank();
        char *getname(unsigned int ninstrument);
        char *getnamenumbered(unsigned int ninstrument);
        void setname(unsigned int ninstrument, const char *newname, int newslot); //if newslot==-1 then this is ignored, else it will be put on that slot
        bool isPADsynth_used(unsigned int ninstrument);

        /**returns 0 if the slot is not empty or 1 if the slot is empty
         * \todo start using bool before facepalm*/
        int emptyslot(unsigned int ninstrument);

        /**Empties out the selected slot*/
        void clearslot(unsigned int ninstrument);
        /**Saves the given Part to slot*/
        void savetoslot(unsigned int ninstrument, Part *part);
        /**Loads the given slot into a Part*/
        void loadfromslot(unsigned int ninstrument, Part *part);

        /**Swaps Slots*/
        void swapslot(unsigned int n1, unsigned int n2);

        int loadbank(const char *bankdirname);
        int newbank(const char *newbankdirname);

        char *bankfiletitle; //this is shown on the UI of the bank (the title of the window)
        int locked();

        void rescanforbanks();

        struct bankstruct {
            char *dir;
            char *name;
        };

        bankstruct banks[MAX_NUM_BANKS];

    private:

        //it adds a filename to the bank
        //if pos is -1 it try to find a position
        //returns -1 if the bank is full, or 0 if the instrument was added
        int addtobank(int pos, const char *filename, const char *name);

        void deletefrombank(int pos);

        void clearbank();

        char defaultinsname[PART_MAX_NAME_LEN];
        char tmpinsname[BANK_SIZE][PART_MAX_NAME_LEN + 20]; //this keeps the numbered names

        struct ins_t {
            bool  used;
            char  name[PART_MAX_NAME_LEN + 1];
            char *filename;
            struct {
                bool PADsynth_used;
            } info;
        } ins[BANK_SIZE];

        char *dirname;

        void scanrootdir(char *rootdir); //scans a root dir for banks
};

#endif

