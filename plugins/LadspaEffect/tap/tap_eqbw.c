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
    
    $Id: tap_eqbw.c,v 1.5 2006/08/09 12:03:24 tszilagyi Exp $
*/


/* This plugin is identical to TAP Equalizer (2141), but it has
 * separate user controls for setting the bandwidth of every filter.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <ladspa.h>
#include "tap_utils.h"

/* The Unique ID of the plugin */
#define ID_MONO        2151


/* Default bandwidth of EQ filters in octaves */
#define BWIDTH        1.0f


/* Port numbers */

#define EQ_CH0G                     0
#define EQ_CH1G                     1
#define EQ_CH2G                     2
#define EQ_CH3G                     3
#define EQ_CH4G                     4
#define EQ_CH5G                     5
#define EQ_CH6G                     6
#define EQ_CH7G                     7

#define EQ_CH0F                     8
#define EQ_CH1F                     9
#define EQ_CH2F                     10
#define EQ_CH3F                     11
#define EQ_CH4F                     12
#define EQ_CH5F                     13
#define EQ_CH6F                     14
#define EQ_CH7F                     15

#define EQ_CH0B                     16
#define EQ_CH1B                     17
#define EQ_CH2B                     18
#define EQ_CH3B                     19
#define EQ_CH4B                     20
#define EQ_CH5B                     21
#define EQ_CH6B                     22
#define EQ_CH7B                     23

#define EQ_INPUT                    24
#define EQ_OUTPUT                   25


/* Total number of ports */
#define PORTCOUNT_MONO  26


static LADSPA_Descriptor *eqDescriptor = NULL;

typedef struct {
	LADSPA_Data *ch0f;
	LADSPA_Data *ch0g;
	LADSPA_Data *ch0b;
	LADSPA_Data *ch1f;
	LADSPA_Data *ch1g;
	LADSPA_Data *ch1b;
	LADSPA_Data *ch2f;
	LADSPA_Data *ch2g;
	LADSPA_Data *ch2b;
	LADSPA_Data *ch3f;
	LADSPA_Data *ch3g;
	LADSPA_Data *ch3b;
	LADSPA_Data *ch4f;
	LADSPA_Data *ch4g;
	LADSPA_Data *ch4b;
	LADSPA_Data *ch5f;
	LADSPA_Data *ch5g;
	LADSPA_Data *ch5b;
	LADSPA_Data *ch6f;
	LADSPA_Data *ch6g;
	LADSPA_Data *ch6b;
	LADSPA_Data *ch7f;
	LADSPA_Data *ch7g;
	LADSPA_Data *ch7b;
	LADSPA_Data *input;
	LADSPA_Data *output;
	biquad *     filters;
	float        fs;
	LADSPA_Data old_ch0f;
	LADSPA_Data old_ch0g;
	LADSPA_Data old_ch0b;
	LADSPA_Data old_ch1f;
	LADSPA_Data old_ch1g;
	LADSPA_Data old_ch1b;
	LADSPA_Data old_ch2f;
	LADSPA_Data old_ch2g;
	LADSPA_Data old_ch2b;
	LADSPA_Data old_ch3f;
	LADSPA_Data old_ch3g;
	LADSPA_Data old_ch3b;
	LADSPA_Data old_ch4f;
	LADSPA_Data old_ch4g;
	LADSPA_Data old_ch4b;
	LADSPA_Data old_ch5f;
	LADSPA_Data old_ch5g;
	LADSPA_Data old_ch5b;
	LADSPA_Data old_ch6f;
	LADSPA_Data old_ch6g;
	LADSPA_Data old_ch6b;
	LADSPA_Data old_ch7f;
	LADSPA_Data old_ch7g;
	LADSPA_Data old_ch7b;

	LADSPA_Data run_adding_gain;
} eq;

const
LADSPA_Descriptor *
ladspa_descriptor(unsigned long index) {
	
	switch (index) {
	case 0:
		return eqDescriptor;
	default:
	        return NULL;
	}
}

static
void
activate_eq(LADSPA_Handle instance) {

	eq *ptr = (eq *)instance;
	biquad *filters = ptr->filters;

	biquad_init(&filters[0]);
	biquad_init(&filters[1]);
	biquad_init(&filters[2]);
	biquad_init(&filters[3]);
	biquad_init(&filters[4]);
	biquad_init(&filters[5]);
	biquad_init(&filters[6]);
	biquad_init(&filters[7]);
}


static
void
cleanup_eq(LADSPA_Handle instance) {

	free(instance);
}


