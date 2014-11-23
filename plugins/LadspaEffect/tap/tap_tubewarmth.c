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

    $Id: tap_tubewarmth.c,v 1.1 2004/08/02 18:14:50 tszilagyi Exp $
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <ladspa.h>
#include "tap_utils.h"


/* The Unique ID of the plugin: */

#define ID_MONO         2158

/* The port numbers for the plugin: */

#define DRIVE            0
#define BLEND            1
#define INPUT            2
#define OUTPUT           3

/* Total number of ports */


#define PORTCOUNT_MONO   4


/* The structure used to hold port connection information and state */

typedef struct {
	LADSPA_Data * drive;
	LADSPA_Data * blend;
	LADSPA_Data * input;
	LADSPA_Data * output;

	LADSPA_Data prev_med;
	LADSPA_Data prev_out;

	LADSPA_Data rdrive;
	LADSPA_Data rbdr;
	LADSPA_Data kpa;
	LADSPA_Data kpb;
	LADSPA_Data kna;
	LADSPA_Data knb;
	LADSPA_Data ap;
	LADSPA_Data an;
	LADSPA_Data imr;
	LADSPA_Data kc;
	LADSPA_Data srct;
	LADSPA_Data sq;
	LADSPA_Data pwrq;

	LADSPA_Data prev_drive;
	LADSPA_Data prev_blend;

	unsigned long sample_rate;
	LADSPA_Data run_adding_gain;
} TubeWarmth;



/* Construct a new plugin instance. */
LADSPA_Handle 
instantiate_TubeWarmth(const LADSPA_Descriptor * Descriptor,
		       unsigned long             sample_rate) {
  
        LADSPA_Handle * ptr;
	
	if ((ptr = malloc(sizeof(TubeWarmth))) != NULL) {
		((TubeWarmth *)ptr)->sample_rate = sample_rate;
		((TubeWarmth *)ptr)->run_adding_gain = 1.0f;

		((TubeWarmth *)ptr)->prev_med = 0.0f;
		((TubeWarmth *)ptr)->prev_out = 0.0f;

		((TubeWarmth *)ptr)->rdrive = 0.0f;
		((TubeWarmth *)ptr)->rbdr = 0.0f;
		((TubeWarmth *)ptr)->kpa = 0.0f;
		((TubeWarmth *)ptr)->kpb = 0.0f;
		((TubeWarmth *)ptr)->kna = 0.0f;
		((TubeWarmth *)ptr)->knb = 0.0f;
		((TubeWarmth *)ptr)->ap = 0.0f;
		((TubeWarmth *)ptr)->an = 0.0f;
		((TubeWarmth *)ptr)->imr = 0.0f;
		((TubeWarmth *)ptr)->kc = 0.0f;
		((TubeWarmth *)ptr)->srct = 0.0f;
		((TubeWarmth *)ptr)->sq = 0.0f;
		((TubeWarmth *)ptr)->pwrq = 0.0f;

                /* These are out of band to force param recalc upon first run() */
		((TubeWarmth *)ptr)->prev_drive = -1.0f;
		((TubeWarmth *)ptr)->prev_blend = -11.0f;

		return ptr;
	}
       	return NULL;
}





/* Connect a port to a data location. */
void 
connect_port_TubeWarmth(LADSPA_Handle Instance,
			unsigned long Port,
			LADSPA_Data * DataLocation) {
	
	TubeWarmth * ptr = (TubeWarmth *)Instance;

	switch (Port) {
	case DRIVE:
		ptr->drive = DataLocation;
		break;
	case BLEND:
		ptr->blend = DataLocation;
		break;
	case INPUT:
		ptr->input = DataLocation;
		break;
	case OUTPUT:
		ptr->output = DataLocation;
		break;
	}
}


#define EPS 0.000000001f

static inline float
M(float x) {

	if ((x > EPS) || (x < -EPS))
		return x;
	else
		return 0.0f;
}

static inline float
D(float x) {

	if (x > EPS)
		return sqrt(x);
	else if (x < -EPS)
		return sqrt(-x);
	else
		return 0.0f;
}

