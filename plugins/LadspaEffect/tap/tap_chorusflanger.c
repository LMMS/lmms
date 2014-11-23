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

    $Id: tap_chorusflanger.c,v 1.3 2004/08/17 09:15:21 tszilagyi Exp $
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <ladspa.h>
#include "tap_utils.h"


/* The Unique ID of the plugin: */

#define ID_STEREO       2159

/* The port numbers for the plugin: */

#define FREQ            0
#define PHASE           1
#define DEPTH           2
#define DELAY           3
#define CONTOUR         4
#define DRYLEVEL        5
#define WETLEVEL        6
#define INPUT_L         7
#define INPUT_R         8
#define OUTPUT_L        9
#define OUTPUT_R       10

/* Total number of ports */
#define PORTCOUNT_STEREO 11


/*
 * Largest buffer lengths needed (at 192 kHz).
 * These are summed up to determine the size of *one* buffer per channel.
 */
#define DEPTH_BUFLEN 450
#define DELAY_BUFLEN 19200

/* Max. frequency setting */
#define MAX_FREQ 5.0f

/* bandwidth of highpass filters (in octaves) */
#define HP_BW 1

/* cosine table for fast computations */
#define COS_TABLE_SIZE 1024
LADSPA_Data cos_table[COS_TABLE_SIZE];


/* The structure used to hold port connection information and state */
typedef struct {
	LADSPA_Data * freq;
	LADSPA_Data * phase;
	LADSPA_Data * depth;
	LADSPA_Data * delay;
	LADSPA_Data * contour;
	LADSPA_Data * drylevel;
	LADSPA_Data * wetlevel;
	LADSPA_Data * input_L;
	LADSPA_Data * input_R;
	LADSPA_Data * output_L;
	LADSPA_Data * output_R;

	LADSPA_Data * ring_L;
	unsigned long buflen_L;
	unsigned long pos_L;
	LADSPA_Data * ring_R;
	unsigned long buflen_R;
	unsigned long pos_R;

	biquad highpass_L;
	biquad highpass_R;

	float cm_phase;
	float dm_phase;

	unsigned long sample_rate;
	LADSPA_Data run_adding_gain;
} ChorusFlanger;


/* Construct a new plugin instance. */
LADSPA_Handle 
instantiate_ChorusFlanger(const LADSPA_Descriptor * Descriptor,
			  unsigned long             sample_rate) {
  
        LADSPA_Handle * ptr;
	
	if ((ptr = malloc(sizeof(ChorusFlanger))) != NULL) {
		((ChorusFlanger *)ptr)->sample_rate = sample_rate;
		((ChorusFlanger *)ptr)->run_adding_gain = 1.0f;


		if ((((ChorusFlanger *)ptr)->ring_L =
		     calloc((DEPTH_BUFLEN + DELAY_BUFLEN) * sample_rate / 192000,
			    sizeof(LADSPA_Data))) == NULL)
			return NULL;
		((ChorusFlanger *)ptr)->buflen_L = (DEPTH_BUFLEN + DELAY_BUFLEN) * sample_rate / 192000;
		((ChorusFlanger *)ptr)->pos_L = 0;

		if ((((ChorusFlanger *)ptr)->ring_R =
		     calloc((DEPTH_BUFLEN + DELAY_BUFLEN) * sample_rate / 192000,
			    sizeof(LADSPA_Data))) == NULL)
			return NULL;
		((ChorusFlanger *)ptr)->buflen_R = (DEPTH_BUFLEN + DELAY_BUFLEN) * sample_rate / 192000;
		((ChorusFlanger *)ptr)->pos_R = 0;


		((ChorusFlanger *)ptr)->cm_phase = 0.0f;
		((ChorusFlanger *)ptr)->dm_phase = 0.0f;

		return ptr;
	}
       	return NULL;
}


void
activate_ChorusFlanger(LADSPA_Handle Instance) {

	ChorusFlanger * ptr = (ChorusFlanger *)Instance;
	unsigned long i;

	for (i = 0; i < (DEPTH_BUFLEN + DELAY_BUFLEN) * ptr->sample_rate / 192000; i++) {
		ptr->ring_L[i] = 0.0f;
		ptr->ring_R[i] = 0.0f;
	}

	biquad_init(&ptr->highpass_L);
	biquad_init(&ptr->highpass_R);
}




