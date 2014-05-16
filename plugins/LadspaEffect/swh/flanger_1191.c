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

#line 10 "flanger_1191.xml"

#include "ladspa-util.h"

#define FLANGER_DELAY_BASE             0
#define FLANGER_DETUNE                 1
#define FLANGER_LAW_FREQ               2
#define FLANGER_FEEDBACK               3
#define FLANGER_INPUT                  4
#define FLANGER_OUTPUT                 5

static LADSPA_Descriptor *flangerDescriptor = NULL;

typedef struct {
	LADSPA_Data *delay_base;
	LADSPA_Data *detune;
	LADSPA_Data *law_freq;
	LADSPA_Data *feedback;
	LADSPA_Data *input;
	LADSPA_Data *output;
	long         count;
	long         delay_pos;
	long         delay_size;
	LADSPA_Data *delay_tbl;
	float        next_law_peak;
	int          next_law_pos;
	long         old_d_base;
	float        prev_law_peak;
	int          prev_law_pos;
	long         sample_rate;
	LADSPA_Data run_adding_gain;
} Flanger;

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
		return flangerDescriptor;
	default:
		return NULL;
	}
}

static void activateFlanger(LADSPA_Handle instance) {
	Flanger *plugin_data = (Flanger *)instance;
	long count = plugin_data->count;
	long delay_pos = plugin_data->delay_pos;
	long delay_size = plugin_data->delay_size;
	LADSPA_Data *delay_tbl = plugin_data->delay_tbl;
	float next_law_peak = plugin_data->next_law_peak;
	int next_law_pos = plugin_data->next_law_pos;
	long old_d_base = plugin_data->old_d_base;
	float prev_law_peak = plugin_data->prev_law_peak;
	int prev_law_pos = plugin_data->prev_law_pos;
	long sample_rate = plugin_data->sample_rate;
#line 39 "flanger_1191.xml"
	memset(delay_tbl, 0, sizeof(LADSPA_Data) * delay_size);
	delay_pos = 0;
	count = 0;
	old_d_base = 0;
	plugin_data->count = count;
	plugin_data->delay_pos = delay_pos;
	plugin_data->delay_size = delay_size;
	plugin_data->delay_tbl = delay_tbl;
	plugin_data->next_law_peak = next_law_peak;
	plugin_data->next_law_pos = next_law_pos;
	plugin_data->old_d_base = old_d_base;
	plugin_data->prev_law_peak = prev_law_peak;
	plugin_data->prev_law_pos = prev_law_pos;
	plugin_data->sample_rate = sample_rate;

}

static void cleanupFlanger(LADSPA_Handle instance) {
#line 46 "flanger_1191.xml"
	Flanger *plugin_data = (Flanger *)instance;
	free(plugin_data->delay_tbl);
	free(instance);
}

