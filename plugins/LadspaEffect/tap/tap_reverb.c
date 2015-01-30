/*                                                     -*- linux-c -*-
    Copyright (C) 2004 Tom Szilagyi
    
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

    $Id: tap_reverb.c,v 1.13 2004/06/15 14:50:55 tszilagyi Exp $
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <ladspa.h>


/* ***** VERY IMPORTANT! *****
 *
 * If you enable this, the plugin will use float arithmetics in DSP
 * calculations.  This usually yields lower average CPU usage, but
 * occasionaly may result in high CPU peaks which cause trouble to you
 * and your JACK server.  The default is to use fixpoint arithmetics
 * (with the following #define commented out).  But (depending on the
 * processor on which you run the code) you may find floating point
 * mode usable.
 */
/* #define REVERB_CALC_FLOAT */



#ifndef REVERB_CALC_FLOAT
typedef signed int sample;
#endif

#ifndef REVERB_CALC_FLOAT
typedef sample rev_t;
#else
typedef LADSPA_Data rev_t;
#endif


#include "tap_reverb_presets.h"



#ifdef REVERB_CALC_FLOAT
#define DENORM(x) (((unsigned char)(((*(unsigned int*)&(x))&0x7f800000)>>23))<103)?0.0f:(x)
#else
/* coefficient for float to sample (signed int) conversion */
/* this allows for about 60 dB headroom above 0dB, if 0 dB is equivalent to 1.0f */
/* As 2^31 equals more than 180 dB, about 120 dB dynamics remains below 0 dB */
#define F2S 2147483
#endif


/* load plugin data from reverb_data[] into an instance */
void
load_plugin_data(LADSPA_Handle Instance) {

	Reverb * ptr = (Reverb *)Instance;
	unsigned long m;
	unsigned int i;


	m = LIMIT(*(ptr->mode),0,NUM_MODES-1);

	/* load combs data */
	ptr->num_combs = 2 * reverb_data[m].num_combs;
	for (i = 0; i < reverb_data[m].num_combs; i++) {
		((COMB_FILTER *)(ptr->combs + 2*i))->buflen = 
			reverb_data[m].combs[i].delay * ptr->sample_rate;
		((COMB_FILTER *)(ptr->combs + 2*i))->feedback = 
			reverb_data[m].combs[i].feedback;
		((COMB_FILTER *)(ptr->combs + 2*i))->freq_resp =
			LIMIT(reverb_data[m].combs[i].freq_resp
			      * powf(ptr->sample_rate / 44100.0f, 0.8f),
			      0.0f, 1.0f);

		((COMB_FILTER *)(ptr->combs + 2*i+1))->buflen =
			((COMB_FILTER *)(ptr->combs + 2*i))->buflen;
		((COMB_FILTER *)(ptr->combs + 2*i+1))->feedback = 
			((COMB_FILTER *)(ptr->combs + 2*i))->feedback;
		((COMB_FILTER *)(ptr->combs + 2*i+1))->feedback = 
			((COMB_FILTER *)(ptr->combs + 2*i))->freq_resp;

		/* set initial values: */
		*(((COMB_FILTER *)(ptr->combs + 2*i))->buffer_pos) = 0;
		*(((COMB_FILTER *)(ptr->combs + 2*i+1))->buffer_pos) = 0;
		((COMB_FILTER *)(ptr->combs + 2*i))->last_out = 0;
		((COMB_FILTER *)(ptr->combs + 2*i+1))->last_out = 0;

		lp_set_params(((COMB_FILTER *)(ptr->combs + 2*i))->filter,
			      2000.0f + 13000.0f * (1 - reverb_data[m].combs[i].freq_resp)
			      * ptr->sample_rate / 44100.0f,
			      BANDPASS_BWIDTH, ptr->sample_rate);
		lp_set_params(((COMB_FILTER *)(ptr->combs + 2*i+1))->filter,
			      2000.0f + 13000.0f * (1 - reverb_data[m].combs[i].freq_resp)
			      * ptr->sample_rate / 44100.0f,
			      BANDPASS_BWIDTH, ptr->sample_rate);
	}

	/* load allps data */
	ptr->num_allps = 2 * reverb_data[m].num_allps;
	for (i = 0; i < reverb_data[m].num_allps; i++) {
		((ALLP_FILTER *)(ptr->allps + 2*i))->buflen = 
			reverb_data[m].allps[i].delay * ptr->sample_rate;
		((ALLP_FILTER *)(ptr->allps + 2*i))->feedback = 
			reverb_data[m].allps[i].feedback;

		((ALLP_FILTER *)(ptr->allps + 2*i+1))->buflen = 
			((ALLP_FILTER *)(ptr->allps + 2*i))->buflen;
		((ALLP_FILTER *)(ptr->allps + 2*i+1))->feedback = 
			((ALLP_FILTER *)(ptr->allps + 2*i))->feedback;

		/* set initial values: */
		*(((ALLP_FILTER *)(ptr->allps + 2*i))->buffer_pos) = 0;
		*(((ALLP_FILTER *)(ptr->allps + 2*i+1))->buffer_pos) = 0;
		((ALLP_FILTER *)(ptr->allps + 2*i))->last_out = 0;
		((ALLP_FILTER *)(ptr->allps + 2*i+1))->last_out = 0;
	}

	/* init bandpass filters */
	lp_set_params((biquad *)(ptr->low_pass), reverb_data[m].bandpass_high,
		      BANDPASS_BWIDTH, ptr->sample_rate);
	hp_set_params((biquad *)(ptr->high_pass), reverb_data[m].bandpass_low,
		      BANDPASS_BWIDTH, ptr->sample_rate);
	lp_set_params((biquad *)(ptr->low_pass + 1), reverb_data[m].bandpass_high,
		      BANDPASS_BWIDTH, ptr->sample_rate);
	hp_set_params((biquad *)(ptr->high_pass + 1), reverb_data[m].bandpass_low,
		      BANDPASS_BWIDTH, ptr->sample_rate);
}