/* Connect a port to a data location. */
void 
connect_port_ChorusFlanger(LADSPA_Handle Instance,
			   unsigned long Port,
			   LADSPA_Data * data) {
	
	ChorusFlanger * ptr = (ChorusFlanger *)Instance;

	switch (Port) {
	case FREQ:
		ptr->freq = data;
		break;
	case PHASE:
		ptr->phase = data;
		break;
	case DEPTH:
		ptr->depth = data;
		break;
	case DELAY:
		ptr->delay = data;
		break;
	case CONTOUR:
		ptr->contour = data;
		break;
	case DRYLEVEL:
		ptr->drylevel = data;
		break;
	case WETLEVEL:
		ptr->wetlevel = data;
		break;
	case INPUT_L:
		ptr->input_L = data;
		break;
	case INPUT_R:
		ptr->input_R = data;
		break;
	case OUTPUT_L:
		ptr->output_L = data;
		break;
	case OUTPUT_R:
		ptr->output_R = data;
		break;
	}
}


void 
run_ChorusFlanger(LADSPA_Handle Instance,
		  unsigned long SampleCount) {
  
	ChorusFlanger * ptr = (ChorusFlanger *)Instance;

	LADSPA_Data freq = LIMIT(*(ptr->freq), 0.0f, MAX_FREQ);
	LADSPA_Data phase = LIMIT(*(ptr->phase), 0.0f, 180.0f) / 180.0f;
	LADSPA_Data depth = 100.0f * ptr->sample_rate / 44100.0f
		* LIMIT(*(ptr->depth),0.0f,100.0f) / 100.0f;
	LADSPA_Data delay = LIMIT(*(ptr->delay),0.0f,100.0f);
	LADSPA_Data contour = LIMIT(*(ptr->contour), 20.0f, 20000.0f);
	LADSPA_Data drylevel = db2lin(LIMIT(*(ptr->drylevel),-90.0f,20.0f));
	LADSPA_Data wetlevel = db2lin(LIMIT(*(ptr->wetlevel),-90.0f,20.0f));
	LADSPA_Data * input_L = ptr->input_L;
	LADSPA_Data * input_R = ptr->input_R;
	LADSPA_Data * output_L = ptr->output_L;
	LADSPA_Data * output_R = ptr->output_R;

	unsigned long sample_index;
	unsigned long sample_count = SampleCount;

	LADSPA_Data in_L = 0.0f;
	LADSPA_Data in_R = 0.0f;
	LADSPA_Data d_L = 0.0f;
	LADSPA_Data d_R = 0.0f;
	LADSPA_Data f_L = 0.0f;
	LADSPA_Data f_R = 0.0f;
	LADSPA_Data out_L = 0.0f;
	LADSPA_Data out_R = 0.0f;

	float phase_L = 0.0f;
	float phase_R = 0.0f;
	float fpos_L = 0.0f;
	float fpos_R = 0.0f;
	float n_L = 0.0f;
	float n_R = 0.0f;
	float rem_L = 0.0f;
	float rem_R = 0.0f;
	float s_a_L, s_a_R, s_b_L, s_b_R;

	float d_pos = 0.0f;

	if (delay < 1.0f)
		delay = 1.0f;
	delay = 100.0f - delay;

	hp_set_params(&ptr->highpass_L, contour, HP_BW, ptr->sample_rate);
	hp_set_params(&ptr->highpass_R, contour, HP_BW, ptr->sample_rate);

	for (sample_index = 0; sample_index < sample_count; sample_index++) {

		in_L = *(input_L++);
		in_R = *(input_R++);

		push_buffer(in_L, ptr->ring_L, ptr->buflen_L, &(ptr->pos_L));
		push_buffer(in_R, ptr->ring_R, ptr->buflen_R, &(ptr->pos_R));
		
		ptr->cm_phase += freq / ptr->sample_rate * COS_TABLE_SIZE;
		
		while (ptr->cm_phase >= COS_TABLE_SIZE)
			ptr->cm_phase -= COS_TABLE_SIZE;
		
		ptr->dm_phase = phase * COS_TABLE_SIZE / 2.0f;
		
		phase_L = ptr->cm_phase;
		phase_R = ptr->cm_phase + ptr->dm_phase;
		while (phase_R >= COS_TABLE_SIZE)
			phase_R -= COS_TABLE_SIZE;
		
		d_pos = delay * ptr->sample_rate / 1000.0f;
		fpos_L = d_pos + depth * (0.5f + 0.5f * cos_table[(unsigned long)phase_L]);
		fpos_R = d_pos + depth * (0.5f + 0.5f * cos_table[(unsigned long)phase_R]);
		
		n_L = floorf(fpos_L);
		n_R = floorf(fpos_R);
		rem_L = fpos_L - n_L;
		rem_R = fpos_R - n_R;
		
		s_a_L = read_buffer(ptr->ring_L, ptr->buflen_L,
				    ptr->pos_L, (unsigned long) n_L);
		s_b_L = read_buffer(ptr->ring_L, ptr->buflen_L,
				    ptr->pos_L, (unsigned long) n_L + 1);
		
		s_a_R = read_buffer(ptr->ring_R, ptr->buflen_R,
				    ptr->pos_R, (unsigned long) n_R);
		s_b_R = read_buffer(ptr->ring_R, ptr->buflen_R,
				    ptr->pos_R, (unsigned long) n_R + 1);
		
		d_L = ((1 - rem_L) * s_a_L + rem_L * s_b_L);
		d_R = ((1 - rem_R) * s_a_R + rem_R * s_b_R);

		f_L = biquad_run(&ptr->highpass_L, d_L);
		f_R = biquad_run(&ptr->highpass_R, d_R);

		out_L = drylevel * in_L + wetlevel * f_L;
		out_R = drylevel * in_R + wetlevel * f_R;

		*(output_L++) = out_L;
		*(output_R++) = out_R;
	}
}


