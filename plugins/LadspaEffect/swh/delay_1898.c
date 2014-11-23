#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include "config.h"
#endif

#ifdef ENABLE_NLS
#include <libintl.h>
#endif

#define         _ISOC9X_SOURCE  1
#define         _ISOC99_SOURCE  1
#define         __USE_ISOC99    1
#define         __USE_ISOC9X    1

#include <math.h>

#include "ladspa.h"

#ifdef WIN32
#define _WINDOWS_DLL_EXPORT_ __declspec(dllexport)
int bIsFirstTime = 1; 
void __attribute__((constructor)) swh_init(); // forward declaration
#else
#define _WINDOWS_DLL_EXPORT_ 
#endif


#include "ladspa-util.h"

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define CALC_DELAY(delaytime) \
  (f_clamp (delaytime * sample_rate, 1.f, (float)(buffer_mask + 1)))

#define DELAY_N_IN                     0
#define DELAY_N_OUT                    1
#define DELAY_N_MAX_DELAY              2
#define DELAY_N_DELAY_TIME             3
#define DELAY_L_IN                     0
#define DELAY_L_OUT                    1
#define DELAY_L_MAX_DELAY              2
#define DELAY_L_DELAY_TIME             3
#define DELAY_C_IN                     0
#define DELAY_C_OUT                    1
#define DELAY_C_MAX_DELAY              2
#define DELAY_C_DELAY_TIME             3

static LADSPA_Descriptor *delay_nDescriptor = NULL;

typedef struct {
	LADSPA_Data *in;
	LADSPA_Data *out;
	LADSPA_Data *max_delay;
	LADSPA_Data *delay_time;
	LADSPA_Data *buffer;
	unsigned int buffer_mask;
	LADSPA_Data  delay_samples;
	LADSPA_Data  last_delay_time;
	unsigned int sample_rate;
	long         write_phase;
	LADSPA_Data run_adding_gain;
} Delay_n;

static LADSPA_Descriptor *delay_lDescriptor = NULL;

typedef struct {
	LADSPA_Data *in;
	LADSPA_Data *out;
	LADSPA_Data *max_delay;
	LADSPA_Data *delay_time;
	LADSPA_Data *buffer;
	unsigned int buffer_mask;
	LADSPA_Data  delay_samples;
	LADSPA_Data  last_delay_time;
	unsigned int sample_rate;
	long         write_phase;
	LADSPA_Data run_adding_gain;
} Delay_l;

static LADSPA_Descriptor *delay_cDescriptor = NULL;

typedef struct {
	LADSPA_Data *in;
	LADSPA_Data *out;
	LADSPA_Data *max_delay;
	LADSPA_Data *delay_time;
	LADSPA_Data *buffer;
	unsigned int buffer_mask;
	LADSPA_Data  delay_samples;
	LADSPA_Data  last_delay_time;
	unsigned int sample_rate;
	long         write_phase;
	LADSPA_Data run_adding_gain;
} Delay_c;

_WINDOWS_DLL_EXPORT_
const LADSPA_Descriptor *ladspa_descriptor(unsigned long index) {

#ifdef WIN32
	if (bIsFirstTime) {
		swh_init();
		bIsFirstTime = 0;
	}
#endif
	switch (index) {
	case 0:
		return delay_nDescriptor;
	case 1:
		return delay_lDescriptor;
	case 2:
		return delay_cDescriptor;
	default:
		return NULL;
	}
}

static void activateDelay_n(LADSPA_Handle instance) {
	Delay_n *plugin_data = (Delay_n *)instance;
	LADSPA_Data *buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	LADSPA_Data delay_samples = plugin_data->delay_samples;
	LADSPA_Data last_delay_time = plugin_data->last_delay_time;
	unsigned int sample_rate = plugin_data->sample_rate;
	long write_phase = plugin_data->write_phase;
	unsigned int minsize, size;
    
	if (plugin_data->max_delay && *plugin_data->max_delay > 0)
	  minsize = sample_rate * *plugin_data->max_delay;
	else if (plugin_data->delay_time)
	  minsize = sample_rate * *plugin_data->delay_time;
	else
	  minsize = sample_rate; /* 1 second default */
    
	size = 1;
	while (size < minsize) size <<= 1;
    
	/* calloc sets the buffer to zero. */
	buffer = calloc(size, sizeof(LADSPA_Data));
	if (buffer)
	  buffer_mask = size - 1;
	else
	  buffer_mask = 0;
	write_phase = 0;
	plugin_data->buffer = buffer;
	plugin_data->buffer_mask = buffer_mask;
	plugin_data->delay_samples = delay_samples;
	plugin_data->last_delay_time = last_delay_time;
	plugin_data->sample_rate = sample_rate;
	plugin_data->write_phase = write_phase;

}