/* push a sample into a comb filter and return the sample falling out */
rev_t
comb_run(rev_t insample, COMB_FILTER * comb) {

	rev_t outsample;
	rev_t pushin;

	pushin = comb->fb_gain * insample + biquad_run(comb->filter, comb->fb_gain * comb->last_out);
#ifdef REVERB_CALC_FLOAT
	pushin = DENORM(pushin);
#endif
	outsample = push_buffer(pushin,
				comb->ringbuffer, comb->buflen, comb->buffer_pos);
#ifdef REVERB_CALC_FLOAT
	outsample = DENORM(outsample);
#endif
	comb->last_out = outsample;

	return outsample;
}


/* push a sample into an allpass filter and return the sample falling out */
rev_t
allp_run(rev_t insample, ALLP_FILTER * allp) {

	rev_t outsample;
	rev_t pushin;
	pushin = allp->in_gain * allp->fb_gain * insample + allp->fb_gain * allp->last_out;
#ifdef REVERB_CALC_FLOAT
	pushin = DENORM(pushin);
#endif
	outsample = push_buffer(pushin,
				allp->ringbuffer, allp->buflen, allp->buffer_pos);
#ifdef REVERB_CALC_FLOAT
	outsample = DENORM(outsample);
#endif
	allp->last_out = outsample;

	return outsample;
}


