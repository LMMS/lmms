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

    $Id: tap_echo.c,v 1.7 2004/12/06 09:32:41 tszilagyi Exp $
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <ladspa.h>
#include "tap_utils.h"

/* The Unique ID of the plugin: */

#define ID_STEREO       2143

/* The port numbers for the plugin: */

#define DELAYTIME_L 0
#define FEEDBACK_L  1
#define DELAYTIME_R 2
#define FEEDBACK_R  3
#define STRENGTH_L  4
#define STRENGTH_R  5
#define DRYLEVEL    6
#define MODE        7
#define HAAS        8
#define REV_OUTCH   9

#define INPUT_L     10
#define OUTPUT_L    11
#define INPUT_R     12
#define OUTPUT_R    13

/* Total number of ports */

#define PORTCOUNT_STEREO 14


/* Maximum delay (ms) */

#define MAX_DELAY        2000


/* The structure used to hold port connection information and state */

typedef struct {
	LADSPA_Data * delaytime_L;
	LADSPA_Data * delaytime_R;
	LADSPA_Data * feedback_L;
	LADSPA_Data * feedback_R;
	LADSPA_Data * strength_L;
	LADSPA_Data * strength_R;
	LADSPA_Data * drylevel;
	LADSPA_Data * mode;
	LADSPA_Data * haas;
	LADSPA_Data * rev_outch;

	LADSPA_Data * input_L;
	LADSPA_Data * output_L;
	LADSPA_Data * input_R;
	LADSPA_Data * output_R;

	unsigned long sample_rate;
	LADSPA_Data mpx_out_L;
	LADSPA_Data mpx_out_R;

	LADSPA_Data * ringbuffer_L;
	LADSPA_Data * ringbuffer_R;
	unsigned long * buffer_pos_L;
	unsigned long * buffer_pos_R;

	LADSPA_Data run_adding_gain;
} Echo;




/* Construct a new plugin instance. */
LADSPA_Handle 
instantiate_Echo(const LADSPA_Descriptor * Descriptor,
		 unsigned long             SampleRate) {
	
	LADSPA_Handle * ptr;
	
	if ((ptr = malloc(sizeof(Echo))) != NULL) {
		((Echo *)ptr)->sample_rate = SampleRate;
		((Echo *)ptr)->run_adding_gain = 1.0f;

		/* allocate memory for ringbuffers and related dynamic vars */
		if ((((Echo *)ptr)->ringbuffer_L = 
		     calloc(MAX_DELAY * ((Echo *)ptr)->sample_rate / 1000,
			    sizeof(LADSPA_Data))) == NULL)
			exit(1);
		if ((((Echo *)ptr)->ringbuffer_R = 
		     calloc(MAX_DELAY * ((Echo *)ptr)->sample_rate / 1000,
			    sizeof(LADSPA_Data))) == NULL)
			exit(1);
		if ((((Echo *)ptr)->buffer_pos_L = calloc(1, sizeof(unsigned long))) == NULL)
			exit(1);
		if ((((Echo *)ptr)->buffer_pos_R = calloc(1, sizeof(unsigned long))) == NULL)
			exit(1);
		
		*(((Echo *)ptr)->buffer_pos_L) = 0;
		*(((Echo *)ptr)->buffer_pos_R) = 0;
		
		return ptr;
	}
	
	return NULL;
}


/* activate a plugin instance */
void
activate_Echo(LADSPA_Handle Instance) {

	Echo * ptr = (Echo *)Instance;
	unsigned int i;
	
	ptr->mpx_out_L = 0;
	ptr->mpx_out_R = 0;
	
	*(ptr->buffer_pos_L) = 0;
	*(ptr->buffer_pos_R) = 0;

	for (i = 0; i < MAX_DELAY * ptr->sample_rate / 1000; i++) {
		ptr->ringbuffer_L[i] = 0.0f;
		ptr->ringbuffer_R[i] = 0.0f;
	}
}


