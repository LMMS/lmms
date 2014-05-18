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

#line 10 "transient_1206.xml"

#include "ladspa-util.h"

#define BUFFER_SIZE 10240
#define SSTAB 0.00001f
#define ASTAB 0.02f

#define TRANSIENT_ATTACK               0
#define TRANSIENT_SUSTAIN              1
#define TRANSIENT_INPUT                2
#define TRANSIENT_OUTPUT               3

static LADSPA_Descriptor *transientDescriptor = NULL;

typedef struct {
	LADSPA_Data *attack;
	LADSPA_Data *sustain;
	LADSPA_Data *input;
	LADSPA_Data *output;
	float *      buffer;
	int          buffer_pos;
	long         count;
	float        fast_buffer_sum;
	float        fast_track;
	float        medi_buffer_sum;
	float        medi_track;
	int          sample_rate;
	float        slow_buffer_sum;
	float        slow_track;
	LADSPA_Data run_adding_gain;
} Transient;

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
		return transientDescriptor;
	default:
		return NULL;
	}
}

static void activateTransient(LADSPA_Handle instance) {
	Transient *plugin_data = (Transient *)instance;
	float *buffer = plugin_data->buffer;
	int buffer_pos = plugin_data->buffer_pos;
	long count = plugin_data->count;
	float fast_buffer_sum = plugin_data->fast_buffer_sum;
	float fast_track = plugin_data->fast_track;
	float medi_buffer_sum = plugin_data->medi_buffer_sum;
	float medi_track = plugin_data->medi_track;
	int sample_rate = plugin_data->sample_rate;
	float slow_buffer_sum = plugin_data->slow_buffer_sum;
	float slow_track = plugin_data->slow_track;
#line 36 "transient_1206.xml"
	memset(buffer, '\0', BUFFER_SIZE * sizeof(float));
	fast_buffer_sum = 0.1;
	medi_buffer_sum = 0.1;
	slow_buffer_sum = 0.1;
	buffer_pos = 0;
	fast_track = 0.1;
	medi_track = 0.1;
	slow_track = 0.1;
	count = 0;
	sample_rate = sample_rate;
	plugin_data->buffer = buffer;
	plugin_data->buffer_pos = buffer_pos;
	plugin_data->count = count;
	plugin_data->fast_buffer_sum = fast_buffer_sum;
	plugin_data->fast_track = fast_track;
	plugin_data->medi_buffer_sum = medi_buffer_sum;
	plugin_data->medi_track = medi_track;
	plugin_data->sample_rate = sample_rate;
	plugin_data->slow_buffer_sum = slow_buffer_sum;
	plugin_data->slow_track = slow_track;

}

static void cleanupTransient(LADSPA_Handle instance) {
#line 49 "transient_1206.xml"
	Transient *plugin_data = (Transient *)instance;
	free(plugin_data->buffer);
	free(instance);
}