void
set_run_adding_gain_ChorusFlanger(LADSPA_Handle Instance, LADSPA_Data gain) {

	ChorusFlanger * ptr = (ChorusFlanger *)Instance;

	ptr->run_adding_gain = gain;
}



void 
run_adding_ChorusFlanger(LADSPA_Handle Instance,
                         unsigned long SampleCount) {
  
	ChorusFlanger * ptr = (ChorusFlanger *)Instance;

	LADSPA_Data freq = LIMIT(*(ptr->freq), 0.0f, MAX_FREQ);
	LADSPA_Data phase = LIMIT(*(ptr->phase), 0.0f, 180.0f) / 180.0f;
	LADSPA_Data depth = 100.0f * ptr->sample_rate / 44100.0f
		* LIMIT(*(ptr->depth),0.0f,100.0f) / 100.0f;
	LADSPA_Data delay = LIMIT(*(ptr->delay),0.0f,100.0f);
	LADSPA_Data contour = LIMIT(*(ptr->contour), 20.0f, 20000.0f);
	LADSPA_Data drylevel = db2lin(LIMIT(*(ptr->drylevel),-90.0f,20.0f));
	LADSPA_Data wetlevel = db2lin(LIMIT(*(ptr->wetlevel),-90.0f,20.0f));
	LADSPA_Data * input_L = ptr->input_L;
	LADSPA_Data * input_R = ptr->input_R;
	LADSPA_Data * output_L = ptr->output_L;
	LADSPA_Data * output_R = ptr->output_R;

	unsigned long sample_index;
	unsigned long sample_count = SampleCount;

	LADSPA_Data in_L = 0.0f;
	LADSPA_Data in_R = 0.0f;
	LADSPA_Data d_L = 0.0f;
	LADSPA_Data d_R = 0.0f;
	LADSPA_Data f_L = 0.0f;
	LADSPA_Data f_R = 0.0f;
	LADSPA_Data out_L = 0.0f;
	LADSPA_Data out_R = 0.0f;

	float phase_L = 0.0f;
	float phase_R = 0.0f;
	float fpos_L = 0.0f;
	float fpos_R = 0.0f;
	float n_L = 0.0f;
	float n_R = 0.0f;
	float rem_L = 0.0f;
	float rem_R = 0.0f;
	float s_a_L, s_a_R, s_b_L, s_b_R;

	float d_pos = 0.0f;

	if (delay < 1.0f)
		delay = 1.0f;
	delay = 100.0f - delay;

	hp_set_params(&ptr->highpass_L, contour, HP_BW, ptr->sample_rate);
	hp_set_params(&ptr->highpass_R, contour, HP_BW, ptr->sample_rate);

	for (sample_index = 0; sample_index < sample_count; sample_index++) {

		in_L = *(input_L++);
		in_R = *(input_R++);

		push_buffer(in_L, ptr->ring_L, ptr->buflen_L, &(ptr->pos_L));
		push_buffer(in_R, ptr->ring_R, ptr->buflen_R, &(ptr->pos_R));
		
		ptr->cm_phase += freq / ptr->sample_rate * COS_TABLE_SIZE;
		
		while (ptr->cm_phase >= COS_TABLE_SIZE)
			ptr->cm_phase -= COS_TABLE_SIZE;
		
		ptr->dm_phase = phase * COS_TABLE_SIZE / 2.0f;
		
		phase_L = ptr->cm_phase;
		phase_R = ptr->cm_phase + ptr->dm_phase;
		while (phase_R >= COS_TABLE_SIZE)
			phase_R -= COS_TABLE_SIZE;
		
		d_pos = delay * ptr->sample_rate / 1000.0f;
		fpos_L = d_pos + depth * (0.5f + 0.5f * cos_table[(unsigned long)phase_L]);
		fpos_R = d_pos + depth * (0.5f + 0.5f * cos_table[(unsigned long)phase_R]);
		
		n_L = floorf(fpos_L);
		n_R = floorf(fpos_R);
		rem_L = fpos_L - n_L;
		rem_R = fpos_R - n_R;
		
		s_a_L = read_buffer(ptr->ring_L, ptr->buflen_L,
				    ptr->pos_L, (unsigned long) n_L);
		s_b_L = read_buffer(ptr->ring_L, ptr->buflen_L,
				    ptr->pos_L, (unsigned long) n_L + 1);
		
		s_a_R = read_buffer(ptr->ring_R, ptr->buflen_R,
				    ptr->pos_R, (unsigned long) n_R);
		s_b_R = read_buffer(ptr->ring_R, ptr->buflen_R,
				    ptr->pos_R, (unsigned long) n_R + 1);
		
		d_L = ((1 - rem_L) * s_a_L + rem_L * s_b_L);
		d_R = ((1 - rem_R) * s_a_R + rem_R * s_b_R);

		f_L = biquad_run(&ptr->highpass_L, d_L);
		f_R = biquad_run(&ptr->highpass_R, d_R);

		out_L = drylevel * in_L + wetlevel * f_L;
		out_R = drylevel * in_R + wetlevel * f_R;

		*(output_L++) += ptr->run_adding_gain * out_L;
		*(output_R++) += ptr->run_adding_gain * out_R;
	}
}