/* compute user-input-dependent reverberator coefficients */
void
comp_coeffs(LADSPA_Handle Instance) {

	Reverb * ptr = (Reverb *)Instance;
	unsigned int i;
	

	if (*(ptr->mode) != ptr->old_mode)
		load_plugin_data(Instance);

	for (i = 0; i < ptr->num_combs / 2; i++) {
		((COMB_FILTER *)(ptr->combs + 2*i))->fb_gain =
			powf(0.001f,
			     1000.0f * ((COMB_FILTER *)(ptr->combs + 2*i))->buflen
			     * (1 + FR_R_COMP * ((COMB_FILTER *)(ptr->combs + 2*i))->freq_resp)
			     / powf(((COMB_FILTER *)(ptr->combs + 2*i))->feedback/100.0f, 0.89f)
			     / *(ptr->decay)
			     / ptr->sample_rate);

		((COMB_FILTER *)(ptr->combs + 2*i+1))->fb_gain = 
			((COMB_FILTER *)(ptr->combs + 2*i))->fb_gain;
		
		if (*(ptr->stereo_enh) > 0.0f) {
			if (i % 2 == 0)
				((COMB_FILTER *)(ptr->combs + 2*i+1))->buflen =
					ENH_STEREO_RATIO * ((COMB_FILTER *)(ptr->combs + 2*i))->buflen;
			else
				((COMB_FILTER *)(ptr->combs + 2*i))->buflen =
					ENH_STEREO_RATIO * ((COMB_FILTER *)(ptr->combs + 2*i+1))->buflen;
		} else {
			if (i % 2 == 0)
				((COMB_FILTER *)(ptr->combs + 2*i+1))->buflen =
					((COMB_FILTER *)(ptr->combs + 2*i))->buflen;
			else
				((COMB_FILTER *)(ptr->combs + 2*i))->buflen =
					((COMB_FILTER *)(ptr->combs + 2*i+1))->buflen;
		}
	}

	for (i = 0; i < ptr->num_allps / 2; i++) {
		((ALLP_FILTER *)(ptr->allps + 2*i))->fb_gain =
			powf(0.001f, 11000.0f * ((ALLP_FILTER *)(ptr->allps + 2*i))->buflen
			     / powf(((ALLP_FILTER *)(ptr->allps + 2*i))->feedback/100.0f, 0.88f)
			     / *(ptr->decay)
			     / ptr->sample_rate);
		
		((ALLP_FILTER *)(ptr->allps + 2*i+1))->fb_gain = 
			((ALLP_FILTER *)(ptr->allps + 2*i))->fb_gain;

		((ALLP_FILTER *)(ptr->allps + 2*i))->in_gain = -0.06f
			/ (((ALLP_FILTER *)(ptr->allps + 2 * i))->feedback/100.0f)
			/ powf((*(ptr->decay) + 3500.0f) / 10000.0f, 1.5f);

		((ALLP_FILTER *)(ptr->allps + 2*i+1))->in_gain = 
			((ALLP_FILTER *)(ptr->allps + 2*i))->in_gain;

		if (*(ptr->stereo_enh) > 0.0f) {
			if (i % 2 == 0)
				((ALLP_FILTER *)(ptr->allps + 2*i+1))->buflen =
					ENH_STEREO_RATIO * ((ALLP_FILTER *)(ptr->allps + 2*i))->buflen;
			else
				((ALLP_FILTER *)(ptr->allps + 2*i))->buflen =
					ENH_STEREO_RATIO * ((ALLP_FILTER *)(ptr->allps + 2*i+1))->buflen;
		} else {
			if (i % 2 == 0)
				((ALLP_FILTER *)(ptr->allps + 2*i+1))->buflen =
					((ALLP_FILTER *)(ptr->allps + 2*i))->buflen;
			else
				((ALLP_FILTER *)(ptr->allps + 2*i))->buflen =
					((ALLP_FILTER *)(ptr->allps + 2*i+1))->buflen;
		}
	}
}