static void cleanupDelay_n(LADSPA_Handle instance) {
	Delay_n *plugin_data = (Delay_n *)instance;
	free(plugin_data->buffer);
	free(instance);
}

static void connectPortDelay_n(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Delay_n *plugin;

	plugin = (Delay_n *)instance;
	switch (port) {
	case DELAY_N_IN:
		plugin->in = data;
		break;
	case DELAY_N_OUT:
		plugin->out = data;
		break;
	case DELAY_N_MAX_DELAY:
		plugin->max_delay = data;
		break;
	case DELAY_N_DELAY_TIME:
		plugin->delay_time = data;
		break;
	}
}

static LADSPA_Handle instantiateDelay_n(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Delay_n *plugin_data = (Delay_n *)malloc(sizeof(Delay_n));
	LADSPA_Data *buffer = NULL;
	unsigned int buffer_mask = 0;
	LADSPA_Data delay_samples = 0;
	LADSPA_Data last_delay_time = 0;
	unsigned int sample_rate = 0;
	long write_phase = 0;

	sample_rate = s_rate;

	plugin_data->buffer = buffer;
	plugin_data->buffer_mask = buffer_mask;
	plugin_data->delay_samples = delay_samples;
	plugin_data->last_delay_time = last_delay_time;
	plugin_data->sample_rate = sample_rate;
	plugin_data->write_phase = write_phase;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runDelay_n(LADSPA_Handle instance, unsigned long sample_count) {
	Delay_n *plugin_data = (Delay_n *)instance;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const in = plugin_data->in;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const out = plugin_data->out;

	/* Delay Time (s) (float value) */
	const LADSPA_Data delay_time = *(plugin_data->delay_time);
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	LADSPA_Data delay_samples = plugin_data->delay_samples;
	LADSPA_Data last_delay_time = plugin_data->last_delay_time;
	unsigned int sample_rate = plugin_data->sample_rate;
	long write_phase = plugin_data->write_phase;

	unsigned int i;

	if (write_phase == 0) {
	  plugin_data->last_delay_time = delay_time;
	  plugin_data->delay_samples = delay_samples = CALC_DELAY (delay_time);
	}
	
	if (delay_time == last_delay_time) {
	  long read_phase = write_phase - (long)delay_samples;
	  LADSPA_Data *readptr = buffer + (read_phase & buffer_mask);
	  LADSPA_Data *writeptr = buffer + (write_phase & buffer_mask);
	  LADSPA_Data *lastptr = buffer + buffer_mask + 1;

	  long remain = sample_count;

	  while (remain) {
	    long read_space = lastptr - readptr;
	    long write_space = lastptr - writeptr;
	    long to_process = MIN (MIN (read_space, remain), write_space);

	    if (to_process == 0)
	      return; // buffer not allocated.

	    remain -= to_process;

	    for (i=0; i<to_process; i++) {
	      float read = *(readptr++);
	      *(writeptr++) = in[i];
	      buffer_write(out[i], read);
	    }

	    if (readptr == lastptr) readptr = buffer;
	    if (writeptr == lastptr) writeptr = buffer;
	  }

	  write_phase += sample_count;
	} else {
	  float next_delay_samples = CALC_DELAY (delay_time);
	  float delay_samples_slope = (next_delay_samples - delay_samples) / sample_count;

	  for (i=0; i<sample_count; i++) {
	    long read_phase;
	    LADSPA_Data read;

	    delay_samples += delay_samples_slope;
	    write_phase++;
	    read_phase = write_phase - (long)delay_samples;

	    read = buffer[read_phase & buffer_mask];
	    buffer[write_phase & buffer_mask] = in[i];
	    buffer_write(out[i], read);
	  }

	  plugin_data->last_delay_time = delay_time;
	  plugin_data->delay_samples = delay_samples;
	}
	
	plugin_data->write_phase = write_phase;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainDelay_n(LADSPA_Handle instance, LADSPA_Data gain) {
	((Delay_n *)instance)->run_adding_gain = gain;
}

static void runAddingDelay_n(LADSPA_Handle instance, unsigned long sample_count) {
	Delay_n *plugin_data = (Delay_n *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const in = plugin_data->in;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const out = plugin_data->out;

	/* Delay Time (s) (float value) */
	const LADSPA_Data delay_time = *(plugin_data->delay_time);
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	LADSPA_Data delay_samples = plugin_data->delay_samples;
	LADSPA_Data last_delay_time = plugin_data->last_delay_time;
	unsigned int sample_rate = plugin_data->sample_rate;
	long write_phase = plugin_data->write_phase;

	unsigned int i;

	if (write_phase == 0) {
	  plugin_data->last_delay_time = delay_time;
	  plugin_data->delay_samples = delay_samples = CALC_DELAY (delay_time);
	}
	
	if (delay_time == last_delay_time) {
	  long read_phase = write_phase - (long)delay_samples;
	  LADSPA_Data *readptr = buffer + (read_phase & buffer_mask);
	  LADSPA_Data *writeptr = buffer + (write_phase & buffer_mask);
	  LADSPA_Data *lastptr = buffer + buffer_mask + 1;

	  long remain = sample_count;

	  while (remain) {
	    long read_space = lastptr - readptr;
	    long write_space = lastptr - writeptr;
	    long to_process = MIN (MIN (read_space, remain), write_space);

	    if (to_process == 0)
	      return; // buffer not allocated.

	    remain -= to_process;

	    for (i=0; i<to_process; i++) {
	      float read = *(readptr++);
	      *(writeptr++) = in[i];
	      buffer_write(out[i], read);
	    }

	    if (readptr == lastptr) readptr = buffer;
	    if (writeptr == lastptr) writeptr = buffer;
	  }

	  write_phase += sample_count;
	} else {
	  float next_delay_samples = CALC_DELAY (delay_time);
	  float delay_samples_slope = (next_delay_samples - delay_samples) / sample_count;

	  for (i=0; i<sample_count; i++) {
	    long read_phase;
	    LADSPA_Data read;

	    delay_samples += delay_samples_slope;
	    write_phase++;
	    read_phase = write_phase - (long)delay_samples;

	    read = buffer[read_phase & buffer_mask];
	    buffer[write_phase & buffer_mask] = in[i];
	    buffer_write(out[i], read);
	  }

	  plugin_data->last_delay_time = delay_time;
	  plugin_data->delay_samples = delay_samples;
	}
	
	plugin_data->write_phase = write_phase;
}

static void activateDelay_l(LADSPA_Handle instance) {
	Delay_l *plugin_data = (Delay_l *)instance;
	LADSPA_Data *buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	LADSPA_Data delay_samples = plugin_data->delay_samples;
	LADSPA_Data last_delay_time = plugin_data->last_delay_time;
	unsigned int sample_rate = plugin_data->sample_rate;
	long write_phase = plugin_data->write_phase;
	unsigned int minsize, size;
    
	if (plugin_data->max_delay && *plugin_data->max_delay > 0)
	  minsize = sample_rate * *plugin_data->max_delay;
	else if (plugin_data->delay_time)
	  minsize = sample_rate * *plugin_data->delay_time;
	else
	  minsize = sample_rate; /* 1 second default */
    
	size = 1;
	while (size < minsize) size <<= 1;
    
	/* calloc sets the buffer to zero. */
	buffer = calloc(size, sizeof(LADSPA_Data));
	if (buffer)
	  buffer_mask = size - 1;
	else
	  buffer_mask = 0;
	write_phase = 0;
	plugin_data->buffer = buffer;
	plugin_data->buffer_mask = buffer_mask;
	plugin_data->delay_samples = delay_samples;
	plugin_data->last_delay_time = last_delay_time;
	plugin_data->sample_rate = sample_rate;
	plugin_data->write_phase = write_phase;

}

static void cleanupDelay_l(LADSPA_Handle instance) {
	Delay_l *plugin_data = (Delay_l *)instance;
	free(plugin_data->buffer);
	free(instance);
}

static void connectPortDelay_l(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Delay_l *plugin;

	plugin = (Delay_l *)instance;
	switch (port) {
	case DELAY_L_IN:
		plugin->in = data;
		break;
	case DELAY_L_OUT:
		plugin->out = data;
		break;
	case DELAY_L_MAX_DELAY:
		plugin->max_delay = data;
		break;
	case DELAY_L_DELAY_TIME:
		plugin->delay_time = data;
		break;
	}
}

static LADSPA_Handle instantiateDelay_l(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Delay_l *plugin_data = (Delay_l *)malloc(sizeof(Delay_l));
	LADSPA_Data *buffer = NULL;
	unsigned int buffer_mask = 0;
	LADSPA_Data delay_samples = 0;
	LADSPA_Data last_delay_time = 0;
	unsigned int sample_rate = 0;
	long write_phase = 0;

	sample_rate = s_rate;

	plugin_data->buffer = buffer;
	plugin_data->buffer_mask = buffer_mask;
	plugin_data->delay_samples = delay_samples;
	plugin_data->last_delay_time = last_delay_time;
	plugin_data->sample_rate = sample_rate;
	plugin_data->write_phase = write_phase;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runDelay_l(LADSPA_Handle instance, unsigned long sample_count) {
	Delay_l *plugin_data = (Delay_l *)instance;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const in = plugin_data->in;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const out = plugin_data->out;

	/* Delay Time (s) (float value) */
	const LADSPA_Data delay_time = *(plugin_data->delay_time);
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	LADSPA_Data delay_samples = plugin_data->delay_samples;
	LADSPA_Data last_delay_time = plugin_data->last_delay_time;
	unsigned int sample_rate = plugin_data->sample_rate;
	long write_phase = plugin_data->write_phase;

	unsigned int i;

	if (write_phase == 0) {
	  plugin_data->last_delay_time = delay_time;
	  plugin_data->delay_samples = delay_samples = CALC_DELAY (delay_time);
	}
	
	if (delay_time == last_delay_time) {
	  long idelay_samples = (long)delay_samples;
	  LADSPA_Data frac = delay_samples - idelay_samples;

	  for (i=0; i<sample_count; i++) {
	    long read_phase = write_phase - (long)delay_samples;
	    LADSPA_Data read;
	    read = LIN_INTERP (frac,
	                           buffer[(read_phase-1) & buffer_mask],
	                           buffer[read_phase & buffer_mask]);
	    buffer[write_phase & buffer_mask] = in[i];
	    buffer_write(out[i], read);
	    write_phase++;
	  }
	} else {
	  float next_delay_samples = CALC_DELAY (delay_time);
	  float delay_samples_slope = (next_delay_samples - delay_samples) / sample_count;

	  for (i=0; i<sample_count; i++) {
	    long read_phase, idelay_samples;
	    LADSPA_Data frac, read;

	    delay_samples += delay_samples_slope;
	    write_phase++;
	    read_phase = write_phase - (long)delay_samples;
	    idelay_samples = (long)delay_samples;
	    frac = delay_samples - idelay_samples;
	    read = LIN_INTERP (frac,
	                       buffer[(read_phase-1) & buffer_mask],
	                       buffer[read_phase & buffer_mask]); 
	    buffer[write_phase & buffer_mask] = in[i];
	    buffer_write(out[i], read);
	  }

	  plugin_data->last_delay_time = delay_time;
	  plugin_data->delay_samples = delay_samples;
	}
	
	plugin_data->write_phase = write_phase;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainDelay_l(LADSPA_Handle instance, LADSPA_Data gain) {
	((Delay_l *)instance)->run_adding_gain = gain;
}

static void runAddingDelay_l(LADSPA_Handle instance, unsigned long sample_count) {
	Delay_l *plugin_data = (Delay_l *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const in = plugin_data->in;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const out = plugin_data->out;

	/* Delay Time (s) (float value) */
	const LADSPA_Data delay_time = *(plugin_data->delay_time);
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	LADSPA_Data delay_samples = plugin_data->delay_samples;
	LADSPA_Data last_delay_time = plugin_data->last_delay_time;
	unsigned int sample_rate = plugin_data->sample_rate;
	long write_phase = plugin_data->write_phase;

	unsigned int i;

	if (write_phase == 0) {
	  plugin_data->last_delay_time = delay_time;
	  plugin_data->delay_samples = delay_samples = CALC_DELAY (delay_time);
	}
	
	if (delay_time == last_delay_time) {
	  long idelay_samples = (long)delay_samples;
	  LADSPA_Data frac = delay_samples - idelay_samples;

	  for (i=0; i<sample_count; i++) {
	    long read_phase = write_phase - (long)delay_samples;
	    LADSPA_Data read;
	    read = LIN_INTERP (frac,
	                           buffer[(read_phase-1) & buffer_mask],
	                           buffer[read_phase & buffer_mask]);
	    buffer[write_phase & buffer_mask] = in[i];
	    buffer_write(out[i], read);
	    write_phase++;
	  }
	} else {
	  float next_delay_samples = CALC_DELAY (delay_time);
	  float delay_samples_slope = (next_delay_samples - delay_samples) / sample_count;

	  for (i=0; i<sample_count; i++) {
	    long read_phase, idelay_samples;
	    LADSPA_Data frac, read;

	    delay_samples += delay_samples_slope;
	    write_phase++;
	    read_phase = write_phase - (long)delay_samples;
	    idelay_samples = (long)delay_samples;
	    frac = delay_samples - idelay_samples;
	    read = LIN_INTERP (frac,
	                       buffer[(read_phase-1) & buffer_mask],
	                       buffer[read_phase & buffer_mask]); 
	    buffer[write_phase & buffer_mask] = in[i];
	    buffer_write(out[i], read);
	  }

	  plugin_data->last_delay_time = delay_time;
	  plugin_data->delay_samples = delay_samples;
	}
	
	plugin_data->write_phase = write_phase;
}

static void activateDelay_c(LADSPA_Handle instance) {
	Delay_c *plugin_data = (Delay_c *)instance;
	LADSPA_Data *buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	LADSPA_Data delay_samples = plugin_data->delay_samples;
	LADSPA_Data last_delay_time = plugin_data->last_delay_time;
	unsigned int sample_rate = plugin_data->sample_rate;
	long write_phase = plugin_data->write_phase;
	unsigned int minsize, size;
    
	if (plugin_data->max_delay && *plugin_data->max_delay > 0)
	  minsize = sample_rate * *plugin_data->max_delay;
	else if (plugin_data->delay_time)
	  minsize = sample_rate * *plugin_data->delay_time;
	else
	  minsize = sample_rate; /* 1 second default */
    
	size = 1;
	while (size < minsize) size <<= 1;
    
	/* calloc sets the buffer to zero. */
	buffer = calloc(size, sizeof(LADSPA_Data));
	if (buffer)
	  buffer_mask = size - 1;
	else
	  buffer_mask = 0;
	write_phase = 0;
	plugin_data->buffer = buffer;
	plugin_data->buffer_mask = buffer_mask;
	plugin_data->delay_samples = delay_samples;
	plugin_data->last_delay_time = last_delay_time;
	plugin_data->sample_rate = sample_rate;
	plugin_data->write_phase = write_phase;

}

static void cleanupDelay_c(LADSPA_Handle instance) {
	Delay_c *plugin_data = (Delay_c *)instance;
	free(plugin_data->buffer);
	free(instance);
}

static void connectPortDelay_c(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Delay_c *plugin;

	plugin = (Delay_c *)instance;
	switch (port) {
	case DELAY_C_IN:
		plugin->in = data;
		break;
	case DELAY_C_OUT:
		plugin->out = data;
		break;
	case DELAY_C_MAX_DELAY:
		plugin->max_delay = data;
		break;
	case DELAY_C_DELAY_TIME:
		plugin->delay_time = data;
		break;
	}
}

static LADSPA_Handle instantiateDelay_c(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Delay_c *plugin_data = (Delay_c *)malloc(sizeof(Delay_c));
	LADSPA_Data *buffer = NULL;
	unsigned int buffer_mask = 0;
	LADSPA_Data delay_samples = 0;
	LADSPA_Data last_delay_time = 0;
	unsigned int sample_rate = 0;
	long write_phase = 0;

	sample_rate = s_rate;

	plugin_data->buffer = buffer;
	plugin_data->buffer_mask = buffer_mask;
	plugin_data->delay_samples = delay_samples;
	plugin_data->last_delay_time = last_delay_time;
	plugin_data->sample_rate = sample_rate;
	plugin_data->write_phase = write_phase;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runDelay_c(LADSPA_Handle instance, unsigned long sample_count) {
	Delay_c *plugin_data = (Delay_c *)instance;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const in = plugin_data->in;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const out = plugin_data->out;

	/* Delay Time (s) (float value) */
	const LADSPA_Data delay_time = *(plugin_data->delay_time);
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	LADSPA_Data delay_samples = plugin_data->delay_samples;
	LADSPA_Data last_delay_time = plugin_data->last_delay_time;
	unsigned int sample_rate = plugin_data->sample_rate;
	long write_phase = plugin_data->write_phase;

	unsigned int i;

	if (write_phase == 0) {
	  plugin_data->last_delay_time = delay_time;
	  plugin_data->delay_samples = delay_samples = CALC_DELAY (delay_time);
	}
	
	if (delay_time == last_delay_time) {
	  long idelay_samples = (long)delay_samples;
	  LADSPA_Data frac = delay_samples - idelay_samples;

	  for (i=0; i<sample_count; i++) {
	    long read_phase = write_phase - (long)delay_samples;
	    LADSPA_Data read = cube_interp (frac,
	                                    buffer[(read_phase-1) & buffer_mask], 
	                                    buffer[read_phase & buffer_mask], 
	                                    buffer[(read_phase+1) & buffer_mask], 
	                                    buffer[(read_phase+2) & buffer_mask]);
	    buffer[write_phase++ & buffer_mask] = in[i];
	    buffer_write(out[i], read);
	  }
	} else {
	  float next_delay_samples = CALC_DELAY (delay_time);
	  float delay_samples_slope = (next_delay_samples - delay_samples) / sample_count;

	  for (i=0; i<sample_count; i++) {
	    long read_phase, idelay_samples;
	    LADSPA_Data frac, read;

	    delay_samples += delay_samples_slope;
	    write_phase++;
	    read_phase = write_phase - (long)delay_samples;
	    idelay_samples = (long)delay_samples;
	    frac = delay_samples - idelay_samples;
	    read = cube_interp (frac,
	                        buffer[(read_phase-1) & buffer_mask], 
	                        buffer[read_phase & buffer_mask], 
	                        buffer[(read_phase+1) & buffer_mask], 
	                        buffer[(read_phase+2) & buffer_mask]);
	    buffer[write_phase & buffer_mask] = in[i];
	    buffer_write(out[i], read);
	  }

	  plugin_data->last_delay_time = delay_time;
	  plugin_data->delay_samples = delay_samples;
	}
	
	plugin_data->write_phase = write_phase;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainDelay_c(LADSPA_Handle instance, LADSPA_Data gain) {
	((Delay_c *)instance)->run_adding_gain = gain;
}

static void runAddingDelay_c(LADSPA_Handle instance, unsigned long sample_count) {
	Delay_c *plugin_data = (Delay_c *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const in = plugin_data->in;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const out = plugin_data->out;

	/* Delay Time (s) (float value) */
	const LADSPA_Data delay_time = *(plugin_data->delay_time);
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	LADSPA_Data delay_samples = plugin_data->delay_samples;
	LADSPA_Data last_delay_time = plugin_data->last_delay_time;
	unsigned int sample_rate = plugin_data->sample_rate;
	long write_phase = plugin_data->write_phase;

	unsigned int i;

	if (write_phase == 0) {
	  plugin_data->last_delay_time = delay_time;
	  plugin_data->delay_samples = delay_samples = CALC_DELAY (delay_time);
	}
	
	if (delay_time == last_delay_time) {
	  long idelay_samples = (long)delay_samples;
	  LADSPA_Data frac = delay_samples - idelay_samples;

	  for (i=0; i<sample_count; i++) {
	    long read_phase = write_phase - (long)delay_samples;
	    LADSPA_Data read = cube_interp (frac,
	                                    buffer[(read_phase-1) & buffer_mask], 
	                                    buffer[read_phase & buffer_mask], 
	                                    buffer[(read_phase+1) & buffer_mask], 
	                                    buffer[(read_phase+2) & buffer_mask]);
	    buffer[write_phase++ & buffer_mask] = in[i];
	    buffer_write(out[i], read);
	  }
	} else {
	  float next_delay_samples = CALC_DELAY (delay_time);
	  float delay_samples_slope = (next_delay_samples - delay_samples) / sample_count;

	  for (i=0; i<sample_count; i++) {
	    long read_phase, idelay_samples;
	    LADSPA_Data frac, read;

	    delay_samples += delay_samples_slope;
	    write_phase++;
	    read_phase = write_phase - (long)delay_samples;
	    idelay_samples = (long)delay_samples;
	    frac = delay_samples - idelay_samples;
	    read = cube_interp (frac,
	                        buffer[(read_phase-1) & buffer_mask], 
	                        buffer[read_phase & buffer_mask], 
	                        buffer[(read_phase+1) & buffer_mask], 
	                        buffer[(read_phase+2) & buffer_mask]);
	    buffer[write_phase & buffer_mask] = in[i];
	    buffer_write(out[i], read);
	  }

	  plugin_data->last_delay_time = delay_time;
	  plugin_data->delay_samples = delay_samples;
	}
	
	plugin_data->write_phase = write_phase;
}

void __attribute__((constructor)) swh_init() {
	char **port_names;
	LADSPA_PortDescriptor *port_descriptors;
	LADSPA_PortRangeHint *port_range_hints;

#ifdef ENABLE_NLS
#define D_(s) dgettext(PACKAGE, s)
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, PACKAGE_LOCALE_DIR);
#else
#define D_(s) (s)
#endif


	delay_nDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (delay_nDescriptor) {
		delay_nDescriptor->UniqueID = 1898;
		delay_nDescriptor->Label = "delay_n";
		delay_nDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		delay_nDescriptor->Name =
		 D_("Simple delay line, noninterpolating");
		delay_nDescriptor->Maker =
		 "Andy Wingo <wingo at pobox dot com>";
		delay_nDescriptor->Copyright =
		 "GPL";
		delay_nDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		delay_nDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		delay_nDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		delay_nDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Input */
		port_descriptors[DELAY_N_IN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[DELAY_N_IN] =
		 D_("Input");
		port_range_hints[DELAY_N_IN].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[DELAY_N_OUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[DELAY_N_OUT] =
		 D_("Output");
		port_range_hints[DELAY_N_OUT].HintDescriptor = 0;

		/* Parameters for Max Delay (s) */
		port_descriptors[DELAY_N_MAX_DELAY] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DELAY_N_MAX_DELAY] =
		 D_("Max Delay (s)");
		port_range_hints[DELAY_N_MAX_DELAY].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[DELAY_N_MAX_DELAY].LowerBound = 0;

		/* Parameters for Delay Time (s) */
		port_descriptors[DELAY_N_DELAY_TIME] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DELAY_N_DELAY_TIME] =
		 D_("Delay Time (s)");
		port_range_hints[DELAY_N_DELAY_TIME].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[DELAY_N_DELAY_TIME].LowerBound = 0;

		delay_nDescriptor->activate = activateDelay_n;
		delay_nDescriptor->cleanup = cleanupDelay_n;
		delay_nDescriptor->connect_port = connectPortDelay_n;
		delay_nDescriptor->deactivate = NULL;
		delay_nDescriptor->instantiate = instantiateDelay_n;
		delay_nDescriptor->run = runDelay_n;
		delay_nDescriptor->run_adding = runAddingDelay_n;
		delay_nDescriptor->set_run_adding_gain = setRunAddingGainDelay_n;
	}

	delay_lDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (delay_lDescriptor) {
		delay_lDescriptor->UniqueID = 1899;
		delay_lDescriptor->Label = "delay_l";
		delay_lDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		delay_lDescriptor->Name =
		 D_("Simple delay line, linear interpolation");
		delay_lDescriptor->Maker =
		 "Andy Wingo <wingo at pobox dot com>";
		delay_lDescriptor->Copyright =
		 "GPL";
		delay_lDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		delay_lDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		delay_lDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		delay_lDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Input */
		port_descriptors[DELAY_L_IN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[DELAY_L_IN] =
		 D_("Input");
		port_range_hints[DELAY_L_IN].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[DELAY_L_OUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[DELAY_L_OUT] =
		 D_("Output");
		port_range_hints[DELAY_L_OUT].HintDescriptor = 0;

		/* Parameters for Max Delay (s) */
		port_descriptors[DELAY_L_MAX_DELAY] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DELAY_L_MAX_DELAY] =
		 D_("Max Delay (s)");
		port_range_hints[DELAY_L_MAX_DELAY].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[DELAY_L_MAX_DELAY].LowerBound = 0;

		/* Parameters for Delay Time (s) */
		port_descriptors[DELAY_L_DELAY_TIME] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DELAY_L_DELAY_TIME] =
		 D_("Delay Time (s)");
		port_range_hints[DELAY_L_DELAY_TIME].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[DELAY_L_DELAY_TIME].LowerBound = 0;

		delay_lDescriptor->activate = activateDelay_l;
		delay_lDescriptor->cleanup = cleanupDelay_l;
		delay_lDescriptor->connect_port = connectPortDelay_l;
		delay_lDescriptor->deactivate = NULL;
		delay_lDescriptor->instantiate = instantiateDelay_l;
		delay_lDescriptor->run = runDelay_l;
		delay_lDescriptor->run_adding = runAddingDelay_l;
		delay_lDescriptor->set_run_adding_gain = setRunAddingGainDelay_l;
	}

	delay_cDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (delay_cDescriptor) {
		delay_cDescriptor->UniqueID = 1900;
		delay_cDescriptor->Label = "delay_c";
		delay_cDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		delay_cDescriptor->Name =
		 D_("Simple delay line, cubic spline interpolation");
		delay_cDescriptor->Maker =
		 "Andy Wingo <wingo at pobox dot com>";
		delay_cDescriptor->Copyright =
		 "GPL";
		delay_cDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		delay_cDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		delay_cDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		delay_cDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Input */
		port_descriptors[DELAY_C_IN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[DELAY_C_IN] =
		 D_("Input");
		port_range_hints[DELAY_C_IN].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[DELAY_C_OUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[DELAY_C_OUT] =
		 D_("Output");
		port_range_hints[DELAY_C_OUT].HintDescriptor = 0;

		/* Parameters for Max Delay (s) */
		port_descriptors[DELAY_C_MAX_DELAY] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DELAY_C_MAX_DELAY] =
		 D_("Max Delay (s)");
		port_range_hints[DELAY_C_MAX_DELAY].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[DELAY_C_MAX_DELAY].LowerBound = 0;

		/* Parameters for Delay Time (s) */
		port_descriptors[DELAY_C_DELAY_TIME] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DELAY_C_DELAY_TIME] =
		 D_("Delay Time (s)");
		port_range_hints[DELAY_C_DELAY_TIME].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[DELAY_C_DELAY_TIME].LowerBound = 0;

		delay_cDescriptor->activate = activateDelay_c;
		delay_cDescriptor->cleanup = cleanupDelay_c;
		delay_cDescriptor->connect_port = connectPortDelay_c;
		delay_cDescriptor->deactivate = NULL;
		delay_cDescriptor->instantiate = instantiateDelay_c;
		delay_cDescriptor->run = runDelay_c;
		delay_cDescriptor->run_adding = runAddingDelay_c;
		delay_cDescriptor->set_run_adding_gain = setRunAddingGainDelay_c;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (delay_nDescriptor) {
		free((LADSPA_PortDescriptor *)delay_nDescriptor->PortDescriptors);
		free((char **)delay_nDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)delay_nDescriptor->PortRangeHints);
		free(delay_nDescriptor);
	}
	if (delay_lDescriptor) {
		free((LADSPA_PortDescriptor *)delay_lDescriptor->PortDescriptors);
		free((char **)delay_lDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)delay_lDescriptor->PortRangeHints);
		free(delay_lDescriptor);
	}
	if (delay_cDescriptor) {
		free((LADSPA_PortDescriptor *)delay_cDescriptor->PortDescriptors);
		free((char **)delay_cDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)delay_cDescriptor->PortRangeHints);
		free(delay_cDescriptor);
	}

}
