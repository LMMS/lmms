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

#line 8 "tape_delay_1211.xml"

#include "ladspa-util.h"

#define BASE_BUFFER 8 // Tape length (inches)

#define TAPEDELAY_SPEED                0
#define TAPEDELAY_DA_DB                1
#define TAPEDELAY_T1D                  2
#define TAPEDELAY_T1A_DB               3
#define TAPEDELAY_T2D                  4
#define TAPEDELAY_T2A_DB               5
#define TAPEDELAY_T3D                  6
#define TAPEDELAY_T3A_DB               7
#define TAPEDELAY_T4D                  8
#define TAPEDELAY_T4A_DB               9
#define TAPEDELAY_INPUT                10
#define TAPEDELAY_OUTPUT               11

static LADSPA_Descriptor *tapeDelayDescriptor = NULL;

typedef struct {
	LADSPA_Data *speed;
	LADSPA_Data *da_db;
	LADSPA_Data *t1d;
	LADSPA_Data *t1a_db;
	LADSPA_Data *t2d;
	LADSPA_Data *t2a_db;
	LADSPA_Data *t3d;
	LADSPA_Data *t3a_db;
	LADSPA_Data *t4d;
	LADSPA_Data *t4a_db;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data *buffer;
	unsigned int buffer_mask;
	unsigned int buffer_size;
	LADSPA_Data  last2_in;
	LADSPA_Data  last3_in;
	LADSPA_Data  last_in;
	unsigned int last_phase;
	float        phase;
	int          sample_rate;
	LADSPA_Data  z0;
	LADSPA_Data  z1;
	LADSPA_Data  z2;
	LADSPA_Data run_adding_gain;
} TapeDelay;

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
		return tapeDelayDescriptor;
	default:
		return NULL;
	}
}

static void activateTapeDelay(LADSPA_Handle instance) {
	TapeDelay *plugin_data = (TapeDelay *)instance;
	LADSPA_Data *buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	unsigned int buffer_size = plugin_data->buffer_size;
	LADSPA_Data last2_in = plugin_data->last2_in;
	LADSPA_Data last3_in = plugin_data->last3_in;
	LADSPA_Data last_in = plugin_data->last_in;
	unsigned int last_phase = plugin_data->last_phase;
	float phase = plugin_data->phase;
	int sample_rate = plugin_data->sample_rate;
	LADSPA_Data z0 = plugin_data->z0;
	LADSPA_Data z1 = plugin_data->z1;
	LADSPA_Data z2 = plugin_data->z2;
#line 38 "tape_delay_1211.xml"
	int i;

	for (i = 0; i < buffer_size; i++) {
	        buffer[i] = 0;
	}
	phase = 0;
	last_phase = 0;
	last_in = 0.0f;
	last2_in = 0.0f;
	last3_in = 0.0f;
	sample_rate = sample_rate;
	z0 = 0.0f;
	z1 = 0.0f;
	z2 = 0.0f;
	plugin_data->buffer = buffer;
	plugin_data->buffer_mask = buffer_mask;
	plugin_data->buffer_size = buffer_size;
	plugin_data->last2_in = last2_in;
	plugin_data->last3_in = last3_in;
	plugin_data->last_in = last_in;
	plugin_data->last_phase = last_phase;
	plugin_data->phase = phase;
	plugin_data->sample_rate = sample_rate;
	plugin_data->z0 = z0;
	plugin_data->z1 = z1;
	plugin_data->z2 = z2;

}

static void cleanupTapeDelay(LADSPA_Handle instance) {
#line 55 "tape_delay_1211.xml"
	TapeDelay *plugin_data = (TapeDelay *)instance;
	free(plugin_data->buffer);
	free(instance);
}