static void connectPortTransient(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Transient *plugin;

	plugin = (Transient *)instance;
	switch (port) {
	case TRANSIENT_ATTACK:
		plugin->attack = data;
		break;
	case TRANSIENT_SUSTAIN:
		plugin->sustain = data;
		break;
	case TRANSIENT_INPUT:
		plugin->input = data;
		break;
	case TRANSIENT_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateTransient(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Transient *plugin_data = (Transient *)malloc(sizeof(Transient));
	float *buffer = NULL;
	int buffer_pos;
	long count;
	float fast_buffer_sum;
	float fast_track;
	float medi_buffer_sum;
	float medi_track;
	int sample_rate;
	float slow_buffer_sum;
	float slow_track;

#line 23 "transient_1206.xml"
	buffer = calloc(BUFFER_SIZE, sizeof(float));
	fast_buffer_sum = 0.1;
	medi_buffer_sum = 0.1;
	slow_buffer_sum = 0.1;
	buffer_pos = 0;
	fast_track = 0.0;
	medi_track = 0.0;
	slow_track = 0.0;
	count = 0;
	sample_rate = s_rate;

	plugin_data->buffer = buffer;
	plugin_data->buffer_pos = buffer_pos;
	plugin_data->count = count;
	plugin_data->fast_buffer_sum = fast_buffer_sum;
	plugin_data->fast_track = fast_track;
	plugin_data->medi_buffer_sum = medi_buffer_sum;
	plugin_data->medi_track = medi_track;
	plugin_data->sample_rate = sample_rate;
	plugin_data->slow_buffer_sum = slow_buffer_sum;
	plugin_data->slow_track = slow_track;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runTransient(LADSPA_Handle instance, unsigned long sample_count) {
	Transient *plugin_data = (Transient *)instance;

	/* Attack speed (float value) */
	const LADSPA_Data attack = *(plugin_data->attack);

	/* Sustain time (float value) */
	const LADSPA_Data sustain = *(plugin_data->sustain);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	float * buffer = plugin_data->buffer;
	int buffer_pos = plugin_data->buffer_pos;
	long count = plugin_data->count;
	float fast_buffer_sum = plugin_data->fast_buffer_sum;
	float fast_track = plugin_data->fast_track;
	float medi_buffer_sum = plugin_data->medi_buffer_sum;
	float medi_track = plugin_data->medi_track;
	int sample_rate = plugin_data->sample_rate;
	float slow_buffer_sum = plugin_data->slow_buffer_sum;
	float slow_track = plugin_data->slow_track;

#line 53 "transient_1206.xml"
	unsigned long pos;
	const int fast_sum_size = (2 * sample_rate) / 1000;
	const int medi_sum_size = (25 * sample_rate) / 1000;
	const int slow_sum_size = (100 * sample_rate) / 1000;
	const float fast_track_lag = 1.5f / fast_sum_size;
	const float medi_track_lag = 1.0f / medi_sum_size;
	const float slow_track_lag = 1.3f / slow_sum_size;
	float ratio;
	LADSPA_Data in;
	
	for (pos = 0; pos < sample_count; pos++) {
	        in = input[pos];
	        buffer[buffer_pos] = fabs(in);
	        fast_buffer_sum += buffer[buffer_pos];
	        medi_buffer_sum += buffer[buffer_pos];
	        slow_buffer_sum += buffer[buffer_pos];
	        fast_buffer_sum -= buffer[MOD(buffer_pos - fast_sum_size, BUFFER_SIZE)];
	        medi_buffer_sum -= buffer[MOD(buffer_pos - medi_sum_size, BUFFER_SIZE)];
	        slow_buffer_sum -= buffer[MOD(buffer_pos - slow_sum_size, BUFFER_SIZE)];
	        if (count++ > slow_sum_size) {
	                fast_track += (fast_buffer_sum/fast_sum_size - fast_track)
	                 * fast_track_lag;
	                medi_track += (medi_buffer_sum/medi_sum_size - medi_track)
	                 * medi_track_lag;
	                slow_track += (slow_buffer_sum/slow_sum_size - slow_track)
	                 * slow_track_lag;
	        }
	
	        // Attack
	        ratio = (fast_track + ASTAB) / (medi_track + ASTAB);
	        if (ratio * attack > 1.0f) {
	                in *= ratio * attack;
	        } else if (ratio * attack < -1.0f) {
	                in /= ratio * -attack;
	        }
	
	        // Sustain
	        ratio = (slow_track + SSTAB) / (medi_track + SSTAB);
	        if (ratio * sustain > 1.0f) {
	                in *= ratio * sustain;
	        } else if (ratio * sustain < -1.0f) {
	                in /= ratio * -sustain;
	        }
	
	        buffer_write(output[pos], in);
	        buffer_pos = (buffer_pos + 1) % BUFFER_SIZE;
	}
	
	plugin_data->count = count;
	plugin_data->fast_track = fast_track;
	plugin_data->medi_track = medi_track;
	plugin_data->slow_track = slow_track;
	plugin_data->buffer_pos = buffer_pos;
	plugin_data->fast_buffer_sum = fast_buffer_sum;
	plugin_data->medi_buffer_sum = medi_buffer_sum;
	plugin_data->slow_buffer_sum = slow_buffer_sum;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainTransient(LADSPA_Handle instance, LADSPA_Data gain) {
	((Transient *)instance)->run_adding_gain = gain;
}

static void runAddingTransient(LADSPA_Handle instance, unsigned long sample_count) {
	Transient *plugin_data = (Transient *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Attack speed (float value) */
	const LADSPA_Data attack = *(plugin_data->attack);

	/* Sustain time (float value) */
	const LADSPA_Data sustain = *(plugin_data->sustain);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	float * buffer = plugin_data->buffer;
	int buffer_pos = plugin_data->buffer_pos;
	long count = plugin_data->count;
	float fast_buffer_sum = plugin_data->fast_buffer_sum;
	float fast_track = plugin_data->fast_track;
	float medi_buffer_sum = plugin_data->medi_buffer_sum;
	float medi_track = plugin_data->medi_track;
	int sample_rate = plugin_data->sample_rate;
	float slow_buffer_sum = plugin_data->slow_buffer_sum;
	float slow_track = plugin_data->slow_track;

#line 53 "transient_1206.xml"
	unsigned long pos;
	const int fast_sum_size = (2 * sample_rate) / 1000;
	const int medi_sum_size = (25 * sample_rate) / 1000;
	const int slow_sum_size = (100 * sample_rate) / 1000;
	const float fast_track_lag = 1.5f / fast_sum_size;
	const float medi_track_lag = 1.0f / medi_sum_size;
	const float slow_track_lag = 1.3f / slow_sum_size;
	float ratio;
	LADSPA_Data in;
	
	for (pos = 0; pos < sample_count; pos++) {
	        in = input[pos];
	        buffer[buffer_pos] = fabs(in);
	        fast_buffer_sum += buffer[buffer_pos];
	        medi_buffer_sum += buffer[buffer_pos];
	        slow_buffer_sum += buffer[buffer_pos];
	        fast_buffer_sum -= buffer[MOD(buffer_pos - fast_sum_size, BUFFER_SIZE)];
	        medi_buffer_sum -= buffer[MOD(buffer_pos - medi_sum_size, BUFFER_SIZE)];
	        slow_buffer_sum -= buffer[MOD(buffer_pos - slow_sum_size, BUFFER_SIZE)];
	        if (count++ > slow_sum_size) {
	                fast_track += (fast_buffer_sum/fast_sum_size - fast_track)
	                 * fast_track_lag;
	                medi_track += (medi_buffer_sum/medi_sum_size - medi_track)
	                 * medi_track_lag;
	                slow_track += (slow_buffer_sum/slow_sum_size - slow_track)
	                 * slow_track_lag;
	        }
	
	        // Attack
	        ratio = (fast_track + ASTAB) / (medi_track + ASTAB);
	        if (ratio * attack > 1.0f) {
	                in *= ratio * attack;
	        } else if (ratio * attack < -1.0f) {
	                in /= ratio * -attack;
	        }
	
	        // Sustain
	        ratio = (slow_track + SSTAB) / (medi_track + SSTAB);
	        if (ratio * sustain > 1.0f) {
	                in *= ratio * sustain;
	        } else if (ratio * sustain < -1.0f) {
	                in /= ratio * -sustain;
	        }
	
	        buffer_write(output[pos], in);
	        buffer_pos = (buffer_pos + 1) % BUFFER_SIZE;
	}
	
	plugin_data->count = count;
	plugin_data->fast_track = fast_track;
	plugin_data->medi_track = medi_track;
	plugin_data->slow_track = slow_track;
	plugin_data->buffer_pos = buffer_pos;
	plugin_data->fast_buffer_sum = fast_buffer_sum;
	plugin_data->medi_buffer_sum = medi_buffer_sum;
	plugin_data->slow_buffer_sum = slow_buffer_sum;
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


	transientDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (transientDescriptor) {
		transientDescriptor->UniqueID = 1206;
		transientDescriptor->Label = "transient";
		transientDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		transientDescriptor->Name =
		 D_("Transient mangler");
		transientDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		transientDescriptor->Copyright =
		 "GPL";
		transientDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		transientDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		transientDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		transientDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Attack speed */
		port_descriptors[TRANSIENT_ATTACK] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TRANSIENT_ATTACK] =
		 D_("Attack speed");
		port_range_hints[TRANSIENT_ATTACK].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[TRANSIENT_ATTACK].LowerBound = -1;
		port_range_hints[TRANSIENT_ATTACK].UpperBound = 1;

		/* Parameters for Sustain time */
		port_descriptors[TRANSIENT_SUSTAIN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TRANSIENT_SUSTAIN] =
		 D_("Sustain time");
		port_range_hints[TRANSIENT_SUSTAIN].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[TRANSIENT_SUSTAIN].LowerBound = -1;
		port_range_hints[TRANSIENT_SUSTAIN].UpperBound = 1;

		/* Parameters for Input */
		port_descriptors[TRANSIENT_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[TRANSIENT_INPUT] =
		 D_("Input");
		port_range_hints[TRANSIENT_INPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[TRANSIENT_INPUT].LowerBound = -1.0;
		port_range_hints[TRANSIENT_INPUT].UpperBound = 1.0;

		/* Parameters for Output */
		port_descriptors[TRANSIENT_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[TRANSIENT_OUTPUT] =
		 D_("Output");
		port_range_hints[TRANSIENT_OUTPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[TRANSIENT_OUTPUT].LowerBound = -1.0;
		port_range_hints[TRANSIENT_OUTPUT].UpperBound = 1.0;

		transientDescriptor->activate = activateTransient;
		transientDescriptor->cleanup = cleanupTransient;
		transientDescriptor->connect_port = connectPortTransient;
		transientDescriptor->deactivate = NULL;
		transientDescriptor->instantiate = instantiateTransient;
		transientDescriptor->run = runTransient;
		transientDescriptor->run_adding = runAddingTransient;
		transientDescriptor->set_run_adding_gain = setRunAddingGainTransient;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (transientDescriptor) {
		free((LADSPA_PortDescriptor *)transientDescriptor->PortDescriptors);
		free((char **)transientDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)transientDescriptor->PortRangeHints);
		free(transientDescriptor);
	}

}
