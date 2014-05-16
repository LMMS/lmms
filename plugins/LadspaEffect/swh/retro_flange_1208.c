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

#line 10 "retro_flange_1208.xml"

#include "ladspa-util.h"

#define BASE_BUFFER 0.001 // Base buffer length (s)

inline LADSPA_Data sat(LADSPA_Data x, float q,  float dist) {
        if (x == q) {
                return 1.0f / dist + q / (1.0f - f_exp(dist * q));
        }
        return ((x - q) / (1.0f - f_exp(-dist * (x - q))) + q /
         (1.0f - f_exp(dist * q)));
}

#define RETROFLANGE_DELAY_DEPTH_AVG    0
#define RETROFLANGE_LAW_FREQ           1
#define RETROFLANGE_INPUT              2
#define RETROFLANGE_OUTPUT             3

static LADSPA_Descriptor *retroFlangeDescriptor = NULL;

typedef struct {
	LADSPA_Data *delay_depth_avg;
	LADSPA_Data *law_freq;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data *buffer;
	long         buffer_size;
	long         count;
	LADSPA_Data *delay_line;
	int          delay_line_length;
	int          delay_pos;
	LADSPA_Data  last_in;
	int          last_law_p;
	int          last_phase;
	int          max_law_p;
	float        next_law_peak;
	int          next_law_pos;
	float        phase;
	float        prev_law_peak;
	int          prev_law_pos;
	long         sample_rate;
	LADSPA_Data  z0;
	LADSPA_Data  z1;
	LADSPA_Data  z2;
	LADSPA_Data run_adding_gain;
} RetroFlange;

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
		return retroFlangeDescriptor;
	default:
		return NULL;
	}
}

static void activateRetroFlange(LADSPA_Handle instance) {
	RetroFlange *plugin_data = (RetroFlange *)instance;
	LADSPA_Data *buffer = plugin_data->buffer;
	long buffer_size = plugin_data->buffer_size;
	long count = plugin_data->count;
	LADSPA_Data *delay_line = plugin_data->delay_line;
	int delay_line_length = plugin_data->delay_line_length;
	int delay_pos = plugin_data->delay_pos;
	LADSPA_Data last_in = plugin_data->last_in;
	int last_law_p = plugin_data->last_law_p;
	int last_phase = plugin_data->last_phase;
	int max_law_p = plugin_data->max_law_p;
	float next_law_peak = plugin_data->next_law_peak;
	int next_law_pos = plugin_data->next_law_pos;
	float phase = plugin_data->phase;
	float prev_law_peak = plugin_data->prev_law_peak;
	int prev_law_pos = plugin_data->prev_law_pos;
	long sample_rate = plugin_data->sample_rate;
	LADSPA_Data z0 = plugin_data->z0;
	LADSPA_Data z1 = plugin_data->z1;
	LADSPA_Data z2 = plugin_data->z2;
#line 57 "retro_flange_1208.xml"
	memset(delay_line, 0, sizeof(float) * delay_line_length);
	memset(buffer, 0, sizeof(LADSPA_Data) * buffer_size);
	z0 = 0.0f;
	z1 = 0.0f;
	z2 = 0.0f;
	
	prev_law_peak = 0.0f;
	next_law_peak = 1.0f;
	prev_law_pos = 0;
	next_law_pos = 10;
	plugin_data->buffer = buffer;
	plugin_data->buffer_size = buffer_size;
	plugin_data->count = count;
	plugin_data->delay_line = delay_line;
	plugin_data->delay_line_length = delay_line_length;
	plugin_data->delay_pos = delay_pos;
	plugin_data->last_in = last_in;
	plugin_data->last_law_p = last_law_p;
	plugin_data->last_phase = last_phase;
	plugin_data->max_law_p = max_law_p;
	plugin_data->next_law_peak = next_law_peak;
	plugin_data->next_law_pos = next_law_pos;
	plugin_data->phase = phase;
	plugin_data->prev_law_peak = prev_law_peak;
	plugin_data->prev_law_pos = prev_law_pos;
	plugin_data->sample_rate = sample_rate;
	plugin_data->z0 = z0;
	plugin_data->z1 = z1;
	plugin_data->z2 = z2;

}