/* Construct a new plugin instance. */
LADSPA_Handle 
instantiate_Reverb(const LADSPA_Descriptor * Descriptor,
		   unsigned long             SampleRate) {
	
	unsigned long i;
	LADSPA_Handle * p;
	Reverb * ptr = NULL;
	
	if ((p = malloc(sizeof(Reverb))) != NULL) {
		((Reverb *)p)->sample_rate = SampleRate;
		((Reverb *)p)->run_adding_gain = 1.0f;

		ptr = (Reverb *)p;

		/* allocate memory for comb/allpass filters and other dynamic vars */
		if ((ptr->combs =
		     calloc(2 * MAX_COMBS, sizeof(COMB_FILTER))) == NULL)
			return NULL;
		for (i = 0; i < 2 * MAX_COMBS; i++) {
			if ((((COMB_FILTER *)(ptr->combs + i))->ringbuffer =
			     calloc((unsigned long)MAX_COMB_DELAY * ptr->sample_rate / 1000,
				    sizeof(LADSPA_Data))) == NULL)
				return NULL;
			if ((((COMB_FILTER *)(ptr->combs + i))->buffer_pos =
			     calloc(1, sizeof(unsigned long))) == NULL)
				return NULL;
			if ((((COMB_FILTER *)(ptr->combs + i))->filter =
			     calloc(1, sizeof(biquad))) == NULL)
				return NULL;
		}

		if ((ptr->allps =
		     calloc(2 * MAX_ALLPS, sizeof(ALLP_FILTER))) == NULL)
			return NULL;
		for (i = 0; i < 2 * MAX_ALLPS; i++) {
			if ((((ALLP_FILTER *)(ptr->allps + i))->ringbuffer =
			     calloc((unsigned long)MAX_ALLP_DELAY * ptr->sample_rate / 1000,
				    sizeof(LADSPA_Data))) == NULL)
				return NULL;
			if ((((ALLP_FILTER *)(ptr->allps + i))->buffer_pos =
			     calloc(1, sizeof(unsigned long))) == NULL)
				return NULL;
		}
		
		if ((ptr->low_pass =
		     calloc(2, sizeof(biquad))) == NULL)
			return NULL;
		if ((ptr->high_pass =
		     calloc(2, sizeof(biquad))) == NULL)
			return NULL;
		
		return p;
	}
	return NULL;
}


/* activate a plugin instance */
void
activate_Reverb(LADSPA_Handle Instance) {

	Reverb * ptr = (Reverb *)Instance;
	unsigned long i,j;

	for (i = 0; i < 2 * MAX_COMBS; i++) {
		for (j = 0; j < (unsigned long)MAX_COMB_DELAY * ptr->sample_rate / 1000; j++)
		        ((COMB_FILTER *)(ptr->combs + i))->ringbuffer[j] = 0.0f;
		*(((COMB_FILTER *)(ptr->combs + i))->buffer_pos) = 0;
		((COMB_FILTER *)(ptr->combs + i))->last_out = 0;
		biquad_init(((COMB_FILTER *)(ptr->combs + i))->filter);
	}

	for (i = 0; i < 2 * MAX_ALLPS; i++) {
		for (j = 0; j < (unsigned long)MAX_ALLP_DELAY * ptr->sample_rate / 1000; j++)
			((ALLP_FILTER *)(ptr->allps + i))->ringbuffer[j] = 0.0f;
		*(((ALLP_FILTER *)(ptr->allps + i))->buffer_pos) = 0;
		((ALLP_FILTER *)(ptr->allps + i))->last_out = 0;
	}

	biquad_init(ptr->low_pass);
	biquad_init((biquad *)(ptr->low_pass + 1));
	biquad_init(ptr->high_pass);
	biquad_init((biquad *)(ptr->high_pass + 1));

	ptr->old_decay = -10.0f;
	ptr->old_stereo_enh = -10.0f;
	ptr->old_mode = -10.0f;
}



/* Connect a port to a data location. */
void 
connect_port_Reverb(LADSPA_Handle Instance,
		    unsigned long Port,
		    LADSPA_Data * DataLocation) {
	
	Reverb * ptr = (Reverb *)Instance;

	switch (Port) {
	case DECAY:
		ptr->decay = DataLocation;
		break;
	case DRYLEVEL:
		ptr->drylevel = DataLocation;
		break;
	case WETLEVEL:
		ptr->wetlevel = DataLocation;
		break;
	case COMBS_EN:
		ptr->combs_en = DataLocation;
		break;
	case ALLPS_EN:
		ptr->allps_en = DataLocation;
		break;
	case BANDPASS_EN:
		ptr->bandpass_en = DataLocation;
		break;
	case STEREO_ENH:
		ptr->stereo_enh = DataLocation;
		break;
	case MODE:
		ptr->mode = DataLocation;
		break;
	case INPUT_L:
		ptr->input_L = DataLocation;
		break;
	case OUTPUT_L:
		ptr->output_L = DataLocation;
		break;
	case INPUT_R:
		ptr->input_R = DataLocation;
		break;
	case OUTPUT_R:
		ptr->output_R = DataLocation;
		break;
	}
}



