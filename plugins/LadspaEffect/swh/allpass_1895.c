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

#define LOG001 -6.9077552789f

static inline float
calc_feedback (float delaytime, float decaytime)
{
  if (delaytime == 0.f)
    return 0.f;
  else if (decaytime > 0.f)
    return exp(LOG001 * delaytime / decaytime);
  else if (decaytime < 0.f)
    return -exp(LOG001 * delaytime / -decaytime);
  else
    return 0.f;
}

void ignore(LADSPA_Data some_var)
{ }

#define ALLPASS_N_IN                   0
#define ALLPASS_N_OUT                  1
#define ALLPASS_N_MAX_DELAY            2
#define ALLPASS_N_DELAY_TIME           3
#define ALLPASS_N_DECAY_TIME           4
#define ALLPASS_L_IN                   0
#define ALLPASS_L_OUT                  1
#define ALLPASS_L_MAX_DELAY            2
#define ALLPASS_L_DELAY_TIME           3
#define ALLPASS_L_DECAY_TIME           4
#define ALLPASS_C_IN                   0
#define ALLPASS_C_OUT                  1
#define ALLPASS_C_MAX_DELAY            2
#define ALLPASS_C_DELAY_TIME           3
#define ALLPASS_C_DECAY_TIME           4

static LADSPA_Descriptor *allpass_nDescriptor = NULL;

typedef struct {
	LADSPA_Data *in;
	LADSPA_Data *out;
	LADSPA_Data *max_delay;
	LADSPA_Data *delay_time;
	LADSPA_Data *decay_time;
	LADSPA_Data *buffer;
	unsigned int buffer_mask;
	LADSPA_Data  delay_samples;
	LADSPA_Data  feedback;
	LADSPA_Data  last_decay_time;
	LADSPA_Data  last_delay_time;
	unsigned int sample_rate;
	long         write_phase;
	LADSPA_Data run_adding_gain;
} Allpass_n;

static LADSPA_Descriptor *allpass_lDescriptor = NULL;

typedef struct {
	LADSPA_Data *in;
	LADSPA_Data *out;
	LADSPA_Data *max_delay;
	LADSPA_Data *delay_time;
	LADSPA_Data *decay_time;
	LADSPA_Data *buffer;
	unsigned int buffer_mask;
	LADSPA_Data  delay_samples;
	LADSPA_Data  feedback;
	LADSPA_Data  last_decay_time;
	LADSPA_Data  last_delay_time;
	unsigned int sample_rate;
	long         write_phase;
	LADSPA_Data run_adding_gain;
} Allpass_l;

static LADSPA_Descriptor *allpass_cDescriptor = NULL;

typedef struct {
	LADSPA_Data *in;
	LADSPA_Data *out;
	LADSPA_Data *max_delay;
	LADSPA_Data *delay_time;
	LADSPA_Data *decay_time;
	LADSPA_Data *buffer;
	unsigned int buffer_mask;
	LADSPA_Data  delay_samples;
	LADSPA_Data  feedback;
	LADSPA_Data  last_decay_time;
	LADSPA_Data  last_delay_time;
	unsigned int sample_rate;
	long         write_phase;
	LADSPA_Data run_adding_gain;
} Allpass_c;

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
		return allpass_nDescriptor;
	case 1:
		return allpass_lDescriptor;
	case 2:
		return allpass_cDescriptor;
	default:
		return NULL;
	}
}

static void activateAllpass_n(LADSPA_Handle instance) {
	Allpass_n *plugin_data = (Allpass_n *)instance;
	LADSPA_Data *buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	LADSPA_Data delay_samples = plugin_data->delay_samples;
	LADSPA_Data feedback = plugin_data->feedback;
	LADSPA_Data last_decay_time = plugin_data->last_decay_time;
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
	plugin_data->feedback = feedback;
	plugin_data->last_decay_time = last_decay_time;
	plugin_data->last_delay_time = last_delay_time;
	plugin_data->sample_rate = sample_rate;
	plugin_data->write_phase = write_phase;

}

static void cleanupAllpass_n(LADSPA_Handle instance) {
	Allpass_n *plugin_data = (Allpass_n *)instance;
	free(plugin_data->buffer);
	free(instance);
}

static void connectPortAllpass_n(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Allpass_n *plugin;

	plugin = (Allpass_n *)instance;
	switch (port) {
	case ALLPASS_N_IN:
		plugin->in = data;
		break;
	case ALLPASS_N_OUT:
		plugin->out = data;
		break;
	case ALLPASS_N_MAX_DELAY:
		plugin->max_delay = data;
		break;
	case ALLPASS_N_DELAY_TIME:
		plugin->delay_time = data;
		break;
	case ALLPASS_N_DECAY_TIME:
		plugin->decay_time = data;
		break;
	}
}

