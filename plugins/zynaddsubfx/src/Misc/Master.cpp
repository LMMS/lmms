/*
  ZynAddSubFX - a software synthesizer

  Master.cpp - It sends Midi Messages to Parts, receives samples from parts,
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

#include "Master.h"

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <unistd.h>

Master::Master()
{
    swaplr = 0;

    pthread_mutex_init(&mutex, NULL);
    fft = new FFTwrapper(OSCIL_SIZE);

    tmpmixl   = new REALTYPE[SOUND_BUFFER_SIZE];
    tmpmixr   = new REALTYPE[SOUND_BUFFER_SIZE];
    audiooutl = new REALTYPE[SOUND_BUFFER_SIZE];
    audiooutr = new REALTYPE[SOUND_BUFFER_SIZE];

    ksoundbuffersample    = -1; //this is only time when this is -1; this means that the GetAudioOutSamples was never called
    ksoundbuffersamplelow = 0.0;
    oldsamplel = 0.0;
    oldsampler = 0.0;
    shutup     = 0;
    for(int npart = 0; npart < NUM_MIDI_PARTS; npart++) {
        vuoutpeakpart[npart] = 1e-9;
        fakepeakpart[npart]  = 0;
    }

    for(int i = 0; i < SOUND_BUFFER_SIZE; i++) {
        audiooutl[i] = 0.0;
        audiooutr[i] = 0.0;
    }

    for(int npart = 0; npart < NUM_MIDI_PARTS; npart++)
        part[npart] = new Part(&microtonal, fft, &mutex);



    //Insertion Effects init
    for(int nefx = 0; nefx < NUM_INS_EFX; nefx++)
        insefx[nefx] = new EffectMgr(1, &mutex);

    //System Effects init
    for(int nefx = 0; nefx < NUM_SYS_EFX; nefx++)
        sysefx[nefx] = new EffectMgr(0, &mutex);
    ;


    defaults();
}

void Master::defaults()
{
    volume = 1.0;
    setPvolume(80);
    setPkeyshift(64);

    for(int npart = 0; npart < NUM_MIDI_PARTS; npart++) {
        part[npart]->defaults();
        part[npart]->Prcvchn = npart % NUM_MIDI_CHANNELS;
    }

    partonoff(0, 1); //enable the first part

    for(int nefx = 0; nefx < NUM_INS_EFX; nefx++) {
        insefx[nefx]->defaults();
        Pinsparts[nefx] = -1;
    }

    //System Effects init
    for(int nefx = 0; nefx < NUM_SYS_EFX; nefx++) {
        sysefx[nefx]->defaults();
        for(int npart = 0; npart < NUM_MIDI_PARTS; npart++)
            //if (nefx==0) setPsysefxvol(npart,nefx,64);
            //else
            setPsysefxvol(npart, nefx, 0);
        ;
        for(int nefxto = 0; nefxto < NUM_SYS_EFX; nefxto++)
            setPsysefxsend(nefx, nefxto, 0);
    }

//	sysefx[0]->changeeffect(1);
    microtonal.defaults();
    ShutUp();
}

/*
 * Note On Messages (velocity=0 for NoteOff)
 */
void Master::NoteOn(unsigned char chan,
                    unsigned char note,
                    unsigned char velocity)
{
    dump.dumpnote(chan, note, velocity);

    noteon(chan, note, velocity);
}

/*
 * Internal Note On (velocity=0 for NoteOff)
 */
void Master::noteon(unsigned char chan,
                    unsigned char note,
                    unsigned char velocity)
{
    int npart;
    if(velocity != 0) {
        for(npart = 0; npart < NUM_MIDI_PARTS; npart++) {
            if(chan == part[npart]->Prcvchn) {
                fakepeakpart[npart] = velocity * 2;
                if(part[npart]->Penabled != 0)
                    part[npart]->NoteOn(note, velocity, keyshift);
            }
        }
    }
    else
        this->NoteOff(chan, note);
    ;
    HDDRecorder.triggernow();
}

/*
 * Note Off Messages
 */
void Master::NoteOff(unsigned char chan, unsigned char note)
{
    dump.dumpnote(chan, note, 0);

    noteoff(chan, note);
}

/*
 * Internal Note Off
 */
void Master::noteoff(unsigned char chan, unsigned char note)
{
    int npart;
    for(npart = 0; npart < NUM_MIDI_PARTS; npart++)
        if((chan == part[npart]->Prcvchn) && (part[npart]->Penabled != 0))
            part[npart]->NoteOff(note);
    ;
}