void 
run_Reverb(LADSPA_Handle Instance,
	   unsigned long SampleCount) {
	
	Reverb * ptr = (Reverb *)Instance;

	unsigned long sample_index;
	unsigned int i;

	LADSPA_Data decay = LIMIT(*(ptr->decay),0.0f,10000.0f);
	LADSPA_Data drylevel = db2lin(LIMIT(*(ptr->drylevel),-70.0f,10.0f));
	LADSPA_Data wetlevel = db2lin(LIMIT(*(ptr->wetlevel),-70.0f,10.0f));
	LADSPA_Data combs_en = LIMIT(*(ptr->combs_en),-2.0f,2.0f);
	LADSPA_Data allps_en = LIMIT(*(ptr->allps_en),-2.0f,2.0f);
	LADSPA_Data bandpass_en = LIMIT(*(ptr->bandpass_en),-2.0f,2.0f);
	LADSPA_Data stereo_enh = LIMIT(*(ptr->stereo_enh),-2.0f,2.0f);
      	LADSPA_Data mode = LIMIT(*(ptr->mode),0,NUM_MODES-1);

	LADSPA_Data * input_L = ptr->input_L;
	LADSPA_Data * output_L = ptr->output_L;
	LADSPA_Data * input_R = ptr->input_R;
	LADSPA_Data * output_R = ptr->output_R;

	rev_t out_L = 0;
	rev_t out_R = 0;
	rev_t in_L = 0;
	rev_t in_R = 0;
	rev_t combs_out_L = 0;
	rev_t combs_out_R = 0;


	/* see if the user changed any control since last run */
	if ((ptr->old_decay != decay) ||
	    (ptr->old_stereo_enh != stereo_enh) ||
	    (ptr->old_mode != mode)) {

		/* re-compute reverberator coefficients */
		comp_coeffs(Instance);

		/* save new values */
		ptr->old_decay = decay;
		ptr->old_stereo_enh = stereo_enh;
		ptr->old_mode = mode;
	}

	for (sample_index = 0; sample_index < SampleCount; sample_index++) {

#ifdef REVERB_CALC_FLOAT		
		in_L = *(input_L++);
		in_R = *(input_R++);
#else
		in_L = (sample)((float)F2S * *(input_L++));
		in_R = (sample)((float)F2S * *(input_R++));
#endif

		combs_out_L = in_L;
		combs_out_R = in_R;

		/* process comb filters */
		if (combs_en > 0.0f) {
			for (i = 0; i < ptr->num_combs / 2; i++) {
				combs_out_L += 
					comb_run(in_L, ((COMB_FILTER *)(ptr->combs + 2*i)));
				combs_out_R += 
					comb_run(in_R, ((COMB_FILTER *)(ptr->combs + 2*i+1)));
			}
		}

		/* process allpass filters */
		if (allps_en > 0.0f) {
			for (i = 0; i < ptr->num_allps / 2; i++) {
				combs_out_L += allp_run(combs_out_L,
						       ((ALLP_FILTER *)(ptr->allps + 2*i)));
				combs_out_R += allp_run(combs_out_R,
						       ((ALLP_FILTER *)(ptr->allps + 2*i+1)));
			}
		}

		/* process bandpass filters */
		if (bandpass_en > 0.0f) {
			combs_out_L = 
				biquad_run(((biquad *)(ptr->low_pass)), combs_out_L);
			combs_out_L = 
				biquad_run(((biquad *)(ptr->high_pass)), combs_out_L);
			combs_out_R = 
				biquad_run(((biquad *)(ptr->low_pass + 1)), combs_out_R);
			combs_out_R = 
				biquad_run(((biquad *)(ptr->high_pass + 1)), combs_out_R);
		}

#ifdef REVERB_CALC_FLOAT
		out_L = in_L * drylevel + combs_out_L * wetlevel;
		out_R = in_R * drylevel + combs_out_R * wetlevel;
		*(output_L++) = out_L;
		*(output_R++) = out_R;
#else
		out_L = (sample)((float)in_L * drylevel + (float)combs_out_L * wetlevel);
		out_R = (sample)((float)in_R * drylevel + (float)combs_out_R * wetlevel);
		*(output_L++) = (float)out_L / (float)F2S;
		*(output_R++) = (float)out_R / (float)F2S;
#endif
	}
}





