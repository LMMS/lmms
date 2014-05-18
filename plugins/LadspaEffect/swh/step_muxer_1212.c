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

#line 10 "step_muxer_1212.xml"

#define FADE_IN  1
#define STABLE   2
#define FADE_OUT 3

#define STEPMUXER_XFADET               0
#define STEPMUXER_CLOCK                1
#define STEPMUXER_INPUT0               2
#define STEPMUXER_INPUT1               3
#define STEPMUXER_INPUT2               4
#define STEPMUXER_INPUT3               5
#define STEPMUXER_INPUT4               6
#define STEPMUXER_INPUT5               7
#define STEPMUXER_INPUT6               8
#define STEPMUXER_INPUT7               9
#define STEPMUXER_OUTPUT               10

static LADSPA_Descriptor *stepMuxerDescriptor = NULL;

typedef struct {
	LADSPA_Data *xfadet;
	LADSPA_Data *clock;
	LADSPA_Data *input0;
	LADSPA_Data *input1;
	LADSPA_Data *input2;
	LADSPA_Data *input3;
	LADSPA_Data *input4;
	LADSPA_Data *input5;
	LADSPA_Data *input6;
	LADSPA_Data *input7;
	LADSPA_Data *output;
	float *      ch_gain;
	int *        ch_state;
	int          current_ch;
	LADSPA_Data  last_clock;
	float        sample_rate;
	LADSPA_Data run_adding_gain;
} StepMuxer;

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
		return stepMuxerDescriptor;
	default:
		return NULL;
	}
}

static void activateStepMuxer(LADSPA_Handle instance) {
	StepMuxer *plugin_data = (StepMuxer *)instance;
	float *ch_gain = plugin_data->ch_gain;
	int *ch_state = plugin_data->ch_state;
	int current_ch = plugin_data->current_ch;
	LADSPA_Data last_clock = plugin_data->last_clock;
	float sample_rate = plugin_data->sample_rate;
#line 31 "step_muxer_1212.xml"
	int i;

	ch_state[0] = STABLE;
	ch_gain[0] = 1.0f;
	for (i = 1; i < 8; i++) {
	  ch_state[i] = STABLE;
	  ch_gain[i] = 0.0f;
	}
	current_ch = 0;
	last_clock = 0.0f;
	sample_rate = sample_rate;
	plugin_data->ch_gain = ch_gain;
	plugin_data->ch_state = ch_state;
	plugin_data->current_ch = current_ch;
	plugin_data->last_clock = last_clock;
	plugin_data->sample_rate = sample_rate;

}

static void cleanupStepMuxer(LADSPA_Handle instance) {
#line 45 "step_muxer_1212.xml"
	StepMuxer *plugin_data = (StepMuxer *)instance;
	free(plugin_data->ch_state);
	free(plugin_data->ch_gain);
	free(instance);
}