static LADSPA_Handle instantiateAllpass_n(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Allpass_n *plugin_data = (Allpass_n *)malloc(sizeof(Allpass_n));
	LADSPA_Data *buffer = NULL;
	unsigned int buffer_mask = 0;
	LADSPA_Data delay_samples = 0;
	LADSPA_Data feedback = 0;
	LADSPA_Data last_decay_time = 0;
	LADSPA_Data last_delay_time = 0;
	unsigned int sample_rate = 0;
	long write_phase = 0;

	sample_rate = s_rate;

	plugin_data->buffer = buffer;
	plugin_data->buffer_mask = buffer_mask;
	plugin_data->delay_samples = delay_samples;
	plugin_data->feedback = feedback;
	plugin_data->last_decay_time = last_decay_time;
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

static void runAllpass_n(LADSPA_Handle instance, unsigned long sample_count) {
	Allpass_n *plugin_data = (Allpass_n *)instance;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const in = plugin_data->in;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const out = plugin_data->out;

	/* Max Delay (s) (float value) */
	const LADSPA_Data max_delay = *(plugin_data->max_delay);

	/* Delay Time (s) (float value) */
	const LADSPA_Data delay_time = *(plugin_data->delay_time);

	/* Decay Time (s) (float value) */
	const LADSPA_Data decay_time = *(plugin_data->decay_time);
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	LADSPA_Data delay_samples = plugin_data->delay_samples;
	LADSPA_Data feedback = plugin_data->feedback;
	LADSPA_Data last_decay_time = plugin_data->last_decay_time;
	LADSPA_Data last_delay_time = plugin_data->last_delay_time;
	unsigned int sample_rate = plugin_data->sample_rate;
	long write_phase = plugin_data->write_phase;

	unsigned int i;

	ignore(max_delay);

	if (write_phase == 0) {
	  plugin_data->last_delay_time = delay_time;
	  plugin_data->last_decay_time = decay_time;
	  plugin_data->delay_samples = delay_samples = CALC_DELAY (delay_time);
	  plugin_data->feedback = feedback = calc_feedback (delay_time, decay_time);
	}

	if (delay_time == last_delay_time) {
	  long read_phase = write_phase - (long)delay_samples;
	  LADSPA_Data *readptr = buffer + (read_phase & buffer_mask);
	  LADSPA_Data *writeptr = buffer + (write_phase & buffer_mask);
	  LADSPA_Data *lastptr = buffer + buffer_mask + 1;

	  if (decay_time == last_decay_time) {
	    long remain = sample_count;

	    while (remain) {
	      long read_space = lastptr - readptr;
	      long write_space = lastptr - writeptr;
	      long to_process = MIN (MIN (read_space, remain), write_space);

	      if (to_process == 0)
	        return; // buffer not allocated.

	      remain -= to_process;

	      for (i=0; i<to_process; i++) {
	        LADSPA_Data read = *(readptr++);
	        LADSPA_Data written = read * feedback + in[i];
	        *(writeptr++) = written;
	        buffer_write(out[i], read - feedback * written);
	      }

	      if (readptr == lastptr) readptr = buffer;
	      if (writeptr == lastptr) writeptr = buffer;
	    }
	  } else {
	    float next_feedback = calc_feedback (delay_time, decay_time);
	    float feedback_slope = (next_feedback - feedback) / sample_count;
	    long remain = sample_count;

	    while (remain) {
	      long read_space = lastptr - readptr;
	      long write_space = lastptr - writeptr;
	      long to_process = MIN (MIN (read_space, remain), write_space);

	      if (to_process == 0)
	        return; // buffer not allocated.

	      remain -= to_process;

	      for (i=0; i<to_process; i++) {
	        LADSPA_Data read = *(readptr++);
	        LADSPA_Data written = read * feedback + in[i];
	        *(writeptr++) = written;
	        buffer_write(out[i], read - feedback * written);
	        feedback += feedback_slope;
	      }

	      if (readptr == lastptr) readptr = buffer;
	      if (writeptr == lastptr) writeptr = buffer;
	    }

	    plugin_data->last_decay_time = decay_time;
	    plugin_data->feedback = feedback;
	  }

	  write_phase += sample_count;
	} else {
	  float next_delay_samples = CALC_DELAY (delay_time);
	  float delay_samples_slope = (next_delay_samples - delay_samples) / sample_count;
	  float next_feedback = calc_feedback (delay_time, decay_time);
	  float feedback_slope = (next_feedback - feedback) / sample_count;

	  for (i=0; i<sample_count; i++) {
	    long read_phase;
	    LADSPA_Data read, written;

	    delay_samples += delay_samples_slope;
	    write_phase++;
	    read_phase = write_phase - (long)delay_samples;
	    read = buffer[read_phase & buffer_mask];

	    written = read * feedback + in[i];
	    buffer[write_phase & buffer_mask] = written;
	    buffer_write(out[i], read - feedback * written);

	    feedback += feedback_slope;
	  }

	  plugin_data->last_delay_time = delay_time;
	  plugin_data->last_decay_time = decay_time;
	  plugin_data->feedback = feedback;
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

static void setRunAddingGainAllpass_n(LADSPA_Handle instance, LADSPA_Data gain) {
	((Allpass_n *)instance)->run_adding_gain = gain;
}

static void runAddingAllpass_n(LADSPA_Handle instance, unsigned long sample_count) {
	Allpass_n *plugin_data = (Allpass_n *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const in = plugin_data->in;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const out = plugin_data->out;

	/* Max Delay (s) (float value) */
	const LADSPA_Data max_delay = *(plugin_data->max_delay);

	/* Delay Time (s) (float value) */
	const LADSPA_Data delay_time = *(plugin_data->delay_time);

	/* Decay Time (s) (float value) */
	const LADSPA_Data decay_time = *(plugin_data->decay_time);
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	LADSPA_Data delay_samples = plugin_data->delay_samples;
	LADSPA_Data feedback = plugin_data->feedback;
	LADSPA_Data last_decay_time = plugin_data->last_decay_time;
	LADSPA_Data last_delay_time = plugin_data->last_delay_time;
	unsigned int sample_rate = plugin_data->sample_rate;
	long write_phase = plugin_data->write_phase;

	unsigned int i;

	ignore(max_delay);

	if (write_phase == 0) {
	  plugin_data->last_delay_time = delay_time;
	  plugin_data->last_decay_time = decay_time;
	  plugin_data->delay_samples = delay_samples = CALC_DELAY (delay_time);
	  plugin_data->feedback = feedback = calc_feedback (delay_time, decay_time);
	}
	
	if (delay_time == last_delay_time) {
	  long read_phase = write_phase - (long)delay_samples;
	  LADSPA_Data *readptr = buffer + (read_phase & buffer_mask);
	  LADSPA_Data *writeptr = buffer + (write_phase & buffer_mask);
	  LADSPA_Data *lastptr = buffer + buffer_mask + 1;

	  if (decay_time == last_decay_time) {
	    long remain = sample_count;

	    while (remain) {
	      long read_space = lastptr - readptr;
	      long write_space = lastptr - writeptr;
	      long to_process = MIN (MIN (read_space, remain), write_space);

	      if (to_process == 0)
	        return; // buffer not allocated.

	      remain -= to_process;

	      for (i=0; i<to_process; i++) {
	        LADSPA_Data read = *(readptr++);
	        LADSPA_Data written = read * feedback + in[i];
	        *(writeptr++) = written;
	        buffer_write(out[i], read - feedback * written);
	      }

	      if (readptr == lastptr) readptr = buffer;
	      if (writeptr == lastptr) writeptr = buffer;
	    }
	  } else {
	    float next_feedback = calc_feedback (delay_time, decay_time);
	    float feedback_slope = (next_feedback - feedback) / sample_count;
	    long remain = sample_count;

	    while (remain) {
	      long read_space = lastptr - readptr;
	      long write_space = lastptr - writeptr;
	      long to_process = MIN (MIN (read_space, remain), write_space);

	      if (to_process == 0)
	        return; // buffer not allocated.

	      remain -= to_process;

	      for (i=0; i<to_process; i++) {
	        LADSPA_Data read = *(readptr++);
	        LADSPA_Data written = read * feedback + in[i];
	        *(writeptr++) = written;
	        buffer_write(out[i], read - feedback * written);
	        feedback += feedback_slope;
	      }

	      if (readptr == lastptr) readptr = buffer;
	      if (writeptr == lastptr) writeptr = buffer;
	    }

	    plugin_data->last_decay_time = decay_time;
	    plugin_data->feedback = feedback;
	  }

	  write_phase += sample_count;
	} else {
	  float next_delay_samples = CALC_DELAY (delay_time);
	  float delay_samples_slope = (next_delay_samples - delay_samples) / sample_count;
	  float next_feedback = calc_feedback (delay_time, decay_time);
	  float feedback_slope = (next_feedback - feedback) / sample_count;

	  for (i=0; i<sample_count; i++) {
	    long read_phase;
	    LADSPA_Data read, written;

	    delay_samples += delay_samples_slope;
	    write_phase++;
	    read_phase = write_phase - (long)delay_samples;
	    read = buffer[read_phase & buffer_mask];

	    written = read * feedback + in[i];
	    buffer[write_phase & buffer_mask] = written;
	    buffer_write(out[i], read - feedback * written);

	    feedback += feedback_slope;
	  }

	  plugin_data->last_delay_time = delay_time;
	  plugin_data->last_decay_time = decay_time;
	  plugin_data->feedback = feedback;
	  plugin_data->delay_samples = delay_samples;
	}
	
	plugin_data->write_phase = write_phase;
}

static void activateAllpass_l(LADSPA_Handle instance) {
	Allpass_l *plugin_data = (Allpass_l *)instance;
	LADSPA_Data *buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	LADSPA_Data delay_samples = plugin_data->delay_samples;
	LADSPA_Data feedback = plugin_data->feedback;
	LADSPA_Data last_decay_time = plugin_data->last_decay_time;
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
	plugin_data->feedback = feedback;
	plugin_data->last_decay_time = last_decay_time;
	plugin_data->last_delay_time = last_delay_time;
	plugin_data->sample_rate = sample_rate;
	plugin_data->write_phase = write_phase;

}

static void cleanupAllpass_l(LADSPA_Handle instance) {
	Allpass_l *plugin_data = (Allpass_l *)instance;
	free(plugin_data->buffer);
	free(instance);
}

static void connectPortAllpass_l(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Allpass_l *plugin;

	plugin = (Allpass_l *)instance;
	switch (port) {
	case ALLPASS_L_IN:
		plugin->in = data;
		break;
	case ALLPASS_L_OUT:
		plugin->out = data;
		break;
	case ALLPASS_L_MAX_DELAY:
		plugin->max_delay = data;
		break;
	case ALLPASS_L_DELAY_TIME:
		plugin->delay_time = data;
		break;
	case ALLPASS_L_DECAY_TIME:
		plugin->decay_time = data;
		break;
	}
}

static LADSPA_Handle instantiateAllpass_l(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Allpass_l *plugin_data = (Allpass_l *)malloc(sizeof(Allpass_l));
	LADSPA_Data *buffer = NULL;
	unsigned int buffer_mask = 0;
	LADSPA_Data delay_samples = 0;
	LADSPA_Data feedback = 0;
	LADSPA_Data last_decay_time = 0;
	LADSPA_Data last_delay_time = 0;
	unsigned int sample_rate = 0;
	long write_phase = 0;

	sample_rate = s_rate;

	plugin_data->buffer = buffer;
	plugin_data->buffer_mask = buffer_mask;
	plugin_data->delay_samples = delay_samples;
	plugin_data->feedback = feedback;
	plugin_data->last_decay_time = last_decay_time;
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

static void runAllpass_l(LADSPA_Handle instance, unsigned long sample_count) {
	Allpass_l *plugin_data = (Allpass_l *)instance;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const in = plugin_data->in;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const out = plugin_data->out;

	/* Delay Time (s) (float value) */
	const LADSPA_Data delay_time = *(plugin_data->delay_time);

	/* Decay Time (s) (float value) */
	const LADSPA_Data decay_time = *(plugin_data->decay_time);
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	LADSPA_Data delay_samples = plugin_data->delay_samples;
	LADSPA_Data feedback = plugin_data->feedback;
	LADSPA_Data last_decay_time = plugin_data->last_decay_time;
	LADSPA_Data last_delay_time = plugin_data->last_delay_time;
	unsigned int sample_rate = plugin_data->sample_rate;
	long write_phase = plugin_data->write_phase;

	unsigned int i;

	if (write_phase == 0) {
	  plugin_data->last_delay_time = delay_time;
	  plugin_data->last_decay_time = decay_time;
	  plugin_data->delay_samples = delay_samples = CALC_DELAY (delay_time);
	  plugin_data->feedback = feedback = calc_feedback (delay_time, decay_time);
	}
	
	if (delay_time == last_delay_time && decay_time == last_decay_time) {
	  long idelay_samples = (long)delay_samples;
	  LADSPA_Data frac = delay_samples - idelay_samples;

	  for (i=0; i<sample_count; i++) {
	    long read_phase = write_phase - (long)delay_samples;
	    LADSPA_Data r1 = buffer[read_phase & buffer_mask];
	    LADSPA_Data r2 = buffer[(read_phase-1) & buffer_mask];
	    LADSPA_Data read = LIN_INTERP (frac, r1, r2);
	    LADSPA_Data written = read * feedback + in[i];

	    buffer[write_phase++ & buffer_mask] = written;
	    buffer_write(out[i], read - feedback * written);
	    write_phase++;
	  }
	} else {
	  float next_delay_samples = CALC_DELAY (delay_time);
	  float delay_samples_slope = (next_delay_samples - delay_samples) / sample_count;
	  float next_feedback = calc_feedback (delay_time, decay_time);
	  float feedback_slope = (next_feedback - feedback) / sample_count;

	  for (i=0; i<sample_count; i++) {
	    long read_phase, idelay_samples;
	    LADSPA_Data read, written, frac;

	    delay_samples += delay_samples_slope;
	    write_phase++;
	    read_phase = write_phase - (long)delay_samples;
	    idelay_samples = (long)delay_samples;
	    frac = delay_samples - idelay_samples;
	    read = LIN_INTERP (frac,
	                       buffer[read_phase & buffer_mask], 
	                       buffer[(read_phase-1) & buffer_mask]);
	    written = read * feedback + in[i];
	    buffer[write_phase & buffer_mask] = written;
	    buffer_write(out[i], read - feedback * written);

	    feedback += feedback_slope;
	  }

	  plugin_data->last_delay_time = delay_time;
	  plugin_data->last_decay_time = decay_time;
	  plugin_data->feedback = feedback;
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

static void setRunAddingGainAllpass_l(LADSPA_Handle instance, LADSPA_Data gain) {
	((Allpass_l *)instance)->run_adding_gain = gain;
}

static void runAddingAllpass_l(LADSPA_Handle instance, unsigned long sample_count) {
	Allpass_l *plugin_data = (Allpass_l *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const in = plugin_data->in;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const out = plugin_data->out;

	/* Delay Time (s) (float value) */
	const LADSPA_Data delay_time = *(plugin_data->delay_time);

	/* Decay Time (s) (float value) */
	const LADSPA_Data decay_time = *(plugin_data->decay_time);
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	LADSPA_Data delay_samples = plugin_data->delay_samples;
	LADSPA_Data feedback = plugin_data->feedback;
	LADSPA_Data last_decay_time = plugin_data->last_decay_time;
	LADSPA_Data last_delay_time = plugin_data->last_delay_time;
	unsigned int sample_rate = plugin_data->sample_rate;
	long write_phase = plugin_data->write_phase;

	unsigned int i;

	if (write_phase == 0) {
	  plugin_data->last_delay_time = delay_time;
	  plugin_data->last_decay_time = decay_time;
	  plugin_data->delay_samples = delay_samples = CALC_DELAY (delay_time);
	  plugin_data->feedback = feedback = calc_feedback (delay_time, decay_time);
	}
	
	if (delay_time == last_delay_time && decay_time == last_decay_time) {
	  long idelay_samples = (long)delay_samples;
	  LADSPA_Data frac = delay_samples - idelay_samples;

	  for (i=0; i<sample_count; i++) {
	    long read_phase = write_phase - (long)delay_samples;
	    LADSPA_Data r1 = buffer[read_phase & buffer_mask];
	    LADSPA_Data r2 = buffer[(read_phase-1) & buffer_mask];
	    LADSPA_Data read = LIN_INTERP (frac, r1, r2);
	    LADSPA_Data written = read * feedback + in[i];

	    buffer[write_phase++ & buffer_mask] = written;
	    buffer_write(out[i], read - feedback * written);
	    write_phase++;
	  }
	} else {
	  float next_delay_samples = CALC_DELAY (delay_time);
	  float delay_samples_slope = (next_delay_samples - delay_samples) / sample_count;
	  float next_feedback = calc_feedback (delay_time, decay_time);
	  float feedback_slope = (next_feedback - feedback) / sample_count;

	  for (i=0; i<sample_count; i++) {
	    long read_phase, idelay_samples;
	    LADSPA_Data read, written, frac;

	    delay_samples += delay_samples_slope;
	    write_phase++;
	    read_phase = write_phase - (long)delay_samples;
	    idelay_samples = (long)delay_samples;
	    frac = delay_samples - idelay_samples;
	    read = LIN_INTERP (frac,
	                       buffer[read_phase & buffer_mask], 
	                       buffer[(read_phase-1) & buffer_mask]);
	    written = read * feedback + in[i];
	    buffer[write_phase & buffer_mask] = written;
	    buffer_write(out[i], read - feedback * written);

	    feedback += feedback_slope;
	  }

	  plugin_data->last_delay_time = delay_time;
	  plugin_data->last_decay_time = decay_time;
	  plugin_data->feedback = feedback;
	  plugin_data->delay_samples = delay_samples;
	}
	
	plugin_data->write_phase = write_phase;
}

static void activateAllpass_c(LADSPA_Handle instance) {
	Allpass_c *plugin_data = (Allpass_c *)instance;
	LADSPA_Data *buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	LADSPA_Data delay_samples = plugin_data->delay_samples;
	LADSPA_Data feedback = plugin_data->feedback;
	LADSPA_Data last_decay_time = plugin_data->last_decay_time;
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
	plugin_data->feedback = feedback;
	plugin_data->last_decay_time = last_decay_time;
	plugin_data->last_delay_time = last_delay_time;
	plugin_data->sample_rate = sample_rate;
	plugin_data->write_phase = write_phase;

}

static void cleanupAllpass_c(LADSPA_Handle instance) {
	Allpass_c *plugin_data = (Allpass_c *)instance;
	free(plugin_data->buffer);
	free(instance);
}

static void connectPortAllpass_c(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Allpass_c *plugin;

	plugin = (Allpass_c *)instance;
	switch (port) {
	case ALLPASS_C_IN:
		plugin->in = data;
		break;
	case ALLPASS_C_OUT:
		plugin->out = data;
		break;
	case ALLPASS_C_MAX_DELAY:
		plugin->max_delay = data;
		break;
	case ALLPASS_C_DELAY_TIME:
		plugin->delay_time = data;
		break;
	case ALLPASS_C_DECAY_TIME:
		plugin->decay_time = data;
		break;
	}
}

static LADSPA_Handle instantiateAllpass_c(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Allpass_c *plugin_data = (Allpass_c *)malloc(sizeof(Allpass_c));
	LADSPA_Data *buffer = NULL;
	unsigned int buffer_mask = 0;
	LADSPA_Data delay_samples = 0;
	LADSPA_Data feedback = 0;
	LADSPA_Data last_decay_time = 0;
	LADSPA_Data last_delay_time = 0;
	unsigned int sample_rate = 0;
	long write_phase = 0;

	sample_rate = s_rate;

	plugin_data->buffer = buffer;
	plugin_data->buffer_mask = buffer_mask;
	plugin_data->delay_samples = delay_samples;
	plugin_data->feedback = feedback;
	plugin_data->last_decay_time = last_decay_time;
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

static void runAllpass_c(LADSPA_Handle instance, unsigned long sample_count) {
	Allpass_c *plugin_data = (Allpass_c *)instance;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const in = plugin_data->in;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const out = plugin_data->out;

	/* Delay Time (s) (float value) */
	const LADSPA_Data delay_time = *(plugin_data->delay_time);

	/* Decay Time (s) (float value) */
	const LADSPA_Data decay_time = *(plugin_data->decay_time);
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	LADSPA_Data delay_samples = plugin_data->delay_samples;
	LADSPA_Data feedback = plugin_data->feedback;
	LADSPA_Data last_decay_time = plugin_data->last_decay_time;
	LADSPA_Data last_delay_time = plugin_data->last_delay_time;
	unsigned int sample_rate = plugin_data->sample_rate;
	long write_phase = plugin_data->write_phase;

	unsigned int i;

	if (write_phase == 0) {
	  plugin_data->last_delay_time = delay_time;
	  plugin_data->last_decay_time = decay_time;
	  plugin_data->delay_samples = delay_samples = CALC_DELAY (delay_time);
	  plugin_data->feedback = feedback = calc_feedback (delay_time, decay_time);
	}
	
	if (delay_time == last_delay_time && decay_time == last_decay_time) {
	  long idelay_samples = (long)delay_samples;
	  LADSPA_Data frac = delay_samples - idelay_samples;

	  for (i=0; i<sample_count; i++) {
	    long read_phase = write_phase - (long)delay_samples;
	    LADSPA_Data read = cube_interp (frac,
	                                    buffer[(read_phase-1) & buffer_mask], 
	                                    buffer[read_phase & buffer_mask], 
	                                    buffer[(read_phase+1) & buffer_mask], 
	                                    buffer[(read_phase+2) & buffer_mask]);
	    LADSPA_Data written = read * feedback + in[i];

	    buffer[write_phase++ & buffer_mask] = written;
	    buffer_write(out[i], read - feedback * written);
	  }
	} else {
	  float next_delay_samples = CALC_DELAY (delay_time);
	  float delay_samples_slope = (next_delay_samples - delay_samples) / sample_count;
	  float next_feedback = calc_feedback (delay_time, decay_time);
	  float feedback_slope = (next_feedback - feedback) / sample_count;

	  for (i=0; i<sample_count; i++) {
	    long read_phase, idelay_samples;
	    LADSPA_Data read, written, frac;

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
	    written = read * feedback + in[i];
	    buffer[write_phase & buffer_mask] = written;
	    buffer_write(out[i], read - feedback * written);

	    feedback += feedback_slope;
	  }

	  plugin_data->last_delay_time = delay_time;
	  plugin_data->last_decay_time = decay_time;
	  plugin_data->feedback = feedback;
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

static void setRunAddingGainAllpass_c(LADSPA_Handle instance, LADSPA_Data gain) {
	((Allpass_c *)instance)->run_adding_gain = gain;
}

static void runAddingAllpass_c(LADSPA_Handle instance, unsigned long sample_count) {
	Allpass_c *plugin_data = (Allpass_c *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const in = plugin_data->in;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const out = plugin_data->out;

	/* Delay Time (s) (float value) */
	const LADSPA_Data delay_time = *(plugin_data->delay_time);

	/* Decay Time (s) (float value) */
	const LADSPA_Data decay_time = *(plugin_data->decay_time);
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	LADSPA_Data delay_samples = plugin_data->delay_samples;
	LADSPA_Data feedback = plugin_data->feedback;
	LADSPA_Data last_decay_time = plugin_data->last_decay_time;
	LADSPA_Data last_delay_time = plugin_data->last_delay_time;
	unsigned int sample_rate = plugin_data->sample_rate;
	long write_phase = plugin_data->write_phase;

	unsigned int i;

	if (write_phase == 0) {
	  plugin_data->last_delay_time = delay_time;
	  plugin_data->last_decay_time = decay_time;
	  plugin_data->delay_samples = delay_samples = CALC_DELAY (delay_time);
	  plugin_data->feedback = feedback = calc_feedback (delay_time, decay_time);
	}
	
	if (delay_time == last_delay_time && decay_time == last_decay_time) {
	  long idelay_samples = (long)delay_samples;
	  LADSPA_Data frac = delay_samples - idelay_samples;

	  for (i=0; i<sample_count; i++) {
	    long read_phase = write_phase - (long)delay_samples;
	    LADSPA_Data read = cube_interp (frac,
	                                    buffer[(read_phase-1) & buffer_mask], 
	                                    buffer[read_phase & buffer_mask], 
	                                    buffer[(read_phase+1) & buffer_mask], 
	                                    buffer[(read_phase+2) & buffer_mask]);
	    LADSPA_Data written = read * feedback + in[i];

	    buffer[write_phase++ & buffer_mask] = written;
	    buffer_write(out[i], read - feedback * written);
	  }
	} else {
	  float next_delay_samples = CALC_DELAY (delay_time);
	  float delay_samples_slope = (next_delay_samples - delay_samples) / sample_count;
	  float next_feedback = calc_feedback (delay_time, decay_time);
	  float feedback_slope = (next_feedback - feedback) / sample_count;

	  for (i=0; i<sample_count; i++) {
	    long read_phase, idelay_samples;
	    LADSPA_Data read, written, frac;

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
	    written = read * feedback + in[i];
	    buffer[write_phase & buffer_mask] = written;
	    buffer_write(out[i], read - feedback * written);

	    feedback += feedback_slope;
	  }

	  plugin_data->last_delay_time = delay_time;
	  plugin_data->last_decay_time = decay_time;
	  plugin_data->feedback = feedback;
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


	allpass_nDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (allpass_nDescriptor) {
		allpass_nDescriptor->UniqueID = 1895;
		allpass_nDescriptor->Label = "allpass_n";
		allpass_nDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		allpass_nDescriptor->Name =
		 D_("Allpass delay line, noninterpolating");
		allpass_nDescriptor->Maker =
		 "Andy Wingo <wingo at pobox dot com>";
		allpass_nDescriptor->Copyright =
		 "GPL";
		allpass_nDescriptor->PortCount = 5;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(5,
		 sizeof(LADSPA_PortDescriptor));
		allpass_nDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(5,
		 sizeof(LADSPA_PortRangeHint));
		allpass_nDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(5, sizeof(char*));
		allpass_nDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Input */
		port_descriptors[ALLPASS_N_IN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[ALLPASS_N_IN] =
		 D_("Input");
		port_range_hints[ALLPASS_N_IN].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[ALLPASS_N_OUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[ALLPASS_N_OUT] =
		 D_("Output");
		port_range_hints[ALLPASS_N_OUT].HintDescriptor = 0;

		/* Parameters for Max Delay (s) */
		port_descriptors[ALLPASS_N_MAX_DELAY] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[ALLPASS_N_MAX_DELAY] =
		 D_("Max Delay (s)");
		port_range_hints[ALLPASS_N_MAX_DELAY].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[ALLPASS_N_MAX_DELAY].LowerBound = 0;

		/* Parameters for Delay Time (s) */
		port_descriptors[ALLPASS_N_DELAY_TIME] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[ALLPASS_N_DELAY_TIME] =
		 D_("Delay Time (s)");
		port_range_hints[ALLPASS_N_DELAY_TIME].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[ALLPASS_N_DELAY_TIME].LowerBound = 0;

		/* Parameters for Decay Time (s) */
		port_descriptors[ALLPASS_N_DECAY_TIME] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[ALLPASS_N_DECAY_TIME] =
		 D_("Decay Time (s)");
		port_range_hints[ALLPASS_N_DECAY_TIME].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[ALLPASS_N_DECAY_TIME].LowerBound = 0;

		allpass_nDescriptor->activate = activateAllpass_n;
		allpass_nDescriptor->cleanup = cleanupAllpass_n;
		allpass_nDescriptor->connect_port = connectPortAllpass_n;
		allpass_nDescriptor->deactivate = NULL;
		allpass_nDescriptor->instantiate = instantiateAllpass_n;
		allpass_nDescriptor->run = runAllpass_n;
		allpass_nDescriptor->run_adding = runAddingAllpass_n;
		allpass_nDescriptor->set_run_adding_gain = setRunAddingGainAllpass_n;
	}

	allpass_lDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (allpass_lDescriptor) {
		allpass_lDescriptor->UniqueID = 1896;
		allpass_lDescriptor->Label = "allpass_l";
		allpass_lDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		allpass_lDescriptor->Name =
		 D_("Allpass delay line, linear interpolation");
		allpass_lDescriptor->Maker =
		 "Andy Wingo <wingo at pobox dot com>";
		allpass_lDescriptor->Copyright =
		 "GPL";
		allpass_lDescriptor->PortCount = 5;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(5,
		 sizeof(LADSPA_PortDescriptor));
		allpass_lDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(5,
		 sizeof(LADSPA_PortRangeHint));
		allpass_lDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(5, sizeof(char*));
		allpass_lDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Input */
		port_descriptors[ALLPASS_L_IN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[ALLPASS_L_IN] =
		 D_("Input");
		port_range_hints[ALLPASS_L_IN].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[ALLPASS_L_OUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[ALLPASS_L_OUT] =
		 D_("Output");
		port_range_hints[ALLPASS_L_OUT].HintDescriptor = 0;

		/* Parameters for Max Delay (s) */
		port_descriptors[ALLPASS_L_MAX_DELAY] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[ALLPASS_L_MAX_DELAY] =
		 D_("Max Delay (s)");
		port_range_hints[ALLPASS_L_MAX_DELAY].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[ALLPASS_L_MAX_DELAY].LowerBound = 0;

		/* Parameters for Delay Time (s) */
		port_descriptors[ALLPASS_L_DELAY_TIME] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[ALLPASS_L_DELAY_TIME] =
		 D_("Delay Time (s)");
		port_range_hints[ALLPASS_L_DELAY_TIME].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[ALLPASS_L_DELAY_TIME].LowerBound = 0;

		/* Parameters for Decay Time (s) */
		port_descriptors[ALLPASS_L_DECAY_TIME] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[ALLPASS_L_DECAY_TIME] =
		 D_("Decay Time (s)");
		port_range_hints[ALLPASS_L_DECAY_TIME].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[ALLPASS_L_DECAY_TIME].LowerBound = 0;

		allpass_lDescriptor->activate = activateAllpass_l;
		allpass_lDescriptor->cleanup = cleanupAllpass_l;
		allpass_lDescriptor->connect_port = connectPortAllpass_l;
		allpass_lDescriptor->deactivate = NULL;
		allpass_lDescriptor->instantiate = instantiateAllpass_l;
		allpass_lDescriptor->run = runAllpass_l;
		allpass_lDescriptor->run_adding = runAddingAllpass_l;
		allpass_lDescriptor->set_run_adding_gain = setRunAddingGainAllpass_l;
	}

	allpass_cDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (allpass_cDescriptor) {
		allpass_cDescriptor->UniqueID = 1897;
		allpass_cDescriptor->Label = "allpass_c";
		allpass_cDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		allpass_cDescriptor->Name =
		 D_("Allpass delay line, cubic spline interpolation");
		allpass_cDescriptor->Maker =
		 "Andy Wingo <wingo at pobox dot com>";
		allpass_cDescriptor->Copyright =
		 "GPL";
		allpass_cDescriptor->PortCount = 5;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(5,
		 sizeof(LADSPA_PortDescriptor));
		allpass_cDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(5,
		 sizeof(LADSPA_PortRangeHint));
		allpass_cDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(5, sizeof(char*));
		allpass_cDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Input */
		port_descriptors[ALLPASS_C_IN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[ALLPASS_C_IN] =
		 D_("Input");
		port_range_hints[ALLPASS_C_IN].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[ALLPASS_C_OUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[ALLPASS_C_OUT] =
		 D_("Output");
		port_range_hints[ALLPASS_C_OUT].HintDescriptor = 0;

		/* Parameters for Max Delay (s) */
		port_descriptors[ALLPASS_C_MAX_DELAY] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[ALLPASS_C_MAX_DELAY] =
		 D_("Max Delay (s)");
		port_range_hints[ALLPASS_C_MAX_DELAY].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[ALLPASS_C_MAX_DELAY].LowerBound = 0;

		/* Parameters for Delay Time (s) */
		port_descriptors[ALLPASS_C_DELAY_TIME] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[ALLPASS_C_DELAY_TIME] =
		 D_("Delay Time (s)");
		port_range_hints[ALLPASS_C_DELAY_TIME].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[ALLPASS_C_DELAY_TIME].LowerBound = 0;

		/* Parameters for Decay Time (s) */
		port_descriptors[ALLPASS_C_DECAY_TIME] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[ALLPASS_C_DECAY_TIME] =
		 D_("Decay Time (s)");
		port_range_hints[ALLPASS_C_DECAY_TIME].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[ALLPASS_C_DECAY_TIME].LowerBound = 0;

		allpass_cDescriptor->activate = activateAllpass_c;
		allpass_cDescriptor->cleanup = cleanupAllpass_c;
		allpass_cDescriptor->connect_port = connectPortAllpass_c;
		allpass_cDescriptor->deactivate = NULL;
		allpass_cDescriptor->instantiate = instantiateAllpass_c;
		allpass_cDescriptor->run = runAllpass_c;
		allpass_cDescriptor->run_adding = runAddingAllpass_c;
		allpass_cDescriptor->set_run_adding_gain = setRunAddingGainAllpass_c;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (allpass_nDescriptor) {
		free((LADSPA_PortDescriptor *)allpass_nDescriptor->PortDescriptors);
		free((char **)allpass_nDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)allpass_nDescriptor->PortRangeHints);
		free(allpass_nDescriptor);
	}
	if (allpass_lDescriptor) {
		free((LADSPA_PortDescriptor *)allpass_lDescriptor->PortDescriptors);
		free((char **)allpass_lDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)allpass_lDescriptor->PortRangeHints);
		free(allpass_lDescriptor);
	}
	if (allpass_cDescriptor) {
		free((LADSPA_PortDescriptor *)allpass_cDescriptor->PortDescriptors);
		free((char **)allpass_cDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)allpass_cDescriptor->PortRangeHints);
		free(allpass_cDescriptor);
	}

}