void
set_run_adding_gain(LADSPA_Handle Instance, LADSPA_Data gain){

	Reverb * ptr;

	ptr = (Reverb *)Instance;

	ptr->run_adding_gain = gain;
}


void 
run_adding_gain_Reverb(LADSPA_Handle Instance,
		       unsigned long SampleCount) {
	
	Reverb * ptr = (Reverb *)Instance;

	unsigned long sample_index;
	unsigned int i;

	LADSPA_Data decay = LIMIT(*(ptr->decay),0.0f,10000.0f);
	LADSPA_Data drylevel = db2lin(LIMIT(*(ptr->drylevel),-70.0f,10.0f));
	LADSPA_Data wetlevel = db2lin(LIMIT(*(ptr->wetlevel),-70.0f,10.0f));
	LADSPA_Data combs_en = LIMIT(*(ptr->combs_en),-2.0f,2.0f);
	LADSPA_Data allps_en = LIMIT(*(ptr->allps_en),-2.0f,2.0f);
	LADSPA_Data bandpass_en = LIMIT(*(ptr->bandpass_en),-2.0f,2.0f);
	LADSPA_Data stereo_enh = LIMIT(*(ptr->stereo_enh),-2.0f,2.0f);
      	LADSPA_Data mode = LIMIT(*(ptr->mode),0,NUM_MODES-1);

	LADSPA_Data * input_L = ptr->input_L;
	LADSPA_Data * output_L = ptr->output_L;
	LADSPA_Data * input_R = ptr->input_R;
	LADSPA_Data * output_R = ptr->output_R;

	rev_t out_L = 0;
	rev_t out_R = 0;
	rev_t in_L = 0;
	rev_t in_R = 0;
	rev_t combs_out_L = 0;
	rev_t combs_out_R = 0;


	/* see if the user changed any control since last run */
	if ((ptr->old_decay != decay) ||
	    (ptr->old_stereo_enh != stereo_enh) ||
	    (ptr->old_mode != mode)) {

		/* re-compute reverberator coefficients */
		comp_coeffs(Instance);

		/* save new values */
		ptr->old_decay = decay;
		ptr->old_stereo_enh = stereo_enh;
		ptr->old_mode = mode;
	}

	for (sample_index = 0; sample_index < SampleCount; sample_index++) {
		
#ifdef REVERB_CALC_FLOAT		
		in_L = *(input_L++);
		in_R = *(input_R++);
#else
		in_L = (sample)((float)F2S * *(input_L++));
		in_R = (sample)((float)F2S * *(input_R++));
#endif

		combs_out_L = in_L;
		combs_out_R = in_R;

		/* process comb filters */
		if (combs_en > 0.0f) {
			for (i = 0; i < ptr->num_combs / 2; i++) {
				combs_out_L += 
					comb_run(in_L, ((COMB_FILTER *)(ptr->combs + 2*i)));
				combs_out_R += 
					comb_run(in_R, ((COMB_FILTER *)(ptr->combs + 2*i+1)));
			}
		}

		/* process allpass filters */
		if (allps_en > 0.0f) {
			for (i = 0; i < ptr->num_allps / 2; i++) {
				combs_out_L += allp_run(combs_out_L,
						       ((ALLP_FILTER *)(ptr->allps + 2*i)));
				combs_out_R += allp_run(combs_out_R,
						       ((ALLP_FILTER *)(ptr->allps + 2*i+1)));
			}
		}

		/* process bandpass filters */
		if (bandpass_en > 0.0f) {
			combs_out_L = 
				biquad_run(((biquad *)(ptr->low_pass)), combs_out_L);
			combs_out_L = 
				biquad_run(((biquad *)(ptr->high_pass)), combs_out_L);
			combs_out_R = 
				biquad_run(((biquad *)(ptr->low_pass + 1)), combs_out_R);
			combs_out_R = 
				biquad_run(((biquad *)(ptr->high_pass + 1)), combs_out_R);
		}

#ifdef REVERB_CALC_FLOAT		
		out_L = in_L * drylevel + combs_out_L * wetlevel;
		out_R = in_R * drylevel + combs_out_R * wetlevel;
		*(output_L++) += out_L * ptr->run_adding_gain;
		*(output_R++) += out_R * ptr->run_adding_gain;
#else
		out_L = (sample)((float)in_L * drylevel + (float)combs_out_L * wetlevel);
		out_R = (sample)((float)in_R * drylevel + (float)combs_out_R * wetlevel);
		*(output_L++) += (float)out_L * ptr->run_adding_gain / (float)F2S;
		*(output_R++) += (float)out_R * ptr->run_adding_gain / (float)F2S;
#endif
	}
}



