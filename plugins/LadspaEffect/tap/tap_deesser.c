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

    $Id: tap_deesser.c,v 1.7 2004/05/01 16:15:06 tszilagyi Exp $
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <ladspa.h>
#include "tap_utils.h"

/* The Unique ID of the plugin: */

#define ID_MONO         2147

/* The port numbers for the plugin: */

#define THRESHOLD       0
#define FREQ            1
#define SIDECHAIN       2
#define MONITOR         3
#define ATTENUAT        4
#define INPUT           5
#define OUTPUT          6


/* Total number of ports */

#define PORTCOUNT_MONO   7


/* Bandwidth of sidechain lowpass/highpass filters */
#define SIDECH_BW       0.3f

/* Used to hold 10 ms gain data, enough for sample rates up to 192 kHz */
#define RINGBUF_SIZE    2000



/* 4 digits precision from 1.000 to 9.999 */
LADSPA_Data log10_table[9000];


/* The structure used to hold port connection information and state */

typedef struct {
	LADSPA_Data * threshold;
	LADSPA_Data * audiomode;
	LADSPA_Data * freq;
	LADSPA_Data * sidechain;
	LADSPA_Data * monitor;
	LADSPA_Data * attenuat;
	LADSPA_Data * input;
	LADSPA_Data * output;

	biquad sidech_lo_filter;
	biquad sidech_hi_filter;
	LADSPA_Data * ringbuffer;
	unsigned long buflen;
	unsigned long pos;
	LADSPA_Data sum;
	LADSPA_Data old_freq;

	unsigned long sample_rate;
	LADSPA_Data run_adding_gain;
} DeEsser;


/* fast linear to decibel conversion using log10_table[] */
LADSPA_Data fast_lin2db(LADSPA_Data lin) {

        unsigned long k;
        int exp = 0;
        LADSPA_Data mant = ABS(lin);

	/* sanity checks */
	if (mant == 0.0f)
		return(-1.0f/0.0f); /* -inf */
	if (mant == 1.0f/0.0f) /* +inf */
		return(mant);

        while (mant < 1.0f) {
                mant *= 10;
                exp --;
        }
        while (mant >= 10.0f) {
                mant /= 10;
                exp ++;
        }

        k = (mant - 0.999999f) * 1000.0f;
        return 20.0f * (log10_table[k] + exp);
}



/* Construct a new plugin instance. */
LADSPA_Handle 
instantiate_DeEsser(const LADSPA_Descriptor * Descriptor,
		    unsigned long             SampleRate) {
	
	LADSPA_Handle * ptr;
	
	if ((ptr = malloc(sizeof(DeEsser))) != NULL) {
		((DeEsser *)ptr)->sample_rate = SampleRate;
		((DeEsser *)ptr)->run_adding_gain = 1.0f;

		/* init filters */
		biquad_init(&((DeEsser *)ptr)->sidech_lo_filter);
		biquad_init(&((DeEsser *)ptr)->sidech_hi_filter);
		
		/* alloc mem for ringbuffer */
		if ((((DeEsser *)ptr)->ringbuffer = 
		     calloc(RINGBUF_SIZE, sizeof(LADSPA_Data))) == NULL)
			return NULL;

                /* 10 ms attenuation data is stored */
		((DeEsser *)ptr)->buflen = ((DeEsser *)ptr)->sample_rate / 100; 

		((DeEsser *)ptr)->pos = 0;
		((DeEsser *)ptr)->sum = 0.0f;
		((DeEsser *)ptr)->old_freq = 0;

		return ptr;
	}
	return NULL;
}


void
activate_DeEsser(LADSPA_Handle Instance) {

	DeEsser * ptr = (DeEsser *)Instance;
	unsigned long i;

	for (i = 0; i < RINGBUF_SIZE; i++)
		ptr->ringbuffer[i] = 0.0f;
}




/* Connect a port to a data location. */
void 
connect_port_DeEsser(LADSPA_Handle Instance,
		     unsigned long Port,
		     LADSPA_Data * DataLocation) {
	
	DeEsser * ptr;
	
	ptr = (DeEsser *)Instance;
	switch (Port) {
	case THRESHOLD:
		ptr->threshold = DataLocation;
		break;
	case FREQ:
		ptr->freq = DataLocation;
		break;
	case SIDECHAIN:
		ptr->sidechain = DataLocation;
		break;
	case MONITOR:
		ptr->monitor = DataLocation;
		break;
	case ATTENUAT:
		ptr->attenuat = DataLocation;
		*(ptr->attenuat) = 0.0f;
		break;
	case INPUT:
		ptr->input = DataLocation;
		break;
	case OUTPUT:
		ptr->output = DataLocation;
		break;
	}
}



