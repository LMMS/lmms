/*
  ZynAddSubFX - a software synthesizer

  Reverb.h - Reverberation effect
  Copyright (C) 2002-2009 Nasca Octavian Paul
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

#ifndef REVERB_H
#define REVERB_H

#include <math.h>
#include "../globals.h"
#include "../DSP/AnalogFilter.h"
#include "../DSP/FFTwrapper.h"
#include "Effect.h"

#define REV_COMBS 8
#define REV_APS 4

/**Creates Reverberation Effects*/

class OverlapAdd{//50% overlap
	public:
		OverlapAdd(int small_buffer_size_,int n_small_buffers_per_half_big_buffer_){
			small_buffer_size=small_buffer_size_;
			n_small_buffers_per_half_big_buffer=n_small_buffers_per_half_big_buffer_;
			half_big_buffer_size=small_buffer_size*n_small_buffers_per_half_big_buffer;
			big_buffer_size=half_big_buffer_size*2;

			new_half_big_buffer_input=new REALTYPE[half_big_buffer_size];
			old_half_big_buffer_input=new REALTYPE[half_big_buffer_size];
			new_half_big_buffer_processed=new REALTYPE[half_big_buffer_size];
			half_big_buffer_output=new REALTYPE[half_big_buffer_size];
			big_buffer=new REALTYPE[big_buffer_size];
			for (int i=0;i<half_big_buffer_size;i++){
				new_half_big_buffer_input[i]=0.0;
				old_half_big_buffer_input[i]=0.0;
				new_half_big_buffer_processed[i]=0.0;
				half_big_buffer_output[i]=0.0;
			};
			for (int i=0;i<big_buffer_size;i++){
				big_buffer[i]=0.0;
			};
			small_buffer_k=0;
		};
		virtual ~OverlapAdd(){
			delete []new_half_big_buffer_input;
			delete []old_half_big_buffer_input;
			delete []new_half_big_buffer_processed;
			delete []half_big_buffer_output;
			delete []big_buffer;
		};

		
		void process(REALTYPE *small_buffer){

			int input_start_pos=small_buffer_size*small_buffer_k; 

			for (int i=0;i<small_buffer_size;i++){
				new_half_big_buffer_input[input_start_pos+i]=small_buffer[i];
			};
			small_buffer_k++;
			if (small_buffer_k>=n_small_buffers_per_half_big_buffer){
				small_buffer_k=0;
				process_big_buffer();
			};

			int output_start_pos=small_buffer_size*small_buffer_k; //check if this is correct

			for (int i=0;i<small_buffer_size;i++){
				small_buffer[i]=half_big_buffer_output[output_start_pos+i];
			};


		};	
	protected:
		int half_big_buffer_size;
		int big_buffer_size;
		REALTYPE *big_buffer;

		virtual void do_process_big_buffer(){//the resulting buffer must be windowed
			for (int i=0;i<big_buffer_size;i++){
				big_buffer[i]*=(1.0-cos(i*M_PI*2.0/big_buffer_size))*0.5;
//				big_buffer[i]*=0.5;
			};
//			printf("BIG_BUFFER:\n"); for (int i=0;i<big_buffer_size;i++) printf(" %g ",big_buffer[i]); printf("\n\n"); 
		};
	private:		
		void process_big_buffer(){
			for (int i=0;i<half_big_buffer_size;i++){
				big_buffer[i]=old_half_big_buffer_input[i];
				big_buffer[i+half_big_buffer_size]=new_half_big_buffer_input[i];
			};
			
			do_process_big_buffer();//process input buffer and get windowed buffer

			for (int i=0;i<half_big_buffer_size;i++){
				old_half_big_buffer_input[i]=new_half_big_buffer_input[i];
			};
			
//				printf("OUT1:\n"); for (int i=0;i<half_big_buffer_size;i++) printf(" %g,%g ",big_buffer[i],new_half_big_buffer_processed[i]); printf("\n\n"); 
			for (int i=0;i<half_big_buffer_size;i++){
				half_big_buffer_output[i]=big_buffer[i]+new_half_big_buffer_processed[i];
				new_half_big_buffer_processed[i]=big_buffer[i+half_big_buffer_size];
			};

		};

