/*
 * iir.c
 * Copyright (C) 2000-2003 Alexander Ehlert
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * A port of my glame lowpass/highpass/bandpass filters due to public demand in #lad
 * Try out the original at glame.sourceforge.net ;-)
 */

#include <string.h>
#include "../config.h"
#include <math.h>
#include <stdlib.h>
#include "iir.h"
	
/* To get better filter accuracy I decided to compute the single
 * stages of the filter seperatly and apply them one by one 
 * to the sample data. According to the DSPGUIDE chapter 20 pp339
 * filters are more stable when applied in stages.
 * Who doesn't like that can still combine
 * all stages to one stage by just convoluting the single stages. 
 * But in the moment it's up to the user that he knows what he's doing. 
 * float accuracy can't be enough for certain parameters.
 */

#define DPRINTF(x)

/* (hopefully) generic description of an iir filter */



iir_stage_t *init_iir_stage(int mode, int nstages, int na, int nb){
	iir_stage_t *dum=NULL;
	int i;
	if ((dum=ALLOCN(1,iir_stage_t))){
		dum->mode=mode;
		dum->nstages=0;
		dum->availst=nstages;
		dum->na=na;
		dum->nb=nb;
		dum->fc=-1.0;
		dum->coeff=(gliirt **)malloc(nstages*sizeof(gliirt *));
		for(i=0;i<nstages;i++)
			dum->coeff[i]=(gliirt *)malloc((na+nb)*sizeof(gliirt));
	}
	return dum;
}

/* be sure to combine stages with some na, nb count! */
void combine_iir_stages(int mode, iir_stage_t* gt, iir_stage_t *first, iir_stage_t *second, int upf, int ups){
	int stages, i, j, cnt;
	
	if ( (upf==-1) && (ups==-1))
		return;

	stages = first->nstages + second->nstages;
	gt->nstages = stages;

	cnt = first->na + first->nb;
	
	/* copy coefficients */
	if (upf!=-1) 
		for(i=0; i<first->nstages; i++)
			for(j=0; j<cnt; j++)
				gt->coeff[i][j]=first->coeff[i][j];

	if (ups!=-1)
		for(i=first->nstages; i<stages; i++)
			for(j=0; j<cnt; j++)
				gt->coeff[i][j]=second->coeff[i-first->nstages][j];
}

void free_iir_stage(iir_stage_t *gt){
	int i;
	for(i=0;i<gt->availst;i++)
		if (gt->coeff[i]) free(gt->coeff[i]);
	if (gt->coeff) free(gt->coeff);
	if (gt) free(gt);
}

/* center: frequency already normalized between 0 and 0.5 of sampling
 * bandwidth given in octaves between lower and upper -3dB point
 */

void calc_2polebandpass(iirf_t* iirf, iir_stage_t* gt, float fc, float bw, long sample_rate)
{
	double omega, alpha, bandwidth, center, gain;
	int i;
	
	if ( (gt->fc==fc) && (gt->bw==bw) )
		return;

	/*reset_iirf_t(iirf, gt,  1);*/
	gt->fc = fc;
	gt->bw = bw;
	gt->nstages = 1;

	fc = CLAMP(fc, 0.0, (float)sample_rate*0.45f); /* if i go all the way up to 0.5 it doesn't work */
		
	center = fc/(float)sample_rate;
	/* bandwidth is given in octaves */
	bandwidth = log((fc+bw*0.5)/MAX(fc-bw*0.5,0.01))/log(2.0);

	omega = 2.0*M_PI*center;
	alpha = sin(omega)*sinh(log(2.0)/2.0*bandwidth*omega/sin(omega));

	gt->coeff[0][0] = alpha;
	gt->coeff[0][1] = 0.0;
	gt->coeff[0][2] = -alpha;
	gt->coeff[0][3] = 2.0 * cos(omega);
	gt->coeff[0][4] = alpha - 1.0;
	gain = 1.0 + alpha;
	for(i=0;i<5;i++)
		gt->coeff[0][i]/=gain;
}