/*
 * Controllers
 */
void Master::SetController(unsigned char chan, unsigned int type, int par)
{
    dump.dumpcontroller(chan, type, par);

    setcontroller(chan, type, par);
}

/*
 * Internal Controllers
 */
void Master::setcontroller(unsigned char chan, unsigned int type, int par)
{
    if((type == C_dataentryhi) || (type == C_dataentrylo)
       || (type == C_nrpnhi) || (type == C_nrpnlo)) { //Process RPN and NRPN by the Master (ignore the chan)
        ctl.setparameternumber(type, par);

        int parhi = -1, parlo = -1, valhi = -1, vallo = -1;
        if(ctl.getnrpn(&parhi, &parlo, &valhi, &vallo) == 0) //this is NRPN
            //fprintf(stderr,"rcv. NRPN: %d %d %d %d\n",parhi,parlo,valhi,vallo);
            switch(parhi) {
            case 0x04: //System Effects
                if(parlo < NUM_SYS_EFX)
                    sysefx[parlo]->seteffectpar_nolock(valhi, vallo);
                ;
                break;
            case 0x08: //Insertion Effects
                if(parlo < NUM_INS_EFX)
                    insefx[parlo]->seteffectpar_nolock(valhi, vallo);
                ;
                break;
            }
        ;
    }
    else {  //other controllers
        for(int npart = 0; npart < NUM_MIDI_PARTS; npart++) //Send the controller to all part assigned to the channel
            if((chan == part[npart]->Prcvchn) && (part[npart]->Penabled != 0))
                part[npart]->SetController(type, par);
        ;

        if(type == C_allsoundsoff) { //cleanup insertion/system FX
            for(int nefx = 0; nefx < NUM_SYS_EFX; ++nefx)
                sysefx[nefx]->cleanup();
            for(int nefx = 0; nefx < NUM_INS_EFX; ++nefx)
                insefx[nefx]->cleanup();
        }
    }
}


/*
 * Enable/Disable a part
 */
void Master::partonoff(int npart, int what)
{
    if(npart >= NUM_MIDI_PARTS)
        return;
    if(what == 0) { //disable part
        fakepeakpart[npart]   = 0;
        part[npart]->Penabled = 0;
        part[npart]->cleanup();
        for(int nefx = 0; nefx < NUM_INS_EFX; nefx++) {
            if(Pinsparts[nefx] == npart)
                insefx[nefx]->cleanup();
            ;
        }
    }
    else {  //enabled
        part[npart]->Penabled = 1;
        fakepeakpart[npart]   = 0;
    }
}

/*
 * Master audio out (the final sound)
 */