/* Throw away a Reverb effect instance. */
void 
cleanup_Reverb(LADSPA_Handle Instance) {

	int i;
	Reverb * ptr = (Reverb *)Instance;

	/* free memory allocated for comb/allpass filters & co. in instantiate_Reverb() */
	for (i = 0; i < 2 * MAX_COMBS; i++) {
		free(((COMB_FILTER *)(ptr->combs + i))->ringbuffer);
		free(((COMB_FILTER *)(ptr->combs + i))->buffer_pos);
		free(((COMB_FILTER *)(ptr->combs + i))->filter);
	}
	for (i = 0; i < 2 * MAX_ALLPS; i++) {
		free(((ALLP_FILTER *)(ptr->allps + i))->ringbuffer);
		free(((ALLP_FILTER *)(ptr->allps + i))->buffer_pos);
	}

	free(ptr->combs);
	free(ptr->allps);
	free(ptr->low_pass);
	free(ptr->high_pass);

	free(Instance);

}



LADSPA_Descriptor * stereo_descriptor = NULL;



/* __attribute__((constructor)) tap_init() is called automatically when the plugin library is first
   loaded. */
void 
__attribute__((constructor)) tap_init() {
	
	char ** port_names;
	LADSPA_PortDescriptor * port_descriptors;
	LADSPA_PortRangeHint * port_range_hints;

	
	if ((stereo_descriptor =
	     (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor))) == NULL)
		exit(1);		
	

	/* init the stereo Reverb */
	
	stereo_descriptor->UniqueID = ID_STEREO;
	stereo_descriptor->Label = strdup("tap_reverb");
	stereo_descriptor->Properties = 0;
	stereo_descriptor->Name = strdup("TAP Reverberator");
	stereo_descriptor->Maker = strdup("Tom Szilagyi");
	stereo_descriptor->Copyright = strdup("GPL");
	stereo_descriptor->PortCount = PORTCOUNT_STEREO;

	if ((port_descriptors = 
	     (LADSPA_PortDescriptor *)calloc(PORTCOUNT_STEREO, sizeof(LADSPA_PortDescriptor))) == NULL)
		exit(1);		

	stereo_descriptor->PortDescriptors = (const LADSPA_PortDescriptor *)port_descriptors;
	port_descriptors[DECAY] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[DRYLEVEL] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[WETLEVEL] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[COMBS_EN] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[ALLPS_EN] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[BANDPASS_EN] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[STEREO_ENH] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[MODE] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;

	port_descriptors[INPUT_L] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT_L] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
	port_descriptors[INPUT_R] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT_R] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;

	if ((port_names = 
	     (char **)calloc(PORTCOUNT_STEREO, sizeof(char *))) == NULL)
		exit(1);		

	stereo_descriptor->PortNames = (const char **)port_names;

	port_names[DECAY] = strdup("Decay [ms]");
	port_names[DRYLEVEL] = strdup("Dry Level [dB]");
	port_names[WETLEVEL] = strdup("Wet Level [dB]");
	port_names[COMBS_EN] = strdup("Comb Filters");
	port_names[ALLPS_EN] = strdup("Allpass Filters");
	port_names[BANDPASS_EN] = strdup("Bandpass Filter");
	port_names[STEREO_ENH] = strdup("Enhanced Stereo");
	port_names[MODE] = strdup("Reverb Type");
	port_names[INPUT_L] = strdup("Input Left");
	port_names[OUTPUT_L] = strdup("Output Left");
	port_names[INPUT_R] = strdup("Input Right");
	port_names[OUTPUT_R] = strdup("Output Right");

	if ((port_range_hints = 
	     ((LADSPA_PortRangeHint *)calloc(PORTCOUNT_STEREO, sizeof(LADSPA_PortRangeHint)))) == NULL)
		exit(1);		

	stereo_descriptor->PortRangeHints = (const LADSPA_PortRangeHint *)port_range_hints;

	port_range_hints[DECAY].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_LOW);
	port_range_hints[DECAY].LowerBound = 0;
	port_range_hints[DECAY].UpperBound = MAX_DECAY;

	port_range_hints[DRYLEVEL].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[DRYLEVEL].LowerBound = -70.0f;
	port_range_hints[DRYLEVEL].UpperBound = +10.0f;

	port_range_hints[WETLEVEL].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[WETLEVEL].LowerBound = -70.0f;
	port_range_hints[WETLEVEL].UpperBound = +10.0f;

	port_range_hints[COMBS_EN].HintDescriptor = 
		(LADSPA_HINT_TOGGLED |
		 LADSPA_HINT_DEFAULT_1);

	port_range_hints[ALLPS_EN].HintDescriptor = 
		(LADSPA_HINT_TOGGLED |
		 LADSPA_HINT_DEFAULT_1);

	port_range_hints[BANDPASS_EN].HintDescriptor = 
		(LADSPA_HINT_TOGGLED |
		 LADSPA_HINT_DEFAULT_1);

	port_range_hints[STEREO_ENH].HintDescriptor = 
		(LADSPA_HINT_TOGGLED |
		 LADSPA_HINT_DEFAULT_1);

	port_range_hints[MODE].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_INTEGER |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[MODE].LowerBound = 0;
	port_range_hints[MODE].UpperBound = NUM_MODES - 0.9f;

	port_range_hints[INPUT_L].HintDescriptor = 0;
	port_range_hints[OUTPUT_L].HintDescriptor = 0;
	port_range_hints[INPUT_R].HintDescriptor = 0;
	port_range_hints[OUTPUT_R].HintDescriptor = 0;


	stereo_descriptor->instantiate = instantiate_Reverb;
	stereo_descriptor->connect_port = connect_port_Reverb;
	stereo_descriptor->activate = activate_Reverb;
	stereo_descriptor->run = run_Reverb;
	stereo_descriptor->run_adding = run_adding_gain_Reverb;
	stereo_descriptor->set_run_adding_gain = set_run_adding_gain;
	stereo_descriptor->deactivate = NULL;
	stereo_descriptor->cleanup = cleanup_Reverb;
	
}


void
delete_descriptor(LADSPA_Descriptor * descriptor) {
	unsigned long index;
	if (descriptor) {
		free((char *)descriptor->Label);
		free((char *)descriptor->Name);
		free((char *)descriptor->Maker);
		free((char *)descriptor->Copyright);
		free((LADSPA_PortDescriptor *)descriptor->PortDescriptors);
		for (index = 0; index < descriptor->PortCount; index++)
			free((char *)(descriptor->PortNames[index]));
		free((char **)descriptor->PortNames);
		free((LADSPA_PortRangeHint *)descriptor->PortRangeHints);
		free(descriptor);
	}
}


/* __attribute__((destructor)) tap_fini() is called automatically when the library is unloaded. */
void
__attribute__((destructor)) tap_fini() {
	delete_descriptor(stereo_descriptor);
}


/* Return a descriptor of the requested plugin type. */
const LADSPA_Descriptor * 
ladspa_descriptor(unsigned long Index) {

	switch (Index) {
	case 0:
		return stereo_descriptor;
	default:
		return NULL;
	}
}