static 
void 
connectPort_eq(LADSPA_Handle instance, unsigned long port, LADSPA_Data *data) {

	eq *plugin;
	
	plugin = (eq *)instance;
	switch (port) {
	case EQ_CH0F:
		plugin->ch0f = data;
		break;
	case EQ_CH0G:
		plugin->ch0g = data;
		break;
	case EQ_CH0B:
		plugin->ch0b = data;
		break;
	case EQ_CH1F:
		plugin->ch1f = data;
		break;
	case EQ_CH1G:
		plugin->ch1g = data;
		break;
	case EQ_CH1B:
		plugin->ch1b = data;
		break;
	case EQ_CH2F:
		plugin->ch2f = data;
		break;
	case EQ_CH2G:
		plugin->ch2g = data;
		break;
	case EQ_CH2B:
		plugin->ch2b = data;
		break;
	case EQ_CH3F:
		plugin->ch3f = data;
		break;
	case EQ_CH3G:
		plugin->ch3g = data;
		break;
	case EQ_CH3B:
		plugin->ch3b = data;
		break;
	case EQ_CH4F:
		plugin->ch4f = data;
		break;
	case EQ_CH4G:
		plugin->ch4g = data;
		break;
	case EQ_CH4B:
		plugin->ch4b = data;
		break;
	case EQ_CH5F:
		plugin->ch5f = data;
		break;
	case EQ_CH5G:
		plugin->ch5g = data;
		break;
	case EQ_CH5B:
		plugin->ch5b = data;
		break;
	case EQ_CH6F:
		plugin->ch6f = data;
		break;
	case EQ_CH6G:
		plugin->ch6g = data;
		break;
	case EQ_CH6B:
		plugin->ch6b = data;
		break;
	case EQ_CH7F:
		plugin->ch7f = data;
		break;
	case EQ_CH7G:
		plugin->ch7g = data;
		break;
	case EQ_CH7B:
		plugin->ch7b = data;
		break;
	case EQ_INPUT:
		plugin->input = data;
		break;
	case EQ_OUTPUT:
		plugin->output = data;
		break;
	}
}

static 
LADSPA_Handle 
instantiate_eq(const LADSPA_Descriptor *descriptor, unsigned long s_rate) {

	eq *ptr = (eq *)malloc(sizeof(eq));
	biquad *filters = NULL;
	float fs;

	fs = s_rate;
	
	memset(ptr, 0, sizeof(eq));

	filters = calloc(8, sizeof(biquad));

	ptr->filters = filters;
	ptr->fs = fs;
	ptr->run_adding_gain = 1.0f;

	ptr->old_ch0f = 100.0f;
	ptr->old_ch0g = 0.0f;
	ptr->old_ch0b = BWIDTH;

	ptr->old_ch1f = 200.0f;
	ptr->old_ch1g = 0.0f;
	ptr->old_ch1b = BWIDTH;

	ptr->old_ch2f = 400.0f;
	ptr->old_ch2g = 0.0f;
	ptr->old_ch2b = BWIDTH;

	ptr->old_ch3f = 1000.0f;
	ptr->old_ch3g = 0.0f;
	ptr->old_ch3b = BWIDTH;

	ptr->old_ch4f = 3000.0f;
	ptr->old_ch4g = 0.0f;
	ptr->old_ch4b = BWIDTH;

	ptr->old_ch5f = 6000.0f;
	ptr->old_ch5g = 0.0f;
	ptr->old_ch5b = BWIDTH;

	ptr->old_ch6f = 12000.0f;
	ptr->old_ch6g = 0.0f;
	ptr->old_ch6b = BWIDTH;

	ptr->old_ch7f = 15000.0f;
	ptr->old_ch7g = 0.0f;
	ptr->old_ch7b = BWIDTH;

	eq_set_params(&filters[0], 100.0f, 0.0f, BWIDTH, fs);
	eq_set_params(&filters[1], 200.0f, 0.0f, BWIDTH, fs);
	eq_set_params(&filters[2], 400.0f, 0.0f, BWIDTH, fs);
	eq_set_params(&filters[3], 1000.0f, 0.0f, BWIDTH, fs);
	eq_set_params(&filters[4], 3000.0f, 0.0f, BWIDTH, fs);
	eq_set_params(&filters[5], 6000.0f, 0.0f, BWIDTH, fs);
	eq_set_params(&filters[6], 12000.0f, 0.0f, BWIDTH, fs);
	eq_set_params(&filters[7], 15000.0f, 0.0f, BWIDTH, fs);

	return (LADSPA_Handle)ptr;
}