static void cleanupRetroFlange(LADSPA_Handle instance) {
#line 70 "retro_flange_1208.xml"
	RetroFlange *plugin_data = (RetroFlange *)instance;
	free(plugin_data->delay_line);
	free(plugin_data->buffer);
	free(instance);
}

static void connectPortRetroFlange(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	RetroFlange *plugin;

	plugin = (RetroFlange *)instance;
	switch (port) {
	case RETROFLANGE_DELAY_DEPTH_AVG:
		plugin->delay_depth_avg = data;
		break;
	case RETROFLANGE_LAW_FREQ:
		plugin->law_freq = data;
		break;
	case RETROFLANGE_INPUT:
		plugin->input = data;
		break;
	case RETROFLANGE_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateRetroFlange(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	RetroFlange *plugin_data = (RetroFlange *)malloc(sizeof(RetroFlange));
	LADSPA_Data *buffer = NULL;
	long buffer_size;
	long count;
	LADSPA_Data *delay_line = NULL;
	int delay_line_length;
	int delay_pos;
	LADSPA_Data last_in;
	int last_law_p;
	int last_phase;
	int max_law_p;
	float next_law_peak;
	int next_law_pos;
	float phase;
	float prev_law_peak;
	int prev_law_pos;
	long sample_rate;
	LADSPA_Data z0;
	LADSPA_Data z1;
	LADSPA_Data z2;

#line 32 "retro_flange_1208.xml"
	sample_rate = s_rate;
	buffer_size = BASE_BUFFER * s_rate;
	buffer = calloc(buffer_size, sizeof(LADSPA_Data));
	phase = 0;
	last_phase = 0;
	last_in = 0.0f;
	max_law_p = s_rate*2;
	last_law_p = -1;
	delay_line_length = sample_rate * 0.01f;
	delay_line = calloc(sizeof(float), delay_line_length);
	
	delay_pos = 0;
	count = 0;
	
	prev_law_peak = 0.0f;
	next_law_peak = 1.0f;
	prev_law_pos = 0;
	next_law_pos = 10;
	
	z0 = 0.0f;
	z1 = 0.0f;
	z2 = 0.0f;

	plugin_data->buffer = buffer;
	plugin_data->buffer_size = buffer_size;
	plugin_data->count = count;
	plugin_data->delay_line = delay_line;
	plugin_data->delay_line_length = delay_line_length;
	plugin_data->delay_pos = delay_pos;
	plugin_data->last_in = last_in;
	plugin_data->last_law_p = last_law_p;
	plugin_data->last_phase = last_phase;
	plugin_data->max_law_p = max_law_p;
	plugin_data->next_law_peak = next_law_peak;
	plugin_data->next_law_pos = next_law_pos;
	plugin_data->phase = phase;
	plugin_data->prev_law_peak = prev_law_peak;
	plugin_data->prev_law_pos = prev_law_pos;
	plugin_data->sample_rate = sample_rate;
	plugin_data->z0 = z0;
	plugin_data->z1 = z1;
	plugin_data->z2 = z2;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runRetroFlange(LADSPA_Handle instance, unsigned long sample_count) {
	RetroFlange *plugin_data = (RetroFlange *)instance;

	/* Average stall (ms) (float value) */
	const LADSPA_Data delay_depth_avg = *(plugin_data->delay_depth_avg);

	/* Flange frequency (Hz) (float value) */
	const LADSPA_Data law_freq = *(plugin_data->law_freq);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data * buffer = plugin_data->buffer;
	long buffer_size = plugin_data->buffer_size;
	long count = plugin_data->count;
	LADSPA_Data * delay_line = plugin_data->delay_line;
	int delay_line_length = plugin_data->delay_line_length;
	int delay_pos = plugin_data->delay_pos;
	LADSPA_Data last_in = plugin_data->last_in;
	int last_law_p = plugin_data->last_law_p;
	int last_phase = plugin_data->last_phase;
	int max_law_p = plugin_data->max_law_p;
	float next_law_peak = plugin_data->next_law_peak;
	int next_law_pos = plugin_data->next_law_pos;
	float phase = plugin_data->phase;
	float prev_law_peak = plugin_data->prev_law_peak;
	int prev_law_pos = plugin_data->prev_law_pos;
	long sample_rate = plugin_data->sample_rate;
	LADSPA_Data z0 = plugin_data->z0;
	LADSPA_Data z1 = plugin_data->z1;
	LADSPA_Data z2 = plugin_data->z2;

#line 75 "retro_flange_1208.xml"
	long int pos;
	int law_p = f_trunc(LIMIT(sample_rate / f_clamp(law_freq, 0.0001f, 100.0f), 1, max_law_p));
	float increment;
	float lin_int, lin_inc;
	int track;
	int fph;
	LADSPA_Data out = 0.0f;
	const float dda_c = f_clamp(delay_depth_avg, 0.0f, 10.0f);
	int dl_used = (dda_c * sample_rate) / 1000;
	float inc_base = 1000.0f * (float)BASE_BUFFER;
	const float delay_depth = 2.0f * dda_c;
	float n_ph, p_ph, law;
	
	for (pos = 0; pos < sample_count; pos++) {
	        // Write into the delay line
	        delay_line[delay_pos] = input[pos];
	        z0 = delay_line[MOD(delay_pos - dl_used, delay_line_length)] + 0.12919609397f*z1 - 0.31050847f*z2;
	        out = sat(z0*0.20466966f + z1*0.40933933f + z2*0.40933933f,
	                        -0.23f, 3.3f);
	        z2 = z1; z1 = z0;
	        delay_pos = (delay_pos + 1) % delay_line_length;
	
	        if ((count++ % law_p) == 0) {
	                // Value for amplitude of law peak
	                next_law_peak = (float)rand() / (float)RAND_MAX;
	                next_law_pos = count + law_p;
	        } else if (count % law_p == law_p / 2) {
	                // Value for amplitude of law peak
	                prev_law_peak = (float)rand() / (float)RAND_MAX;
	                prev_law_pos = count + law_p;
	        }
	
	        n_ph = (float)(law_p - abs(next_law_pos - count))/(float)law_p;
	        p_ph = n_ph + 0.5f;
	        if (p_ph > 1.0f) {
	                p_ph -= 1.0f;
	        }
	        law = f_sin_sq(3.1415926f*p_ph)*prev_law_peak +
	                f_sin_sq(3.1415926f*n_ph)*next_law_peak;
	
	        increment = inc_base / (delay_depth * law + 0.2);
	        fph = f_trunc(phase);
	        last_phase = fph;
	        lin_int = phase - (float)fph;
	        out += LIN_INTERP(lin_int, buffer[(fph+1) % buffer_size],
	         buffer[(fph+2) % buffer_size]);
	        phase += increment;
	        lin_inc = 1.0f / (floor(phase) - last_phase + 1);
	        lin_inc = lin_inc > 1.0f ? 1.0f : lin_inc;
	        lin_int = 0.0f;
	        for (track = last_phase; track < phase; track++) {
	                lin_int += lin_inc;
	                buffer[track % buffer_size] =
	                 LIN_INTERP(lin_int, last_in, input[pos]);
	        }
	        last_in = input[pos];
	        buffer_write(output[pos], out * 0.707f);
	        if (phase >= buffer_size) {
	                phase -= buffer_size;
	        }
	}
	
	// Store current phase in instance
	plugin_data->phase = phase;
	plugin_data->prev_law_peak = prev_law_peak;
	plugin_data->next_law_peak = next_law_peak;
	plugin_data->prev_law_pos = prev_law_pos;
	plugin_data->next_law_pos = next_law_pos;
	plugin_data->last_phase = last_phase;
	plugin_data->last_in = last_in;
	plugin_data->count = count;
	plugin_data->last_law_p = last_law_p;
	plugin_data->delay_pos = delay_pos;
	plugin_data->z0 = z0;
	plugin_data->z1 = z1;
	plugin_data->z2 = z2;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainRetroFlange(LADSPA_Handle instance, LADSPA_Data gain) {
	((RetroFlange *)instance)->run_adding_gain = gain;
}

static void runAddingRetroFlange(LADSPA_Handle instance, unsigned long sample_count) {
	RetroFlange *plugin_data = (RetroFlange *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Average stall (ms) (float value) */
	const LADSPA_Data delay_depth_avg = *(plugin_data->delay_depth_avg);

	/* Flange frequency (Hz) (float value) */
	const LADSPA_Data law_freq = *(plugin_data->law_freq);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data * buffer = plugin_data->buffer;
	long buffer_size = plugin_data->buffer_size;
	long count = plugin_data->count;
	LADSPA_Data * delay_line = plugin_data->delay_line;
	int delay_line_length = plugin_data->delay_line_length;
	int delay_pos = plugin_data->delay_pos;
	LADSPA_Data last_in = plugin_data->last_in;
	int last_law_p = plugin_data->last_law_p;
	int last_phase = plugin_data->last_phase;
	int max_law_p = plugin_data->max_law_p;
	float next_law_peak = plugin_data->next_law_peak;
	int next_law_pos = plugin_data->next_law_pos;
	float phase = plugin_data->phase;
	float prev_law_peak = plugin_data->prev_law_peak;
	int prev_law_pos = plugin_data->prev_law_pos;
	long sample_rate = plugin_data->sample_rate;
	LADSPA_Data z0 = plugin_data->z0;
	LADSPA_Data z1 = plugin_data->z1;
	LADSPA_Data z2 = plugin_data->z2;

#line 75 "retro_flange_1208.xml"
	long int pos;
	int law_p = f_trunc(LIMIT(sample_rate / f_clamp(law_freq, 0.0001f, 100.0f), 1, max_law_p));
	float increment;
	float lin_int, lin_inc;
	int track;
	int fph;
	LADSPA_Data out = 0.0f;
	const float dda_c = f_clamp(delay_depth_avg, 0.0f, 10.0f);
	int dl_used = (dda_c * sample_rate) / 1000;
	float inc_base = 1000.0f * (float)BASE_BUFFER;
	const float delay_depth = 2.0f * dda_c;
	float n_ph, p_ph, law;
	
	for (pos = 0; pos < sample_count; pos++) {
	        // Write into the delay line
	        delay_line[delay_pos] = input[pos];
	        z0 = delay_line[MOD(delay_pos - dl_used, delay_line_length)] + 0.12919609397f*z1 - 0.31050847f*z2;
	        out = sat(z0*0.20466966f + z1*0.40933933f + z2*0.40933933f,
	                        -0.23f, 3.3f);
	        z2 = z1; z1 = z0;
	        delay_pos = (delay_pos + 1) % delay_line_length;
	
	        if ((count++ % law_p) == 0) {
	                // Value for amplitude of law peak
	                next_law_peak = (float)rand() / (float)RAND_MAX;
	                next_law_pos = count + law_p;
	        } else if (count % law_p == law_p / 2) {
	                // Value for amplitude of law peak
	                prev_law_peak = (float)rand() / (float)RAND_MAX;
	                prev_law_pos = count + law_p;
	        }
	
	        n_ph = (float)(law_p - abs(next_law_pos - count))/(float)law_p;
	        p_ph = n_ph + 0.5f;
	        if (p_ph > 1.0f) {
	                p_ph -= 1.0f;
	        }
	        law = f_sin_sq(3.1415926f*p_ph)*prev_law_peak +
	                f_sin_sq(3.1415926f*n_ph)*next_law_peak;
	
	        increment = inc_base / (delay_depth * law + 0.2);
	        fph = f_trunc(phase);
	        last_phase = fph;
	        lin_int = phase - (float)fph;
	        out += LIN_INTERP(lin_int, buffer[(fph+1) % buffer_size],
	         buffer[(fph+2) % buffer_size]);
	        phase += increment;
	        lin_inc = 1.0f / (floor(phase) - last_phase + 1);
	        lin_inc = lin_inc > 1.0f ? 1.0f : lin_inc;
	        lin_int = 0.0f;
	        for (track = last_phase; track < phase; track++) {
	                lin_int += lin_inc;
	                buffer[track % buffer_size] =
	                 LIN_INTERP(lin_int, last_in, input[pos]);
	        }
	        last_in = input[pos];
	        buffer_write(output[pos], out * 0.707f);
	        if (phase >= buffer_size) {
	                phase -= buffer_size;
	        }
	}
	
	// Store current phase in instance
	plugin_data->phase = phase;
	plugin_data->prev_law_peak = prev_law_peak;
	plugin_data->next_law_peak = next_law_peak;
	plugin_data->prev_law_pos = prev_law_pos;
	plugin_data->next_law_pos = next_law_pos;
	plugin_data->last_phase = last_phase;
	plugin_data->last_in = last_in;
	plugin_data->count = count;
	plugin_data->last_law_p = last_law_p;
	plugin_data->delay_pos = delay_pos;
	plugin_data->z0 = z0;
	plugin_data->z1 = z1;
	plugin_data->z2 = z2;
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


	retroFlangeDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (retroFlangeDescriptor) {
		retroFlangeDescriptor->UniqueID = 1208;
		retroFlangeDescriptor->Label = "retroFlange";
		retroFlangeDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		retroFlangeDescriptor->Name =
		 D_("Retro Flanger");
		retroFlangeDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		retroFlangeDescriptor->Copyright =
		 "GPL";
		retroFlangeDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		retroFlangeDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		retroFlangeDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		retroFlangeDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Average stall (ms) */
		port_descriptors[RETROFLANGE_DELAY_DEPTH_AVG] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[RETROFLANGE_DELAY_DEPTH_AVG] =
		 D_("Average stall (ms)");
		port_range_hints[RETROFLANGE_DELAY_DEPTH_AVG].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[RETROFLANGE_DELAY_DEPTH_AVG].LowerBound = 0;
		port_range_hints[RETROFLANGE_DELAY_DEPTH_AVG].UpperBound = 10;

		/* Parameters for Flange frequency (Hz) */
		port_descriptors[RETROFLANGE_LAW_FREQ] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[RETROFLANGE_LAW_FREQ] =
		 D_("Flange frequency (Hz)");
		port_range_hints[RETROFLANGE_LAW_FREQ].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[RETROFLANGE_LAW_FREQ].LowerBound = 0.5;
		port_range_hints[RETROFLANGE_LAW_FREQ].UpperBound = 8;

		/* Parameters for Input */
		port_descriptors[RETROFLANGE_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[RETROFLANGE_INPUT] =
		 D_("Input");
		port_range_hints[RETROFLANGE_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[RETROFLANGE_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[RETROFLANGE_OUTPUT] =
		 D_("Output");
		port_range_hints[RETROFLANGE_OUTPUT].HintDescriptor = 0;

		retroFlangeDescriptor->activate = activateRetroFlange;
		retroFlangeDescriptor->cleanup = cleanupRetroFlange;
		retroFlangeDescriptor->connect_port = connectPortRetroFlange;
		retroFlangeDescriptor->deactivate = NULL;
		retroFlangeDescriptor->instantiate = instantiateRetroFlange;
		retroFlangeDescriptor->run = runRetroFlange;
		retroFlangeDescriptor->run_adding = runAddingRetroFlange;
		retroFlangeDescriptor->set_run_adding_gain = setRunAddingGainRetroFlange;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (retroFlangeDescriptor) {
		free((LADSPA_PortDescriptor *)retroFlangeDescriptor->PortDescriptors);
		free((char **)retroFlangeDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)retroFlangeDescriptor->PortRangeHints);
		free(retroFlangeDescriptor);
	}

}
