/*
  ZynAddSubFX - a software synthesizer

  Bank.h - Instrument Bank
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

#include <string>
#include <vector>

//entries in a bank
#define BANK_SIZE 160

/**The instrument Bank*/
class Bank
{
    public:
        /**Constructor*/
        Bank();
        ~Bank();
        std::string getname(unsigned int ninstrument);
        std::string getnamenumbered(unsigned int ninstrument);
        void setname(unsigned int ninstrument,
                     const std::string &newname,
                     int newslot);                                                       //if newslot==-1 then this is ignored, else it will be put on that slot
        bool isPADsynth_used(unsigned int ninstrument);

        /**returns true when slot is empty*/
        bool emptyslot(unsigned int ninstrument);

        /**Empties out the selected slot*/
        void clearslot(unsigned int ninstrument);
        /**Saves the given Part to slot*/
        void savetoslot(unsigned int ninstrument, class Part * part);
        /**Loads the given slot into a Part*/
        void loadfromslot(unsigned int ninstrument, class Part * part);

        /**Swaps Slots*/
        void swapslot(unsigned int n1, unsigned int n2);

        int loadbank(std::string bankdirname);
        int newbank(std::string newbankdirname);

        std::string bankfiletitle; //this is shown on the UI of the bank (the title of the window)
        int locked();

        void rescanforbanks();

        struct bankstruct {
            bool operator<(const bankstruct &b) const;
            std::string dir;
            std::string name;
        };

        std::vector<bankstruct> banks;

    private:

        //it adds a filename to the bank
        //if pos is -1 it try to find a position
        //returns -1 if the bank is full, or 0 if the instrument was added
        int addtobank(int pos, std::string filename, std::string name);

        void deletefrombank(int pos);

        void clearbank();

        std::string defaultinsname;

        struct ins_t {
            ins_t();
            bool used;
            std::string name;
            std::string filename;
            struct {
                bool PADsynth_used;
            } info;
        } ins[BANK_SIZE];

        std::string dirname;

        void scanrootdir(std::string rootdir); //scans a root dir for banks
};

#endif
