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

class Controller{
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
        void setportamento(int value);
	void setresonancecenter(int value);
	void setresonancebw(int value);


	void setparameternumber(unsigned int type,int value);//used for RPN and NRPN's
	int getnrpn(int *parhi, int *parlo, int *valhi, int *vallo);

	int initportamento(REALTYPE oldfreq,REALTYPE newfreq,bool legatoflag);//returns 1 if the portamento's conditions are true, else return 0
	void updateportamento(); //update portamento values 

    // Controllers values 
    struct {//Pitch Wheel
	int data;
	short int bendrange;//bendrange is in cents
	REALTYPE relfreq;//the relative frequency (default is 1.0)
    } pitchwheel;
    
    struct{//Expression
	int data;
	REALTYPE relvolume;
	unsigned char receive;
    } expression;

    struct{//Panning
	int data;
	REALTYPE pan;
	unsigned char depth;
    } panning;
    	

    struct{//Filter cutoff
	int data;
	REALTYPE relfreq;
	unsigned char depth;
    } filtercutoff;

    struct{//Filter Q
	int data;
	REALTYPE relq;
	unsigned char depth;
    } filterq;

    struct{//Bandwidth
	int data;
	REALTYPE relbw;
	unsigned char depth;
	unsigned char exponential;
    } bandwidth;

    struct {//Modulation Wheel
	int data;
	REALTYPE relmod;
	unsigned char depth;
	unsigned char exponential;
    } modwheel;

    struct{//FM amplitude
	int data;
	REALTYPE relamp;
	unsigned char receive;
    } fmamp;

    struct{//Volume
	int data;
	REALTYPE volume;
	unsigned char receive;
    } volume;

    struct{//Sustain
	int data,sustain;
	unsigned char receive;
    } sustain;

    struct{//Portamento
	//parameters
	int data;
	unsigned char portamento;
	
	//pitchthresh is the threshold of enabling protamento
	//pitchthreshtype -> enable the portamento only below(0)/above(1) the threshold
	unsigned char receive,time,pitchthresh,pitchthreshtype;

	//'up portanemto' means when the frequency is rising (eg: the portamento is from 200Hz to 300 Hz)
	//'down portanemto' means when the frequency is lowering (eg: the portamento is from 300Hz to 200 Hz)
	unsigned char updowntimestretch;//this value represent how the portamento time is reduced
	//0 - for down portamento, 1..63 - the up portamento's time is smaller than the down portamento
	//64 - the portamento time is always the same
	//64-126 - the down portamento's time is smaller than the up portamento
	//127 - for upper portamento

	REALTYPE freqrap;//this value is used to compute the actual portamento
	int noteusing;//this is used by the Part:: for knowing which note uses the portamento
	int used;//if a the portamento is used by a note
	//internal data
	REALTYPE x,dx;//x is from 0.0 (start portamento) to 1.0 (finished portamento), dx is x increment
	REALTYPE origfreqrap;// this is used for computing oldfreq value from x
    } portamento;
    
    struct{//Resonance Center Frequency
	int data;
	REALTYPE relcenter;
	unsigned char depth;
    } resonancecenter;

    struct{//Resonance Bandwidth
	int data;
	REALTYPE relbw;
	unsigned char depth;
    } resonancebandwidth;
    

    /* RPN and NPRPN */
    struct{//nrpn
	int parhi,parlo;
	int valhi,vallo;
	unsigned char receive;//this is saved to disk by Master
    } NRPN;
    
    private:
};





#endif