void Master::AudioOut(REALTYPE *outl, REALTYPE *outr)
{
    int i, npart, nefx;

    /*    //test!!!!!!!!!!!!! se poate bloca aici (mutex)
        if (seq.play){
        int type,par1,par2,again,midichan;
        int ntrack=1;
    //	    do{
            again=seq.getevent(ntrack,&midichan,&type,&par1,&par2);
            if (type>0) {
    //		printf("aaa\n");

                    if (type==1){//note_on or note_off
                if (par2!=0) NoteOn(midichan,par1,par2);
                    else NoteOff(midichan,par1);
                    };
            };
    //	    } while (again);
        };
    */


//    printf("zzzz\n");


    //Swaps the Left channel with Right Channel (if it is asked for)
    if(swaplr != 0) {
        REALTYPE *tmp = outl;
        outl = outr;
        outr = tmp;
    }

    //clean up the output samples
    for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
        outl[i] = 0.0;
        outr[i] = 0.0;
    }

    //Compute part samples and store them part[npart]->partoutl,partoutr
    for(npart = 0; npart < NUM_MIDI_PARTS; npart++)
        if(part[npart]->Penabled != 0)
            part[npart]->ComputePartSmps();

    //Insertion effects
    for(nefx = 0; nefx < NUM_INS_EFX; nefx++) {
        if(Pinsparts[nefx] >= 0) {
            int efxpart = Pinsparts[nefx];
            if(part[efxpart]->Penabled != 0)
                insefx[nefx]->out(part[efxpart]->partoutl,
                                  part[efxpart]->partoutr);
        }
    }


    //Apply the part volumes and pannings (after insertion effects)
    for(npart = 0; npart < NUM_MIDI_PARTS; npart++) {
        if(part[npart]->Penabled == 0)
            continue;

        REALTYPE newvol_l = part[npart]->volume;
        REALTYPE newvol_r = part[npart]->volume;
        REALTYPE oldvol_l = part[npart]->oldvolumel;
        REALTYPE oldvol_r = part[npart]->oldvolumer;
        REALTYPE pan      = part[npart]->panning;
        if(pan < 0.5)
            newvol_l *= pan * 2.0;
        else
            newvol_r *= (1.0 - pan) * 2.0;

        if(ABOVE_AMPLITUDE_THRESHOLD(oldvol_l, newvol_l)
           || ABOVE_AMPLITUDE_THRESHOLD(oldvol_r, newvol_r)) { //the volume or the panning has changed and needs interpolation
            for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
                REALTYPE vol_l = INTERPOLATE_AMPLITUDE(oldvol_l,
                                                       newvol_l,
                                                       i,
                                                       SOUND_BUFFER_SIZE);
                REALTYPE vol_r = INTERPOLATE_AMPLITUDE(oldvol_r,
                                                       newvol_r,
                                                       i,
                                                       SOUND_BUFFER_SIZE);
                part[npart]->partoutl[i] *= vol_l;
                part[npart]->partoutr[i] *= vol_r;
            }
            part[npart]->oldvolumel = newvol_l;
            part[npart]->oldvolumer = newvol_r;
        }
        else {
            for(i = 0; i < SOUND_BUFFER_SIZE; i++) { //the volume did not changed
                part[npart]->partoutl[i] *= newvol_l;
                part[npart]->partoutr[i] *= newvol_r;
            }
        }
    }


    //System effects
    for(nefx = 0; nefx < NUM_SYS_EFX; nefx++) {
        if(sysefx[nefx]->geteffect() == 0)
            continue;                              //the effect is disabled

        //Clean up the samples used by the system effects
        for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
            tmpmixl[i] = 0.0;
            tmpmixr[i] = 0.0;
        }

        //Mix the channels according to the part settings about System Effect
        for(npart = 0; npart < NUM_MIDI_PARTS; npart++) {
            //skip if the part has no output to effect
            if(Psysefxvol[nefx][npart] == 0)
                continue;

            //skip if the part is disabled
            if(part[npart]->Penabled == 0)
                continue;

            //the output volume of each part to system effect
            REALTYPE vol = sysefxvol[nefx][npart];
            for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
                tmpmixl[i] += part[npart]->partoutl[i] * vol;
                tmpmixr[i] += part[npart]->partoutr[i] * vol;
            }
        }

        // system effect send to next ones
        for(int nefxfrom = 0; nefxfrom < nefx; nefxfrom++) {
            if(Psysefxsend[nefxfrom][nefx] != 0) {
                REALTYPE v = sysefxsend[nefxfrom][nefx];
                for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
                    tmpmixl[i] += sysefx[nefxfrom]->efxoutl[i] * v;
                    tmpmixr[i] += sysefx[nefxfrom]->efxoutr[i] * v;
                }
            }
        }

        sysefx[nefx]->out(tmpmixl, tmpmixr);

        //Add the System Effect to sound output
        REALTYPE outvol = sysefx[nefx]->sysefxgetvolume();
        for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
            outl[i] += tmpmixl[i] * outvol;
            outr[i] += tmpmixr[i] * outvol;
        }
    }

    //Mix all parts
    for(npart = 0; npart < NUM_MIDI_PARTS; npart++) {
        for(i = 0; i < SOUND_BUFFER_SIZE; i++) { //the volume did not changed
            outl[i] += part[npart]->partoutl[i];
            outr[i] += part[npart]->partoutr[i];
        }
    }

    //Insertion effects for Master Out
    for(nefx = 0; nefx < NUM_INS_EFX; nefx++)
        if(Pinsparts[nefx] == -2)
            insefx[nefx]->out(outl, outr);
    ;

    //Master Volume
    for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
        outl[i] *= volume;
        outr[i] *= volume;
    }

    //Peak computation (for vumeters)
    vuoutpeakl = 1e-12;
    vuoutpeakr = 1e-12;
    for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
        if(fabs(outl[i]) > vuoutpeakl)
            vuoutpeakl = fabs(outl[i]);
        if(fabs(outr[i]) > vuoutpeakr)
            vuoutpeakr = fabs(outr[i]);
    }
    if((vuoutpeakl > 1.0) || (vuoutpeakr > 1.0))
        vuclipped = 1;
    if(vumaxoutpeakl < vuoutpeakl)
        vumaxoutpeakl = vuoutpeakl;
    if(vumaxoutpeakr < vuoutpeakr)
        vumaxoutpeakr = vuoutpeakr;

    //RMS Peak computation (for vumeters)
    vurmspeakl = 1e-12;
    vurmspeakr = 1e-12;
    for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
        vurmspeakl += outl[i] * outl[i];
        vurmspeakr += outr[i] * outr[i];
    }
    vurmspeakl = sqrt(vurmspeakl / SOUND_BUFFER_SIZE);
    vurmspeakr = sqrt(vurmspeakr / SOUND_BUFFER_SIZE);

    //Part Peak computation (for Part vumeters or fake part vumeters)
    for(npart = 0; npart < NUM_MIDI_PARTS; npart++) {
        vuoutpeakpart[npart] = 1.0e-12;
        if(part[npart]->Penabled != 0) {
            REALTYPE *outl = part[npart]->partoutl,
            *outr = part[npart]->partoutr;
            for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
                REALTYPE tmp = fabs(outl[i] + outr[i]);
                if(tmp > vuoutpeakpart[npart])
                    vuoutpeakpart[npart] = tmp;
            }
            vuoutpeakpart[npart] *= volume;
        }
        else
        if(fakepeakpart[npart] > 1)
            fakepeakpart[npart]--;
        ;
    }


    //Shutup if it is asked (with fade-out)
    if(shutup != 0) {
        for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
            REALTYPE tmp =
                (SOUND_BUFFER_SIZE - i) / (REALTYPE) SOUND_BUFFER_SIZE;
            outl[i] *= tmp;
            outr[i] *= tmp;
        }
        ShutUp();
    }

    //update the LFO's time
    LFOParams::time++;

    if(HDDRecorder.recording())
        HDDRecorder.recordbuffer(outl, outr);
    dump.inctick();
}