/* Throw away a ChorusFlanger effect instance. */
void 
cleanup_ChorusFlanger(LADSPA_Handle Instance) {

  	ChorusFlanger * ptr = (ChorusFlanger *)Instance;
	free(ptr->ring_L);
	free(ptr->ring_R);
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
        int i;
	
	if ((stereo_descriptor = 
	     (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor))) == NULL)
		exit(1);

        for (i = 0; i < COS_TABLE_SIZE; i++)
                cos_table[i] = cosf(i * 2.0f * M_PI / COS_TABLE_SIZE);

	stereo_descriptor->UniqueID = ID_STEREO;
	stereo_descriptor->Label = strdup("tap_chorusflanger");
	stereo_descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
	stereo_descriptor->Name = strdup("TAP Chorus/Flanger");
	stereo_descriptor->Maker = strdup("Tom Szilagyi");
	stereo_descriptor->Copyright = strdup("GPL");
	stereo_descriptor->PortCount = PORTCOUNT_STEREO;

	if ((port_descriptors =
	     (LADSPA_PortDescriptor *)calloc(PORTCOUNT_STEREO, sizeof(LADSPA_PortDescriptor))) == NULL)
		exit(1);

	stereo_descriptor->PortDescriptors = (const LADSPA_PortDescriptor *)port_descriptors;
	port_descriptors[FREQ] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[PHASE] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[DEPTH] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[DELAY] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[CONTOUR] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[DRYLEVEL] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[WETLEVEL] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[INPUT_L] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[INPUT_R] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT_L] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT_R] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;

	if ((port_names = 
	     (char **)calloc(PORTCOUNT_STEREO, sizeof(char *))) == NULL)
		exit(1);

	stereo_descriptor->PortNames = (const char **)port_names;
	port_names[FREQ] = strdup("Frequency [Hz]");
	port_names[PHASE] = strdup("L/R Phase Shift [deg]");
	port_names[DEPTH] = strdup("Depth [%]");
	port_names[DELAY] = strdup("Delay [ms]");
	port_names[CONTOUR] = strdup("Contour [Hz]");
	port_names[DRYLEVEL] = strdup("Dry Level [dB]");
	port_names[WETLEVEL] = strdup("Wet Level [dB]");
	port_names[INPUT_L] = strdup("Input_L");
	port_names[INPUT_R] = strdup("Input_R");
	port_names[OUTPUT_L] = strdup("Output_L");
	port_names[OUTPUT_R] = strdup("Output_R");

	if ((port_range_hints = 
	     ((LADSPA_PortRangeHint *)calloc(PORTCOUNT_STEREO, sizeof(LADSPA_PortRangeHint)))) == NULL)
		exit(1);

	stereo_descriptor->PortRangeHints = (const LADSPA_PortRangeHint *)port_range_hints;
	port_range_hints[FREQ].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_LOW);
	port_range_hints[PHASE].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_MIDDLE);
	port_range_hints[DEPTH].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_HIGH);
	port_range_hints[DELAY].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_LOW);
	port_range_hints[CONTOUR].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_100);
	port_range_hints[DRYLEVEL].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[WETLEVEL].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[FREQ].LowerBound = 0.0f;
	port_range_hints[FREQ].UpperBound = MAX_FREQ;
	port_range_hints[PHASE].LowerBound = 0.0f;
	port_range_hints[PHASE].UpperBound = 180.0f;
	port_range_hints[DEPTH].LowerBound = 0.0f;
	port_range_hints[DEPTH].UpperBound = 100.0f;
	port_range_hints[DELAY].LowerBound = 0.0f;
	port_range_hints[DELAY].UpperBound = 100.0f;
	port_range_hints[CONTOUR].LowerBound = 20.0f;
	port_range_hints[CONTOUR].UpperBound = 20000.0f;
	port_range_hints[DRYLEVEL].LowerBound = -90.0f;
	port_range_hints[DRYLEVEL].UpperBound = +20.0f;
	port_range_hints[WETLEVEL].LowerBound = -90.0f;
	port_range_hints[WETLEVEL].UpperBound = +20.0f;
	port_range_hints[INPUT_L].HintDescriptor = 0;
	port_range_hints[INPUT_R].HintDescriptor = 0;
	port_range_hints[OUTPUT_L].HintDescriptor = 0;
	port_range_hints[OUTPUT_R].HintDescriptor = 0;
	stereo_descriptor->instantiate = instantiate_ChorusFlanger;
	stereo_descriptor->connect_port = connect_port_ChorusFlanger;
	stereo_descriptor->activate = activate_ChorusFlanger;
	stereo_descriptor->run = run_ChorusFlanger;
	stereo_descriptor->run_adding = run_adding_ChorusFlanger;
	stereo_descriptor->set_run_adding_gain = set_run_adding_gain_ChorusFlanger;
	stereo_descriptor->deactivate = NULL;
	stereo_descriptor->cleanup = cleanup_ChorusFlanger;
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
