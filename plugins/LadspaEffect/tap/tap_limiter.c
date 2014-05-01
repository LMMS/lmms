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

    $Id: tap_limiter.c,v 1.5 2004/02/21 17:33:36 tszilagyi Exp $
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <ladspa.h>
#include "tap_utils.h"

/* The Unique ID of the plugin: */

#define ID_MONO         2145

/* The port numbers for the plugin: */

#define LIMIT_VOL       0
#define OUT_VOL         1
#define LATENCY         2
#define INPUT           3
#define OUTPUT          4

/* Total number of ports */

#define PORTCOUNT_MONO   5


/* Size of a ringbuffer that must be large enough to hold audio
 * between two zero-crosses in any case (or you'll hear
 * distortion). 40 Hz sound at 192kHz yields a half-period of 2400
 * samples, so this should be enough.
 */
#define RINGBUF_SIZE 2500


/* The structure used to hold port connection information and state */

typedef struct {
	LADSPA_Data * limit_vol;
	LADSPA_Data * out_vol;
	LADSPA_Data * latency;
	LADSPA_Data * input;
	LADSPA_Data * output;

	LADSPA_Data * ringbuffer;
	unsigned long buflen;
	unsigned long pos;
	unsigned long ready_num;

	unsigned long sample_rate;
	LADSPA_Data run_adding_gain;
} Limiter;




/* Construct a new plugin instance. */
LADSPA_Handle 
instantiate_Limiter(const LADSPA_Descriptor * Descriptor,
		    unsigned long             sample_rate) {
	
	LADSPA_Handle * ptr;
	
	if ((ptr = malloc(sizeof(Limiter))) != NULL) {
		((Limiter *)ptr)->sample_rate = sample_rate;
		((Limiter *)ptr)->run_adding_gain = 1.0f;

		if ((((Limiter *)ptr)->ringbuffer = 
		     calloc(RINGBUF_SIZE, sizeof(LADSPA_Data))) == NULL)
			return NULL;

		/* 80 Hz is the lowest frequency with which zero-crosses were
		 * observed to occur (this corresponds to 40 Hz signal frequency).
		 */
		((Limiter *)ptr)->buflen = ((Limiter *)ptr)->sample_rate / 80;

		((Limiter *)ptr)->pos = 0;
		((Limiter *)ptr)->ready_num = 0;

		return ptr;
	}
       	return NULL;
}


void
activate_Limiter(LADSPA_Handle Instance) {

	Limiter * ptr = (Limiter *)Instance;
	unsigned long i;

	for (i = 0; i < RINGBUF_SIZE; i++)
		ptr->ringbuffer[i] = 0.0f;
}





