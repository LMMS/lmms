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

#line 10 "giant_flange_1437.xml"

#include <sys/types.h>
#include "ladspa-util.h"

#define INT_SCALE   16384.0f
/* INT_SCALE reciprocal includes factor of two scaling */
#define INT_SCALE_R 0.000030517578125f

#define MAX_AMP 1.0f
#define CLIP 0.8f
#define CLIP_A ((MAX_AMP - CLIP) * (MAX_AMP - CLIP))
#define CLIP_B (MAX_AMP - 2.0f * CLIP)

#define GIANTFLANGE_DELDOUBLE          0
#define GIANTFLANGE_FREQ1              1
#define GIANTFLANGE_DELAY1             2
#define GIANTFLANGE_FREQ2              3
#define GIANTFLANGE_DELAY2             4
#define GIANTFLANGE_FEEDBACK           5
#define GIANTFLANGE_WET                6
#define GIANTFLANGE_INPUT              7
#define GIANTFLANGE_OUTPUT             8

static LADSPA_Descriptor *giantFlangeDescriptor = NULL;

typedef struct {
	LADSPA_Data *deldouble;
	LADSPA_Data *freq1;
	LADSPA_Data *delay1;
	LADSPA_Data *freq2;
	LADSPA_Data *delay2;
	LADSPA_Data *feedback;
	LADSPA_Data *wet;
	LADSPA_Data *input;
	LADSPA_Data *output;
	int16_t *    buffer;
	unsigned int buffer_mask;
	unsigned int buffer_pos;
	float        fs;
	float        x1;
	float        x2;
	float        y1;
	float        y2;
	LADSPA_Data run_adding_gain;
} GiantFlange;

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
		return giantFlangeDescriptor;
	default:
		return NULL;
	}
}

static void activateGiantFlange(LADSPA_Handle instance) {
	GiantFlange *plugin_data = (GiantFlange *)instance;
	int16_t *buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	unsigned int buffer_pos = plugin_data->buffer_pos;
	float fs = plugin_data->fs;
	float x1 = plugin_data->x1;
	float x2 = plugin_data->x2;
	float y1 = plugin_data->y1;
	float y2 = plugin_data->y2;
#line 51 "giant_flange_1437.xml"
	memset(buffer, 0, (buffer_mask + 1) * sizeof(int16_t));
	plugin_data->buffer = buffer;
	plugin_data->buffer_mask = buffer_mask;
	plugin_data->buffer_pos = buffer_pos;
	plugin_data->fs = fs;
	plugin_data->x1 = x1;
	plugin_data->x2 = x2;
	plugin_data->y1 = y1;
	plugin_data->y2 = y2;

}

static void cleanupGiantFlange(LADSPA_Handle instance) {
#line 55 "giant_flange_1437.xml"
	GiantFlange *plugin_data = (GiantFlange *)instance;
	free(plugin_data->buffer);
	free(instance);
}