/* chebyshev calculates coefficients for a chebyshev filter
 * a,b coefficients
 * n   number of poles(2,4,6,...)
 * m   0..lowpass, 1..highpass
 * fc  cutoff frequency in percent of samplerate
 * pr  percent ripple in passband (0.5 is optimal)
 *
 * Code from DSPGUIDE Chapter 20, pp341 
 * online version http://www.dspguide.com
 */

#define chebtype double

int chebyshev_stage(iir_stage_t *gt, int n){
	chebtype h,rp,ip,es,kx,vx,t,w,m,d,k,gain;
	chebtype x[3], y[2], a[3], b[2];
	int res=-1,i;
	
	if (n>gt->availst)
		goto _error;
	if (gt->na+gt->nb!=5)
		goto _error;
	
	h=M_PI/((chebtype)gt->np*2.0)+n*M_PI/(chebtype)gt->np;
	rp=-cos(h);
	ip=sin(h);

	if(gt->ppr>0.0) {
		h=100.0/(100.0-gt->ppr);
		es=sqrt(h*h-1.0);
		h=1.0/es;
		vx=1.0/(chebtype)gt->np*log(h+sqrt(h*h+1.0));
		kx=1.0/(chebtype)gt->np*log(h+sqrt(h*h-1.0));
		kx=(exp(kx)+exp(-kx))/2.0;
		h=exp(vx);
		rp*=(h-1.0/h)*0.5/kx;
		ip*=(h+1.0/h)*0.5/kx;
	}

	t=2.0*tan(0.5);
	w=2.0*M_PI*gt->fc;
	m=rp*rp+ip*ip;
	d=4.0-4.0*rp*t+m*t*t;
	x[0]=t*t/d;
	x[1]=2*x[0];
	x[2]=x[0];
	y[0]=(8.0-2.0*m*t*t)/d;
	y[1]=(-4.0-4.0*rp*t-m*t*t)/d;

	if (gt->mode==IIR_STAGE_HIGHPASS)
		k=-cos(w*0.5+0.5)/cos(w*0.5-0.5);
	else
		k=sin(0.5-w*0.5)/sin(0.5+w*0.5);
	d=1+y[0]*k-y[1]*k*k;
	a[0]=(x[0]-x[1]*k+x[2]*k*k)/d;
	a[1]=(-2.0*x[0]*k+x[1]+x[1]*k*k-2.0*x[2]*k)/d;
	a[2]=(x[0]*k*k-x[1]*k+x[2])/d;
	b[0]=(2.0*k+y[0]+y[0]*k*k-2.0*y[1]*k)/d;
	b[1]=(-k*k-y[0]*k+y[1])/d;
	if(gt->mode==IIR_STAGE_HIGHPASS){
		a[1]=-a[1];
		b[0]=-b[0];
	}

	if(gt->mode==IIR_STAGE_HIGHPASS)
		gain=(a[0]-a[1]+a[2])/(1.0+b[0]-b[1]);
	else
		gain=(a[0]+a[1]+a[2])/(1.0-b[0]-b[1]);
	for(i=0;i<3;i++) a[i]/=gain;

	gt->coeff[n][0]=(gliirt)(a[0]);
	gt->coeff[n][1]=(gliirt)(a[1]);
	gt->coeff[n][2]=(gliirt)(a[2]);
	gt->coeff[n][3]=(gliirt)(b[0]);
	gt->coeff[n][4]=(gliirt)(b[1]);
	
	res=0;
_error:
	return res;
}

int chebyshev(iirf_t* iirf, iir_stage_t* gt, int n, int mode, float fc, float pr){
	int i;
	
	if ( (gt->fc==fc) && (gt->np==n) && (gt->ppr=pr) )
		return -1;

	if (n%2!=0)
		return -1;
		
	if ((mode!=IIR_STAGE_HIGHPASS) && (mode!=IIR_STAGE_LOWPASS))      
		return -1;	
	
	fc=CLAMP(fc, 0.0001f, 0.4999f);

	if ((n/2)>gt->nstages)
		reset_iirf_t(iirf,gt,n/2);
	
	gt->ppr=pr;
	gt->fc=fc;
	gt->np=n;
	gt->nstages=n/2;
	for(i=0;i<n/2;i++)
		chebyshev_stage(gt,i);	

	return 0;
}	