static void connectPortTapeDelay(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	TapeDelay *plugin;

	plugin = (TapeDelay *)instance;
	switch (port) {
	case TAPEDELAY_SPEED:
		plugin->speed = data;
		break;
	case TAPEDELAY_DA_DB:
		plugin->da_db = data;
		break;
	case TAPEDELAY_T1D:
		plugin->t1d = data;
		break;
	case TAPEDELAY_T1A_DB:
		plugin->t1a_db = data;
		break;
	case TAPEDELAY_T2D:
		plugin->t2d = data;
		break;
	case TAPEDELAY_T2A_DB:
		plugin->t2a_db = data;
		break;
	case TAPEDELAY_T3D:
		plugin->t3d = data;
		break;
	case TAPEDELAY_T3A_DB:
		plugin->t3a_db = data;
		break;
	case TAPEDELAY_T4D:
		plugin->t4d = data;
		break;
	case TAPEDELAY_T4A_DB:
		plugin->t4a_db = data;
		break;
	case TAPEDELAY_INPUT:
		plugin->input = data;
		break;
	case TAPEDELAY_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateTapeDelay(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	TapeDelay *plugin_data = (TapeDelay *)malloc(sizeof(TapeDelay));
	LADSPA_Data *buffer = NULL;
	unsigned int buffer_mask;
	unsigned int buffer_size;
	LADSPA_Data last2_in;
	LADSPA_Data last3_in;
	LADSPA_Data last_in;
	unsigned int last_phase;
	float phase;
	int sample_rate;
	LADSPA_Data z0;
	LADSPA_Data z1;
	LADSPA_Data z2;

#line 21 "tape_delay_1211.xml"
	unsigned int mbs = BASE_BUFFER * s_rate;
	sample_rate = s_rate;
	for (buffer_size = 4096; buffer_size < mbs;
	     buffer_size *= 2);
	buffer = malloc(buffer_size * sizeof(LADSPA_Data));
	buffer_mask = buffer_size - 1;
	phase = 0;
	last_phase = 0;
	last_in = 0.0f;
	last2_in = 0.0f;
	last3_in = 0.0f;
	z0 = 0.0f;
	z1 = 0.0f;
	z2 = 0.0f;

	plugin_data->buffer = buffer;
	plugin_data->buffer_mask = buffer_mask;
	plugin_data->buffer_size = buffer_size;
	plugin_data->last2_in = last2_in;
	plugin_data->last3_in = last3_in;
	plugin_data->last_in = last_in;
	plugin_data->last_phase = last_phase;
	plugin_data->phase = phase;
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

static void runTapeDelay(LADSPA_Handle instance, unsigned long sample_count) {
	TapeDelay *plugin_data = (TapeDelay *)instance;

	/* Tape speed (inches/sec, 1=normal) (float value) */
	const LADSPA_Data speed = *(plugin_data->speed);

	/* Dry level (dB) (float value) */
	const LADSPA_Data da_db = *(plugin_data->da_db);

	/* Tap 1 distance (inches) (float value) */
	const LADSPA_Data t1d = *(plugin_data->t1d);

	/* Tap 1 level (dB) (float value) */
	const LADSPA_Data t1a_db = *(plugin_data->t1a_db);

	/* Tap 2 distance (inches) (float value) */
	const LADSPA_Data t2d = *(plugin_data->t2d);

	/* Tap 2 level (dB) (float value) */
	const LADSPA_Data t2a_db = *(plugin_data->t2a_db);

	/* Tap 3 distance (inches) (float value) */
	const LADSPA_Data t3d = *(plugin_data->t3d);

	/* Tap 3 level (dB) (float value) */
	const LADSPA_Data t3a_db = *(plugin_data->t3a_db);

	/* Tap 4 distance (inches) (float value) */
	const LADSPA_Data t4d = *(plugin_data->t4d);

	/* Tap 4 level (dB) (float value) */
	const LADSPA_Data t4a_db = *(plugin_data->t4a_db);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	unsigned int buffer_size = plugin_data->buffer_size;
	LADSPA_Data last2_in = plugin_data->last2_in;
	LADSPA_Data last3_in = plugin_data->last3_in;
	LADSPA_Data last_in = plugin_data->last_in;
	unsigned int last_phase = plugin_data->last_phase;
	float phase = plugin_data->phase;
	int sample_rate = plugin_data->sample_rate;
	LADSPA_Data z0 = plugin_data->z0;
	LADSPA_Data z1 = plugin_data->z1;
	LADSPA_Data z2 = plugin_data->z2;

#line 59 "tape_delay_1211.xml"
	unsigned int pos;
	float increment = f_clamp(speed, 0.0f, 40.0f);
	float lin_int, lin_inc;
	unsigned int track;
	unsigned int fph;
	LADSPA_Data out;
	
	const float da = DB_CO(da_db);
	const float t1a = DB_CO(t1a_db);
	const float t2a = DB_CO(t2a_db);
	const float t3a = DB_CO(t3a_db);
	const float t4a = DB_CO(t4a_db);
	const unsigned int t1d_s = f_round(t1d * sample_rate);
	const unsigned int t2d_s = f_round(t2d * sample_rate);
	const unsigned int t3d_s = f_round(t3d * sample_rate);
	const unsigned int t4d_s = f_round(t4d * sample_rate);
	
	for (pos = 0; pos < sample_count; pos++) {
	        fph = f_trunc(phase);
	        last_phase = fph;
	        lin_int = phase - (float)fph;
	
	        out = buffer[(unsigned int)(fph - t1d_s) & buffer_mask] * t1a;
	        out += buffer[(unsigned int)(fph - t2d_s) & buffer_mask] * t2a;
	        out += buffer[(unsigned int)(fph - t3d_s) & buffer_mask] * t3a;
	        out += buffer[(unsigned int)(fph - t4d_s) & buffer_mask] * t4a;
	
	        phase += increment;
	        lin_inc = 1.0f / (floor(phase) - last_phase + 1);
	        lin_inc = lin_inc > 1.0f ? 1.0f : lin_inc;
	        lin_int = 0.0f;
	        for (track = last_phase; track < phase; track++) {
	                lin_int += lin_inc;
	                buffer[track & buffer_mask] =
	                 cube_interp(lin_int, last3_in, last2_in, last_in, input[pos]);
	        }
	        last3_in = last2_in;
	        last2_in = last_in;
	        last_in = input[pos];
	        out += input[pos] * da;
	        buffer_write(output[pos], out);
	        if (phase >= buffer_size) {
	                phase -= buffer_size;
	        }
	}
	
	// Store current phase in instance
	plugin_data->phase = phase;
	plugin_data->last_phase = last_phase;
	plugin_data->last_in = last_in;
	plugin_data->last2_in = last2_in;
	plugin_data->last3_in = last3_in;
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

static void setRunAddingGainTapeDelay(LADSPA_Handle instance, LADSPA_Data gain) {
	((TapeDelay *)instance)->run_adding_gain = gain;
}

static void runAddingTapeDelay(LADSPA_Handle instance, unsigned long sample_count) {
	TapeDelay *plugin_data = (TapeDelay *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Tape speed (inches/sec, 1=normal) (float value) */
	const LADSPA_Data speed = *(plugin_data->speed);

	/* Dry level (dB) (float value) */
	const LADSPA_Data da_db = *(plugin_data->da_db);

	/* Tap 1 distance (inches) (float value) */
	const LADSPA_Data t1d = *(plugin_data->t1d);

	/* Tap 1 level (dB) (float value) */
	const LADSPA_Data t1a_db = *(plugin_data->t1a_db);

	/* Tap 2 distance (inches) (float value) */
	const LADSPA_Data t2d = *(plugin_data->t2d);

	/* Tap 2 level (dB) (float value) */
	const LADSPA_Data t2a_db = *(plugin_data->t2a_db);

	/* Tap 3 distance (inches) (float value) */
	const LADSPA_Data t3d = *(plugin_data->t3d);

	/* Tap 3 level (dB) (float value) */
	const LADSPA_Data t3a_db = *(plugin_data->t3a_db);

	/* Tap 4 distance (inches) (float value) */
	const LADSPA_Data t4d = *(plugin_data->t4d);

	/* Tap 4 level (dB) (float value) */
	const LADSPA_Data t4a_db = *(plugin_data->t4a_db);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	unsigned int buffer_size = plugin_data->buffer_size;
	LADSPA_Data last2_in = plugin_data->last2_in;
	LADSPA_Data last3_in = plugin_data->last3_in;
	LADSPA_Data last_in = plugin_data->last_in;
	unsigned int last_phase = plugin_data->last_phase;
	float phase = plugin_data->phase;
	int sample_rate = plugin_data->sample_rate;
	LADSPA_Data z0 = plugin_data->z0;
	LADSPA_Data z1 = plugin_data->z1;
	LADSPA_Data z2 = plugin_data->z2;

#line 59 "tape_delay_1211.xml"
	unsigned int pos;
	float increment = f_clamp(speed, 0.0f, 40.0f);
	float lin_int, lin_inc;
	unsigned int track;
	unsigned int fph;
	LADSPA_Data out;
	
	const float da = DB_CO(da_db);
	const float t1a = DB_CO(t1a_db);
	const float t2a = DB_CO(t2a_db);
	const float t3a = DB_CO(t3a_db);
	const float t4a = DB_CO(t4a_db);
	const unsigned int t1d_s = f_round(t1d * sample_rate);
	const unsigned int t2d_s = f_round(t2d * sample_rate);
	const unsigned int t3d_s = f_round(t3d * sample_rate);
	const unsigned int t4d_s = f_round(t4d * sample_rate);
	
	for (pos = 0; pos < sample_count; pos++) {
	        fph = f_trunc(phase);
	        last_phase = fph;
	        lin_int = phase - (float)fph;
	
	        out = buffer[(unsigned int)(fph - t1d_s) & buffer_mask] * t1a;
	        out += buffer[(unsigned int)(fph - t2d_s) & buffer_mask] * t2a;
	        out += buffer[(unsigned int)(fph - t3d_s) & buffer_mask] * t3a;
	        out += buffer[(unsigned int)(fph - t4d_s) & buffer_mask] * t4a;
	
	        phase += increment;
	        lin_inc = 1.0f / (floor(phase) - last_phase + 1);
	        lin_inc = lin_inc > 1.0f ? 1.0f : lin_inc;
	        lin_int = 0.0f;
	        for (track = last_phase; track < phase; track++) {
	                lin_int += lin_inc;
	                buffer[track & buffer_mask] =
	                 cube_interp(lin_int, last3_in, last2_in, last_in, input[pos]);
	        }
	        last3_in = last2_in;
	        last2_in = last_in;
	        last_in = input[pos];
	        out += input[pos] * da;
	        buffer_write(output[pos], out);
	        if (phase >= buffer_size) {
	                phase -= buffer_size;
	        }
	}
	
	// Store current phase in instance
	plugin_data->phase = phase;
	plugin_data->last_phase = last_phase;
	plugin_data->last_in = last_in;
	plugin_data->last2_in = last2_in;
	plugin_data->last3_in = last3_in;
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


	tapeDelayDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (tapeDelayDescriptor) {
		tapeDelayDescriptor->UniqueID = 1211;
		tapeDelayDescriptor->Label = "tapeDelay";
		tapeDelayDescriptor->Properties =
		 0;
		tapeDelayDescriptor->Name =
		 D_("Tape Delay Simulation");
		tapeDelayDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		tapeDelayDescriptor->Copyright =
		 "GPL";
		tapeDelayDescriptor->PortCount = 12;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(12,
		 sizeof(LADSPA_PortDescriptor));
		tapeDelayDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(12,
		 sizeof(LADSPA_PortRangeHint));
		tapeDelayDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(12, sizeof(char*));
		tapeDelayDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Tape speed (inches/sec, 1=normal) */
		port_descriptors[TAPEDELAY_SPEED] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TAPEDELAY_SPEED] =
		 D_("Tape speed (inches/sec, 1=normal)");
		port_range_hints[TAPEDELAY_SPEED].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[TAPEDELAY_SPEED].LowerBound = 0;
		port_range_hints[TAPEDELAY_SPEED].UpperBound = 10;

		/* Parameters for Dry level (dB) */
		port_descriptors[TAPEDELAY_DA_DB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TAPEDELAY_DA_DB] =
		 D_("Dry level (dB)");
		port_range_hints[TAPEDELAY_DA_DB].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MINIMUM;
		port_range_hints[TAPEDELAY_DA_DB].LowerBound = -90;
		port_range_hints[TAPEDELAY_DA_DB].UpperBound = 0;

		/* Parameters for Tap 1 distance (inches) */
		port_descriptors[TAPEDELAY_T1D] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TAPEDELAY_T1D] =
		 D_("Tap 1 distance (inches)");
		port_range_hints[TAPEDELAY_T1D].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[TAPEDELAY_T1D].LowerBound = 0;
		port_range_hints[TAPEDELAY_T1D].UpperBound = 4;

		/* Parameters for Tap 1 level (dB) */
		port_descriptors[TAPEDELAY_T1A_DB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TAPEDELAY_T1A_DB] =
		 D_("Tap 1 level (dB)");
		port_range_hints[TAPEDELAY_T1A_DB].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[TAPEDELAY_T1A_DB].LowerBound = -90;
		port_range_hints[TAPEDELAY_T1A_DB].UpperBound = 0;

		/* Parameters for Tap 2 distance (inches) */
		port_descriptors[TAPEDELAY_T2D] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TAPEDELAY_T2D] =
		 D_("Tap 2 distance (inches)");
		port_range_hints[TAPEDELAY_T2D].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[TAPEDELAY_T2D].LowerBound = 0;
		port_range_hints[TAPEDELAY_T2D].UpperBound = 4;

		/* Parameters for Tap 2 level (dB) */
		port_descriptors[TAPEDELAY_T2A_DB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TAPEDELAY_T2A_DB] =
		 D_("Tap 2 level (dB)");
		port_range_hints[TAPEDELAY_T2A_DB].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MINIMUM;
		port_range_hints[TAPEDELAY_T2A_DB].LowerBound = -90;
		port_range_hints[TAPEDELAY_T2A_DB].UpperBound = 0;

		/* Parameters for Tap 3 distance (inches) */
		port_descriptors[TAPEDELAY_T3D] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TAPEDELAY_T3D] =
		 D_("Tap 3 distance (inches)");
		port_range_hints[TAPEDELAY_T3D].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[TAPEDELAY_T3D].LowerBound = 0;
		port_range_hints[TAPEDELAY_T3D].UpperBound = 4;

		/* Parameters for Tap 3 level (dB) */
		port_descriptors[TAPEDELAY_T3A_DB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TAPEDELAY_T3A_DB] =
		 D_("Tap 3 level (dB)");
		port_range_hints[TAPEDELAY_T3A_DB].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MINIMUM;
		port_range_hints[TAPEDELAY_T3A_DB].LowerBound = -90;
		port_range_hints[TAPEDELAY_T3A_DB].UpperBound = 0;

		/* Parameters for Tap 4 distance (inches) */
		port_descriptors[TAPEDELAY_T4D] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TAPEDELAY_T4D] =
		 D_("Tap 4 distance (inches)");
		port_range_hints[TAPEDELAY_T4D].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_HIGH;
		port_range_hints[TAPEDELAY_T4D].LowerBound = 0;
		port_range_hints[TAPEDELAY_T4D].UpperBound = 4;

		/* Parameters for Tap 4 level (dB) */
		port_descriptors[TAPEDELAY_T4A_DB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TAPEDELAY_T4A_DB] =
		 D_("Tap 4 level (dB)");
		port_range_hints[TAPEDELAY_T4A_DB].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MINIMUM;
		port_range_hints[TAPEDELAY_T4A_DB].LowerBound = -90;
		port_range_hints[TAPEDELAY_T4A_DB].UpperBound = 0;

		/* Parameters for Input */
		port_descriptors[TAPEDELAY_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[TAPEDELAY_INPUT] =
		 D_("Input");
		port_range_hints[TAPEDELAY_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[TAPEDELAY_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[TAPEDELAY_OUTPUT] =
		 D_("Output");
		port_range_hints[TAPEDELAY_OUTPUT].HintDescriptor = 0;

		tapeDelayDescriptor->activate = activateTapeDelay;
		tapeDelayDescriptor->cleanup = cleanupTapeDelay;
		tapeDelayDescriptor->connect_port = connectPortTapeDelay;
		tapeDelayDescriptor->deactivate = NULL;
		tapeDelayDescriptor->instantiate = instantiateTapeDelay;
		tapeDelayDescriptor->run = runTapeDelay;
		tapeDelayDescriptor->run_adding = runAddingTapeDelay;
		tapeDelayDescriptor->set_run_adding_gain = setRunAddingGainTapeDelay;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (tapeDelayDescriptor) {
		free((LADSPA_PortDescriptor *)tapeDelayDescriptor->PortDescriptors);
		free((char **)tapeDelayDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)tapeDelayDescriptor->PortRangeHints);
		free(tapeDelayDescriptor);
	}

}
