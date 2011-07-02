/*

        Copyright (C) 1999 Juhana Sadeharju
                       kouhia at nic.funet.fi

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "gverbdsp.h"
#include "gverb.h"
#include "../ladspa-util.h"

ty_gverb *gverb_new(int srate, float maxroomsize, float roomsize,
		    float revtime,
		    float damping, float spread,
		    float inputbandwidth, float earlylevel,
		    float taillevel)
{
  ty_gverb *p;
  float ga,gb,gt;
  int i,n;
  float r;
  float diffscale;
  int a,b,c,cc,d,dd,e;
  float spread1,spread2;

  p = (ty_gverb *)malloc(sizeof(ty_gverb));
  p->rate = srate;
  p->fdndamping = damping;
  p->maxroomsize = maxroomsize;
  p->roomsize = roomsize;
  p->revtime = revtime;
  p->earlylevel = earlylevel;
  p->taillevel = taillevel;

  p->maxdelay = p->rate*p->maxroomsize/340.0;
  p->largestdelay = p->rate*p->roomsize/340.0;


  /* Input damper */

  p->inputbandwidth = inputbandwidth;
  p->inputdamper = damper_make(1.0 - p->inputbandwidth);


  /* FDN section */


  p->fdndels = (ty_fixeddelay **)calloc(FDNORDER, sizeof(ty_fixeddelay *));
  for(i = 0; i < FDNORDER; i++) {
    p->fdndels[i] = fixeddelay_make((int)p->maxdelay+1000);
  }
  p->fdngains = (float *)calloc(FDNORDER, sizeof(float));
  p->fdnlens = (int *)calloc(FDNORDER, sizeof(int));

  p->fdndamps = (ty_damper **)calloc(FDNORDER, sizeof(ty_damper *));
  for(i = 0; i < FDNORDER; i++) {
    p->fdndamps[i] = damper_make(p->fdndamping);
  }

  ga = 60.0;
  gt = p->revtime;
  ga = powf(10.0f,-ga/20.0f);
  n = p->rate*gt;
  p->alpha = pow((double)ga, 1.0/(double)n);

  gb = 0.0;
  for(i = 0; i < FDNORDER; i++) {
    if (i == 0) gb = 1.000000*p->largestdelay;
    if (i == 1) gb = 0.816490*p->largestdelay;
    if (i == 2) gb = 0.707100*p->largestdelay;
    if (i == 3) gb = 0.632450*p->largestdelay;

#if 0
    p->fdnlens[i] = nearest_prime((int)gb, 0.5);
#else
    p->fdnlens[i] = f_round(gb);
#endif
    p->fdngains[i] = -powf((float)p->alpha,p->fdnlens[i]);
  }

  p->d = (float *)calloc(FDNORDER, sizeof(float));
  p->u = (float *)calloc(FDNORDER, sizeof(float));
  p->f = (float *)calloc(FDNORDER, sizeof(float));

  /* Diffuser section */

  diffscale = (float)p->fdnlens[3]/(210+159+562+410);
  spread1 = spread;
  spread2 = 3.0*spread;

  b = 210;
  r = 0.125541;
  a = spread1*r;
  c = 210+159+a;
  cc = c-b;
  r = 0.854046;
  a = spread2*r;
  d = 210+159+562+a;
  dd = d-c;
  e = 1341-d;

  p->ldifs = (ty_diffuser **)calloc(4, sizeof(ty_diffuser *));
  p->ldifs[0] = diffuser_make((int)(diffscale*b),0.75);
  p->ldifs[1] = diffuser_make((int)(diffscale*cc),0.75);
  p->ldifs[2] = diffuser_make((int)(diffscale*dd),0.625);
  p->ldifs[3] = diffuser_make((int)(diffscale*e),0.625);

  b = 210;
  r = -0.568366;
  a = spread1*r;
  c = 210+159+a;
  cc = c-b;
  r = -0.126815;
  a = spread2*r;
  d = 210+159+562+a;
  dd = d-c;
  e = 1341-d;

  p->rdifs = (ty_diffuser **)calloc(4, sizeof(ty_diffuser *));
  p->rdifs[0] = diffuser_make((int)(diffscale*b),0.75);
  p->rdifs[1] = diffuser_make((int)(diffscale*cc),0.75);
  p->rdifs[2] = diffuser_make((int)(diffscale*dd),0.625);
  p->rdifs[3] = diffuser_make((int)(diffscale*e),0.625);



  /* Tapped delay section */

  p->tapdelay = fixeddelay_make(44000);
  p->taps = (int *)calloc(FDNORDER, sizeof(int));
  p->tapgains = (float *)calloc(FDNORDER, sizeof(float));

  p->taps[0] = 5+0.410*p->largestdelay;
  p->taps[1] = 5+0.300*p->largestdelay;
  p->taps[2] = 5+0.155*p->largestdelay;
  p->taps[3] = 5+0.000*p->largestdelay;

  for(i = 0; i < FDNORDER; i++) {
    p->tapgains[i] = pow(p->alpha,(double)p->taps[i]);
  }

  return(p);
}

void gverb_free(ty_gverb *p)
{
  int i;

  damper_free(p->inputdamper);
  for(i = 0; i < FDNORDER; i++) {
    fixeddelay_free(p->fdndels[i]);
    damper_free(p->fdndamps[i]);
    diffuser_free(p->ldifs[i]);
    diffuser_free(p->rdifs[i]);
  }
  free(p->fdndels);
  free(p->fdngains);
  free(p->fdnlens);
  free(p->fdndamps);
  free(p->d);
  free(p->u);
  free(p->f);
  free(p->ldifs);
  free(p->rdifs);
  free(p->taps);
  free(p->tapgains);
  fixeddelay_free(p->tapdelay);
  free(p);
}

void gverb_flush(ty_gverb *p)
{
  int i;

  damper_flush(p->inputdamper);
  for(i = 0; i < FDNORDER; i++) {
    fixeddelay_flush(p->fdndels[i]);
    damper_flush(p->fdndamps[i]);
    diffuser_flush(p->ldifs[i]);
    diffuser_flush(p->rdifs[i]);
  }
  memset(p->d, 0, FDNORDER * sizeof(float));
  memset(p->u, 0, FDNORDER * sizeof(float));
  memset(p->f, 0, FDNORDER * sizeof(float));
  fixeddelay_flush(p->tapdelay);
}

/* swh: other functions are now in the .h file for inlining */