static void connectPortFlanger(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Flanger *plugin;

	plugin = (Flanger *)instance;
	switch (port) {
	case FLANGER_DELAY_BASE:
		plugin->delay_base = data;
		break;
	case FLANGER_DETUNE:
		plugin->detune = data;
		break;
	case FLANGER_LAW_FREQ:
		plugin->law_freq = data;
		break;
	case FLANGER_FEEDBACK:
		plugin->feedback = data;
		break;
	case FLANGER_INPUT:
		plugin->input = data;
		break;
	case FLANGER_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateFlanger(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Flanger *plugin_data = (Flanger *)malloc(sizeof(Flanger));
	long count;
	long delay_pos;
	long delay_size;
	LADSPA_Data *delay_tbl = NULL;
	float next_law_peak;
	int next_law_pos;
	long old_d_base;
	float prev_law_peak;
	int prev_law_pos;
	long sample_rate;

#line 21 "flanger_1191.xml"
	int min_size;
	
	sample_rate = s_rate;
	
	prev_law_peak = 0.0f;
	next_law_peak = 1.0f;
	prev_law_pos = 0;
	next_law_pos = 10;
	
	min_size = sample_rate * 0.04f;
	for (delay_size = 1024; delay_size < min_size; delay_size *= 2);
	delay_tbl = malloc(sizeof(LADSPA_Data) * delay_size);
	delay_pos = 0;
	count = 0;
	old_d_base = 0;

	plugin_data->count = count;
	plugin_data->delay_pos = delay_pos;
	plugin_data->delay_size = delay_size;
	plugin_data->delay_tbl = delay_tbl;
	plugin_data->next_law_peak = next_law_peak;
	plugin_data->next_law_pos = next_law_pos;
	plugin_data->old_d_base = old_d_base;
	plugin_data->prev_law_peak = prev_law_peak;
	plugin_data->prev_law_pos = prev_law_pos;
	plugin_data->sample_rate = sample_rate;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runFlanger(LADSPA_Handle instance, unsigned long sample_count) {
	Flanger *plugin_data = (Flanger *)instance;

	/* Delay base (ms) (float value) */
	const LADSPA_Data delay_base = *(plugin_data->delay_base);

	/* Max slowdown (ms) (float value) */
	const LADSPA_Data detune = *(plugin_data->detune);

	/* LFO frequency (Hz) (float value) */
	const LADSPA_Data law_freq = *(plugin_data->law_freq);

	/* Feedback (float value) */
	const LADSPA_Data feedback = *(plugin_data->feedback);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	long count = plugin_data->count;
	long delay_pos = plugin_data->delay_pos;
	long delay_size = plugin_data->delay_size;
	LADSPA_Data * delay_tbl = plugin_data->delay_tbl;
	float next_law_peak = plugin_data->next_law_peak;
	int next_law_pos = plugin_data->next_law_pos;
	long old_d_base = plugin_data->old_d_base;
	float prev_law_peak = plugin_data->prev_law_peak;
	int prev_law_pos = plugin_data->prev_law_pos;
	long sample_rate = plugin_data->sample_rate;

#line 50 "flanger_1191.xml"
	unsigned long pos;
	long d_base, new_d_base;
	LADSPA_Data out;
	float delay_depth;
	float dp; // float delay position
	float dp_frac; // fractional part
	long dp_idx; // integer delay index
	long law_p; // period of law
	float frac = 0.0f, step; // Portion the way through the block
	float law; /* law amplitude */
	float n_ph, p_ph;
	const float fb = f_clamp(feedback, -0.999f, 0.999f);
	
	// Set law params
	law_p = (float)sample_rate / law_freq;
	if (law_p < 1) {
	        law_p = 1;
	}
	
	// Calculate base delay size in samples
	new_d_base = (LIMIT(f_round(delay_base), 0, 25) * sample_rate) / 1000;
	
	// Calculate delay depth in samples
	delay_depth = f_clamp(detune * (float)sample_rate * 0.001f, 0.0f, delay_size - new_d_base - 1.0f);
	
	step = 1.0f/sample_count;
	for (pos = 0; pos < sample_count; pos++) {
	        if (count % law_p == 0) {
	                // Value for amplitude of law peak
	                next_law_peak = (float)rand() / (float)RAND_MAX;
	                next_law_pos = count + law_p;
	        } else if (count % law_p == law_p / 2) {
	                // Value for amplitude of law peak
	                prev_law_peak = (float)rand() / (float)RAND_MAX;
	                prev_law_pos = count + law_p;
	        }
	
	        // Calculate position in delay table
	        d_base = LIN_INTERP(frac, old_d_base, new_d_base);
	        n_ph = (float)(law_p - abs(next_law_pos - count))/(float)law_p;
	        p_ph = n_ph + 0.5f;
	        while (p_ph > 1.0f) {
	                p_ph -= 1.0f;
	        }
	        law = f_sin_sq(3.1415926f*p_ph)*prev_law_peak +
	                f_sin_sq(3.1415926f*n_ph)*next_law_peak;
	
	        dp = (float)(delay_pos - d_base) - (delay_depth * law);
	        // Get the integer part
	        dp_idx = f_round(dp - 0.5f);
	        // Get the fractional part
	        dp_frac = dp - dp_idx;
	
	        // Accumulate into output buffer
	        out = cube_interp(dp_frac, delay_tbl[(dp_idx-1) & (delay_size-1)], delay_tbl[dp_idx & (delay_size-1)], delay_tbl[(dp_idx+1) & (delay_size-1)], delay_tbl[(dp_idx+2) & (delay_size-1)]);
	
	        // Store new delayed value
	        delay_tbl[delay_pos] = flush_to_zero(input[pos] + (fb * out));
	        // Sometimes the delay can pick up NaN values, I'm not sure why
	        // and this is easier than fixing it
	        if (isnan(delay_tbl[delay_pos])) {
	                delay_tbl[delay_pos] = 0.0f;
	        }
	
	        out = f_clamp(delay_tbl[delay_pos] * 0.707f, -1.0, 1.0);
	        buffer_write(output[pos], out);
	
	        frac += step;
	        delay_pos = (delay_pos + 1) & (delay_size-1);
	
	        count++;
	}
	
	plugin_data->count = count;
	plugin_data->prev_law_peak = prev_law_peak;
	plugin_data->next_law_peak = next_law_peak;
	plugin_data->prev_law_pos = prev_law_pos;
	plugin_data->next_law_pos = next_law_pos;
	plugin_data->delay_pos = delay_pos;
	plugin_data->old_d_base = new_d_base;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainFlanger(LADSPA_Handle instance, LADSPA_Data gain) {
	((Flanger *)instance)->run_adding_gain = gain;
}

static void runAddingFlanger(LADSPA_Handle instance, unsigned long sample_count) {
	Flanger *plugin_data = (Flanger *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Delay base (ms) (float value) */
	const LADSPA_Data delay_base = *(plugin_data->delay_base);

	/* Max slowdown (ms) (float value) */
	const LADSPA_Data detune = *(plugin_data->detune);

	/* LFO frequency (Hz) (float value) */
	const LADSPA_Data law_freq = *(plugin_data->law_freq);

	/* Feedback (float value) */
	const LADSPA_Data feedback = *(plugin_data->feedback);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	long count = plugin_data->count;
	long delay_pos = plugin_data->delay_pos;
	long delay_size = plugin_data->delay_size;
	LADSPA_Data * delay_tbl = plugin_data->delay_tbl;
	float next_law_peak = plugin_data->next_law_peak;
	int next_law_pos = plugin_data->next_law_pos;
	long old_d_base = plugin_data->old_d_base;
	float prev_law_peak = plugin_data->prev_law_peak;
	int prev_law_pos = plugin_data->prev_law_pos;
	long sample_rate = plugin_data->sample_rate;

#line 50 "flanger_1191.xml"
	unsigned long pos;
	long d_base, new_d_base;
	LADSPA_Data out;
	float delay_depth;
	float dp; // float delay position
	float dp_frac; // fractional part
	long dp_idx; // integer delay index
	long law_p; // period of law
	float frac = 0.0f, step; // Portion the way through the block
	float law; /* law amplitude */
	float n_ph, p_ph;
	const float fb = f_clamp(feedback, -0.999f, 0.999f);
	
	// Set law params
	law_p = (float)sample_rate / law_freq;
	if (law_p < 1) {
	        law_p = 1;
	}
	
	// Calculate base delay size in samples
	new_d_base = (LIMIT(f_round(delay_base), 0, 25) * sample_rate) / 1000;
	
	// Calculate delay depth in samples
	delay_depth = f_clamp(detune * (float)sample_rate * 0.001f, 0.0f, delay_size - new_d_base - 1.0f);
	
	step = 1.0f/sample_count;
	for (pos = 0; pos < sample_count; pos++) {
	        if (count % law_p == 0) {
	                // Value for amplitude of law peak
	                next_law_peak = (float)rand() / (float)RAND_MAX;
	                next_law_pos = count + law_p;
	        } else if (count % law_p == law_p / 2) {
	                // Value for amplitude of law peak
	                prev_law_peak = (float)rand() / (float)RAND_MAX;
	                prev_law_pos = count + law_p;
	        }
	
	        // Calculate position in delay table
	        d_base = LIN_INTERP(frac, old_d_base, new_d_base);
	        n_ph = (float)(law_p - abs(next_law_pos - count))/(float)law_p;
	        p_ph = n_ph + 0.5f;
	        while (p_ph > 1.0f) {
	                p_ph -= 1.0f;
	        }
	        law = f_sin_sq(3.1415926f*p_ph)*prev_law_peak +
	                f_sin_sq(3.1415926f*n_ph)*next_law_peak;
	
	        dp = (float)(delay_pos - d_base) - (delay_depth * law);
	        // Get the integer part
	        dp_idx = f_round(dp - 0.5f);
	        // Get the fractional part
	        dp_frac = dp - dp_idx;
	
	        // Accumulate into output buffer
	        out = cube_interp(dp_frac, delay_tbl[(dp_idx-1) & (delay_size-1)], delay_tbl[dp_idx & (delay_size-1)], delay_tbl[(dp_idx+1) & (delay_size-1)], delay_tbl[(dp_idx+2) & (delay_size-1)]);
	
	        // Store new delayed value
	        delay_tbl[delay_pos] = flush_to_zero(input[pos] + (fb * out));
	        // Sometimes the delay can pick up NaN values, I'm not sure why
	        // and this is easier than fixing it
	        if (isnan(delay_tbl[delay_pos])) {
	                delay_tbl[delay_pos] = 0.0f;
	        }
	
	        out = f_clamp(delay_tbl[delay_pos] * 0.707f, -1.0, 1.0);
	        buffer_write(output[pos], out);
	
	        frac += step;
	        delay_pos = (delay_pos + 1) & (delay_size-1);
	
	        count++;
	}
	
	plugin_data->count = count;
	plugin_data->prev_law_peak = prev_law_peak;
	plugin_data->next_law_peak = next_law_peak;
	plugin_data->prev_law_pos = prev_law_pos;
	plugin_data->next_law_pos = next_law_pos;
	plugin_data->delay_pos = delay_pos;
	plugin_data->old_d_base = new_d_base;
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


	flangerDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (flangerDescriptor) {
		flangerDescriptor->UniqueID = 1191;
		flangerDescriptor->Label = "flanger";
		flangerDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		flangerDescriptor->Name =
		 D_("Flanger");
		flangerDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		flangerDescriptor->Copyright =
		 "GPL";
		flangerDescriptor->PortCount = 6;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(6,
		 sizeof(LADSPA_PortDescriptor));
		flangerDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(6,
		 sizeof(LADSPA_PortRangeHint));
		flangerDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(6, sizeof(char*));
		flangerDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Delay base (ms) */
		port_descriptors[FLANGER_DELAY_BASE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[FLANGER_DELAY_BASE] =
		 D_("Delay base (ms)");
		port_range_hints[FLANGER_DELAY_BASE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[FLANGER_DELAY_BASE].LowerBound = 0.1;
		port_range_hints[FLANGER_DELAY_BASE].UpperBound = 25;

		/* Parameters for Max slowdown (ms) */
		port_descriptors[FLANGER_DETUNE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[FLANGER_DETUNE] =
		 D_("Max slowdown (ms)");
		port_range_hints[FLANGER_DETUNE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[FLANGER_DETUNE].LowerBound = 0;
		port_range_hints[FLANGER_DETUNE].UpperBound = 10;

		/* Parameters for LFO frequency (Hz) */
		port_descriptors[FLANGER_LAW_FREQ] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[FLANGER_LAW_FREQ] =
		 D_("LFO frequency (Hz)");
		port_range_hints[FLANGER_LAW_FREQ].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW | LADSPA_HINT_LOGARITHMIC;
		port_range_hints[FLANGER_LAW_FREQ].LowerBound = 0.05;
		port_range_hints[FLANGER_LAW_FREQ].UpperBound = 100;

		/* Parameters for Feedback */
		port_descriptors[FLANGER_FEEDBACK] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[FLANGER_FEEDBACK] =
		 D_("Feedback");
		port_range_hints[FLANGER_FEEDBACK].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[FLANGER_FEEDBACK].LowerBound = -1;
		port_range_hints[FLANGER_FEEDBACK].UpperBound = 1;

		/* Parameters for Input */
		port_descriptors[FLANGER_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[FLANGER_INPUT] =
		 D_("Input");
		port_range_hints[FLANGER_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[FLANGER_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[FLANGER_OUTPUT] =
		 D_("Output");
		port_range_hints[FLANGER_OUTPUT].HintDescriptor = 0;

		flangerDescriptor->activate = activateFlanger;
		flangerDescriptor->cleanup = cleanupFlanger;
		flangerDescriptor->connect_port = connectPortFlanger;
		flangerDescriptor->deactivate = NULL;
		flangerDescriptor->instantiate = instantiateFlanger;
		flangerDescriptor->run = runFlanger;
		flangerDescriptor->run_adding = runAddingFlanger;
		flangerDescriptor->set_run_adding_gain = setRunAddingGainFlanger;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (flangerDescriptor) {
		free((LADSPA_PortDescriptor *)flangerDescriptor->PortDescriptors);
		free((char **)flangerDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)flangerDescriptor->PortRangeHints);
		free(flangerDescriptor);
	}

}