void 
run_DeEsser(LADSPA_Handle Instance,
	    unsigned long SampleCount) {
  
	DeEsser * ptr = (DeEsser *)Instance;

	LADSPA_Data * input = ptr->input;
	LADSPA_Data * output = ptr->output;
	LADSPA_Data threshold = LIMIT(*(ptr->threshold),-50.0f,10.0f);
	LADSPA_Data freq = LIMIT(*(ptr->freq),2000.0f,16000.0f);
	LADSPA_Data sidechain = LIMIT(*(ptr->sidechain),0.0f,1.0f);
	LADSPA_Data monitor = LIMIT(*(ptr->monitor),0.0f,1.0f);
	unsigned long sample_index;

	LADSPA_Data in = 0;
	LADSPA_Data out = 0;
	LADSPA_Data sidech = 0;
	LADSPA_Data ampl_db = 0.0f;
	LADSPA_Data attn = 0.0f;
	LADSPA_Data max_attn = 0.0f;


	if (ptr->old_freq != freq) {
		lp_set_params(&ptr->sidech_lo_filter, freq, SIDECH_BW, ptr->sample_rate);
		hp_set_params(&ptr->sidech_hi_filter, freq, SIDECH_BW, ptr->sample_rate);
		ptr->old_freq = freq;
	}

	for (sample_index = 0; sample_index < SampleCount; sample_index++) {

		in = *(input++);

		/* process sidechain filters */
		sidech = biquad_run(&ptr->sidech_hi_filter, in);
		if (sidechain > 0.1f)
			sidech = biquad_run(&ptr->sidech_lo_filter, sidech);

		ampl_db = fast_lin2db(sidech);
		if (ampl_db <= threshold)
			attn = 0.0f;
		else
			attn = -0.5f * (ampl_db - threshold);

		ptr->sum += attn;
		ptr->sum -= push_buffer(attn, ptr->ringbuffer, ptr->buflen, &ptr->pos);

		if (-1.0f * ptr->sum > max_attn)
			max_attn = -0.01f * ptr->sum;

		in *= db2lin(ptr->sum / 100.0f);


		/* output selector */
		if (monitor > 0.1f)
			out = sidech;
		else
			out = in;

		*(output++) = out;
		*(ptr->attenuat) = LIMIT(max_attn,0,10);
	}
}


void
set_run_adding_gain_DeEsser(LADSPA_Handle Instance, LADSPA_Data gain) {

	DeEsser * ptr = (DeEsser *)Instance;

	ptr->run_adding_gain = gain;
}



void 
run_adding_DeEsser(LADSPA_Handle Instance,
		   unsigned long SampleCount) {
  
	DeEsser * ptr = (DeEsser *)Instance;

	LADSPA_Data * input = ptr->input;
	LADSPA_Data * output = ptr->output;
	LADSPA_Data threshold = LIMIT(*(ptr->threshold),-50.0f,10.0f);
	LADSPA_Data freq = LIMIT(*(ptr->freq),2000.0f,16000.0f);
	LADSPA_Data sidechain = LIMIT(*(ptr->sidechain),0.0f,1.0f);
	LADSPA_Data monitor = LIMIT(*(ptr->monitor),0.0f,1.0f);
	unsigned long sample_index;

	LADSPA_Data in = 0;
	LADSPA_Data out = 0;
	LADSPA_Data sidech = 0;
	LADSPA_Data ampl_db = 0.0f;
	LADSPA_Data attn = 0.0f;
	LADSPA_Data max_attn = 0.0f;


	if (ptr->old_freq != freq) {
		lp_set_params(&ptr->sidech_lo_filter, freq, SIDECH_BW, ptr->sample_rate);
		hp_set_params(&ptr->sidech_hi_filter, freq, SIDECH_BW, ptr->sample_rate);
		ptr->old_freq = freq;
	}

	for (sample_index = 0; sample_index < SampleCount; sample_index++) {

		in = *(input++);

		/* process sidechain filters */
		sidech = biquad_run(&ptr->sidech_hi_filter, in);
		if (sidechain > 0.1f)
			sidech = biquad_run(&ptr->sidech_lo_filter, sidech);

		ampl_db = 20.0f * log10f(sidech);
		if (ampl_db <= threshold)
			attn = 0.0f;
		else
			attn = -0.5f * (ampl_db - threshold);

		ptr->sum += attn;
		ptr->sum -= push_buffer(attn, ptr->ringbuffer, ptr->buflen, &ptr->pos);

		if (-1.0f * ptr->sum > max_attn)
			max_attn = -0.01f * ptr->sum;

		in *= db2lin(ptr->sum / 100.0f);


		/* output selector */
		if (monitor > 0.1f)
			out = sidech;
		else
			out = in;

		*(output++) += ptr->run_adding_gain * out;
		*(ptr->attenuat) = LIMIT(max_attn,0,10);
	}
}