void 
run_TubeWarmth(LADSPA_Handle Instance,
	       unsigned long SampleCount) {
  
	TubeWarmth * ptr = (TubeWarmth *)Instance;
	LADSPA_Data * input = ptr->input;
	LADSPA_Data * output = ptr->output;
	LADSPA_Data drive = LIMIT(*(ptr->drive),0.1f,10.0f);
	LADSPA_Data blend = LIMIT(*(ptr->blend),-10.0f,10.0f);

	unsigned long sample_index;
	unsigned long sample_count = SampleCount;
	unsigned long sample_rate = ptr->sample_rate;

	LADSPA_Data rdrive = ptr->rdrive;
	LADSPA_Data rbdr = ptr->rbdr;
	LADSPA_Data kpa = ptr->kpa;
	LADSPA_Data kpb = ptr->kpb;
	LADSPA_Data kna = ptr->kna;
	LADSPA_Data knb = ptr->knb;
	LADSPA_Data ap = ptr->ap;
	LADSPA_Data an = ptr->an;
	LADSPA_Data imr = ptr->imr;
	LADSPA_Data kc = ptr->kc;
	LADSPA_Data srct = ptr->srct;
	LADSPA_Data sq = ptr->sq;
	LADSPA_Data pwrq = ptr->pwrq;

	LADSPA_Data prev_med;
	LADSPA_Data prev_out;
	LADSPA_Data in;
	LADSPA_Data med;
	LADSPA_Data out;

	if ((ptr->prev_drive != drive) || (ptr->prev_blend != blend)) {

		rdrive = 12.0f / drive;
		rbdr = rdrive / (10.5f - blend) * 780.0f / 33.0f;
		kpa = D(2.0f * (rdrive*rdrive) - 1.0f) + 1.0f;
		kpb = (2.0f - kpa) / 2.0f;
		ap = ((rdrive*rdrive) - kpa + 1.0f) / 2.0f;
		kc = kpa / D(2.0f * D(2.0f * (rdrive*rdrive) - 1.0f) - 2.0f * rdrive*rdrive);

		srct = (0.1f * sample_rate) / (0.1f * sample_rate + 1.0f);
		sq = kc*kc + 1.0f;
		knb = -1.0f * rbdr / D(sq);
		kna = 2.0f * kc * rbdr / D(sq);
		an = rbdr*rbdr / sq;
		imr = 2.0f * knb + D(2.0f * kna + 4.0f * an - 1.0f);
		pwrq = 2.0f / (imr + 1.0f);

		ptr->prev_drive = drive;
		ptr->prev_blend = blend;
	}

	for (sample_index = 0; sample_index < sample_count; sample_index++) {

		in = *(input++);
		prev_med = ptr->prev_med;
		prev_out = ptr->prev_out;

		if (in >= 0.0f) {
			med = (D(ap + in * (kpa - in)) + kpb) * pwrq;
		} else {
			med = (D(an - in * (kna + in)) + knb) * pwrq * -1.0f;
		}

		out = srct * (med - prev_med + prev_out);

		if (out < -1.0f)
			out = -1.0f;
		
		*(output++) = out;

		ptr->prev_med = M(med);
		ptr->prev_out = M(out);
	}

	ptr->rdrive = rdrive;
	ptr->rbdr = rbdr;
	ptr->kpa = kpa;
	ptr->kpb = kpb;
	ptr->kna = kna;
	ptr->knb = knb;
	ptr->ap = ap;
	ptr->an = an;
	ptr->imr = imr;
	ptr->kc = kc;
	ptr->srct = srct;
	ptr->sq = sq;
	ptr->pwrq = pwrq;
}



void
set_run_adding_gain_TubeWarmth(LADSPA_Handle Instance, LADSPA_Data gain) {

	TubeWarmth * ptr = (TubeWarmth *)Instance;

	ptr->run_adding_gain = gain;
}



void 
run_adding_TubeWarmth(LADSPA_Handle Instance,
		      unsigned long SampleCount) {
  
	TubeWarmth * ptr = (TubeWarmth *)Instance;
	LADSPA_Data * input = ptr->input;
	LADSPA_Data * output = ptr->output;
	LADSPA_Data drive = LIMIT(*(ptr->drive),0.1f,10.0f);
	LADSPA_Data blend = LIMIT(*(ptr->blend),-10.0f,10.0f);

	unsigned long sample_index;
	unsigned long sample_count = SampleCount;
	unsigned long sample_rate = ptr->sample_rate;

	LADSPA_Data rdrive = ptr->rdrive;
	LADSPA_Data rbdr = ptr->rbdr;
	LADSPA_Data kpa = ptr->kpa;
	LADSPA_Data kpb = ptr->kpb;
	LADSPA_Data kna = ptr->kna;
	LADSPA_Data knb = ptr->knb;
	LADSPA_Data ap = ptr->ap;
	LADSPA_Data an = ptr->an;
	LADSPA_Data imr = ptr->imr;
	LADSPA_Data kc = ptr->kc;
	LADSPA_Data srct = ptr->srct;
	LADSPA_Data sq = ptr->sq;
	LADSPA_Data pwrq = ptr->pwrq;

	LADSPA_Data prev_med;
	LADSPA_Data prev_out;
	LADSPA_Data in;
	LADSPA_Data med;
	LADSPA_Data out;

	if ((ptr->prev_drive != drive) || (ptr->prev_blend != blend)) {

		rdrive = 12.0f / drive;
		rbdr = rdrive / (10.5f - blend) * 780.0f / 33.0f;
		kpa = D(2.0f * (rdrive*rdrive) - 1.0f) + 1.0f;
		kpb = (2.0f - kpa) / 2.0f;
		ap = ((rdrive*rdrive) - kpa + 1.0f) / 2.0f;
		kc = kpa / D(2.0f * D(2.0f * (rdrive*rdrive) - 1.0f) - 2.0f * rdrive*rdrive);

		srct = (0.1f * sample_rate) / (0.1f * sample_rate + 1.0f);
		sq = kc*kc + 1.0f;
		knb = -1.0f * rbdr / D(sq);
		kna = 2.0f * kc * rbdr / D(sq);
		an = rbdr*rbdr / sq;
		imr = 2.0f * knb + D(2.0f * kna + 4.0f * an - 1.0f);
		pwrq = 2.0f / (imr + 1.0f);

		ptr->prev_drive = drive;
		ptr->prev_blend = blend;
	}

	for (sample_index = 0; sample_index < sample_count; sample_index++) {

		in = *(input++);
		prev_med = ptr->prev_med;
		prev_out = ptr->prev_out;

		if (in >= 0.0f) {
			med = (D(ap + in * (kpa - in)) + kpb) * pwrq;
		} else {
			med = (D(an - in * (kna + in)) + knb) * pwrq * -1.0f;
		}

		out = srct * (med - prev_med + prev_out);

		if (out < -1.0f)
			out = -1.0f;
		
		*(output++) += out * ptr->run_adding_gain;

		ptr->prev_med = M(med);
		ptr->prev_out = M(out);
	}

	ptr->rdrive = rdrive;
	ptr->rbdr = rbdr;
	ptr->kpa = kpa;
	ptr->kpb = kpb;
	ptr->kna = kna;
	ptr->knb = knb;
	ptr->ap = ap;
	ptr->an = an;
	ptr->imr = imr;
	ptr->kc = kc;
	ptr->srct = srct;
	ptr->sq = sq;
	ptr->pwrq = pwrq;
}




