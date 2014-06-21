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

#define COMB_N_IN                      0
#define COMB_N_OUT                     1
#define COMB_N_MAX_DELAY               2
#define COMB_N_DELAY_TIME              3
#define COMB_N_DECAY_TIME              4
#define COMB_L_IN                      0
#define COMB_L_OUT                     1
#define COMB_L_MAX_DELAY               2
#define COMB_L_DELAY_TIME              3
#define COMB_L_DECAY_TIME              4
#define COMB_C_IN                      0
#define COMB_C_OUT                     1
#define COMB_C_MAX_DELAY               2
#define COMB_C_DELAY_TIME              3
#define COMB_C_DECAY_TIME              4

static LADSPA_Descriptor *comb_nDescriptor = NULL;

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
} Comb_n;

static LADSPA_Descriptor *comb_lDescriptor = NULL;

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
} Comb_l;

static LADSPA_Descriptor *comb_cDescriptor = NULL;

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
} Comb_c;

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
		return comb_nDescriptor;
	case 1:
		return comb_lDescriptor;
	case 2:
		return comb_cDescriptor;
	default:
		return NULL;
	}
}

static void activateComb_n(LADSPA_Handle instance) {
	Comb_n *plugin_data = (Comb_n *)instance;
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

static void cleanupComb_n(LADSPA_Handle instance) {
	Comb_n *plugin_data = (Comb_n *)instance;
	free(plugin_data->buffer);
	free(instance);
}

static void connectPortComb_n(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Comb_n *plugin;

	plugin = (Comb_n *)instance;
	switch (port) {
	case COMB_N_IN:
		plugin->in = data;
		break;
	case COMB_N_OUT:
		plugin->out = data;
		break;
	case COMB_N_MAX_DELAY:
		plugin->max_delay = data;
		break;
	case COMB_N_DELAY_TIME:
		plugin->delay_time = data;
		break;
	case COMB_N_DECAY_TIME:
		plugin->decay_time = data;
		break;
	}
}

static LADSPA_Handle instantiateComb_n(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Comb_n *plugin_data = (Comb_n *)malloc(sizeof(Comb_n));
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

static void runComb_n(LADSPA_Handle instance, unsigned long sample_count) {
	Comb_n *plugin_data = (Comb_n *)instance;

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

	i = max_delay; /* stop gcc complaining */

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
	        *(writeptr++) = read * feedback + in[i];
	        buffer_write(out[i], read);
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
	        *(writeptr++) = read * feedback + in[i];
	        buffer_write(out[i], read);
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
	    LADSPA_Data read;

	    delay_samples += delay_samples_slope;
	    write_phase++;
	    read_phase = write_phase - (long)delay_samples;
	    read = buffer[read_phase & buffer_mask];

	    buffer[write_phase & buffer_mask] = read * feedback + in[i];
	    buffer_write(out[i], read);

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

static void setRunAddingGainComb_n(LADSPA_Handle instance, LADSPA_Data gain) {
	((Comb_n *)instance)->run_adding_gain = gain;
}

static void runAddingComb_n(LADSPA_Handle instance, unsigned long sample_count) {
	Comb_n *plugin_data = (Comb_n *)instance;
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

	i = max_delay; /* stop gcc complaining */

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
	        *(writeptr++) = read * feedback + in[i];
	        buffer_write(out[i], read);
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
	        *(writeptr++) = read * feedback + in[i];
	        buffer_write(out[i], read);
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
	    LADSPA_Data read;

	    delay_samples += delay_samples_slope;
	    write_phase++;
	    read_phase = write_phase - (long)delay_samples;
	    read = buffer[read_phase & buffer_mask];

	    buffer[write_phase & buffer_mask] = read * feedback + in[i];
	    buffer_write(out[i], read);

	    feedback += feedback_slope;
	  }

	  plugin_data->last_delay_time = delay_time;
	  plugin_data->last_decay_time = decay_time;
	  plugin_data->feedback = feedback;
	  plugin_data->delay_samples = delay_samples;
	}
	
	plugin_data->write_phase = write_phase;
}

static void activateComb_l(LADSPA_Handle instance) {
	Comb_l *plugin_data = (Comb_l *)instance;
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

static void cleanupComb_l(LADSPA_Handle instance) {
	Comb_l *plugin_data = (Comb_l *)instance;
	free(plugin_data->buffer);
	free(instance);
}

static void connectPortComb_l(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Comb_l *plugin;

	plugin = (Comb_l *)instance;
	switch (port) {
	case COMB_L_IN:
		plugin->in = data;
		break;
	case COMB_L_OUT:
		plugin->out = data;
		break;
	case COMB_L_MAX_DELAY:
		plugin->max_delay = data;
		break;
	case COMB_L_DELAY_TIME:
		plugin->delay_time = data;
		break;
	case COMB_L_DECAY_TIME:
		plugin->decay_time = data;
		break;
	}
}

static LADSPA_Handle instantiateComb_l(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Comb_l *plugin_data = (Comb_l *)malloc(sizeof(Comb_l));
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

static void runComb_l(LADSPA_Handle instance, unsigned long sample_count) {
	Comb_l *plugin_data = (Comb_l *)instance;

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

	i = max_delay;

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

	    buffer[write_phase & buffer_mask] = read * feedback + in[i];
	    buffer_write(out[i], read);
	    write_phase++;
	  }
	} else {
	  float next_delay_samples = CALC_DELAY (delay_time);
	  float delay_samples_slope = (next_delay_samples - delay_samples) / sample_count;
	  float next_feedback = calc_feedback (delay_time, decay_time);
	  float feedback_slope = (next_feedback - feedback) / sample_count;

	  for (i=0; i<sample_count; i++) {
	    long read_phase, idelay_samples;
	    LADSPA_Data read, frac;

	    delay_samples += delay_samples_slope;
	    write_phase++;
	    read_phase = write_phase - (long)delay_samples;
	    idelay_samples = (long)delay_samples;
	    frac = delay_samples - idelay_samples;
	    read = LIN_INTERP (frac,
	                       buffer[read_phase & buffer_mask], 
	                       buffer[(read_phase-1) & buffer_mask]);
	    buffer[write_phase & buffer_mask] = read * feedback + in[i];
	    buffer_write(out[i], read);

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

static void setRunAddingGainComb_l(LADSPA_Handle instance, LADSPA_Data gain) {
	((Comb_l *)instance)->run_adding_gain = gain;
}

static void runAddingComb_l(LADSPA_Handle instance, unsigned long sample_count) {
	Comb_l *plugin_data = (Comb_l *)instance;
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

	i = max_delay;

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

	    buffer[write_phase & buffer_mask] = read * feedback + in[i];
	    buffer_write(out[i], read);
	    write_phase++;
	  }
	} else {
	  float next_delay_samples = CALC_DELAY (delay_time);
	  float delay_samples_slope = (next_delay_samples - delay_samples) / sample_count;
	  float next_feedback = calc_feedback (delay_time, decay_time);
	  float feedback_slope = (next_feedback - feedback) / sample_count;

	  for (i=0; i<sample_count; i++) {
	    long read_phase, idelay_samples;
	    LADSPA_Data read, frac;

	    delay_samples += delay_samples_slope;
	    write_phase++;
	    read_phase = write_phase - (long)delay_samples;
	    idelay_samples = (long)delay_samples;
	    frac = delay_samples - idelay_samples;
	    read = LIN_INTERP (frac,
	                       buffer[read_phase & buffer_mask], 
	                       buffer[(read_phase-1) & buffer_mask]);
	    buffer[write_phase & buffer_mask] = read * feedback + in[i];
	    buffer_write(out[i], read);

	    feedback += feedback_slope;
	  }

	  plugin_data->last_delay_time = delay_time;
	  plugin_data->last_decay_time = decay_time;
	  plugin_data->feedback = feedback;
	  plugin_data->delay_samples = delay_samples;
	}
	
	plugin_data->write_phase = write_phase;
}

static void activateComb_c(LADSPA_Handle instance) {
	Comb_c *plugin_data = (Comb_c *)instance;
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

static void cleanupComb_c(LADSPA_Handle instance) {
	Comb_c *plugin_data = (Comb_c *)instance;
	free(plugin_data->buffer);
	free(instance);
}

static void connectPortComb_c(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Comb_c *plugin;

	plugin = (Comb_c *)instance;
	switch (port) {
	case COMB_C_IN:
		plugin->in = data;
		break;
	case COMB_C_OUT:
		plugin->out = data;
		break;
	case COMB_C_MAX_DELAY:
		plugin->max_delay = data;
		break;
	case COMB_C_DELAY_TIME:
		plugin->delay_time = data;
		break;
	case COMB_C_DECAY_TIME:
		plugin->decay_time = data;
		break;
	}
}

static LADSPA_Handle instantiateComb_c(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Comb_c *plugin_data = (Comb_c *)malloc(sizeof(Comb_c));
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

static void runComb_c(LADSPA_Handle instance, unsigned long sample_count) {
	Comb_c *plugin_data = (Comb_c *)instance;

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

	i = max_delay;

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

	    buffer[write_phase++ & buffer_mask] = read * feedback + in[i];
	    buffer_write(out[i], read);
	  }
	} else {
	  float next_delay_samples = CALC_DELAY (delay_time);
	  float delay_samples_slope = (next_delay_samples - delay_samples) / sample_count;
	  float next_feedback = calc_feedback (delay_time, decay_time);
	  float feedback_slope = (next_feedback - feedback) / sample_count;

	  for (i=0; i<sample_count; i++) {
	    long read_phase, idelay_samples;
	    LADSPA_Data read, frac;

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

	    buffer[write_phase & buffer_mask] = read * feedback + in[i];
	    buffer_write(out[i], read);

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

static void setRunAddingGainComb_c(LADSPA_Handle instance, LADSPA_Data gain) {
	((Comb_c *)instance)->run_adding_gain = gain;
}

static void runAddingComb_c(LADSPA_Handle instance, unsigned long sample_count) {
	Comb_c *plugin_data = (Comb_c *)instance;
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

	i = max_delay;

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

	    buffer[write_phase++ & buffer_mask] = read * feedback + in[i];
	    buffer_write(out[i], read);
	  }
	} else {
	  float next_delay_samples = CALC_DELAY (delay_time);
	  float delay_samples_slope = (next_delay_samples - delay_samples) / sample_count;
	  float next_feedback = calc_feedback (delay_time, decay_time);
	  float feedback_slope = (next_feedback - feedback) / sample_count;

	  for (i=0; i<sample_count; i++) {
	    long read_phase, idelay_samples;
	    LADSPA_Data read, frac;

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

	    buffer[write_phase & buffer_mask] = read * feedback + in[i];
	    buffer_write(out[i], read);

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


	comb_nDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (comb_nDescriptor) {
		comb_nDescriptor->UniqueID = 1889;
		comb_nDescriptor->Label = "comb_n";
		comb_nDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		comb_nDescriptor->Name =
		 D_("Comb delay line, noninterpolating");
		comb_nDescriptor->Maker =
		 "Andy Wingo <wingo at pobox dot com>";
		comb_nDescriptor->Copyright =
		 "GPL";
		comb_nDescriptor->PortCount = 5;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(5,
		 sizeof(LADSPA_PortDescriptor));
		comb_nDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(5,
		 sizeof(LADSPA_PortRangeHint));
		comb_nDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(5, sizeof(char*));
		comb_nDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Input */
		port_descriptors[COMB_N_IN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[COMB_N_IN] =
		 D_("Input");
		port_range_hints[COMB_N_IN].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[COMB_N_OUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[COMB_N_OUT] =
		 D_("Output");
		port_range_hints[COMB_N_OUT].HintDescriptor = 0;

		/* Parameters for Max Delay (s) */
		port_descriptors[COMB_N_MAX_DELAY] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[COMB_N_MAX_DELAY] =
		 D_("Max Delay (s)");
		port_range_hints[COMB_N_MAX_DELAY].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[COMB_N_MAX_DELAY].LowerBound = 0;

		/* Parameters for Delay Time (s) */
		port_descriptors[COMB_N_DELAY_TIME] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[COMB_N_DELAY_TIME] =
		 D_("Delay Time (s)");
		port_range_hints[COMB_N_DELAY_TIME].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[COMB_N_DELAY_TIME].LowerBound = 0;

		/* Parameters for Decay Time (s) */
		port_descriptors[COMB_N_DECAY_TIME] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[COMB_N_DECAY_TIME] =
		 D_("Decay Time (s)");
		port_range_hints[COMB_N_DECAY_TIME].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[COMB_N_DECAY_TIME].LowerBound = 0;

		comb_nDescriptor->activate = activateComb_n;
		comb_nDescriptor->cleanup = cleanupComb_n;
		comb_nDescriptor->connect_port = connectPortComb_n;
		comb_nDescriptor->deactivate = NULL;
		comb_nDescriptor->instantiate = instantiateComb_n;
		comb_nDescriptor->run = runComb_n;
		comb_nDescriptor->run_adding = runAddingComb_n;
		comb_nDescriptor->set_run_adding_gain = setRunAddingGainComb_n;
	}

	comb_lDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (comb_lDescriptor) {
		comb_lDescriptor->UniqueID = 1887;
		comb_lDescriptor->Label = "comb_l";
		comb_lDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		comb_lDescriptor->Name =
		 D_("Comb delay line, linear interpolation");
		comb_lDescriptor->Maker =
		 "Andy Wingo <wingo at pobox dot com>";
		comb_lDescriptor->Copyright =
		 "GPL";
		comb_lDescriptor->PortCount = 5;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(5,
		 sizeof(LADSPA_PortDescriptor));
		comb_lDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(5,
		 sizeof(LADSPA_PortRangeHint));
		comb_lDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(5, sizeof(char*));
		comb_lDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Input */
		port_descriptors[COMB_L_IN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[COMB_L_IN] =
		 D_("Input");
		port_range_hints[COMB_L_IN].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[COMB_L_OUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[COMB_L_OUT] =
		 D_("Output");
		port_range_hints[COMB_L_OUT].HintDescriptor = 0;

		/* Parameters for Max Delay (s) */
		port_descriptors[COMB_L_MAX_DELAY] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[COMB_L_MAX_DELAY] =
		 D_("Max Delay (s)");
		port_range_hints[COMB_L_MAX_DELAY].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[COMB_L_MAX_DELAY].LowerBound = 0;

		/* Parameters for Delay Time (s) */
		port_descriptors[COMB_L_DELAY_TIME] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[COMB_L_DELAY_TIME] =
		 D_("Delay Time (s)");
		port_range_hints[COMB_L_DELAY_TIME].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[COMB_L_DELAY_TIME].LowerBound = 0;

		/* Parameters for Decay Time (s) */
		port_descriptors[COMB_L_DECAY_TIME] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[COMB_L_DECAY_TIME] =
		 D_("Decay Time (s)");
		port_range_hints[COMB_L_DECAY_TIME].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[COMB_L_DECAY_TIME].LowerBound = 0;

		comb_lDescriptor->activate = activateComb_l;
		comb_lDescriptor->cleanup = cleanupComb_l;
		comb_lDescriptor->connect_port = connectPortComb_l;
		comb_lDescriptor->deactivate = NULL;
		comb_lDescriptor->instantiate = instantiateComb_l;
		comb_lDescriptor->run = runComb_l;
		comb_lDescriptor->run_adding = runAddingComb_l;
		comb_lDescriptor->set_run_adding_gain = setRunAddingGainComb_l;
	}

	comb_cDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (comb_cDescriptor) {
		comb_cDescriptor->UniqueID = 1888;
		comb_cDescriptor->Label = "comb_c";
		comb_cDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		comb_cDescriptor->Name =
		 D_("Comb delay line, cubic spline interpolation");
		comb_cDescriptor->Maker =
		 "Andy Wingo <wingo at pobox dot com>";
		comb_cDescriptor->Copyright =
		 "GPL";
		comb_cDescriptor->PortCount = 5;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(5,
		 sizeof(LADSPA_PortDescriptor));
		comb_cDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(5,
		 sizeof(LADSPA_PortRangeHint));
		comb_cDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(5, sizeof(char*));
		comb_cDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Input */
		port_descriptors[COMB_C_IN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[COMB_C_IN] =
		 D_("Input");
		port_range_hints[COMB_C_IN].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[COMB_C_OUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[COMB_C_OUT] =
		 D_("Output");
		port_range_hints[COMB_C_OUT].HintDescriptor = 0;

		/* Parameters for Max Delay (s) */
		port_descriptors[COMB_C_MAX_DELAY] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[COMB_C_MAX_DELAY] =
		 D_("Max Delay (s)");
		port_range_hints[COMB_C_MAX_DELAY].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[COMB_C_MAX_DELAY].LowerBound = 0;

		/* Parameters for Delay Time (s) */
		port_descriptors[COMB_C_DELAY_TIME] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[COMB_C_DELAY_TIME] =
		 D_("Delay Time (s)");
		port_range_hints[COMB_C_DELAY_TIME].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[COMB_C_DELAY_TIME].LowerBound = 0;

		/* Parameters for Decay Time (s) */
		port_descriptors[COMB_C_DECAY_TIME] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[COMB_C_DECAY_TIME] =
		 D_("Decay Time (s)");
		port_range_hints[COMB_C_DECAY_TIME].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[COMB_C_DECAY_TIME].LowerBound = 0;

		comb_cDescriptor->activate = activateComb_c;
		comb_cDescriptor->cleanup = cleanupComb_c;
		comb_cDescriptor->connect_port = connectPortComb_c;
		comb_cDescriptor->deactivate = NULL;
		comb_cDescriptor->instantiate = instantiateComb_c;
		comb_cDescriptor->run = runComb_c;
		comb_cDescriptor->run_adding = runAddingComb_c;
		comb_cDescriptor->set_run_adding_gain = setRunAddingGainComb_c;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (comb_nDescriptor) {
		free((LADSPA_PortDescriptor *)comb_nDescriptor->PortDescriptors);
		free((char **)comb_nDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)comb_nDescriptor->PortRangeHints);
		free(comb_nDescriptor);
	}
	if (comb_lDescriptor) {
		free((LADSPA_PortDescriptor *)comb_lDescriptor->PortDescriptors);
		free((char **)comb_lDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)comb_lDescriptor->PortRangeHints);
		free(comb_lDescriptor);
	}
	if (comb_cDescriptor) {
		free((LADSPA_PortDescriptor *)comb_cDescriptor->PortDescriptors);
		free((char **)comb_cDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)comb_cDescriptor->PortRangeHints);
		free(comb_cDescriptor);
	}

}