void Master::GetAudioOutSamples(int nsamples,
                                int samplerate,
                                REALTYPE *outl,
                                REALTYPE *outr)
{
    if(ksoundbuffersample == -1) { //first time
        AudioOut(&audiooutl[0], &audiooutr[0]);
        ksoundbuffersample = 0;
    }


    if(samplerate == SAMPLE_RATE) { //no resample
        int ksample = 0;
        while(ksample < nsamples) {
            outl[ksample] = audiooutl[ksoundbuffersample];
            outr[ksample] = audiooutr[ksoundbuffersample];

            ksample++;
            ksoundbuffersample++;
            if(ksoundbuffersample >= SOUND_BUFFER_SIZE) {
                AudioOut(&audiooutl[0], &audiooutr[0]);
                ksoundbuffersample = 0;
            }
        }
    }
    else {  //Resample
        int      ksample = 0;
        REALTYPE srinc   = SAMPLE_RATE / (REALTYPE)samplerate;

        while(ksample < nsamples) {
            if(ksoundbuffersample != 0) {
                outl[ksample] = audiooutl[ksoundbuffersample]
                                * ksoundbuffersamplelow
                                + audiooutl[ksoundbuffersample
                                            - 1] * (1.0 - ksoundbuffersamplelow);
                outr[ksample] = audiooutr[ksoundbuffersample]
                                * ksoundbuffersamplelow
                                + audiooutr[ksoundbuffersample
                                            - 1] * (1.0 - ksoundbuffersamplelow);
            }
            else {
                outl[ksample] = audiooutl[ksoundbuffersample]
                                * ksoundbuffersamplelow
                                + oldsamplel * (1.0 - ksoundbuffersamplelow);
                outr[ksample] = audiooutr[ksoundbuffersample]
                                * ksoundbuffersamplelow
                                + oldsampler * (1.0 - ksoundbuffersamplelow);
            }

            ksample++;

            ksoundbuffersamplelow += srinc;
            if(ksoundbuffersamplelow >= 1.0) {
                ksoundbuffersample   += (int) floor(ksoundbuffersamplelow);
                ksoundbuffersamplelow = ksoundbuffersamplelow - floor(
                    ksoundbuffersamplelow);
            }

            if(ksoundbuffersample >= SOUND_BUFFER_SIZE) {
                oldsamplel = audiooutl[SOUND_BUFFER_SIZE - 1];
                oldsampler = audiooutr[SOUND_BUFFER_SIZE - 1];
                AudioOut(&audiooutl[0], &audiooutr[0]);
                ksoundbuffersample = 0;
            }
        }
    }
}