/* Connect a port to a data location. */
void 
connect_port_Limiter(LADSPA_Handle Instance,
		     unsigned long Port,
		     LADSPA_Data * DataLocation) {
	
	Limiter * ptr = (Limiter *)Instance;

	switch (Port) {
	case LIMIT_VOL:
		ptr->limit_vol = DataLocation;
		break;
	case OUT_VOL:
		ptr->out_vol = DataLocation;
		break;
	case LATENCY:
		ptr->latency = DataLocation;
		*(ptr->latency) = ptr->buflen;  /* IS THIS LEGAL? */
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
run_Limiter(LADSPA_Handle Instance,
	    unsigned long SampleCount) {
  
	Limiter * ptr = (Limiter *)Instance;

	LADSPA_Data * input = ptr->input;
	LADSPA_Data * output = ptr->output;
	LADSPA_Data limit_vol = db2lin(LIMIT(*(ptr->limit_vol),-30.0f,20.0f));
	LADSPA_Data out_vol = db2lin(LIMIT(*(ptr->out_vol),-30.0f,20.0f));
	unsigned long sample_index;
	unsigned long sample_count = SampleCount;
	unsigned long index_offs = 0;
	unsigned long i;
	LADSPA_Data max_value = 0;
	LADSPA_Data section_gain = 0;
	unsigned long run_length;
	unsigned long total_length = 0;


	while (total_length < sample_count) {

		run_length = ptr->buflen;
		if (total_length + run_length > sample_count)
			run_length = sample_count - total_length;
		
		while (ptr->ready_num < run_length) {
			if (read_buffer(ptr->ringbuffer, ptr->buflen,
					ptr->pos, ptr->ready_num) >= 0.0f) {
				index_offs = 0;
				while ((read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos,
						    ptr->ready_num + index_offs) >= 0.0f) &&
				       (ptr->ready_num + index_offs < run_length)) {
					index_offs++;
				}
			} else {
				index_offs = 0;
				while ((read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos,
						    ptr->ready_num + index_offs) <= 0.0f) &&
				       (ptr->ready_num + index_offs < run_length)) {
					index_offs++;
				}
			}
			
			/* search for max value in scanned halfcycle */
			max_value = 0;
			for (i = ptr->ready_num; i < ptr->ready_num + index_offs; i++) {
				if (fabs(read_buffer(ptr->ringbuffer, ptr->buflen,
						     ptr->pos, i)) > max_value)
					max_value = fabs(read_buffer(ptr->ringbuffer,
								     ptr->buflen, ptr->pos, i));
			}
			section_gain = limit_vol / max_value;
			if (max_value > limit_vol)
				for (i = ptr->ready_num; i < ptr->ready_num + index_offs; i++) {
					write_buffer(read_buffer(ptr->ringbuffer, ptr->buflen,
								 ptr->pos, i) * section_gain,
						     ptr->ringbuffer, ptr->buflen, ptr->pos, i);
				}
			ptr->ready_num += index_offs;			
		}
		
		/* push run_length values out of ringbuffer, feed with input */
		for (sample_index = 0; sample_index < run_length; sample_index++) {
			*(output++) = out_vol * 
				push_buffer(*(input++), ptr->ringbuffer,
					    ptr->buflen, &(ptr->pos));
		}
		ptr->ready_num -= run_length;
		total_length += run_length;
	}	
	*(ptr->latency) = ptr->buflen;
}



void
set_run_adding_gain_Limiter(LADSPA_Handle Instance, LADSPA_Data gain) {

	Limiter * ptr = (Limiter *)Instance;

	ptr->run_adding_gain = gain;
}



void 
run_adding_Limiter(LADSPA_Handle Instance,
		   unsigned long SampleCount) {
  
	Limiter * ptr = (Limiter *)Instance;

	LADSPA_Data * input = ptr->input;
	LADSPA_Data * output = ptr->output;
	LADSPA_Data limit_vol = db2lin(LIMIT(*(ptr->limit_vol),-30.0f,20.0f));
	LADSPA_Data out_vol = db2lin(LIMIT(*(ptr->out_vol),-30.0f,20.0f));
	unsigned long sample_index;
	unsigned long sample_count = SampleCount;
	unsigned long index_offs = 0;
	unsigned long i;
	LADSPA_Data max_value = 0;
	LADSPA_Data section_gain = 0;
	unsigned long run_length;
	unsigned long total_length = 0;


	while (total_length < sample_count) {

		run_length = ptr->buflen;
		if (total_length + run_length > sample_count)
			run_length = sample_count - total_length;
		
		while (ptr->ready_num < run_length) {
			if (read_buffer(ptr->ringbuffer, ptr->buflen,
					ptr->pos, ptr->ready_num) >= 0.0f) {
				index_offs = 0;
				while ((read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos,
						    ptr->ready_num + index_offs) >= 0.0f) &&
				       (ptr->ready_num + index_offs < run_length)) {
					index_offs++;
				}
			} else {
				index_offs = 0;
				while ((read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos,
						    ptr->ready_num + index_offs) <= 0.0f) &&
				       (ptr->ready_num + index_offs < run_length)) {
					index_offs++;
				}
			}
			
			/* search for max value in scanned halfcycle */
			max_value = 0;
			for (i = ptr->ready_num; i < ptr->ready_num + index_offs; i++) {
				if (fabs(read_buffer(ptr->ringbuffer, ptr->buflen,
						     ptr->pos, i)) > max_value)
					max_value = fabs(read_buffer(ptr->ringbuffer,
								     ptr->buflen, ptr->pos, i));
			}
			section_gain = limit_vol / max_value;
			if (max_value > limit_vol)
				for (i = ptr->ready_num; i < ptr->ready_num + index_offs; i++) {
					write_buffer(read_buffer(ptr->ringbuffer, ptr->buflen,
								 ptr->pos, i) * section_gain,
						     ptr->ringbuffer, ptr->buflen, ptr->pos, i);
				}
			ptr->ready_num += index_offs;			
		}
		
		/* push run_length values out of ringbuffer, feed with input */
		for (sample_index = 0; sample_index < run_length; sample_index++) {
			*(output++) += ptr->run_adding_gain * out_vol * 
				push_buffer(*(input++), ptr->ringbuffer,
					    ptr->buflen, &(ptr->pos));
		}
		ptr->ready_num -= run_length;
		total_length += run_length;
	}	
	*(ptr->latency) = ptr->buflen;
}