static void connectPortGiantFlange(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	GiantFlange *plugin;

	plugin = (GiantFlange *)instance;
	switch (port) {
	case GIANTFLANGE_DELDOUBLE:
		plugin->deldouble = data;
		break;
	case GIANTFLANGE_FREQ1:
		plugin->freq1 = data;
		break;
	case GIANTFLANGE_DELAY1:
		plugin->delay1 = data;
		break;
	case GIANTFLANGE_FREQ2:
		plugin->freq2 = data;
		break;
	case GIANTFLANGE_DELAY2:
		plugin->delay2 = data;
		break;
	case GIANTFLANGE_FEEDBACK:
		plugin->feedback = data;
		break;
	case GIANTFLANGE_WET:
		plugin->wet = data;
		break;
	case GIANTFLANGE_INPUT:
		plugin->input = data;
		break;
	case GIANTFLANGE_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateGiantFlange(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	GiantFlange *plugin_data = (GiantFlange *)malloc(sizeof(GiantFlange));
	int16_t *buffer = NULL;
	unsigned int buffer_mask;
	unsigned int buffer_pos;
	float fs;
	float x1;
	float x2;
	float y1;
	float y2;

#line 35 "giant_flange_1437.xml"
	int buffer_size = 32768;

	fs = s_rate;
	while (buffer_size < fs * 10.5f) {
	  buffer_size *= 2;
	}
	buffer = calloc(buffer_size, sizeof(int16_t));
	buffer_mask = buffer_size - 1;
	buffer_pos = 0;
	x1 = 0.5f;
	y1 = 0.0f;
	x2 = 0.5f;
	y2 = 0.0f;

	plugin_data->buffer = buffer;
	plugin_data->buffer_mask = buffer_mask;
	plugin_data->buffer_pos = buffer_pos;
	plugin_data->fs = fs;
	plugin_data->x1 = x1;
	plugin_data->x2 = x2;
	plugin_data->y1 = y1;
	plugin_data->y2 = y2;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runGiantFlange(LADSPA_Handle instance, unsigned long sample_count) {
	GiantFlange *plugin_data = (GiantFlange *)instance;

	/* Double delay (float value) */
	const LADSPA_Data deldouble = *(plugin_data->deldouble);

	/* LFO frequency 1 (Hz) (float value) */
	const LADSPA_Data freq1 = *(plugin_data->freq1);

	/* Delay 1 range (s) (float value) */
	const LADSPA_Data delay1 = *(plugin_data->delay1);

	/* LFO frequency 2 (Hz) (float value) */
	const LADSPA_Data freq2 = *(plugin_data->freq2);

	/* Delay 2 range (s) (float value) */
	const LADSPA_Data delay2 = *(plugin_data->delay2);

	/* Feedback (float value) */
	const LADSPA_Data feedback = *(plugin_data->feedback);

	/* Dry/Wet level (float value) */
	const LADSPA_Data wet = *(plugin_data->wet);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	int16_t * buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	unsigned int buffer_pos = plugin_data->buffer_pos;
	float fs = plugin_data->fs;
	float x1 = plugin_data->x1;
	float x2 = plugin_data->x2;
	float y1 = plugin_data->y1;
	float y2 = plugin_data->y2;

#line 59 "giant_flange_1437.xml"
	unsigned long pos;
	const float omega1 = 6.2831852f * (freq1 / fs);
	const float omega2 = 6.2831852f * (freq2 / fs);
	float fb;
	float d1, d2;
	float d1out, d2out;
	float fbs;

	if (feedback > 99.0f) {
	  fb = 0.99f;
	} else if (feedback < -99.0f) {
	  fb = -0.99f;
	} else {
	  fb = feedback * 0.01f;
	}

	if (f_round(deldouble)) {
	  const float dr1 = delay1 * fs * 0.25f;
	  const float dr2 = delay2 * fs * 0.25f;

	for (pos = 0; pos < sample_count; pos++) {
	  /* Write input into delay line */
	  buffer[buffer_pos] = f_round(input[pos] * INT_SCALE);

	  /* Calcuate delays */
	  d1 = (x1 + 1.0f) * dr1;
	  d2 = (y2 + 1.0f) * dr2;

	  d1out = buffer[(buffer_pos - f_round(d1)) & buffer_mask] * INT_SCALE_R;
	  d2out = buffer[(buffer_pos - f_round(d2)) & buffer_mask] * INT_SCALE_R;

	  /* Add feedback, must be done afterwards for case where delay = 0 */
	  fbs = input[pos] + (d1out + d2out) * fb;
	  if(fbs < CLIP && fbs > -CLIP) {
	    buffer[buffer_pos] = fbs * INT_SCALE;
	  } else if (fbs > 0.0f) {
	    buffer[buffer_pos] = (MAX_AMP - (CLIP_A / (CLIP_B + fbs))) *
	                                  INT_SCALE;
	  } else {
	    buffer[buffer_pos] =  (MAX_AMP - (CLIP_A / (CLIP_B - fbs))) *
	                                  -INT_SCALE;
	  }

	  /* Write output */
	  buffer_write(output[pos], LIN_INTERP(wet, input[pos], d1out + d2out));

	  if (pos % 2) {
	    buffer_pos = (buffer_pos + 1) & buffer_mask;
	  }

	  /* Run LFOs */
	  x1 -= omega1 * y1;
	  y1 += omega1 * x1;
	  x2 -= omega2 * y2;
	  y2 += omega2 * x2;
	}
	} else {
	  const float dr1 = delay1 * fs * 0.5f;
	  const float dr2 = delay2 * fs * 0.5f;

	for (pos = 0; pos < sample_count; pos++) {
	  /* Write input into delay line */
	  buffer[buffer_pos] = f_round(input[pos] * INT_SCALE);

	  /* Calcuate delays */
	  d1 = (x1 + 1.0f) * dr1;
	  d2 = (y2 + 1.0f) * dr2;

	  d1out = buffer[(buffer_pos - f_round(d1)) & buffer_mask] * INT_SCALE_R;
	  d2out = buffer[(buffer_pos - f_round(d2)) & buffer_mask] * INT_SCALE_R;

	  /* Add feedback, must be done afterwards for case where delay = 0 */
	  fbs = input[pos] + (d1out + d2out) * fb;
	  if(fbs < CLIP && fbs > -CLIP) {
	          buffer[buffer_pos] = fbs * INT_SCALE;
	  } else if (fbs > 0.0f) {
	          buffer[buffer_pos] = (MAX_AMP - (CLIP_A / (CLIP_B + fbs))) *
	                                  INT_SCALE;
	  } else {
	          buffer[buffer_pos] =  (MAX_AMP - (CLIP_A / (CLIP_B - fbs))) *
	                                  -INT_SCALE;
	  }

	  /* Write output */
	  buffer_write(output[pos], LIN_INTERP(wet, input[pos], d1out + d2out));

	  buffer_pos = (buffer_pos + 1) & buffer_mask;

	  /* Run LFOs */
	  x1 -= omega1 * y1;
	  y1 += omega1 * x1;
	  x2 -= omega2 * y2;
	  y2 += omega2 * x2;
	}
	}

	plugin_data->x1 = x1;
	plugin_data->y1 = y1;
	plugin_data->x2 = x2;
	plugin_data->y2 = y2;
	plugin_data->buffer_pos = buffer_pos;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainGiantFlange(LADSPA_Handle instance, LADSPA_Data gain) {
	((GiantFlange *)instance)->run_adding_gain = gain;
}

static void runAddingGiantFlange(LADSPA_Handle instance, unsigned long sample_count) {
	GiantFlange *plugin_data = (GiantFlange *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Double delay (float value) */
	const LADSPA_Data deldouble = *(plugin_data->deldouble);

	/* LFO frequency 1 (Hz) (float value) */
	const LADSPA_Data freq1 = *(plugin_data->freq1);

	/* Delay 1 range (s) (float value) */
	const LADSPA_Data delay1 = *(plugin_data->delay1);

	/* LFO frequency 2 (Hz) (float value) */
	const LADSPA_Data freq2 = *(plugin_data->freq2);

	/* Delay 2 range (s) (float value) */
	const LADSPA_Data delay2 = *(plugin_data->delay2);

	/* Feedback (float value) */
	const LADSPA_Data feedback = *(plugin_data->feedback);

	/* Dry/Wet level (float value) */
	const LADSPA_Data wet = *(plugin_data->wet);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	int16_t * buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	unsigned int buffer_pos = plugin_data->buffer_pos;
	float fs = plugin_data->fs;
	float x1 = plugin_data->x1;
	float x2 = plugin_data->x2;
	float y1 = plugin_data->y1;
	float y2 = plugin_data->y2;

#line 59 "giant_flange_1437.xml"
	unsigned long pos;
	const float omega1 = 6.2831852f * (freq1 / fs);
	const float omega2 = 6.2831852f * (freq2 / fs);
	float fb;
	float d1, d2;
	float d1out, d2out;
	float fbs;

	if (feedback > 99.0f) {
	  fb = 0.99f;
	} else if (feedback < -99.0f) {
	  fb = -0.99f;
	} else {
	  fb = feedback * 0.01f;
	}

	if (f_round(deldouble)) {
	  const float dr1 = delay1 * fs * 0.25f;
	  const float dr2 = delay2 * fs * 0.25f;

	for (pos = 0; pos < sample_count; pos++) {
	  /* Write input into delay line */
	  buffer[buffer_pos] = f_round(input[pos] * INT_SCALE);

	  /* Calcuate delays */
	  d1 = (x1 + 1.0f) * dr1;
	  d2 = (y2 + 1.0f) * dr2;

	  d1out = buffer[(buffer_pos - f_round(d1)) & buffer_mask] * INT_SCALE_R;
	  d2out = buffer[(buffer_pos - f_round(d2)) & buffer_mask] * INT_SCALE_R;

	  /* Add feedback, must be done afterwards for case where delay = 0 */
	  fbs = input[pos] + (d1out + d2out) * fb;
	  if(fbs < CLIP && fbs > -CLIP) {
	    buffer[buffer_pos] = fbs * INT_SCALE;
	  } else if (fbs > 0.0f) {
	    buffer[buffer_pos] = (MAX_AMP - (CLIP_A / (CLIP_B + fbs))) *
	                                  INT_SCALE;
	  } else {
	    buffer[buffer_pos] =  (MAX_AMP - (CLIP_A / (CLIP_B - fbs))) *
	                                  -INT_SCALE;
	  }

	  /* Write output */
	  buffer_write(output[pos], LIN_INTERP(wet, input[pos], d1out + d2out));

	  if (pos % 2) {
	    buffer_pos = (buffer_pos + 1) & buffer_mask;
	  }

	  /* Run LFOs */
	  x1 -= omega1 * y1;
	  y1 += omega1 * x1;
	  x2 -= omega2 * y2;
	  y2 += omega2 * x2;
	}
	} else {
	  const float dr1 = delay1 * fs * 0.5f;
	  const float dr2 = delay2 * fs * 0.5f;

	for (pos = 0; pos < sample_count; pos++) {
	  /* Write input into delay line */
	  buffer[buffer_pos] = f_round(input[pos] * INT_SCALE);

	  /* Calcuate delays */
	  d1 = (x1 + 1.0f) * dr1;
	  d2 = (y2 + 1.0f) * dr2;

	  d1out = buffer[(buffer_pos - f_round(d1)) & buffer_mask] * INT_SCALE_R;
	  d2out = buffer[(buffer_pos - f_round(d2)) & buffer_mask] * INT_SCALE_R;

	  /* Add feedback, must be done afterwards for case where delay = 0 */
	  fbs = input[pos] + (d1out + d2out) * fb;
	  if(fbs < CLIP && fbs > -CLIP) {
	          buffer[buffer_pos] = fbs * INT_SCALE;
	  } else if (fbs > 0.0f) {
	          buffer[buffer_pos] = (MAX_AMP - (CLIP_A / (CLIP_B + fbs))) *
	                                  INT_SCALE;
	  } else {
	          buffer[buffer_pos] =  (MAX_AMP - (CLIP_A / (CLIP_B - fbs))) *
	                                  -INT_SCALE;
	  }

	  /* Write output */
	  buffer_write(output[pos], LIN_INTERP(wet, input[pos], d1out + d2out));

	  buffer_pos = (buffer_pos + 1) & buffer_mask;

	  /* Run LFOs */
	  x1 -= omega1 * y1;
	  y1 += omega1 * x1;
	  x2 -= omega2 * y2;
	  y2 += omega2 * x2;
	}
	}

	plugin_data->x1 = x1;
	plugin_data->y1 = y1;
	plugin_data->x2 = x2;
	plugin_data->y2 = y2;
	plugin_data->buffer_pos = buffer_pos;
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


	giantFlangeDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (giantFlangeDescriptor) {
		giantFlangeDescriptor->UniqueID = 1437;
		giantFlangeDescriptor->Label = "giantFlange";
		giantFlangeDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		giantFlangeDescriptor->Name =
		 D_("Giant flange");
		giantFlangeDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		giantFlangeDescriptor->Copyright =
		 "GPL";
		giantFlangeDescriptor->PortCount = 9;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(9,
		 sizeof(LADSPA_PortDescriptor));
		giantFlangeDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(9,
		 sizeof(LADSPA_PortRangeHint));
		giantFlangeDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(9, sizeof(char*));
		giantFlangeDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Double delay */
		port_descriptors[GIANTFLANGE_DELDOUBLE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GIANTFLANGE_DELDOUBLE] =
		 D_("Double delay");
		port_range_hints[GIANTFLANGE_DELDOUBLE].HintDescriptor = 0;

		/* Parameters for LFO frequency 1 (Hz) */
		port_descriptors[GIANTFLANGE_FREQ1] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GIANTFLANGE_FREQ1] =
		 D_("LFO frequency 1 (Hz)");
		port_range_hints[GIANTFLANGE_FREQ1].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[GIANTFLANGE_FREQ1].LowerBound = 0;
		port_range_hints[GIANTFLANGE_FREQ1].UpperBound = 30.0;

		/* Parameters for Delay 1 range (s) */
		port_descriptors[GIANTFLANGE_DELAY1] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GIANTFLANGE_DELAY1] =
		 D_("Delay 1 range (s)");
		port_range_hints[GIANTFLANGE_DELAY1].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[GIANTFLANGE_DELAY1].LowerBound = 0;
		port_range_hints[GIANTFLANGE_DELAY1].UpperBound = 10.5;

		/* Parameters for LFO frequency 2 (Hz) */
		port_descriptors[GIANTFLANGE_FREQ2] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GIANTFLANGE_FREQ2] =
		 D_("LFO frequency 2 (Hz)");
		port_range_hints[GIANTFLANGE_FREQ2].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[GIANTFLANGE_FREQ2].LowerBound = 0;
		port_range_hints[GIANTFLANGE_FREQ2].UpperBound = 30.0;

		/* Parameters for Delay 2 range (s) */
		port_descriptors[GIANTFLANGE_DELAY2] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GIANTFLANGE_DELAY2] =
		 D_("Delay 2 range (s)");
		port_range_hints[GIANTFLANGE_DELAY2].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[GIANTFLANGE_DELAY2].LowerBound = 0;
		port_range_hints[GIANTFLANGE_DELAY2].UpperBound = 10.5;

		/* Parameters for Feedback */
		port_descriptors[GIANTFLANGE_FEEDBACK] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GIANTFLANGE_FEEDBACK] =
		 D_("Feedback");
		port_range_hints[GIANTFLANGE_FEEDBACK].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[GIANTFLANGE_FEEDBACK].LowerBound = -100;
		port_range_hints[GIANTFLANGE_FEEDBACK].UpperBound = 100;

		/* Parameters for Dry/Wet level */
		port_descriptors[GIANTFLANGE_WET] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GIANTFLANGE_WET] =
		 D_("Dry/Wet level");
		port_range_hints[GIANTFLANGE_WET].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[GIANTFLANGE_WET].LowerBound = 0;
		port_range_hints[GIANTFLANGE_WET].UpperBound = 1;

		/* Parameters for Input */
		port_descriptors[GIANTFLANGE_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[GIANTFLANGE_INPUT] =
		 D_("Input");
		port_range_hints[GIANTFLANGE_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[GIANTFLANGE_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[GIANTFLANGE_OUTPUT] =
		 D_("Output");
		port_range_hints[GIANTFLANGE_OUTPUT].HintDescriptor = 0;

		giantFlangeDescriptor->activate = activateGiantFlange;
		giantFlangeDescriptor->cleanup = cleanupGiantFlange;
		giantFlangeDescriptor->connect_port = connectPortGiantFlange;
		giantFlangeDescriptor->deactivate = NULL;
		giantFlangeDescriptor->instantiate = instantiateGiantFlange;
		giantFlangeDescriptor->run = runGiantFlange;
		giantFlangeDescriptor->run_adding = runAddingGiantFlange;
		giantFlangeDescriptor->set_run_adding_gain = setRunAddingGainGiantFlange;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (giantFlangeDescriptor) {
		free((LADSPA_PortDescriptor *)giantFlangeDescriptor->PortDescriptors);
		free((char **)giantFlangeDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)giantFlangeDescriptor->PortRangeHints);
		free(giantFlangeDescriptor);
	}

}