/* Throw away a TubeWarmth effect instance. */
void 
cleanup_TubeWarmth(LADSPA_Handle Instance) {

	free(Instance);
}



LADSPA_Descriptor * mono_descriptor = NULL;



/* __attribute__((constructor)) tap_init() is called automatically when the plugin library is first
   loaded. */
void 
__attribute__((constructor)) tap_init() {
	
	char ** port_names;
	LADSPA_PortDescriptor * port_descriptors;
	LADSPA_PortRangeHint * port_range_hints;
	
	if ((mono_descriptor = 
	     (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor))) == NULL)
		exit(1);
	

	mono_descriptor->UniqueID = ID_MONO;
	mono_descriptor->Label = strdup("tap_tubewarmth");
	mono_descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
	mono_descriptor->Name = strdup("TAP TubeWarmth");
	mono_descriptor->Maker = strdup("Tom Szilagyi");
	mono_descriptor->Copyright = strdup("GPL");
	mono_descriptor->PortCount = PORTCOUNT_MONO;

	if ((port_descriptors =
	     (LADSPA_PortDescriptor *)calloc(PORTCOUNT_MONO, sizeof(LADSPA_PortDescriptor))) == NULL)
		exit(1);

	mono_descriptor->PortDescriptors = (const LADSPA_PortDescriptor *)port_descriptors;
	port_descriptors[DRIVE] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[BLEND] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[INPUT] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;

	if ((port_names = 
	     (char **)calloc(PORTCOUNT_MONO, sizeof(char *))) == NULL)
		exit(1);

	mono_descriptor->PortNames = (const char **)port_names;
	port_names[DRIVE] = strdup("Drive");
	port_names[BLEND] = strdup("Tape--Tube Blend");
	port_names[INPUT] = strdup("Input");
	port_names[OUTPUT] = strdup("Output");

	if ((port_range_hints = 
	     ((LADSPA_PortRangeHint *)calloc(PORTCOUNT_MONO, sizeof(LADSPA_PortRangeHint)))) == NULL)
		exit(1);

	mono_descriptor->PortRangeHints	= (const LADSPA_PortRangeHint *)port_range_hints;
	port_range_hints[DRIVE].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_LOW);
	port_range_hints[BLEND].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_MAXIMUM);
	port_range_hints[DRIVE].LowerBound = 0.1f;
	port_range_hints[DRIVE].UpperBound = 10.0f;
	port_range_hints[BLEND].LowerBound = -10.0f;
	port_range_hints[BLEND].UpperBound = 10.0f;
	port_range_hints[INPUT].HintDescriptor = 0;
	port_range_hints[OUTPUT].HintDescriptor = 0;
	mono_descriptor->instantiate = instantiate_TubeWarmth;
	mono_descriptor->connect_port = connect_port_TubeWarmth;
	mono_descriptor->activate = NULL;
	mono_descriptor->run = run_TubeWarmth;
	mono_descriptor->run_adding = run_adding_TubeWarmth;
	mono_descriptor->set_run_adding_gain = set_run_adding_gain_TubeWarmth;
	mono_descriptor->deactivate = NULL;
	mono_descriptor->cleanup = cleanup_TubeWarmth;
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