static 
void 
run_eq(LADSPA_Handle instance, unsigned long sample_count) {

	eq * ptr = (eq *)instance;

	const LADSPA_Data ch0f = LIMIT(*(ptr->ch0f),40.0f,280.0f);
	const LADSPA_Data ch0g = LIMIT(*(ptr->ch0g),-50.0f,20.0f);
	const LADSPA_Data ch0b = LIMIT(*(ptr->ch0b),0.1f,5.0f);
	const LADSPA_Data ch1f = LIMIT(*(ptr->ch1f),100.0f,500.0f);
	const LADSPA_Data ch1g = LIMIT(*(ptr->ch1g),-50.0f,20.0f);
	const LADSPA_Data ch1b = LIMIT(*(ptr->ch1b),0.1f,5.0f);
	const LADSPA_Data ch2f = LIMIT(*(ptr->ch2f),200.0f,1000.0f);
	const LADSPA_Data ch2g = LIMIT(*(ptr->ch2g),-50.0f,20.0f);
	const LADSPA_Data ch2b = LIMIT(*(ptr->ch2b),0.1f,5.0f);
	const LADSPA_Data ch3f = LIMIT(*(ptr->ch3f),400.0f,2800.0f);
	const LADSPA_Data ch3g = LIMIT(*(ptr->ch3g),-50.0f,20.0f);
	const LADSPA_Data ch3b = LIMIT(*(ptr->ch3b),0.1f,5.0f);
	const LADSPA_Data ch4f = LIMIT(*(ptr->ch4f),1000.0f,5000.0f);
	const LADSPA_Data ch4g = LIMIT(*(ptr->ch4g),-50.0f,20.0f);
	const LADSPA_Data ch4b = LIMIT(*(ptr->ch4b),0.1f,5.0f);
	const LADSPA_Data ch5f = LIMIT(*(ptr->ch5f),3000.0f,9000.0f);
	const LADSPA_Data ch5g = LIMIT(*(ptr->ch5g),-50.0f,20.0f);
	const LADSPA_Data ch5b = LIMIT(*(ptr->ch5b),0.1f,5.0f);
	const LADSPA_Data ch6f = LIMIT(*(ptr->ch6f),6000.0f,18000.0f);
	const LADSPA_Data ch6g = LIMIT(*(ptr->ch6g),-50.0f,20.0f);
	const LADSPA_Data ch6b = LIMIT(*(ptr->ch6b),0.1f,5.0f);
	const LADSPA_Data ch7f = LIMIT(*(ptr->ch7f),10000.0f,20000.0f);
	const LADSPA_Data ch7g = LIMIT(*(ptr->ch7g),-50.0f,20.0f);
	const LADSPA_Data ch7b = LIMIT(*(ptr->ch7b),0.1f,5.0f);

	const LADSPA_Data * input = ptr->input;
	LADSPA_Data * output = ptr->output;

	biquad * filters = ptr->filters;
	float fs = ptr->fs;

	unsigned long pos;
	float samp;


	if ((ch0f != ptr->old_ch0f) ||
	    (ch0g != ptr->old_ch0g) ||
	    (ch0b != ptr->old_ch0b)) {
		ptr->old_ch0f = ch0f;
		ptr->old_ch0g = ch0g;
		ptr->old_ch0b = ch0b;
		eq_set_params(&filters[0], ch0f, ch0g, ch0b, fs);
	}
	if ((ch1f != ptr->old_ch1f) ||
	    (ch1g != ptr->old_ch1g) ||
	    (ch1b != ptr->old_ch1b)) {
		ptr->old_ch1f = ch1f;
		ptr->old_ch1g = ch1g;
		ptr->old_ch1b = ch1b;
		eq_set_params(&filters[1], ch1f, ch1g, ch1b, fs);
	}
	if ((ch2f != ptr->old_ch2f) ||
	    (ch2g != ptr->old_ch2g) ||
	    (ch2b != ptr->old_ch2b)) {
		ptr->old_ch2f = ch2f;
		ptr->old_ch2g = ch2g;
		ptr->old_ch2b = ch2b;
		eq_set_params(&filters[2], ch2f, ch2g, ch2b, fs);
	}
	if ((ch3f != ptr->old_ch3f) ||
	    (ch3g != ptr->old_ch3g) ||
	    (ch3b != ptr->old_ch3b)) {
		ptr->old_ch3f = ch3f;
		ptr->old_ch3g = ch3g;
		ptr->old_ch3b = ch3b;
		eq_set_params(&filters[3], ch3f, ch3g, ch3b, fs);
	}
	if ((ch4f != ptr->old_ch4f) ||
	    (ch4g != ptr->old_ch4g) ||
	    (ch4b != ptr->old_ch4b)) {
		ptr->old_ch4f = ch4f;
		ptr->old_ch4g = ch4g;
		ptr->old_ch4b = ch4b;
		eq_set_params(&filters[4], ch4f, ch4g, ch4b, fs);
	}
	if ((ch5f != ptr->old_ch5f) ||
	    (ch5g != ptr->old_ch5g) ||
	    (ch5b != ptr->old_ch5b)) {
		ptr->old_ch5f = ch5f;
		ptr->old_ch5g = ch5g;
		ptr->old_ch5b = ch5b;
		eq_set_params(&filters[5], ch5f, ch5g, ch5b, fs);
	}
	if ((ch6f != ptr->old_ch6f) ||
	    (ch6g != ptr->old_ch6g) ||
	    (ch6b != ptr->old_ch6b)) {
		ptr->old_ch6f = ch6f;
		ptr->old_ch6g = ch6g;
		ptr->old_ch6b = ch6b;
		eq_set_params(&filters[6], ch6f, ch6g, ch6b, fs);
	}
	if ((ch7f != ptr->old_ch7f) ||
	    (ch7g != ptr->old_ch7g) ||
	    (ch7b != ptr->old_ch7b)) {
		ptr->old_ch7f = ch7f;
		ptr->old_ch7g = ch7g;
		ptr->old_ch7b = ch7b;
		eq_set_params(&filters[7], ch7f, ch7g, ch7b, fs);
	}

	for (pos = 0; pos < sample_count; pos++) {
		samp = input[pos];
		if (ch0g != 0.0f)
			samp = biquad_run(&filters[0], samp);
		if (ch1g != 0.0f)
			samp = biquad_run(&filters[1], samp);
		if (ch2g != 0.0f)
			samp = biquad_run(&filters[2], samp);
		if (ch3g != 0.0f)
			samp = biquad_run(&filters[3], samp);
		if (ch4g != 0.0f)
			samp = biquad_run(&filters[4], samp);
		if (ch5g != 0.0f)
			samp = biquad_run(&filters[5], samp);
		if (ch6g != 0.0f)
			samp = biquad_run(&filters[6], samp);
		if (ch7g != 0.0f)
			samp = biquad_run(&filters[7], samp);
		output[pos] = samp;
	}
}