		int small_buffer_size;
		int n_small_buffers_per_half_big_buffer;
		int small_buffer_k;

		REALTYPE *old_half_big_buffer_input,*new_half_big_buffer_input;
		REALTYPE *new_half_big_buffer_processed;

		REALTYPE *half_big_buffer_output;	
};

class ReverbBandwidth: public OverlapAdd{
	public:
		ReverbBandwidth (int small_buffer_size_,int n_small_buffers_per_half_big_buffer_);
		~ReverbBandwidth();
		void do_spread(int nfreq,REALTYPE *freq1,REALTYPE *freq2, REALTYPE bandwidth);
		void set_bandwidth(REALTYPE par){
			if (par<0.0) par=0.0;
			if (par>1.0) par=1.0;
			bandwidth=par;
		};
	private:
		void do_process_big_buffer();
		FFTwrapper *fft;
		FFTFREQS freqs;
		REALTYPE *srcfreq,*destfreq,*tmpfreq;
		REALTYPE *window;
		REALTYPE bandwidth;
};

class Reverb:public Effect
{
public:
    Reverb(const int &insertion_,REALTYPE *efxoutl_,REALTYPE *efxoutr_);
    ~Reverb();
    void out(REALTYPE *smps_l,REALTYPE *smps_r);
    void cleanup();

    void setpreset(unsigned char npreset);
    void changepar(const int &npar,const unsigned char &value);
    unsigned char getpar(const int &npar)const;

private:
    //Parametrii
    /**Amount of the reverb*/
    unsigned char Pvolume;

    /**Left/Right Panning*/
    unsigned char Ppan;

    /**duration of reverb*/
    unsigned char Ptime;

    /**Initial delay*/
    unsigned char Pidelay;

    /**Initial delay feedback*/
    unsigned char Pidelayfb;

    /**delay between ER/Reverbs*/
    unsigned char Prdelay;

    /**EarlyReflections/Reverb Balance*/
    unsigned char Perbalance;

    /**HighPassFilter*/
    unsigned char Plpf;

    /**LowPassFilter*/
    unsigned char Phpf;

    /**Low/HighFrequency Damping
         * \todo 0..63 lpf,64=off,65..127=hpf(TODO)*/
    unsigned char Plohidamp;

    /**Reverb type*/
    unsigned char Ptype;

    /**Room Size*/
    unsigned char Proomsize;

	/**Bandwidth */
	unsigned char Pbandwidth;

    //parameter control
    void setvolume(const unsigned char &Pvolume);
    void setpan(const unsigned char &Ppan);
    void settime(const unsigned char &Ptime);
    void setlohidamp(unsigned char Plohidamp);
    void setidelay(const unsigned char &Pidelay);
    void setidelayfb(const unsigned char &Pidelayfb);
    void sethpf(const unsigned char &Phpf);
    void setlpf(const unsigned char &Plpf);
    void settype( unsigned char Ptype);
    void setroomsize(const unsigned char &Proomsize);
    void setbandwidth(const unsigned char &Pbandwidth);

    REALTYPE pan,erbalance;
    //Parametrii 2
    int lohidamptype;/**<0=disable,1=highdamp(lowpass),2=lowdamp(highpass)*/
    int idelaylen,rdelaylen;
    int idelayk;
    REALTYPE lohifb,idelayfb,roomsize,rs;//rs is used to "normalise" the volume according to the roomsize
    int comblen[REV_COMBS*2];
    int aplen[REV_APS*2];
	ReverbBandwidth *bandwidth;

    //Internal Variables

    REALTYPE *comb[REV_COMBS*2];

    int combk[REV_COMBS*2];
    REALTYPE combfb[REV_COMBS*2];/**<feedback-ul fiecarui filtru "comb"*/
    REALTYPE lpcomb[REV_COMBS*2];/**<pentru Filtrul LowPass*/

    REALTYPE *ap[REV_APS*2];

    int apk[REV_APS*2];

    REALTYPE *idelay;
    AnalogFilter *lpf,*hpf;//filters
    REALTYPE *inputbuf;

    void processmono(int ch,REALTYPE *output);
};

#endif

