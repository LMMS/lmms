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

#line 9 "multivoice_chorus_1201.xml"

#include "ladspa-util.h"
#define MAX_LAWS 7

#define MULTIVOICECHORUS_VOICES        0
#define MULTIVOICECHORUS_DELAY_BASE    1
#define MULTIVOICECHORUS_VOICE_SPREAD  2
#define MULTIVOICECHORUS_DETUNE        3
#define MULTIVOICECHORUS_LAW_FREQ      4
#define MULTIVOICECHORUS_ATTENDB       5
#define MULTIVOICECHORUS_INPUT         6
#define MULTIVOICECHORUS_OUTPUT        7

static LADSPA_Descriptor *multivoiceChorusDescriptor = NULL;

typedef struct {
	LADSPA_Data *voices;
	LADSPA_Data *delay_base;
	LADSPA_Data *voice_spread;
	LADSPA_Data *detune;
	LADSPA_Data *law_freq;
	LADSPA_Data *attendb;
	LADSPA_Data *input;
	LADSPA_Data *output;
	long         count;
	unsigned int delay_mask;
	unsigned int delay_pos;
	unsigned int delay_size;
	float *      delay_tbl;
	float *      dp_curr;
	float *      dp_targ;
	int          last_law_p;
	int          law_pos;
	int          law_roll;
	int          max_law_p;
	float *      next_peak_amp;
	unsigned int *next_peak_pos;
	float *      prev_peak_amp;
	unsigned int *prev_peak_pos;
	long         sample_rate;
	LADSPA_Data run_adding_gain;
} MultivoiceChorus;

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
		return multivoiceChorusDescriptor;
	default:
		return NULL;
	}
}

static void activateMultivoiceChorus(LADSPA_Handle instance) {
	MultivoiceChorus *plugin_data = (MultivoiceChorus *)instance;
	long count = plugin_data->count;
	unsigned int delay_mask = plugin_data->delay_mask;
	unsigned int delay_pos = plugin_data->delay_pos;
	unsigned int delay_size = plugin_data->delay_size;
	float *delay_tbl = plugin_data->delay_tbl;
	float *dp_curr = plugin_data->dp_curr;
	float *dp_targ = plugin_data->dp_targ;
	int last_law_p = plugin_data->last_law_p;
	int law_pos = plugin_data->law_pos;
	int law_roll = plugin_data->law_roll;
	int max_law_p = plugin_data->max_law_p;
	float *next_peak_amp = plugin_data->next_peak_amp;
	unsigned int *next_peak_pos = plugin_data->next_peak_pos;
	float *prev_peak_amp = plugin_data->prev_peak_amp;
	unsigned int *prev_peak_pos = plugin_data->prev_peak_pos;
	long sample_rate = plugin_data->sample_rate;
#line 46 "multivoice_chorus_1201.xml"
	memset(delay_tbl, 0, sizeof(float) * delay_size);
	memset(prev_peak_pos, 0, sizeof(unsigned int) * MAX_LAWS);
	memset(next_peak_pos, 0, sizeof(unsigned int) * MAX_LAWS);
	memset(prev_peak_amp, 0, sizeof(float) * MAX_LAWS);
	memset(next_peak_amp, 0, sizeof(float) * MAX_LAWS);
	memset(dp_targ, 0, sizeof(float) * MAX_LAWS);
	memset(dp_curr, 0, sizeof(float) * MAX_LAWS);
	plugin_data->count = count;
	plugin_data->delay_mask = delay_mask;
	plugin_data->delay_pos = delay_pos;
	plugin_data->delay_size = delay_size;
	plugin_data->delay_tbl = delay_tbl;
	plugin_data->dp_curr = dp_curr;
	plugin_data->dp_targ = dp_targ;
	plugin_data->last_law_p = last_law_p;
	plugin_data->law_pos = law_pos;
	plugin_data->law_roll = law_roll;
	plugin_data->max_law_p = max_law_p;
	plugin_data->next_peak_amp = next_peak_amp;
	plugin_data->next_peak_pos = next_peak_pos;
	plugin_data->prev_peak_amp = prev_peak_amp;
	plugin_data->prev_peak_pos = prev_peak_pos;
	plugin_data->sample_rate = sample_rate;

}