void
set_run_adding_gain(LADSPA_Handle instance, LADSPA_Data gain) {

	eq * ptr = (eq *)instance;

	ptr->run_adding_gain = gain;
}



static
void
run_adding_eq(LADSPA_Handle instance, unsigned long sample_count) {

	eq * ptr = (eq *)instance;

	const LADSPA_Data ch0f = LIMIT(*(ptr->ch0f),40.0f,280.0f);
	const LADSPA_Data ch0g = LIMIT(*(ptr->ch0g),-50.0f,20.0f);
	const LADSPA_Data ch0b = LIMIT(*(ptr->ch0b),0.1f,5.0f);
	const LADSPA_Data ch1f = LIMIT(*(ptr->ch1f),100.0f,500.0f);
	const LADSPA_Data ch1g = LIMIT(*(ptr->ch1g),-50.0f,20.0f);
	const LADSPA_Data ch1b = LIMIT(*(ptr->ch1b),0.1f,5.0f);
	const LADSPA_Data ch2f = LIMIT(*(ptr->ch2f),200.0f,1000.0f);
	const LADSPA_Data ch2g = LIMIT(*(ptr->ch2g),-50.0f,20.0f);
	const LADSPA_Data ch2b = LIMIT(*(ptr->ch2b),0.1f,5.0f);
	const LADSPA_Data ch3f = LIMIT(*(ptr->ch3f),400.0f,2800.0f);
	const LADSPA_Data ch3g = LIMIT(*(ptr->ch3g),-50.0f,20.0f);
	const LADSPA_Data ch3b = LIMIT(*(ptr->ch3b),0.1f,5.0f);
	const LADSPA_Data ch4f = LIMIT(*(ptr->ch4f),1000.0f,5000.0f);
	const LADSPA_Data ch4g = LIMIT(*(ptr->ch4g),-50.0f,20.0f);
	const LADSPA_Data ch4b = LIMIT(*(ptr->ch4b),0.1f,5.0f);
	const LADSPA_Data ch5f = LIMIT(*(ptr->ch5f),3000.0f,9000.0f);
	const LADSPA_Data ch5g = LIMIT(*(ptr->ch5g),-50.0f,20.0f);
	const LADSPA_Data ch5b = LIMIT(*(ptr->ch5b),0.1f,5.0f);
	const LADSPA_Data ch6f = LIMIT(*(ptr->ch6f),6000.0f,18000.0f);
	const LADSPA_Data ch6g = LIMIT(*(ptr->ch6g),-50.0f,20.0f);
	const LADSPA_Data ch6b = LIMIT(*(ptr->ch6b),0.1f,5.0f);
	const LADSPA_Data ch7f = LIMIT(*(ptr->ch7f),10000.0f,20000.0f);
	const LADSPA_Data ch7g = LIMIT(*(ptr->ch7g),-50.0f,20.0f);
	const LADSPA_Data ch7b = LIMIT(*(ptr->ch7b),0.1f,5.0f);

	const LADSPA_Data * input = ptr->input;
	LADSPA_Data * output = ptr->output;

	biquad * filters = ptr->filters;
	float fs = ptr->fs;

	unsigned long pos;
	float samp;


	if ((ch0f != ptr->old_ch0f) ||
	    (ch0g != ptr->old_ch0g) ||
	    (ch0b != ptr->old_ch0b)) {
		ptr->old_ch0f = ch0f;
		ptr->old_ch0g = ch0g;
		ptr->old_ch0b = ch0b;
		eq_set_params(&filters[0], ch0f, ch0g, ch0b, fs);
	}
	if ((ch1f != ptr->old_ch1f) ||
	    (ch1g != ptr->old_ch1g) ||
	    (ch1b != ptr->old_ch1b)) {
		ptr->old_ch1f = ch1f;
		ptr->old_ch1g = ch1g;
		ptr->old_ch1b = ch1b;
		eq_set_params(&filters[1], ch1f, ch1g, ch1b, fs);
	}
	if ((ch2f != ptr->old_ch2f) ||
	    (ch2g != ptr->old_ch2g) ||
	    (ch2b != ptr->old_ch2b)) {
		ptr->old_ch2f = ch2f;
		ptr->old_ch2g = ch2g;
		ptr->old_ch2b = ch2b;
		eq_set_params(&filters[2], ch2f, ch2g, ch2b, fs);
	}
	if ((ch3f != ptr->old_ch3f) ||
	    (ch3g != ptr->old_ch3g) ||
	    (ch3b != ptr->old_ch3b)) {
		ptr->old_ch3f = ch3f;
		ptr->old_ch3g = ch3g;
		ptr->old_ch3b = ch3b;
		eq_set_params(&filters[3], ch3f, ch3g, ch3b, fs);
	}
	if ((ch4f != ptr->old_ch4f) ||
	    (ch4g != ptr->old_ch4g) ||
	    (ch4b != ptr->old_ch4b)) {
		ptr->old_ch4f = ch4f;
		ptr->old_ch4g = ch4g;
		ptr->old_ch4b = ch4b;
		eq_set_params(&filters[4], ch4f, ch4g, ch4b, fs);
	}
	if ((ch5f != ptr->old_ch5f) ||
	    (ch5g != ptr->old_ch5g) ||
	    (ch5b != ptr->old_ch5b)) {
		ptr->old_ch5f = ch5f;
		ptr->old_ch5g = ch5g;
		ptr->old_ch5b = ch5b;
		eq_set_params(&filters[5], ch5f, ch5g, ch5b, fs);
	}
	if ((ch6f != ptr->old_ch6f) ||
	    (ch6g != ptr->old_ch6g) ||
	    (ch6b != ptr->old_ch6b)) {
		ptr->old_ch6f = ch6f;
		ptr->old_ch6g = ch6g;
		ptr->old_ch6b = ch6b;
		eq_set_params(&filters[6], ch6f, ch6g, ch6b, fs);
	}
	if ((ch7f != ptr->old_ch7f) ||
	    (ch7g != ptr->old_ch7g) ||
	    (ch7b != ptr->old_ch7b)) {
		ptr->old_ch7f = ch7f;
		ptr->old_ch7g = ch7g;
		ptr->old_ch7b = ch7b;
		eq_set_params(&filters[7], ch7f, ch7g, ch7b, fs);
	}

	for (pos = 0; pos < sample_count; pos++) {
		samp = input[pos];
		if (ch0g != 0.0f)
			samp = biquad_run(&filters[0], samp);
		if (ch1g != 0.0f)
			samp = biquad_run(&filters[1], samp);
		if (ch2g != 0.0f)
			samp = biquad_run(&filters[2], samp);
		if (ch3g != 0.0f)
			samp = biquad_run(&filters[3], samp);
		if (ch4g != 0.0f)
			samp = biquad_run(&filters[4], samp);
		if (ch5g != 0.0f)
			samp = biquad_run(&filters[5], samp);
		if (ch6g != 0.0f)
			samp = biquad_run(&filters[6], samp);
		if (ch7g != 0.0f)
			samp = biquad_run(&filters[7], samp);
		output[pos] += ptr->run_adding_gain * samp;
	}
}