Master::~Master()
{
    for(int npart = 0; npart < NUM_MIDI_PARTS; npart++)
        delete part[npart];
    for(int nefx = 0; nefx < NUM_INS_EFX; nefx++)
        delete insefx[nefx];
    for(int nefx = 0; nefx < NUM_SYS_EFX; nefx++)
        delete sysefx[nefx];

    delete [] audiooutl;
    delete [] audiooutr;
    delete [] tmpmixl;
    delete [] tmpmixr;
    delete (fft);

    pthread_mutex_destroy(&mutex);
}


/*
 * Parameter control
 */
void Master::setPvolume(char Pvolume_)
{
    Pvolume = Pvolume_;
    volume  = dB2rap((Pvolume - 96.0) / 96.0 * 40.0);
}

void Master::setPkeyshift(char Pkeyshift_)
{
    Pkeyshift = Pkeyshift_;
    keyshift  = (int)Pkeyshift - 64;
}


void Master::setPsysefxvol(int Ppart, int Pefx, char Pvol)
{
    Psysefxvol[Pefx][Ppart] = Pvol;
    sysefxvol[Pefx][Ppart]  = pow(0.1, (1.0 - Pvol / 96.0) * 2.0);
}

void Master::setPsysefxsend(int Pefxfrom, int Pefxto, char Pvol)
{
    Psysefxsend[Pefxfrom][Pefxto] = Pvol;
    sysefxsend[Pefxfrom][Pefxto]  = pow(0.1, (1.0 - Pvol / 96.0) * 2.0);
}


/*
 * Panic! (Clean up all parts and effects)
 */
void Master::ShutUp()
{
    for(int npart = 0; npart < NUM_MIDI_PARTS; npart++) {
        part[npart]->cleanup();
        fakepeakpart[npart] = 0;
    }
    for(int nefx = 0; nefx < NUM_INS_EFX; nefx++)
        insefx[nefx]->cleanup();
    for(int nefx = 0; nefx < NUM_SYS_EFX; nefx++)
        sysefx[nefx]->cleanup();
    vuresetpeaks();
    shutup = 0;
}


/*
 * Reset peaks and clear the "cliped" flag (for VU-meter)
 */
void Master::vuresetpeaks()
{
    vuoutpeakl    = 1e-9;
    vuoutpeakr    = 1e-9;
    vumaxoutpeakl = 1e-9;
    vumaxoutpeakr = 1e-9;
    vuclipped     = 0;
}



void Master::applyparameters()
{
    for(int npart = 0; npart < NUM_MIDI_PARTS; npart++)
        part[npart]->applyparameters();
    ;
}

void Master::add2XML(XMLwrapper *xml)
{
    xml->addpar("volume", Pvolume);
    xml->addpar("key_shift", Pkeyshift);
    xml->addparbool("nrpn_receive", ctl.NRPN.receive);

    xml->beginbranch("MICROTONAL");
    microtonal.add2XML(xml);
    xml->endbranch();

    for(int npart = 0; npart < NUM_MIDI_PARTS; npart++) {
        xml->beginbranch("PART", npart);
        part[npart]->add2XML(xml);
        xml->endbranch();
    }

    xml->beginbranch("SYSTEM_EFFECTS");
    for(int nefx = 0; nefx < NUM_SYS_EFX; nefx++) {
        xml->beginbranch("SYSTEM_EFFECT", nefx);
        xml->beginbranch("EFFECT");
        sysefx[nefx]->add2XML(xml);
        xml->endbranch();

        for(int pefx = 0; pefx < NUM_MIDI_PARTS; pefx++) {
            xml->beginbranch("VOLUME", pefx);
            xml->addpar("vol", Psysefxvol[nefx][pefx]);
            xml->endbranch();
        }

        for(int tonefx = nefx + 1; tonefx < NUM_SYS_EFX; tonefx++) {
            xml->beginbranch("SENDTO", tonefx);
            xml->addpar("send_vol", Psysefxsend[nefx][tonefx]);
            xml->endbranch();
        }


        xml->endbranch();
    }
    xml->endbranch();

    xml->beginbranch("INSERTION_EFFECTS");
    for(int nefx = 0; nefx < NUM_INS_EFX; nefx++) {
        xml->beginbranch("INSERTION_EFFECT", nefx);
        xml->addpar("part", Pinsparts[nefx]);

        xml->beginbranch("EFFECT");
        insefx[nefx]->add2XML(xml);
        xml->endbranch();
        xml->endbranch();
    }

    xml->endbranch();
}