static void connectPortStepMuxer(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	StepMuxer *plugin;

	plugin = (StepMuxer *)instance;
	switch (port) {
	case STEPMUXER_XFADET:
		plugin->xfadet = data;
		break;
	case STEPMUXER_CLOCK:
		plugin->clock = data;
		break;
	case STEPMUXER_INPUT0:
		plugin->input0 = data;
		break;
	case STEPMUXER_INPUT1:
		plugin->input1 = data;
		break;
	case STEPMUXER_INPUT2:
		plugin->input2 = data;
		break;
	case STEPMUXER_INPUT3:
		plugin->input3 = data;
		break;
	case STEPMUXER_INPUT4:
		plugin->input4 = data;
		break;
	case STEPMUXER_INPUT5:
		plugin->input5 = data;
		break;
	case STEPMUXER_INPUT6:
		plugin->input6 = data;
		break;
	case STEPMUXER_INPUT7:
		plugin->input7 = data;
		break;
	case STEPMUXER_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateStepMuxer(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	StepMuxer *plugin_data = (StepMuxer *)malloc(sizeof(StepMuxer));
	float *ch_gain = NULL;
	int *ch_state = NULL;
	int current_ch;
	LADSPA_Data last_clock;
	float sample_rate;

#line 23 "step_muxer_1212.xml"
	sample_rate = s_rate;
	ch_state = malloc(sizeof(int) * 8);
	ch_gain = malloc(sizeof(float) * 8);
	current_ch = 0;
	last_clock = 0.0f;

	plugin_data->ch_gain = ch_gain;
	plugin_data->ch_state = ch_state;
	plugin_data->current_ch = current_ch;
	plugin_data->last_clock = last_clock;
	plugin_data->sample_rate = sample_rate;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runStepMuxer(LADSPA_Handle instance, unsigned long sample_count) {
	StepMuxer *plugin_data = (StepMuxer *)instance;

	/* Crossfade time (in ms) (float value) */
	const LADSPA_Data xfadet = *(plugin_data->xfadet);

	/* Clock (array of floats of length sample_count) */
	const LADSPA_Data * const clock = plugin_data->clock;

	/* Input 1 (array of floats of length sample_count) */
	const LADSPA_Data * const input0 = plugin_data->input0;

	/* Input 2 (array of floats of length sample_count) */
	const LADSPA_Data * const input1 = plugin_data->input1;

	/* Input 3 (array of floats of length sample_count) */
	const LADSPA_Data * const input2 = plugin_data->input2;

	/* Input 4 (array of floats of length sample_count) */
	const LADSPA_Data * const input3 = plugin_data->input3;

	/* Input 5 (array of floats of length sample_count) */
	const LADSPA_Data * const input4 = plugin_data->input4;

	/* Input 6 (array of floats of length sample_count) */
	const LADSPA_Data * const input5 = plugin_data->input5;

	/* Input 7 (array of floats of length sample_count) */
	const LADSPA_Data * const input6 = plugin_data->input6;

	/* Input 8 (array of floats of length sample_count) */
	const LADSPA_Data * const input7 = plugin_data->input7;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	float * ch_gain = plugin_data->ch_gain;
	int * ch_state = plugin_data->ch_state;
	int current_ch = plugin_data->current_ch;
	LADSPA_Data last_clock = plugin_data->last_clock;
	float sample_rate = plugin_data->sample_rate;

#line 50 "step_muxer_1212.xml"
	unsigned long pos;
	float fade_inc = 1.0f / (xfadet * sample_rate * 1000.0f);
	float accum;
	int ch;

	for (pos = 0; pos < sample_count; pos++) {

	  // Calculate output value for this sample
	  accum = 0.0f;
	  accum += input0[pos] * ch_gain[0];
	  accum += input1[pos] * ch_gain[1];
	  accum += input2[pos] * ch_gain[2];
	  accum += input3[pos] * ch_gain[3];
	  accum += input4[pos] * ch_gain[4];
	  accum += input5[pos] * ch_gain[5];
	  accum += input6[pos] * ch_gain[6];
	  accum += input7[pos] * ch_gain[7];
	  buffer_write(output[pos], accum);

	  // Run crossfades
	  for (ch = 0; ch < 8; ch++) {

	    // Channel is still being faded in
	    if (ch_state[ch] == FADE_IN) {
	      ch_gain[ch] += fade_inc;
	      if (ch_gain[ch] >= 1.0f) {
	        ch_gain[ch] = 1.0f;
	        ch_state[ch] = STABLE;
	      }

	    // Channel is still being faded out
	    } else if (ch_state[ch] == FADE_OUT) {
	      ch_gain[ch] -= fade_inc;
	      if (ch_gain[ch] <= 0.0f) {
	        ch_gain[ch] = 0.0f;
	        ch_state[ch] = STABLE;
	      }
	    }
	  }

	  // Check for clock signal
	  if (last_clock <= 0.0f && clock[pos] > 0.0f) {
	    ch_state[current_ch] = FADE_OUT;
	    current_ch = (current_ch + 1) % 8;
	    ch_state[current_ch] = FADE_IN;
	  }
	}

	// Save state data
	plugin_data->current_ch = current_ch;
	plugin_data->last_clock = last_clock;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainStepMuxer(LADSPA_Handle instance, LADSPA_Data gain) {
	((StepMuxer *)instance)->run_adding_gain = gain;
}

static void runAddingStepMuxer(LADSPA_Handle instance, unsigned long sample_count) {
	StepMuxer *plugin_data = (StepMuxer *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Crossfade time (in ms) (float value) */
	const LADSPA_Data xfadet = *(plugin_data->xfadet);

	/* Clock (array of floats of length sample_count) */
	const LADSPA_Data * const clock = plugin_data->clock;

	/* Input 1 (array of floats of length sample_count) */
	const LADSPA_Data * const input0 = plugin_data->input0;

	/* Input 2 (array of floats of length sample_count) */
	const LADSPA_Data * const input1 = plugin_data->input1;

	/* Input 3 (array of floats of length sample_count) */
	const LADSPA_Data * const input2 = plugin_data->input2;

	/* Input 4 (array of floats of length sample_count) */
	const LADSPA_Data * const input3 = plugin_data->input3;

	/* Input 5 (array of floats of length sample_count) */
	const LADSPA_Data * const input4 = plugin_data->input4;

	/* Input 6 (array of floats of length sample_count) */
	const LADSPA_Data * const input5 = plugin_data->input5;

	/* Input 7 (array of floats of length sample_count) */
	const LADSPA_Data * const input6 = plugin_data->input6;

	/* Input 8 (array of floats of length sample_count) */
	const LADSPA_Data * const input7 = plugin_data->input7;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	float * ch_gain = plugin_data->ch_gain;
	int * ch_state = plugin_data->ch_state;
	int current_ch = plugin_data->current_ch;
	LADSPA_Data last_clock = plugin_data->last_clock;
	float sample_rate = plugin_data->sample_rate;

#line 50 "step_muxer_1212.xml"
	unsigned long pos;
	float fade_inc = 1.0f / (xfadet * sample_rate * 1000.0f);
	float accum;
	int ch;

	for (pos = 0; pos < sample_count; pos++) {

	  // Calculate output value for this sample
	  accum = 0.0f;
	  accum += input0[pos] * ch_gain[0];
	  accum += input1[pos] * ch_gain[1];
	  accum += input2[pos] * ch_gain[2];
	  accum += input3[pos] * ch_gain[3];
	  accum += input4[pos] * ch_gain[4];
	  accum += input5[pos] * ch_gain[5];
	  accum += input6[pos] * ch_gain[6];
	  accum += input7[pos] * ch_gain[7];
	  buffer_write(output[pos], accum);

	  // Run crossfades
	  for (ch = 0; ch < 8; ch++) {

	    // Channel is still being faded in
	    if (ch_state[ch] == FADE_IN) {
	      ch_gain[ch] += fade_inc;
	      if (ch_gain[ch] >= 1.0f) {
	        ch_gain[ch] = 1.0f;
	        ch_state[ch] = STABLE;
	      }

	    // Channel is still being faded out
	    } else if (ch_state[ch] == FADE_OUT) {
	      ch_gain[ch] -= fade_inc;
	      if (ch_gain[ch] <= 0.0f) {
	        ch_gain[ch] = 0.0f;
	        ch_state[ch] = STABLE;
	      }
	    }
	  }

	  // Check for clock signal
	  if (last_clock <= 0.0f && clock[pos] > 0.0f) {
	    ch_state[current_ch] = FADE_OUT;
	    current_ch = (current_ch + 1) % 8;
	    ch_state[current_ch] = FADE_IN;
	  }
	}

	// Save state data
	plugin_data->current_ch = current_ch;
	plugin_data->last_clock = last_clock;
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


	stepMuxerDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (stepMuxerDescriptor) {
		stepMuxerDescriptor->UniqueID = 1212;
		stepMuxerDescriptor->Label = "stepMuxer";
		stepMuxerDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		stepMuxerDescriptor->Name =
		 D_("Step Demuxer");
		stepMuxerDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		stepMuxerDescriptor->Copyright =
		 "GPL";
		stepMuxerDescriptor->PortCount = 11;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(11,
		 sizeof(LADSPA_PortDescriptor));
		stepMuxerDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(11,
		 sizeof(LADSPA_PortRangeHint));
		stepMuxerDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(11, sizeof(char*));
		stepMuxerDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Crossfade time (in ms) */
		port_descriptors[STEPMUXER_XFADET] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[STEPMUXER_XFADET] =
		 D_("Crossfade time (in ms)");
		port_range_hints[STEPMUXER_XFADET].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[STEPMUXER_XFADET].LowerBound = 0;
		port_range_hints[STEPMUXER_XFADET].UpperBound = 100;

		/* Parameters for Clock */
		port_descriptors[STEPMUXER_CLOCK] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[STEPMUXER_CLOCK] =
		 D_("Clock");
		port_range_hints[STEPMUXER_CLOCK].HintDescriptor = 0;

		/* Parameters for Input 1 */
		port_descriptors[STEPMUXER_INPUT0] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[STEPMUXER_INPUT0] =
		 D_("Input 1");
		port_range_hints[STEPMUXER_INPUT0].HintDescriptor = 0;

		/* Parameters for Input 2 */
		port_descriptors[STEPMUXER_INPUT1] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[STEPMUXER_INPUT1] =
		 D_("Input 2");
		port_range_hints[STEPMUXER_INPUT1].HintDescriptor = 0;

		/* Parameters for Input 3 */
		port_descriptors[STEPMUXER_INPUT2] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[STEPMUXER_INPUT2] =
		 D_("Input 3");
		port_range_hints[STEPMUXER_INPUT2].HintDescriptor = 0;

		/* Parameters for Input 4 */
		port_descriptors[STEPMUXER_INPUT3] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[STEPMUXER_INPUT3] =
		 D_("Input 4");
		port_range_hints[STEPMUXER_INPUT3].HintDescriptor = 0;

		/* Parameters for Input 5 */
		port_descriptors[STEPMUXER_INPUT4] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[STEPMUXER_INPUT4] =
		 D_("Input 5");
		port_range_hints[STEPMUXER_INPUT4].HintDescriptor = 0;

		/* Parameters for Input 6 */
		port_descriptors[STEPMUXER_INPUT5] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[STEPMUXER_INPUT5] =
		 D_("Input 6");
		port_range_hints[STEPMUXER_INPUT5].HintDescriptor = 0;

		/* Parameters for Input 7 */
		port_descriptors[STEPMUXER_INPUT6] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[STEPMUXER_INPUT6] =
		 D_("Input 7");
		port_range_hints[STEPMUXER_INPUT6].HintDescriptor = 0;

		/* Parameters for Input 8 */
		port_descriptors[STEPMUXER_INPUT7] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[STEPMUXER_INPUT7] =
		 D_("Input 8");
		port_range_hints[STEPMUXER_INPUT7].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[STEPMUXER_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[STEPMUXER_OUTPUT] =
		 D_("Output");
		port_range_hints[STEPMUXER_OUTPUT].HintDescriptor = 0;

		stepMuxerDescriptor->activate = activateStepMuxer;
		stepMuxerDescriptor->cleanup = cleanupStepMuxer;
		stepMuxerDescriptor->connect_port = connectPortStepMuxer;
		stepMuxerDescriptor->deactivate = NULL;
		stepMuxerDescriptor->instantiate = instantiateStepMuxer;
		stepMuxerDescriptor->run = runStepMuxer;
		stepMuxerDescriptor->run_adding = runAddingStepMuxer;
		stepMuxerDescriptor->set_run_adding_gain = setRunAddingGainStepMuxer;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (stepMuxerDescriptor) {
		free((LADSPA_PortDescriptor *)stepMuxerDescriptor->PortDescriptors);
		free((char **)stepMuxerDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)stepMuxerDescriptor->PortRangeHints);
		free(stepMuxerDescriptor);
	}

}
