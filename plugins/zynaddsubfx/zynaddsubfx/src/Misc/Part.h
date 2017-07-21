/*
  ZynAddSubFX - a software synthesizer

  Part.h - Part implementation
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

#ifndef PART_H
#define PART_H

#define MAX_INFO_TEXT_SIZE 1000

#include "../globals.h"
#include "../Params/Controller.h"
#include "../Misc/Microtonal.h"

#include <pthread.h>
#include <list> // For the monomemnotes list.

class EffectMgr;
class ADnoteParameters;
class SUBnoteParameters;
class PADnoteParameters;
class SynthNote;
class XMLWrapper;
class FFTwrapper;

/** Part implementation*/
class Part
{
    public:
        /**Constructor
         * @param microtonal_ Pointer to the microtonal object
         * @param fft_ Pointer to the FFTwrapper
         * @param mutex_ Pointer to the master pthread_mutex_t*/
        Part(Microtonal *microtonal_, FFTwrapper *fft_, pthread_mutex_t *mutex_);
        /**Destructor*/
        ~Part();

        // Midi commands implemented
        void NoteOn(unsigned char note,
                    unsigned char velocity,
                    int masterkeyshift);
        void NoteOff(unsigned char note);
        void PolyphonicAftertouch(unsigned char note,
                                  unsigned char velocity,
                                  int masterkeyshift);
        void AllNotesOff(); //panic
        void SetController(unsigned int type, int par);
        void RelaseSustainedKeys(); //this is called when the sustain pedal is relased
        void RelaseAllKeys(); //this is called on AllNotesOff controller

        /* The synthesizer part output */
        void ComputePartSmps(); //Part output

        //instrumentonly: 0 - save all, 1 - save only instrumnet, 2 - save only instrument without the name(used in bank)


        //saves the instrument settings to a XML file
        //returns 0 for ok or <0 if there is an error
        int saveXML(const char *filename);
        int loadXMLinstrument(const char *filename);

        void add2XML(XMLwrapper *xml);
        void add2XMLinstrument(XMLwrapper *xml);

        void defaults();
        void defaultsinstrument();

        void applyparameters(bool lockmutex = true);

        void getfromXML(XMLwrapper *xml);
        void getfromXMLinstrument(XMLwrapper *xml);

        void cleanup(bool final = false);

        //the part's kit
        struct {
            unsigned char      Penabled, Pmuted, Pminkey, Pmaxkey;
            unsigned char     *Pname;
            unsigned char      Padenabled, Psubenabled, Ppadenabled;
            unsigned char      Psendtoparteffect;
            ADnoteParameters  *adpars;
            SUBnoteParameters *subpars;
            PADnoteParameters *padpars;
        } kit[NUM_KIT_ITEMS];


        //Part parameters
        void setkeylimit(unsigned char Pkeylimit);
        void setkititemstatus(int kititem, int Penabled_);

        unsigned char Penabled; /**<if the part is enabled*/
        unsigned char Pvolume; /**<part volume*/
        unsigned char Pminkey; /**<the minimum key that the part receives noteon messages*/
        unsigned char Pmaxkey; //the maximum key that the part receives noteon messages
        void setPvolume(char Pvolume);
        unsigned char Pkeyshift; //Part keyshift
        unsigned char Prcvchn; //from what midi channel it receive commnads
        unsigned char Ppanning; //part panning
        void setPpanning(char Ppanning);
        unsigned char Pvelsns; //velocity sensing (amplitude velocity scale)
        unsigned char Pveloffs; //velocity offset
        unsigned char Pnoteon; //if the part receives NoteOn messages
        unsigned char Pkitmode; //if the kitmode is enabled
        unsigned char Pdrummode; //if all keys are mapped and the system is 12tET (used for drums)

        unsigned char Ppolymode; //Part mode - 0=monophonic , 1=polyphonic
        unsigned char Plegatomode; // 0=normal, 1=legato
        unsigned char Pkeylimit; //how many keys are alowed to be played same time (0=off), the older will be relased

        unsigned char *Pname; //name of the instrument
        struct { //instrument additional information
            unsigned char Ptype;
            unsigned char Pauthor[MAX_INFO_TEXT_SIZE + 1];
            unsigned char Pcomments[MAX_INFO_TEXT_SIZE + 1];
        } info;


        float *partoutl; //Left channel output of the part
        float *partoutr; //Right channel output of the part

        float *partfxinputl[NUM_PART_EFX + 1], //Left and right signal that pass thru part effects;
        *partfxinputr[NUM_PART_EFX + 1];          //partfxinput l/r [NUM_PART_EFX] is for "no effect" buffer

        enum NoteStatus {
            KEY_OFF, KEY_PLAYING, KEY_RELASED_AND_SUSTAINED, KEY_RELASED
        };

        float volume, oldvolumel, oldvolumer; //this is applied by Master
        float panning; //this is applied by Master, too

        Controller ctl; //Part controllers

        EffectMgr    *partefx[NUM_PART_EFX]; //insertion part effects (they are part of the instrument)
        unsigned char Pefxroute[NUM_PART_EFX]; //how the effect's output is routed(to next effect/to out)
        bool Pefxbypass[NUM_PART_EFX]; //if the effects are bypassed


        pthread_mutex_t *mutex;
        pthread_mutex_t load_mutex;

        int lastnote;

    private:
        void RunNote(unsigned k);
        void KillNotePos(int pos);
        void RelaseNotePos(int pos);
        void MonoMemRenote(); // MonoMem stuff.

        int killallnotes; //is set to 1 if I want to kill all notes

        struct PartNotes {
            NoteStatus status;
            int note; //if there is no note playing, the "note"=-1
            int itemsplaying;
            struct {
                SynthNote *adnote,
                   *subnote,
                   *padnote;
                int sendtoparteffect;
            } kititem[NUM_KIT_ITEMS];
            int time;
        };

        int  lastpos, lastposb; // To keep track of previously used pos and posb.
        bool lastlegatomodevalid; // To keep track of previous legatomodevalid.

        // MonoMem stuff
        std::list<unsigned char> monomemnotes; // A list to remember held notes.
        struct {
            unsigned char velocity;
            int mkeyshift; // I'm not sure masterkeyshift should be remembered.
        } monomem[256];
        /* 256 is to cover all possible note values.
           monomem[] is used in conjunction with the list to
           store the velocity and masterkeyshift values of a given note (the list only store note values).
           For example 'monomem[note].velocity' would be the velocity value of the note 'note'.*/

        PartNotes partnote[POLIPHONY];

        float oldfreq;    //this is used for portamento
        Microtonal *microtonal;
        FFTwrapper *fft;
};

#endif