/* Throw away a Limiter effect instance. */
void 
cleanup_Limiter(LADSPA_Handle Instance) {

	Limiter * ptr = (Limiter *)Instance;
	free(ptr->ringbuffer);
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
	mono_descriptor->Label = strdup("tap_limiter");
	mono_descriptor->Properties = 0;
	mono_descriptor->Name = strdup("TAP Scaling Limiter");
	mono_descriptor->Maker = strdup("Tom Szilagyi");
	mono_descriptor->Copyright = strdup("GPL");
	mono_descriptor->PortCount = PORTCOUNT_MONO;

	if ((port_descriptors =
	     (LADSPA_PortDescriptor *)calloc(PORTCOUNT_MONO, sizeof(LADSPA_PortDescriptor))) == NULL)
		exit(1);

	mono_descriptor->PortDescriptors = (const LADSPA_PortDescriptor *)port_descriptors;
	port_descriptors[LIMIT_VOL] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[OUT_VOL] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[LATENCY] = LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
	port_descriptors[INPUT] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;

	if ((port_names = 
	     (char **)calloc(PORTCOUNT_MONO, sizeof(char *))) == NULL)
		exit(1);

	mono_descriptor->PortNames = (const char **)port_names;
	port_names[LIMIT_VOL] = strdup("Limit Level [dB]");
	port_names[OUT_VOL] = strdup("Output Volume [dB]");
	port_names[LATENCY] = strdup("latency");
	port_names[INPUT] = strdup("Input");
	port_names[OUTPUT] = strdup("Output");

	if ((port_range_hints = 
	     ((LADSPA_PortRangeHint *)calloc(PORTCOUNT_MONO, sizeof(LADSPA_PortRangeHint)))) == NULL)
		exit(1);

	mono_descriptor->PortRangeHints	= (const LADSPA_PortRangeHint *)port_range_hints;
	port_range_hints[LIMIT_VOL].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[OUT_VOL].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[LATENCY].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_MAXIMUM);
	port_range_hints[LIMIT_VOL].LowerBound = -30;
	port_range_hints[LIMIT_VOL].UpperBound = +20;
	port_range_hints[OUT_VOL].LowerBound = -30;
	port_range_hints[OUT_VOL].UpperBound = +20;
	port_range_hints[LATENCY].LowerBound = 0;
	port_range_hints[LATENCY].UpperBound = RINGBUF_SIZE + 0.1f;
	port_range_hints[INPUT].HintDescriptor = 0;
	port_range_hints[OUTPUT].HintDescriptor = 0;
	mono_descriptor->instantiate = instantiate_Limiter;
	mono_descriptor->connect_port = connect_port_Limiter;
	mono_descriptor->activate = activate_Limiter;
	mono_descriptor->run = run_Limiter;
	mono_descriptor->run_adding = run_adding_Limiter;
	mono_descriptor->set_run_adding_gain = set_run_adding_gain_Limiter;
	mono_descriptor->deactivate = NULL;
	mono_descriptor->cleanup = cleanup_Limiter;
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