/* Throw away a DeEsser effect instance. */
void 
cleanup_DeEsser(LADSPA_Handle Instance) {

	DeEsser * ptr = (DeEsser *)Instance;
	free(ptr->ringbuffer);
	free(Instance);
}



LADSPA_Descriptor * mono_descriptor = NULL;



/* __attribute__((constructor)) tap_init() is called automatically when the plugin library is first
   loaded. */
void 
__attribute__((constructor)) tap_init() {
	
	int i;
	char ** port_names;
	LADSPA_PortDescriptor * port_descriptors;
	LADSPA_PortRangeHint * port_range_hints;
	
	if ((mono_descriptor = 
	     (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor))) == NULL)
		exit(1);
	

	/* compute the log10 table */
        for (i = 0; i < 9000; i++)
                log10_table[i] = log10f(1.0f + i / 1000.0f);


	mono_descriptor->UniqueID = ID_MONO;
	mono_descriptor->Label = strdup("tap_deesser");
	mono_descriptor->Properties = 0;
	mono_descriptor->Name = strdup("TAP DeEsser");
	mono_descriptor->Maker = strdup("Tom Szilagyi");
	mono_descriptor->Copyright = strdup("GPL");
	mono_descriptor->PortCount = PORTCOUNT_MONO;

	if ((port_descriptors =
	     (LADSPA_PortDescriptor *)calloc(PORTCOUNT_MONO, sizeof(LADSPA_PortDescriptor))) == NULL)
		exit(1);

	mono_descriptor->PortDescriptors = (const LADSPA_PortDescriptor *)port_descriptors;
	port_descriptors[THRESHOLD] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[FREQ] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[SIDECHAIN] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[MONITOR] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[ATTENUAT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
	port_descriptors[INPUT] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;

	if ((port_names = 
	     (char **)calloc(PORTCOUNT_MONO, sizeof(char *))) == NULL)
		exit(1);

	mono_descriptor->PortNames = (const char **)port_names;
	port_names[THRESHOLD] = strdup("Threshold Level [dB]");
	port_names[FREQ] = strdup("Frequency [Hz]");
	port_names[SIDECHAIN] = strdup("Sidechain Filter");
	port_names[MONITOR] = strdup("Monitor");
	port_names[ATTENUAT] = strdup("Attenuation [dB]");
	port_names[INPUT] = strdup("Input");
	port_names[OUTPUT] = strdup("Output");

	if ((port_range_hints = 
	     ((LADSPA_PortRangeHint *)calloc(PORTCOUNT_MONO, sizeof(LADSPA_PortRangeHint)))) == NULL)
		exit(1);

	mono_descriptor->PortRangeHints	= (const LADSPA_PortRangeHint *)port_range_hints;
	port_range_hints[THRESHOLD].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[FREQ].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_LOW);
	port_range_hints[SIDECHAIN].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_INTEGER |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[MONITOR].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_INTEGER |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[ATTENUAT].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[THRESHOLD].LowerBound = -50;
	port_range_hints[THRESHOLD].UpperBound = 10;
	port_range_hints[FREQ].LowerBound = 2000;
	port_range_hints[FREQ].UpperBound = 16000;
	port_range_hints[SIDECHAIN].LowerBound = 0.0f;
	port_range_hints[SIDECHAIN].UpperBound = 1.01f;
	port_range_hints[MONITOR].LowerBound = 0.0f;
	port_range_hints[MONITOR].UpperBound = 1.01f;
	port_range_hints[ATTENUAT].LowerBound = 0.0f;
	port_range_hints[ATTENUAT].UpperBound = 10.0f;
	port_range_hints[INPUT].HintDescriptor = 0;
	port_range_hints[OUTPUT].HintDescriptor = 0;
	mono_descriptor->instantiate = instantiate_DeEsser;
	mono_descriptor->connect_port = connect_port_DeEsser;
	mono_descriptor->activate = activate_DeEsser;
	mono_descriptor->run = run_DeEsser;
	mono_descriptor->run_adding = run_adding_DeEsser;
	mono_descriptor->set_run_adding_gain = set_run_adding_gain_DeEsser;
	mono_descriptor->deactivate = NULL;
	mono_descriptor->cleanup = cleanup_DeEsser;
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
	delete_descriptor(mono_descriptor);
}


/* Return a descriptor of the requested plugin type. */
const LADSPA_Descriptor * 
ladspa_descriptor(unsigned long Index) {

	switch (Index) {
	case 0:
		return mono_descriptor;
	default:
		return NULL;
	}
}