/* Connect a port to a data location. */
void 
connect_port_Echo(LADSPA_Handle Instance,
		   unsigned long Port,
		   LADSPA_Data * DataLocation) {
	
	Echo * ptr;
	
	ptr = (Echo *)Instance;
	switch (Port) {
	case DELAYTIME_L:
		ptr->delaytime_L = DataLocation;
		break;
	case DELAYTIME_R:
		ptr->delaytime_R = DataLocation;
		break;
	case FEEDBACK_L:
		ptr->feedback_L = DataLocation;
		break;
	case FEEDBACK_R:
		ptr->feedback_R = DataLocation;
		break;
	case STRENGTH_L:
		ptr->strength_L = DataLocation;
		break;
	case STRENGTH_R:
		ptr->strength_R = DataLocation;
		break;
	case MODE:
		ptr->mode = DataLocation;
		break;
	case HAAS:
		ptr->haas = DataLocation;
		break;
	case REV_OUTCH:
		ptr->rev_outch = DataLocation;
		break;
	case DRYLEVEL:
		ptr->drylevel = DataLocation;
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


#define EPS 0.00000001f

static inline float
M(float x) {

        if ((x > EPS) || (x < -EPS))
                return x;
        else
                return 0.0f;
}

void 
run_Echo(LADSPA_Handle Instance,
	 unsigned long SampleCount) {
	
	Echo * ptr;
	unsigned long sample_index;

	LADSPA_Data delaytime_L;
	LADSPA_Data delaytime_R;
	LADSPA_Data feedback_L;
	LADSPA_Data feedback_R;
	LADSPA_Data strength_L;
	LADSPA_Data strength_R;
	LADSPA_Data drylevel;
	LADSPA_Data mode;
	LADSPA_Data haas;
	LADSPA_Data rev_outch;

	LADSPA_Data * input_L;
	LADSPA_Data * output_L;
	LADSPA_Data * input_R;
	LADSPA_Data * output_R;

	unsigned long sample_rate;
	unsigned long buflen_L;
	unsigned long buflen_R;

	LADSPA_Data out_L = 0;
	LADSPA_Data out_R = 0;
	LADSPA_Data in_L = 0;
	LADSPA_Data in_R = 0;

	ptr = (Echo *)Instance;

	delaytime_L = LIMIT(*(ptr->delaytime_L),0.0f,2000.0f);
	delaytime_R = LIMIT(*(ptr->delaytime_R),0.0f,2000.0f);
	feedback_L = LIMIT(*(ptr->feedback_L) / 100.0, 0.0f, 100.0f);
	feedback_R = LIMIT(*(ptr->feedback_R) / 100.0, 0.0f, 100.0f);
        strength_L = db2lin(LIMIT(*(ptr->strength_L),-70.0f,10.0f));
	strength_R = db2lin(LIMIT(*(ptr->strength_R),-70.0f,10.0f));
	drylevel = db2lin(LIMIT(*(ptr->drylevel),-70.0f,10.0f));
	mode = LIMIT(*(ptr->mode),-2.0f,2.0f);
	haas = LIMIT(*(ptr->haas),-2.0f,2.0f);
	rev_outch = LIMIT(*(ptr->rev_outch),-2.0f,2.0f);

      	input_L = ptr->input_L;
	output_L = ptr->output_L;
      	input_R = ptr->input_R;
	output_R = ptr->output_R;

	sample_rate = ptr->sample_rate;
	buflen_L = delaytime_L * sample_rate / 1000;
	buflen_R = delaytime_R * sample_rate / 1000;


	for (sample_index = 0; sample_index < SampleCount; sample_index++) {
		
		in_L = *(input_L++);
		in_R = *(input_R++);

		out_L = in_L * drylevel + ptr->mpx_out_L * strength_L;
		out_R = in_R * drylevel + ptr->mpx_out_R * strength_R;

		if (haas > 0.0f)
			in_R = 0.0f;

		if (mode <= 0.0f) {
			ptr->mpx_out_L = 
				M(push_buffer(in_L + ptr->mpx_out_L * feedback_L,
					      ptr->ringbuffer_L, buflen_L, ptr->buffer_pos_L));
			ptr->mpx_out_R =
				M(push_buffer(in_R + ptr->mpx_out_R * feedback_R,
					      ptr->ringbuffer_R, buflen_R, ptr->buffer_pos_R));
		} else {
			ptr->mpx_out_R =
				M(push_buffer(in_L + ptr->mpx_out_L * feedback_L,
					      ptr->ringbuffer_L, buflen_L, ptr->buffer_pos_L));
			ptr->mpx_out_L =
				M(push_buffer(in_R + ptr->mpx_out_R * feedback_R,
					      ptr->ringbuffer_R, buflen_R, ptr->buffer_pos_R));
		}

		if (rev_outch <= 0.0f) {
			*(output_L++) = out_L;
			*(output_R++) = out_R;
		} else {
			*(output_L++) = out_R;
			*(output_R++) = out_L;
		}
	}
}





void
set_run_adding_gain(LADSPA_Handle Instance, LADSPA_Data gain){

	Echo * ptr;

	ptr = (Echo *)Instance;

	ptr->run_adding_gain = gain;
}


void 
run_adding_gain_Echo(LADSPA_Handle Instance,
	 unsigned long SampleCount) {
	
	Echo * ptr;
	unsigned long sample_index;

	LADSPA_Data delaytime_L;
	LADSPA_Data delaytime_R;
	LADSPA_Data feedback_L;
	LADSPA_Data feedback_R;
	LADSPA_Data strength_L;
	LADSPA_Data strength_R;
	LADSPA_Data drylevel;
	LADSPA_Data mode;
	LADSPA_Data haas;
	LADSPA_Data rev_outch;

	LADSPA_Data * input_L;
	LADSPA_Data * output_L;
	LADSPA_Data * input_R;
	LADSPA_Data * output_R;

	unsigned long sample_rate;
	unsigned long buflen_L;
	unsigned long buflen_R;

	LADSPA_Data out_L = 0;
	LADSPA_Data out_R = 0;
	LADSPA_Data in_L = 0;
	LADSPA_Data in_R = 0;

	ptr = (Echo *)Instance;

	delaytime_L = LIMIT(*(ptr->delaytime_L),0.0f,2000.0f);
	delaytime_R = LIMIT(*(ptr->delaytime_R),0.0f,2000.0f);
	feedback_L = LIMIT(*(ptr->feedback_L) / 100.0, 0.0f, 100.0f);
	feedback_R = LIMIT(*(ptr->feedback_R) / 100.0, 0.0f, 100.0f);
        strength_L = db2lin(LIMIT(*(ptr->strength_L),-70.0f,10.0f));
	strength_R = db2lin(LIMIT(*(ptr->strength_R),-70.0f,10.0f));
	drylevel = db2lin(LIMIT(*(ptr->drylevel),-70.0f,10.0f));
	mode = LIMIT(*(ptr->mode),-2.0f,2.0f);
	haas = LIMIT(*(ptr->haas),-2.0f,2.0f);
	rev_outch = LIMIT(*(ptr->rev_outch),-2.0f,2.0f);

      	input_L = ptr->input_L;
	output_L = ptr->output_L;
      	input_R = ptr->input_R;
	output_R = ptr->output_R;

	sample_rate = ptr->sample_rate;
	buflen_L = delaytime_L * sample_rate / 1000;
	buflen_R = delaytime_R * sample_rate / 1000;


	for (sample_index = 0; sample_index < SampleCount; sample_index++) {
		
		in_L = *(input_L++);
		in_R = *(input_R++);

		out_L = in_L * drylevel + ptr->mpx_out_L * strength_L;
		out_R = in_R * drylevel + ptr->mpx_out_R * strength_R;

		if (haas > 0.0f)
			in_R = 0.0f;

		if (mode <= 0.0f) {
			ptr->mpx_out_L = 
				M(push_buffer(in_L + ptr->mpx_out_L * feedback_L,
					      ptr->ringbuffer_L, buflen_L, ptr->buffer_pos_L));
			ptr->mpx_out_R =
				M(push_buffer(in_R + ptr->mpx_out_R * feedback_R,
					      ptr->ringbuffer_R, buflen_R, ptr->buffer_pos_R));
		} else {
			ptr->mpx_out_R =
				M(push_buffer(in_L + ptr->mpx_out_L * feedback_L,
					      ptr->ringbuffer_L, buflen_L, ptr->buffer_pos_L));
			ptr->mpx_out_L =
				M(push_buffer(in_R + ptr->mpx_out_R * feedback_R,
					      ptr->ringbuffer_R, buflen_R, ptr->buffer_pos_R));
		}

		if (rev_outch <= 0.0f) {
			*(output_L++) += out_L * ptr->run_adding_gain;
			*(output_R++) += out_R * ptr->run_adding_gain;
		} else {
			*(output_L++) += out_R * ptr->run_adding_gain;
			*(output_R++) += out_L * ptr->run_adding_gain;
		}
	}
}



/* Throw away an Echo effect instance. */
void 
cleanup_Echo(LADSPA_Handle Instance) {

	Echo * ptr = (Echo *)Instance;

	free(ptr->ringbuffer_L);
	free(ptr->ringbuffer_R);
	free(ptr->buffer_pos_L);
	free(ptr->buffer_pos_R);

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
	

	/* init the stereo Echo */
	
	stereo_descriptor->UniqueID = ID_STEREO;
	stereo_descriptor->Label = strdup("tap_stereo_echo");
	stereo_descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
	stereo_descriptor->Name = strdup("TAP Stereo Echo");
	stereo_descriptor->Maker = strdup("Tom Szilagyi");
	stereo_descriptor->Copyright = strdup("GPL");
	stereo_descriptor->PortCount = PORTCOUNT_STEREO;

	if ((port_descriptors = 
	     (LADSPA_PortDescriptor *)calloc(PORTCOUNT_STEREO, sizeof(LADSPA_PortDescriptor))) == NULL)
		exit(1);		

	stereo_descriptor->PortDescriptors = (const LADSPA_PortDescriptor *)port_descriptors;
	port_descriptors[DELAYTIME_L] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[DELAYTIME_R] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[FEEDBACK_L] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[FEEDBACK_R] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[STRENGTH_L] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[STRENGTH_R] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[DRYLEVEL] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[MODE] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[HAAS] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[REV_OUTCH] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;

	port_descriptors[INPUT_L] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT_L] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
	port_descriptors[INPUT_R] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT_R] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;

	if ((port_names = 
	     (char **)calloc(PORTCOUNT_STEREO, sizeof(char *))) == NULL)
		exit(1);		

	stereo_descriptor->PortNames = (const char **)port_names;

	port_names[DELAYTIME_L] = strdup("L Delay [ms]");
	port_names[DELAYTIME_R] = strdup("R/Haas Delay [ms]");
	port_names[FEEDBACK_L] = strdup("L Feedback [%]");
	port_names[FEEDBACK_R] = strdup("R/Haas Feedback [%]");
	port_names[STRENGTH_L] = strdup("L Echo Level [dB]");
	port_names[STRENGTH_R] = strdup("R Echo Level [dB]");
	port_names[DRYLEVEL] = strdup("Dry Level [dB]");
	port_names[MODE] = strdup("Cross Mode");
	port_names[HAAS] = strdup("Haas Effect");
	port_names[REV_OUTCH] = strdup("Swap Outputs");

	port_names[INPUT_L] = strdup("Input Left");
	port_names[OUTPUT_L] = strdup("Output Left");
	port_names[INPUT_R] = strdup("Input Right");
	port_names[OUTPUT_R] = strdup("Output Right");

	if ((port_range_hints = 
	     ((LADSPA_PortRangeHint *)calloc(PORTCOUNT_STEREO, sizeof(LADSPA_PortRangeHint)))) == NULL)
		exit(1);		

	stereo_descriptor->PortRangeHints = (const LADSPA_PortRangeHint *)port_range_hints;

	port_range_hints[DELAYTIME_L].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_100);
	port_range_hints[DELAYTIME_L].LowerBound = 0;
	port_range_hints[DELAYTIME_L].UpperBound = MAX_DELAY;

	port_range_hints[DELAYTIME_R].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_100);
	port_range_hints[DELAYTIME_R].LowerBound = 0;
	port_range_hints[DELAYTIME_R].UpperBound = MAX_DELAY;

	port_range_hints[FEEDBACK_L].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[FEEDBACK_L].LowerBound = 0;
	port_range_hints[FEEDBACK_L].UpperBound = 100;

	port_range_hints[FEEDBACK_R].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[FEEDBACK_R].LowerBound = 0;
	port_range_hints[FEEDBACK_R].UpperBound = 100;

	port_range_hints[STRENGTH_L].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[STRENGTH_L].LowerBound = -70;
	port_range_hints[STRENGTH_L].UpperBound = 10;

	port_range_hints[STRENGTH_R].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[STRENGTH_R].LowerBound = -70;
	port_range_hints[STRENGTH_R].UpperBound = 10;

	port_range_hints[MODE].HintDescriptor = 
		(LADSPA_HINT_TOGGLED |
		 LADSPA_HINT_DEFAULT_0);

	port_range_hints[HAAS].HintDescriptor = 
		(LADSPA_HINT_TOGGLED |
		 LADSPA_HINT_DEFAULT_0);

	port_range_hints[REV_OUTCH].HintDescriptor = 
		(LADSPA_HINT_TOGGLED |
		 LADSPA_HINT_DEFAULT_0);

	port_range_hints[DRYLEVEL].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[DRYLEVEL].LowerBound = -70;
	port_range_hints[DRYLEVEL].UpperBound = 10;


	port_range_hints[INPUT_L].HintDescriptor = 0;
	port_range_hints[OUTPUT_L].HintDescriptor = 0;
	port_range_hints[INPUT_R].HintDescriptor = 0;
	port_range_hints[OUTPUT_R].HintDescriptor = 0;


	stereo_descriptor->instantiate = instantiate_Echo;
	stereo_descriptor->connect_port = connect_port_Echo;
	stereo_descriptor->activate = activate_Echo;
	stereo_descriptor->run = run_Echo;
	stereo_descriptor->run_adding = run_adding_gain_Echo;
	stereo_descriptor->set_run_adding_gain = set_run_adding_gain;
	stereo_descriptor->deactivate = NULL;
	stereo_descriptor->cleanup = cleanup_Echo;
	
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

const 
LADSPA_Descriptor * 
ladspa_descriptor(unsigned long Index) {

	switch (Index) {
	case 0:
		return stereo_descriptor;
	default:
		return NULL;
	}
}