void
__attribute__((constructor)) tap_init() {

	char **port_names;
	LADSPA_PortDescriptor *port_descriptors;
	LADSPA_PortRangeHint *port_range_hints;

	eqDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (eqDescriptor) {
		eqDescriptor->UniqueID = ID_MONO;
		eqDescriptor->Label = "tap_equalizer_bw";
		eqDescriptor->Properties = 0;
		eqDescriptor->Name = "TAP Equalizer/BW";
		eqDescriptor->Maker = "Tom Szilagyi";
		eqDescriptor->Copyright = "GPL";
		eqDescriptor->PortCount = PORTCOUNT_MONO;
		
		port_descriptors = 
			(LADSPA_PortDescriptor *)calloc(PORTCOUNT_MONO,
			 sizeof(LADSPA_PortDescriptor));
		eqDescriptor->PortDescriptors =
			(const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = 
			(LADSPA_PortRangeHint *)calloc(PORTCOUNT_MONO,
			 sizeof(LADSPA_PortRangeHint));
		eqDescriptor->PortRangeHints =
			(const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(PORTCOUNT_MONO, sizeof(char*));
		eqDescriptor->PortNames =
			(const char **)port_names;



		/* Parameters for CH0 freq [Hz] */
		port_descriptors[EQ_CH0F] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[EQ_CH0F] =
		 "Band 1 Freq [Hz]";
		port_range_hints[EQ_CH0F].HintDescriptor =
			LADSPA_HINT_BOUNDED_BELOW | 
			LADSPA_HINT_BOUNDED_ABOVE | 
			LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[EQ_CH0F].LowerBound = 40;
		port_range_hints[EQ_CH0F].UpperBound = 280;
		/* Parameters for CH0 gain [dB] */
		port_descriptors[EQ_CH0G] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[EQ_CH0G] =
		 "Band 1 Gain [dB]";
		port_range_hints[EQ_CH0G].HintDescriptor =
			LADSPA_HINT_BOUNDED_BELOW | 
			LADSPA_HINT_BOUNDED_ABOVE | 
			LADSPA_HINT_DEFAULT_0;
		port_range_hints[EQ_CH0G].LowerBound = -50;
		port_range_hints[EQ_CH0G].UpperBound = +20;
		/* Parameters for CH0 bandwidth [octaves] */
		port_descriptors[EQ_CH0B] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[EQ_CH0B] =
		 "Band 1 Bandwidth [octaves]";
		port_range_hints[EQ_CH0B].HintDescriptor =
			LADSPA_HINT_BOUNDED_BELOW | 
			LADSPA_HINT_BOUNDED_ABOVE | 
			LADSPA_HINT_DEFAULT_1;
		port_range_hints[EQ_CH0B].LowerBound = 0.1f;
		port_range_hints[EQ_CH0B].UpperBound = 5.0f;




		/* Parameters for CH1 freq [Hz] */
		port_descriptors[EQ_CH1F] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[EQ_CH1F] =
		 "Band 2 Freq [Hz]";
		port_range_hints[EQ_CH1F].HintDescriptor =
			LADSPA_HINT_BOUNDED_BELOW | 
			LADSPA_HINT_BOUNDED_ABOVE | 
			LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[EQ_CH1F].LowerBound = 100;
		port_range_hints[EQ_CH1F].UpperBound = 500;
		/* Parameters for CH1 gain [dB] */
		port_descriptors[EQ_CH1G] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[EQ_CH1G] =
		 "Band 2 Gain [dB]";
		port_range_hints[EQ_CH1G].HintDescriptor =
			LADSPA_HINT_BOUNDED_BELOW | 
			LADSPA_HINT_BOUNDED_ABOVE | 
			LADSPA_HINT_DEFAULT_0;
		port_range_hints[EQ_CH1G].LowerBound = -50;
		port_range_hints[EQ_CH1G].UpperBound = +20;
		/* Parameters for CH1 bandwidth [octaves] */
		port_descriptors[EQ_CH1B] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[EQ_CH1B] =
		 "Band 2 Bandwidth [octaves]";
		port_range_hints[EQ_CH1B].HintDescriptor =
			LADSPA_HINT_BOUNDED_BELOW | 
			LADSPA_HINT_BOUNDED_ABOVE | 
			LADSPA_HINT_DEFAULT_1;
		port_range_hints[EQ_CH1B].LowerBound = 0.1f;
		port_range_hints[EQ_CH1B].UpperBound = 5.0f;




		/* Parameters for CH2 freq [Hz] */
		port_descriptors[EQ_CH2F] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[EQ_CH2F] =
		 "Band 3 Freq [Hz]";
		port_range_hints[EQ_CH2F].HintDescriptor =
			LADSPA_HINT_BOUNDED_BELOW | 
			LADSPA_HINT_BOUNDED_ABOVE | 
			LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[EQ_CH2F].LowerBound = 200;
		port_range_hints[EQ_CH2F].UpperBound = 1000;
		/* Parameters for CH2 gain [dB] */
		port_descriptors[EQ_CH2G] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[EQ_CH2G] =
		 "Band 3 Gain [dB]";
		port_range_hints[EQ_CH2G].HintDescriptor =
			LADSPA_HINT_BOUNDED_BELOW | 
			LADSPA_HINT_BOUNDED_ABOVE | 
			LADSPA_HINT_DEFAULT_0;
		port_range_hints[EQ_CH2G].LowerBound = -50;
		port_range_hints[EQ_CH2G].UpperBound = +20;
		/* Parameters for CH2 bandwidth [octaves] */
		port_descriptors[EQ_CH2B] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[EQ_CH2B] =
		 "Band 3 Bandwidth [octaves]";
		port_range_hints[EQ_CH2B].HintDescriptor =
			LADSPA_HINT_BOUNDED_BELOW | 
			LADSPA_HINT_BOUNDED_ABOVE | 
			LADSPA_HINT_DEFAULT_1;
		port_range_hints[EQ_CH2B].LowerBound = 0.1f;
		port_range_hints[EQ_CH2B].UpperBound = 5.0f;




		/* Parameters for CH3 freq [Hz] */
		port_descriptors[EQ_CH3F] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[EQ_CH3F] =
		 "Band 4 Freq [Hz]";
		port_range_hints[EQ_CH3F].HintDescriptor =
			LADSPA_HINT_BOUNDED_BELOW | 
			LADSPA_HINT_BOUNDED_ABOVE | 
			LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[EQ_CH3F].LowerBound = 400;
		port_range_hints[EQ_CH3F].UpperBound = 2800;
		/* Parameters for CH3 gain [dB] */
		port_descriptors[EQ_CH3G] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[EQ_CH3G] =
		 "Band 4 Gain [dB]";
		port_range_hints[EQ_CH3G].HintDescriptor =
			LADSPA_HINT_BOUNDED_BELOW | 
			LADSPA_HINT_BOUNDED_ABOVE | 
			LADSPA_HINT_DEFAULT_0;
		port_range_hints[EQ_CH3G].LowerBound = -50;
		port_range_hints[EQ_CH3G].UpperBound = +20;
		/* Parameters for CH3 bandwidth [octaves] */
		port_descriptors[EQ_CH3B] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[EQ_CH3B] =
		 "Band 4 Bandwidth [octaves]";
		port_range_hints[EQ_CH3B].HintDescriptor =
			LADSPA_HINT_BOUNDED_BELOW | 
			LADSPA_HINT_BOUNDED_ABOVE | 
			LADSPA_HINT_DEFAULT_1;
		port_range_hints[EQ_CH3B].LowerBound = 0.1f;
		port_range_hints[EQ_CH3B].UpperBound = 5.0f;




		/* Parameters for CH4 freq [Hz] */
		port_descriptors[EQ_CH4F] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[EQ_CH4F] =
		 "Band 5 Freq [Hz]";
		port_range_hints[EQ_CH4F].HintDescriptor =
			LADSPA_HINT_BOUNDED_BELOW | 
			LADSPA_HINT_BOUNDED_ABOVE | 
			LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[EQ_CH4F].LowerBound = 1000;
		port_range_hints[EQ_CH4F].UpperBound = 5000;
		/* Parameters for CH4 gain [dB] */
		port_descriptors[EQ_CH4G] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[EQ_CH4G] =
		 "Band 5 Gain [dB]";
		port_range_hints[EQ_CH4G].HintDescriptor =
			LADSPA_HINT_BOUNDED_BELOW | 
			LADSPA_HINT_BOUNDED_ABOVE | 
			LADSPA_HINT_DEFAULT_0;
		port_range_hints[EQ_CH4G].LowerBound = -50;
		port_range_hints[EQ_CH4G].UpperBound = +20;
		/* Parameters for CH4 bandwidth [octaves] */
		port_descriptors[EQ_CH4B] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[EQ_CH4B] =
		 "Band 5 Bandwidth [octaves]";
		port_range_hints[EQ_CH4B].HintDescriptor =
			LADSPA_HINT_BOUNDED_BELOW | 
			LADSPA_HINT_BOUNDED_ABOVE | 
			LADSPA_HINT_DEFAULT_1;
		port_range_hints[EQ_CH4B].LowerBound = 0.1f;
		port_range_hints[EQ_CH4B].UpperBound = 5.0f;




		/* Parameters for CH5 freq [Hz] */
		port_descriptors[EQ_CH5F] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[EQ_CH5F] =
		 "Band 6 Freq [Hz]";
		port_range_hints[EQ_CH5F].HintDescriptor =
			LADSPA_HINT_BOUNDED_BELOW | 
			LADSPA_HINT_BOUNDED_ABOVE | 
			LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[EQ_CH5F].LowerBound = 3000;
		port_range_hints[EQ_CH5F].UpperBound = 9000;
		/* Parameters for CH5 gain [dB] */
		port_descriptors[EQ_CH5G] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[EQ_CH5G] =
		 "Band 6 Gain [dB]";
		port_range_hints[EQ_CH5G].HintDescriptor =
			LADSPA_HINT_BOUNDED_BELOW | 
			LADSPA_HINT_BOUNDED_ABOVE | 
			LADSPA_HINT_DEFAULT_0;
		port_range_hints[EQ_CH5G].LowerBound = -50;
		port_range_hints[EQ_CH5G].UpperBound = +20;
		/* Parameters for CH5 bandwidth [octaves] */
		port_descriptors[EQ_CH5B] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[EQ_CH5B] =
		 "Band 6 Bandwidth [octaves]";
		port_range_hints[EQ_CH5B].HintDescriptor =
			LADSPA_HINT_BOUNDED_BELOW | 
			LADSPA_HINT_BOUNDED_ABOVE | 
			LADSPA_HINT_DEFAULT_1;
		port_range_hints[EQ_CH5B].LowerBound = 0.1f;
		port_range_hints[EQ_CH5B].UpperBound = 5.0f;




		/* Parameters for CH6 freq [Hz] */
		port_descriptors[EQ_CH6F] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[EQ_CH6F] =
		 "Band 7 Freq [Hz]";
		port_range_hints[EQ_CH6F].HintDescriptor =
			LADSPA_HINT_BOUNDED_BELOW | 
			LADSPA_HINT_BOUNDED_ABOVE | 
			LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[EQ_CH6F].LowerBound = 6000;
		port_range_hints[EQ_CH6F].UpperBound = 18000;
		/* Parameters for CH6 gain [dB] */
		port_descriptors[EQ_CH6G] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[EQ_CH6G] =
		 "Band 7 Gain [dB]";
		port_range_hints[EQ_CH6G].HintDescriptor =
			LADSPA_HINT_BOUNDED_BELOW | 
			LADSPA_HINT_BOUNDED_ABOVE | 
			LADSPA_HINT_DEFAULT_0;
		port_range_hints[EQ_CH6G].LowerBound = -50;
		port_range_hints[EQ_CH6G].UpperBound = +20;
		/* Parameters for CH6 bandwidth [octaves] */
		port_descriptors[EQ_CH6B] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[EQ_CH6B] =
		 "Band 7 Bandwidth [octaves]";
		port_range_hints[EQ_CH6B].HintDescriptor =
			LADSPA_HINT_BOUNDED_BELOW | 
			LADSPA_HINT_BOUNDED_ABOVE | 
			LADSPA_HINT_DEFAULT_1;
		port_range_hints[EQ_CH6B].LowerBound = 0.1f;
		port_range_hints[EQ_CH6B].UpperBound = 5.0f;




		/* Parameters for CH7 freq [Hz] */
		port_descriptors[EQ_CH7F] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[EQ_CH7F] =
		 "Band 8 Freq [Hz]";
		port_range_hints[EQ_CH7F].HintDescriptor =
			LADSPA_HINT_BOUNDED_BELOW | 
			LADSPA_HINT_BOUNDED_ABOVE | 
			LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[EQ_CH7F].LowerBound = 10000;
		port_range_hints[EQ_CH7F].UpperBound = 20000;
		/* Parameters for CH7 gain [dB] */
		port_descriptors[EQ_CH7G] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[EQ_CH7G] =
		 "Band 8 Gain [dB]";
		port_range_hints[EQ_CH7G].HintDescriptor =
			LADSPA_HINT_BOUNDED_BELOW | 
			LADSPA_HINT_BOUNDED_ABOVE | 
			LADSPA_HINT_DEFAULT_0;
		port_range_hints[EQ_CH7G].LowerBound = -50;
		port_range_hints[EQ_CH7G].UpperBound = +20;
		/* Parameters for CH7 bandwidth [octaves] */
		port_descriptors[EQ_CH7B] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[EQ_CH7B] =
		 "Band 8 Bandwidth [octaves]";
		port_range_hints[EQ_CH7B].HintDescriptor =
			LADSPA_HINT_BOUNDED_BELOW | 
			LADSPA_HINT_BOUNDED_ABOVE | 
			LADSPA_HINT_DEFAULT_1;
		port_range_hints[EQ_CH7B].LowerBound = 0.1f;
		port_range_hints[EQ_CH7B].UpperBound = 5.0f;




		/* Parameters for Input */
		port_descriptors[EQ_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[EQ_INPUT] =
		 "Input";
		port_range_hints[EQ_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[EQ_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[EQ_OUTPUT] =
		 "Output";
		port_range_hints[EQ_OUTPUT].HintDescriptor = 0;

		eqDescriptor->activate = activate_eq;
		eqDescriptor->cleanup = cleanup_eq;
		eqDescriptor->connect_port = connectPort_eq;
		eqDescriptor->deactivate = NULL;
		eqDescriptor->instantiate = instantiate_eq;
		eqDescriptor->run = run_eq;
		eqDescriptor->run_adding = run_adding_eq;
		eqDescriptor->set_run_adding_gain = set_run_adding_gain;
	}
}


void 
__attribute__((destructor)) tap_fini() {

	if (eqDescriptor) {
		free((LADSPA_PortDescriptor *)eqDescriptor->PortDescriptors);
		free((char **)eqDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)eqDescriptor->PortRangeHints);
		free(eqDescriptor);
	}
	
}