int Master::getalldata(char **data)
{
    XMLwrapper *xml = new XMLwrapper();

    xml->beginbranch("MASTER");

    pthread_mutex_lock(&mutex);
    add2XML(xml);
    pthread_mutex_unlock(&mutex);

    xml->endbranch();

    *data = xml->getXMLdata();
    delete (xml);
    return strlen(*data) + 1;
}

void Master::putalldata(char *data, int size)
{
    XMLwrapper *xml = new XMLwrapper();
    if(!xml->putXMLdata(data)) {
        delete (xml);
        return;
    }

    if(xml->enterbranch("MASTER") == 0)
        return;

    pthread_mutex_lock(&mutex);
    getfromXML(xml);
    pthread_mutex_unlock(&mutex);

    xml->exitbranch();

    delete (xml);
}

int Master::saveXML(const char *filename)
{
    XMLwrapper *xml = new XMLwrapper();

    xml->beginbranch("MASTER");
    add2XML(xml);
    xml->endbranch();

    int result = xml->saveXMLfile(filename);
    delete (xml);
    return result;
}



int Master::loadXML(const char *filename)
{
    XMLwrapper *xml = new XMLwrapper();
    if(xml->loadXMLfile(filename) < 0) {
        delete (xml);
        return -1;
    }

    if(xml->enterbranch("MASTER") == 0)
        return -10;
    getfromXML(xml);
    xml->exitbranch();

    delete (xml);
    return 0;
}

void Master::getfromXML(XMLwrapper *xml)
{
    setPvolume(xml->getpar127("volume", Pvolume));
    setPkeyshift(xml->getpar127("key_shift", Pkeyshift));
    ctl.NRPN.receive = xml->getparbool("nrpn_receive", ctl.NRPN.receive);


    part[0]->Penabled = 0;
    for(int npart = 0; npart < NUM_MIDI_PARTS; npart++) {
        if(xml->enterbranch("PART", npart) == 0)
            continue;
        part[npart]->getfromXML(xml);
        xml->exitbranch();
    }

    if(xml->enterbranch("MICROTONAL")) {
        microtonal.getfromXML(xml);
        xml->exitbranch();
    }

    sysefx[0]->changeeffect(0);
    if(xml->enterbranch("SYSTEM_EFFECTS")) {
        for(int nefx = 0; nefx < NUM_SYS_EFX; nefx++) {
            if(xml->enterbranch("SYSTEM_EFFECT", nefx) == 0)
                continue;
            if(xml->enterbranch("EFFECT")) {
                sysefx[nefx]->getfromXML(xml);
                xml->exitbranch();
            }

            for(int partefx = 0; partefx < NUM_MIDI_PARTS; partefx++) {
                if(xml->enterbranch("VOLUME", partefx) == 0)
                    continue;
                setPsysefxvol(partefx, nefx,
                              xml->getpar127("vol", Psysefxvol[partefx][nefx]));
                xml->exitbranch();
            }

            for(int tonefx = nefx + 1; tonefx < NUM_SYS_EFX; tonefx++) {
                if(xml->enterbranch("SENDTO", tonefx) == 0)
                    continue;
                setPsysefxsend(nefx, tonefx,
                               xml->getpar127("send_vol",
                                              Psysefxsend[nefx][tonefx]));
                xml->exitbranch();
            }
            xml->exitbranch();
        }
        xml->exitbranch();
    }


    if(xml->enterbranch("INSERTION_EFFECTS")) {
        for(int nefx = 0; nefx < NUM_INS_EFX; nefx++) {
            if(xml->enterbranch("INSERTION_EFFECT", nefx) == 0)
                continue;
            Pinsparts[nefx] = xml->getpar("part",
                                          Pinsparts[nefx],
                                          -2,
                                          NUM_MIDI_PARTS);
            if(xml->enterbranch("EFFECT")) {
                insefx[nefx]->getfromXML(xml);
                xml->exitbranch();
            }
            xml->exitbranch();
        }

        xml->exitbranch();
    }
}

