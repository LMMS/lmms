/*
  ZynAddSubFX - a software synthesizer

  MIDIFile.C - MIDI file loader
  Copyright (C) 2003-2005 Nasca Octavian Paul
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

#include <stdio.h>
#include <string.h>
#include "MIDIFile.h"


MIDIFile::MIDIFile()
{
    midifile=NULL;
    midifilesize=0;
    midifilek=0;
    midieof=false;
    me=NULL;
};

MIDIFile::~MIDIFile()
{
    clearmidifile();
};

int MIDIFile::loadfile(const char *filename)
{
    clearmidifile();

    FILE *file=fopen(filename,"r");
    if (file==NULL) return(-1);

    char header[4];
    ZERO(header,4);
    fread(header,4,1,file);

    //test to see if this a midi file
    if ((header[0]!='M')||(header[1]!='T')||(header[2]!='h')||(header[3]!='d')) {
        fclose(file);
        return(-1);
    };

    //get the filesize
    fseek(file,0,SEEK_END);
    midifilesize=ftell(file);
    rewind(file);

    midifile=new unsigned char[midifilesize];
    ZERO(midifile,midifilesize);
    fread(midifile,midifilesize,1,file);
    fclose(file);

//    for (int i=0;i<midifilesize;i++) printf("%2x ",midifile[i]);
//    printf("\n");


    return(0);
};

int MIDIFile::parsemidifile(MIDIEvents *me_)
{
    this->me=me_;

    //read the header
    int chunk=getint32();//MThd
    if (chunk!=0x4d546864) return(-1);
    int size=getint32();
    if (size!=6) return(-1);//header is always 6 bytes long


    int format=getint16();
    printf("format %d\n",format);

    int ntracks=getint16();//this is always 1 if the format is "0"
    printf("ntracks %d\n",ntracks);

    int division=getint16();
    printf("division %d\n",division);
    if (division>=0) {//delta time units in each a quater note
//	tick=???;
    } else {//SMPTE (frames/second and ticks/frame)
        printf("ERROR:in MIDIFile.C::parsemidifile() - SMPTE not implemented yet.");
    };

    if (ntracks>=NUM_MIDI_TRACKS) ntracks=NUM_MIDI_TRACKS-1;

    for (int n=0;n<ntracks;n++) {
        if (parsetrack(n)<0) {
            clearmidifile();
            return(-1);
        };
    };

    printf("\n\nCURRENT File position is = 0x%x\n",midifilek);
    printf("\nMIDI file succesfully parsed.\n");
//    printf("\n0x%x\n",getbyte());

    this->me=NULL;
    return(0);
};

//private members


int MIDIFile::parsetrack(int ntrack)
{
    printf("\n--==*Reading track %d **==--\n",ntrack);

    int chunk=getint32();//MTrk
    if (chunk!=0x4d54726b) return(-1);

    int size=getint32();
    printf("size = %d\n",size);

    int oldmidifilek=midifilek;

    unsigned char lastmsg=0;
    unsigned int dt=0;

    while (!midieof) {
        unsigned int msgdeltatime=getvarint32();

///	printf("MSGDELTATIME = %d\n",msgdeltatime);

//	dt+=msgdeltatime;

        int msg=peekbyte();
///	printf("raw msg=0x%x     ",msg);
        if (msg<0x80) {
            msg=lastmsg;
        } else {
            lastmsg=msg;
            getbyte();
        };
///	printf("msg=0x%x\n",msg);

//	dt+=msgdeltatime;
        add_dt(ntrack, msgdeltatime);

        unsigned int mtype,mlength;

        switch (msg) {
        case 0x80 ... 0x8f://note on off
            parsenoteoff(ntrack,msg & 0x0f,dt);
            dt=0;
            break;
        case 0x90 ... 0x9f://note on (or note off)
            parsenoteon(ntrack,msg & 0x0f,dt);
            dt=0;
            break;
        case 0xa0 ... 0xaf://aftertouch - ignored
            skipnbytes(2);
            break;
        case 0xb0 ... 0xbf://control change
            parsecontrolchange(ntrack,msg & 0x0f,dt);
            dt=0;
            break;
        case 0xc0 ... 0xcf://program change - ignored
            skipnbytes(1);
            break;
        case 0xd0 ... 0xdf://channel pressure - ignored
            skipnbytes(1);
            break;
        case 0xe0 ... 0xef://channel mode messages
            skipnbytes(2);
            break;
        case 0xf0://sysex - ignored
            while (getbyte()!=0xf7) {
                if (midieof) break;
            };
            break;
        case 0xf7://sysex (another type) - ignored
            skipnbytes(getvarint32());
            break;

        case 0xff://meta-event
            mtype=getbyte();
            mlength=getbyte();
            parsemetaevent(mtype,mlength);
            break;

        default:
            getbyte();
            printf("UNKNOWN message! 0x%x\n",msg);
            return(-1);
            break;
        };



        if (midieof) return(-1);

        if ((midifilek-oldmidifilek)==size) break;
        else if ((midifilek-oldmidifilek)>size) return(-1);
//    if (size!=6) return(-1);//header is always 6 bytes long
    };

    printf("End Track\n\n");

    return(0);
};


void MIDIFile::parsenoteoff(char ntrack,char chan,unsigned int dt)
{
    unsigned char note;
    note=getbyte();

    unsigned char noteoff_velocity=getbyte();//unused by zynaddsubfx
    noteoff_velocity=0;
    if (chan>=NUM_MIDI_CHANNELS) return;

    me->tmpevent.deltatime=convertdt(dt);
    me->tmpevent.type=1;
    me->tmpevent.par1=note;
    me->tmpevent.par2=0;
    me->tmpevent.channel=chan;

    printf("Note off:%d \n",note);

    ///test
//    ntrack=0;

    me->writeevent(&me->miditrack[(int)ntrack].record,&me->tmpevent);

};


void MIDIFile::parsenoteon(char ntrack,char chan,unsigned int dt)
{
    unsigned char note,vel;
    note=getbyte();
    vel=getbyte();

//    printf("ntrack=%d\n",ntrack);
    printf("[dt %d ]  Note on:%d %d\n",dt,note,vel);

    if (chan>=NUM_MIDI_CHANNELS) return;

    me->tmpevent.deltatime=convertdt(dt);
    me->tmpevent.type=1;
    me->tmpevent.par1=note;
    me->tmpevent.par2=vel;
    me->tmpevent.channel=chan;
    me->writeevent(&me->miditrack[(int)ntrack].record,&me->tmpevent);



};

void MIDIFile::parsecontrolchange(char ntrack,char chan,unsigned int dt)
{
    unsigned char control,value;
    control=getbyte();
    value=getbyte();

    if (chan>=NUM_MIDI_CHANNELS) return;

    printf("[dt %d] Control change:%d %d\n",dt,control,value);

    me->tmpevent.deltatime=convertdt(dt);
    me->tmpevent.type=2;
    me->tmpevent.par1=control;//???????????? ma uit la Sequencer::recordnote() din varianele vechi de zyn
    me->tmpevent.par2=value;
    me->tmpevent.channel=chan;
    me->writeevent(&me->miditrack[(int)ntrack].record,&me->tmpevent);

};

void MIDIFile::parsepitchwheel(char ntrack,char chan, unsigned int dt)
{
    unsigned char valhi,vallo;
    vallo=getbyte();
    valhi=getbyte();

    if (chan>=NUM_MIDI_CHANNELS) return;

    int value=(int)valhi*128+vallo;

    printf("[dt %d] Pitch wheel:%d\n",dt,value);

};

void MIDIFile::parsemetaevent(unsigned char mtype,unsigned char mlength)
{
    int oldmidifilek=midifilek;
    printf("meta-event type=0x%x  length=%d\n",mtype,mlength);



    midifilek=oldmidifilek+mlength;

};

void MIDIFile::add_dt(char ntrack, unsigned int dt)
{
    me->tmpevent.deltatime=convertdt(dt);
    me->tmpevent.type=255;
    me->tmpevent.par1=0;
    me->tmpevent.par2=0;
    me->tmpevent.channel=0;
    me->writeevent(&me->miditrack[(int)ntrack].record,&me->tmpevent);
};


unsigned int MIDIFile::convertdt(unsigned int dt)
{
    double result=dt;
    printf("DT=%d\n",dt);

    return((int) (result*15.0));
};


void MIDIFile::clearmidifile()
{
    if (midifile!=NULL) delete(midifile);
    midifile=NULL;
    midifilesize=0;
    midifilek=0;
    midieof=false;
    data.tick=0.05;
};

unsigned char MIDIFile::getbyte()
{
    if (midifilek>=midifilesize) {
        midieof=true;
        return(0);
    };

///    printf("(%d) ",midifile[midifilek]);
    return(midifile[midifilek++]);
};

unsigned char MIDIFile::peekbyte()
{
    if (midifilek>=midifilesize) {
        midieof=true;
        return(0);
    };
    return(midifile[midifilek]);
};

unsigned int MIDIFile::getint32()
{
    unsigned int result=0;
    for (int i=0;i<4;i++) {
        result=result*256+getbyte();
    };
    if (midieof) result=0;
    return(result);
};

unsigned short int MIDIFile::getint16()
{
    unsigned short int result=0;
    for (int i=0;i<2;i++) {
        result=result*256+getbyte();
    };
    if (midieof) result=0;
    return(result);
};

unsigned int MIDIFile::getvarint32()
{
    unsigned long result=0;
    unsigned char b;

///    printf("\n[start]");

    if ((result = getbyte()) & 0x80) {
        result &= 0x7f;
        do  {
            b=getbyte();
            result = (result << 7) + (b & 0x7f);
        } while (b & 0x80);
    }
///    printf("[end - result= %d]\n",result);
    return result;
};


void MIDIFile::skipnbytes(int n)
{
    midifilek+=n;
    if (midifilek>=midifilesize) {
        midifilek=midifilesize-1;
        midieof=true;
    };
};