static void cleanupMultivoiceChorus(LADSPA_Handle instance) {
#line 56 "multivoice_chorus_1201.xml"
	MultivoiceChorus *plugin_data = (MultivoiceChorus *)instance;
	free(plugin_data->delay_tbl);
	free(plugin_data->prev_peak_pos);
	free(plugin_data->next_peak_pos);
	free(plugin_data->prev_peak_amp);
	free(plugin_data->next_peak_amp);
	free(plugin_data->dp_targ);
	free(plugin_data->dp_curr);
	free(instance);
}

static void connectPortMultivoiceChorus(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	MultivoiceChorus *plugin;

	plugin = (MultivoiceChorus *)instance;
	switch (port) {
	case MULTIVOICECHORUS_VOICES:
		plugin->voices = data;
		break;
	case MULTIVOICECHORUS_DELAY_BASE:
		plugin->delay_base = data;
		break;
	case MULTIVOICECHORUS_VOICE_SPREAD:
		plugin->voice_spread = data;
		break;
	case MULTIVOICECHORUS_DETUNE:
		plugin->detune = data;
		break;
	case MULTIVOICECHORUS_LAW_FREQ:
		plugin->law_freq = data;
		break;
	case MULTIVOICECHORUS_ATTENDB:
		plugin->attendb = data;
		break;
	case MULTIVOICECHORUS_INPUT:
		plugin->input = data;
		break;
	case MULTIVOICECHORUS_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateMultivoiceChorus(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	MultivoiceChorus *plugin_data = (MultivoiceChorus *)malloc(sizeof(MultivoiceChorus));
	long count;
	unsigned int delay_mask;
	unsigned int delay_pos;
	unsigned int delay_size;
	float *delay_tbl = NULL;
	float *dp_curr = NULL;
	float *dp_targ = NULL;
	int last_law_p;
	int law_pos;
	int law_roll;
	int max_law_p;
	float *next_peak_amp = NULL;
	unsigned int *next_peak_pos = NULL;
	float *prev_peak_amp = NULL;
	unsigned int *prev_peak_pos = NULL;
	long sample_rate;

#line 20 "multivoice_chorus_1201.xml"
	int min_size;
	
	sample_rate = s_rate;
	
	max_law_p = s_rate/2;
	last_law_p = -1;
	law_pos = 0;
	law_roll = 0;
	
	min_size = sample_rate / 10;
	for (delay_size = 1024; delay_size < min_size; delay_size *= 2);
	delay_mask = delay_size - 1;
	delay_tbl = calloc(sizeof(float), delay_size);
	delay_pos = 0;
	
	prev_peak_pos = malloc(sizeof(unsigned int) * MAX_LAWS);
	next_peak_pos = malloc(sizeof(unsigned int) * MAX_LAWS);
	prev_peak_amp = malloc(sizeof(float) * MAX_LAWS);
	next_peak_amp = malloc(sizeof(float) * MAX_LAWS);
	dp_targ = malloc(sizeof(float) * MAX_LAWS);
	dp_curr = malloc(sizeof(float) * MAX_LAWS);
	
	count = 0;

	plugin_data->count = count;
	plugin_data->delay_mask = delay_mask;
	plugin_data->delay_pos = delay_pos;
	plugin_data->delay_size = delay_size;
	plugin_data->delay_tbl = delay_tbl;
	plugin_data->dp_curr = dp_curr;
	plugin_data->dp_targ = dp_targ;
	plugin_data->last_law_p = last_law_p;
	plugin_data->law_pos = law_pos;
	plugin_data->law_roll = law_roll;
	plugin_data->max_law_p = max_law_p;
	plugin_data->next_peak_amp = next_peak_amp;
	plugin_data->next_peak_pos = next_peak_pos;
	plugin_data->prev_peak_amp = prev_peak_amp;
	plugin_data->prev_peak_pos = prev_peak_pos;
	plugin_data->sample_rate = sample_rate;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runMultivoiceChorus(LADSPA_Handle instance, unsigned long sample_count) {
	MultivoiceChorus *plugin_data = (MultivoiceChorus *)instance;

	/* Number of voices (float value) */
	const LADSPA_Data voices = *(plugin_data->voices);

	/* Delay base (ms) (float value) */
	const LADSPA_Data delay_base = *(plugin_data->delay_base);

	/* Voice separation (ms) (float value) */
	const LADSPA_Data voice_spread = *(plugin_data->voice_spread);

	/* Detune (%) (float value) */
	const LADSPA_Data detune = *(plugin_data->detune);

	/* LFO frequency (Hz) (float value) */
	const LADSPA_Data law_freq = *(plugin_data->law_freq);

	/* Output attenuation (dB) (float value) */
	const LADSPA_Data attendb = *(plugin_data->attendb);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	long count = plugin_data->count;
	unsigned int delay_mask = plugin_data->delay_mask;
	unsigned int delay_pos = plugin_data->delay_pos;
	unsigned int delay_size = plugin_data->delay_size;
	float * delay_tbl = plugin_data->delay_tbl;
	float * dp_curr = plugin_data->dp_curr;
	float * dp_targ = plugin_data->dp_targ;
	int last_law_p = plugin_data->last_law_p;
	int law_pos = plugin_data->law_pos;
	int law_roll = plugin_data->law_roll;
	int max_law_p = plugin_data->max_law_p;
	float * next_peak_amp = plugin_data->next_peak_amp;
	unsigned int * next_peak_pos = plugin_data->next_peak_pos;
	float * prev_peak_amp = plugin_data->prev_peak_amp;
	unsigned int * prev_peak_pos = plugin_data->prev_peak_pos;
	long sample_rate = plugin_data->sample_rate;

#line 66 "multivoice_chorus_1201.xml"
	unsigned long pos;
	int d_base, t;
	LADSPA_Data out;
	float delay_depth;
	float dp; // float delay position
	float dp_frac; // fractional part
	int dp_idx; // Integer delay index
	int laws, law_separation, base_offset;
	int law_p; // Period of law
	float atten; // Attenuation
	
	// Set law params
	laws = LIMIT(f_round(voices) - 1, 0, 7);
	law_p = LIMIT(f_round(sample_rate/f_clamp(law_freq, 0.0001f, 1000.0f)), 1, max_law_p);
	if (laws > 0) {
	        law_separation = law_p / laws;
	} else {
	        law_separation = 0;
	}
	
	// Calculate voice spread in samples
	base_offset = (f_clamp(voice_spread, 0.0f, 2.0f) * sample_rate) / 1000;
	// Calculate base delay size in samples
	d_base = (f_clamp(delay_base, 5.0f, 40.0f) * sample_rate) / 1000;
	// Calculate delay depth in samples
	delay_depth = f_clamp((law_p * f_clamp(detune, 0.0f, 10.0f)) / (100.0f * M_PI), 0.0f, delay_size - d_base - 1 - (base_offset * laws));
	
	// Calculate output attenuation
	atten = DB_CO(f_clamp(attendb, -100.0, 24.0));
	
	for (pos = 0; pos < sample_count; pos++) {
	        // N times per law 'frequency' splurge a new set of windowed data
	        // into one of the N law buffers. Keeps the laws out of phase.
	        if (laws > 0 && (count % law_separation) == 0) {
	                next_peak_amp[law_roll] = (float)rand() / (float)RAND_MAX;
	                next_peak_pos[law_roll] = count + law_p;
	        }
	        if (laws > 0 && (count % law_separation) == law_separation/2) {
	                prev_peak_amp[law_roll] = (float)rand() / (float)RAND_MAX;
	                prev_peak_pos[law_roll] = count + law_p;
	                // Pick the next law to be changed
	                law_roll = (law_roll + 1) % laws;
	        }
	
	        out = input[pos];
	        if (count % 16 < laws) {
	                unsigned int t = count % 16;
	                // Calculate sinus phases
	                float n_ph = (float)(law_p - abs(next_peak_pos[t] - count))/law_p;
	                float p_ph = n_ph + 0.5f;
	                if (p_ph > 1.0f) {
	                        p_ph -= 1.0f;
	                }
	
	                dp_targ[t] = f_sin_sq(3.1415926f*p_ph)*prev_peak_amp[t] + f_sin_sq(3.1415926f*n_ph)*next_peak_amp[t];
	        }
	        for (t=0; t<laws; t++) {
	                dp_curr[t] = 0.9f*dp_curr[t] + 0.1f*dp_targ[t];
	                //dp_curr[t] = dp_targ[t];
	                dp = (float)(delay_pos + d_base - (t*base_offset)) - delay_depth * dp_curr[t];
	                // Get the integer part
	                dp_idx = f_round(dp-0.5f);
	                // Get the fractional part
	                dp_frac = dp - dp_idx;
	                // Calculate the modulo'd table index
	                dp_idx = dp_idx & delay_mask;
	
	                // Accumulate into output buffer
	                out += cube_interp(dp_frac, delay_tbl[(dp_idx-1) & delay_mask], delay_tbl[dp_idx], delay_tbl[(dp_idx+1) & delay_mask], delay_tbl[(dp_idx+2) & delay_mask]);
	        }
	        law_pos = (law_pos + 1) % (max_law_p * 2);
	
	        // Store new delay value
	        delay_tbl[delay_pos] = input[pos];
	        delay_pos = (delay_pos + 1) & delay_mask;
	
	        buffer_write(output[pos], out * atten);
	        count++;
	}
	
	plugin_data->count = count;
	plugin_data->law_pos = law_pos;
	plugin_data->last_law_p = last_law_p;
	plugin_data->law_roll = law_roll;
	plugin_data->delay_pos = delay_pos;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainMultivoiceChorus(LADSPA_Handle instance, LADSPA_Data gain) {
	((MultivoiceChorus *)instance)->run_adding_gain = gain;
}

static void runAddingMultivoiceChorus(LADSPA_Handle instance, unsigned long sample_count) {
	MultivoiceChorus *plugin_data = (MultivoiceChorus *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Number of voices (float value) */
	const LADSPA_Data voices = *(plugin_data->voices);

	/* Delay base (ms) (float value) */
	const LADSPA_Data delay_base = *(plugin_data->delay_base);

	/* Voice separation (ms) (float value) */
	const LADSPA_Data voice_spread = *(plugin_data->voice_spread);

	/* Detune (%) (float value) */
	const LADSPA_Data detune = *(plugin_data->detune);

	/* LFO frequency (Hz) (float value) */
	const LADSPA_Data law_freq = *(plugin_data->law_freq);

	/* Output attenuation (dB) (float value) */
	const LADSPA_Data attendb = *(plugin_data->attendb);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	long count = plugin_data->count;
	unsigned int delay_mask = plugin_data->delay_mask;
	unsigned int delay_pos = plugin_data->delay_pos;
	unsigned int delay_size = plugin_data->delay_size;
	float * delay_tbl = plugin_data->delay_tbl;
	float * dp_curr = plugin_data->dp_curr;
	float * dp_targ = plugin_data->dp_targ;
	int last_law_p = plugin_data->last_law_p;
	int law_pos = plugin_data->law_pos;
	int law_roll = plugin_data->law_roll;
	int max_law_p = plugin_data->max_law_p;
	float * next_peak_amp = plugin_data->next_peak_amp;
	unsigned int * next_peak_pos = plugin_data->next_peak_pos;
	float * prev_peak_amp = plugin_data->prev_peak_amp;
	unsigned int * prev_peak_pos = plugin_data->prev_peak_pos;
	long sample_rate = plugin_data->sample_rate;

#line 66 "multivoice_chorus_1201.xml"
	unsigned long pos;
	int d_base, t;
	LADSPA_Data out;
	float delay_depth;
	float dp; // float delay position
	float dp_frac; // fractional part
	int dp_idx; // Integer delay index
	int laws, law_separation, base_offset;
	int law_p; // Period of law
	float atten; // Attenuation
	
	// Set law params
	laws = LIMIT(f_round(voices) - 1, 0, 7);
	law_p = LIMIT(f_round(sample_rate/f_clamp(law_freq, 0.0001f, 1000.0f)), 1, max_law_p);
	if (laws > 0) {
	        law_separation = law_p / laws;
	} else {
	        law_separation = 0;
	}
	
	// Calculate voice spread in samples
	base_offset = (f_clamp(voice_spread, 0.0f, 2.0f) * sample_rate) / 1000;
	// Calculate base delay size in samples
	d_base = (f_clamp(delay_base, 5.0f, 40.0f) * sample_rate) / 1000;
	// Calculate delay depth in samples
	delay_depth = f_clamp((law_p * f_clamp(detune, 0.0f, 10.0f)) / (100.0f * M_PI), 0.0f, delay_size - d_base - 1 - (base_offset * laws));
	
	// Calculate output attenuation
	atten = DB_CO(f_clamp(attendb, -100.0, 24.0));
	
	for (pos = 0; pos < sample_count; pos++) {
	        // N times per law 'frequency' splurge a new set of windowed data
	        // into one of the N law buffers. Keeps the laws out of phase.
	        if (laws > 0 && (count % law_separation) == 0) {
	                next_peak_amp[law_roll] = (float)rand() / (float)RAND_MAX;
	                next_peak_pos[law_roll] = count + law_p;
	        }
	        if (laws > 0 && (count % law_separation) == law_separation/2) {
	                prev_peak_amp[law_roll] = (float)rand() / (float)RAND_MAX;
	                prev_peak_pos[law_roll] = count + law_p;
	                // Pick the next law to be changed
	                law_roll = (law_roll + 1) % laws;
	        }
	
	        out = input[pos];
	        if (count % 16 < laws) {
	                unsigned int t = count % 16;
	                // Calculate sinus phases
	                float n_ph = (float)(law_p - abs(next_peak_pos[t] - count))/law_p;
	                float p_ph = n_ph + 0.5f;
	                if (p_ph > 1.0f) {
	                        p_ph -= 1.0f;
	                }
	
	                dp_targ[t] = f_sin_sq(3.1415926f*p_ph)*prev_peak_amp[t] + f_sin_sq(3.1415926f*n_ph)*next_peak_amp[t];
	        }
	        for (t=0; t<laws; t++) {
	                dp_curr[t] = 0.9f*dp_curr[t] + 0.1f*dp_targ[t];
	                //dp_curr[t] = dp_targ[t];
	                dp = (float)(delay_pos + d_base - (t*base_offset)) - delay_depth * dp_curr[t];
	                // Get the integer part
	                dp_idx = f_round(dp-0.5f);
	                // Get the fractional part
	                dp_frac = dp - dp_idx;
	                // Calculate the modulo'd table index
	                dp_idx = dp_idx & delay_mask;
	
	                // Accumulate into output buffer
	                out += cube_interp(dp_frac, delay_tbl[(dp_idx-1) & delay_mask], delay_tbl[dp_idx], delay_tbl[(dp_idx+1) & delay_mask], delay_tbl[(dp_idx+2) & delay_mask]);
	        }
	        law_pos = (law_pos + 1) % (max_law_p * 2);
	
	        // Store new delay value
	        delay_tbl[delay_pos] = input[pos];
	        delay_pos = (delay_pos + 1) & delay_mask;
	
	        buffer_write(output[pos], out * atten);
	        count++;
	}
	
	plugin_data->count = count;
	plugin_data->law_pos = law_pos;
	plugin_data->last_law_p = last_law_p;
	plugin_data->law_roll = law_roll;
	plugin_data->delay_pos = delay_pos;
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


	multivoiceChorusDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (multivoiceChorusDescriptor) {
		multivoiceChorusDescriptor->UniqueID = 1201;
		multivoiceChorusDescriptor->Label = "multivoiceChorus";
		multivoiceChorusDescriptor->Properties =
		 0;
		multivoiceChorusDescriptor->Name =
		 D_("Multivoice Chorus");
		multivoiceChorusDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		multivoiceChorusDescriptor->Copyright =
		 "GPL";
		multivoiceChorusDescriptor->PortCount = 8;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(8,
		 sizeof(LADSPA_PortDescriptor));
		multivoiceChorusDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(8,
		 sizeof(LADSPA_PortRangeHint));
		multivoiceChorusDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(8, sizeof(char*));
		multivoiceChorusDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Number of voices */
		port_descriptors[MULTIVOICECHORUS_VOICES] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[MULTIVOICECHORUS_VOICES] =
		 D_("Number of voices");
		port_range_hints[MULTIVOICECHORUS_VOICES].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_1;
		port_range_hints[MULTIVOICECHORUS_VOICES].LowerBound = 1;
		port_range_hints[MULTIVOICECHORUS_VOICES].UpperBound = 8;

		/* Parameters for Delay base (ms) */
		port_descriptors[MULTIVOICECHORUS_DELAY_BASE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[MULTIVOICECHORUS_DELAY_BASE] =
		 D_("Delay base (ms)");
		port_range_hints[MULTIVOICECHORUS_DELAY_BASE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MINIMUM;
		port_range_hints[MULTIVOICECHORUS_DELAY_BASE].LowerBound = 10;
		port_range_hints[MULTIVOICECHORUS_DELAY_BASE].UpperBound = 40;

		/* Parameters for Voice separation (ms) */
		port_descriptors[MULTIVOICECHORUS_VOICE_SPREAD] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[MULTIVOICECHORUS_VOICE_SPREAD] =
		 D_("Voice separation (ms)");
		port_range_hints[MULTIVOICECHORUS_VOICE_SPREAD].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[MULTIVOICECHORUS_VOICE_SPREAD].LowerBound = 0;
		port_range_hints[MULTIVOICECHORUS_VOICE_SPREAD].UpperBound = 2;

		/* Parameters for Detune (%) */
		port_descriptors[MULTIVOICECHORUS_DETUNE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[MULTIVOICECHORUS_DETUNE] =
		 D_("Detune (%)");
		port_range_hints[MULTIVOICECHORUS_DETUNE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[MULTIVOICECHORUS_DETUNE].LowerBound = 0;
		port_range_hints[MULTIVOICECHORUS_DETUNE].UpperBound = 5;

		/* Parameters for LFO frequency (Hz) */
		port_descriptors[MULTIVOICECHORUS_LAW_FREQ] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[MULTIVOICECHORUS_LAW_FREQ] =
		 D_("LFO frequency (Hz)");
		port_range_hints[MULTIVOICECHORUS_LAW_FREQ].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[MULTIVOICECHORUS_LAW_FREQ].LowerBound = 2;
		port_range_hints[MULTIVOICECHORUS_LAW_FREQ].UpperBound = 30;

		/* Parameters for Output attenuation (dB) */
		port_descriptors[MULTIVOICECHORUS_ATTENDB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[MULTIVOICECHORUS_ATTENDB] =
		 D_("Output attenuation (dB)");
		port_range_hints[MULTIVOICECHORUS_ATTENDB].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[MULTIVOICECHORUS_ATTENDB].LowerBound = -20;
		port_range_hints[MULTIVOICECHORUS_ATTENDB].UpperBound = 0;

		/* Parameters for Input */
		port_descriptors[MULTIVOICECHORUS_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[MULTIVOICECHORUS_INPUT] =
		 D_("Input");
		port_range_hints[MULTIVOICECHORUS_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[MULTIVOICECHORUS_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[MULTIVOICECHORUS_OUTPUT] =
		 D_("Output");
		port_range_hints[MULTIVOICECHORUS_OUTPUT].HintDescriptor = 0;

		multivoiceChorusDescriptor->activate = activateMultivoiceChorus;
		multivoiceChorusDescriptor->cleanup = cleanupMultivoiceChorus;
		multivoiceChorusDescriptor->connect_port = connectPortMultivoiceChorus;
		multivoiceChorusDescriptor->deactivate = NULL;
		multivoiceChorusDescriptor->instantiate = instantiateMultivoiceChorus;
		multivoiceChorusDescriptor->run = runMultivoiceChorus;
		multivoiceChorusDescriptor->run_adding = runAddingMultivoiceChorus;
		multivoiceChorusDescriptor->set_run_adding_gain = setRunAddingGainMultivoiceChorus;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (multivoiceChorusDescriptor) {
		free((LADSPA_PortDescriptor *)multivoiceChorusDescriptor->PortDescriptors);
		free((char **)multivoiceChorusDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)multivoiceChorusDescriptor->PortRangeHints);
		free(multivoiceChorusDescriptor);
	}

}
