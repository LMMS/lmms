/*
  ZynAddSubFX - a software synthesizer

  Master.h - It sends Midi Messages to Parts, receives samples from parts,
             process them with system/insertion effects and mix them
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

#ifndef MASTER_H
#define MASTER_H

#include "../globals.h"
#include "../Effects/EffectMgr.h"
#include "Part.h"
#include "../Output/Recorder.h"
#include "Microtonal.h"

#include "Bank.h"
#include "Dump.h"
#include "../Seq/Sequencer.h"
#include "XMLwrapper.h"

extern Dump dump;
/** It sends Midi Messages to Parts, receives samples from parts,
 *  process them with system/insertion effects and mix them */
class Master
{
    public:
        /** Constructor*/
        Master();
        /** Destructor*/
        ~Master();

        /**Saves all settings to a XML file
         * @return 0 for ok or <0 if there is an error*/
        int saveXML(const char *filename);

        /**This adds the parameters to the XML data*/
        void add2XML(XMLwrapper *xml);

        void defaults();


        /**loads all settings from a XML file
         * @return 0 for ok or -1 if there is an error*/
        int loadXML(const char *filename);
        void applyparameters();

        void getfromXML(XMLwrapper *xml);

        /**get all data to a newly allocated array (used for VST)
         * @return the datasize*/
        int getalldata(char **data);
        /**put all data from the *data array to zynaddsubfx parameters (used for VST)*/
        void putalldata(char *data, int size);



        //Midi IN
        void NoteOn(unsigned char chan,
                    unsigned char note,
                    unsigned char velocity);
        void NoteOff(unsigned char chan, unsigned char note);
        void SetController(unsigned char chan, unsigned int type, int par);
        //void NRPN...


        void ShutUp();
        int shutup;

        /**Audio Output*/
        void AudioOut(REALTYPE *outl, REALTYPE *outr);
        /**Audio Output (for callback mode). This allows the program to be controled by an external program*/
        void GetAudioOutSamples(int nsamples,
                                int samplerate,
                                REALTYPE *outl,
                                REALTYPE *outr);


        void partonoff(int npart, int what);

        /**parts \todo see if this can be made to be dynamic*/
        Part *part[NUM_MIDI_PARTS];

        //parameters
        unsigned char Pvolume;
        unsigned char Pkeyshift;
        unsigned char Psysefxvol[NUM_SYS_EFX][NUM_MIDI_PARTS];
        unsigned char Psysefxsend[NUM_SYS_EFX][NUM_SYS_EFX];

        //parameters control
        void setPvolume(char Pvolume_);
        void setPkeyshift(char Pkeyshift_);
        void setPsysefxvol(int Ppart, int Pefx, char Pvol);
        void setPsysefxsend(int Pefxfrom, int Pefxto, char Pvol);

        //effects
        EffectMgr *sysefx[NUM_SYS_EFX]; //system
        EffectMgr *insefx[NUM_INS_EFX]; //insertion
//	void swapcopyeffects(int what,int type,int neff1,int neff2);

        //HDD recorder
        Recorder HDDRecorder;

        //part that's apply the insertion effect; -1 to disable
        short int Pinsparts[NUM_INS_EFX];

        //peaks for VU-meter
        void vuresetpeaks();
        REALTYPE vuoutpeakl, vuoutpeakr, vumaxoutpeakl, vumaxoutpeakr,
                 vurmspeakl, vurmspeakr;
        int vuclipped;

        //peaks for part VU-meters
        REALTYPE      vuoutpeakpart[NUM_MIDI_PARTS];
        unsigned char fakepeakpart[NUM_MIDI_PARTS]; //this is used to compute the "peak" when the part is disabled

        Controller ctl;
        int swaplr; //1 if L and R are swapped

        //Sequencer
        Sequencer seq;

        //other objects
        Microtonal microtonal;
        Bank bank;

        FFTwrapper     *fft;
        pthread_mutex_t mutex;

    private:
        REALTYPE volume;
        REALTYPE sysefxvol[NUM_SYS_EFX][NUM_MIDI_PARTS];
        REALTYPE sysefxsend[NUM_SYS_EFX][NUM_SYS_EFX];

        //Temporary mixing samples for part samples which is sent to system effect
        REALTYPE *tmpmixl;
        REALTYPE *tmpmixr;


        int keyshift;

        //Audio Output samples (if it used GetAudioOutSamples - eg. for Jack output; elsewhere is unused)
        REALTYPE *audiooutl;
        REALTYPE *audiooutr;

        int      ksoundbuffersample; //this is used to know if there is need to call AudioOut by GetAudioOutSamples method
        REALTYPE ksoundbuffersamplelow; //this is used for resampling (eg. if Jack samplerate!= SAMPLE_RATE)
        REALTYPE oldsamplel, oldsampler; //this is used for resampling

        //These are called by the NoteOn, NoteOff,SetController (which are from external sources like MIDI, Virtual Keyboard)
        //and are called by internal parts of the program (like sequencer)
        void noteon(unsigned char chan,
                    unsigned char note,
                    unsigned char velocity);
        void noteoff(unsigned char chan, unsigned char note);
        void setcontroller(unsigned char chan, unsigned int type, int par);
};


#endif

