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

#line 9 "ringmod_1188.xml"

#include "ladspa-util.h"

int refcount;
LADSPA_Data *sin_tbl, *tri_tbl, *saw_tbl, *squ_tbl;
long sample_rate;

#define RINGMOD_2I1O_DEPTH             0
#define RINGMOD_2I1O_INPUT             1
#define RINGMOD_2I1O_MODULATOR         2
#define RINGMOD_2I1O_OUTPUT            3
#define RINGMOD_1I1O1L_DEPTHP          0
#define RINGMOD_1I1O1L_FREQ            1
#define RINGMOD_1I1O1L_SIN             2
#define RINGMOD_1I1O1L_TRI             3
#define RINGMOD_1I1O1L_SAW             4
#define RINGMOD_1I1O1L_SQU             5
#define RINGMOD_1I1O1L_INPUT           6
#define RINGMOD_1I1O1L_OUTPUT          7

static LADSPA_Descriptor *ringmod_2i1oDescriptor = NULL;

typedef struct {
	LADSPA_Data *depth;
	LADSPA_Data *input;
	LADSPA_Data *modulator;
	LADSPA_Data *output;
	LADSPA_Data run_adding_gain;
} Ringmod_2i1o;

static LADSPA_Descriptor *ringmod_1i1o1lDescriptor = NULL;

typedef struct {
	LADSPA_Data *depthp;
	LADSPA_Data *freq;
	LADSPA_Data *sin;
	LADSPA_Data *tri;
	LADSPA_Data *saw;
	LADSPA_Data *squ;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data  offset;
	LADSPA_Data run_adding_gain;
} Ringmod_1i1o1l;

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
		return ringmod_2i1oDescriptor;
	case 1:
		return ringmod_1i1o1lDescriptor;
	default:
		return NULL;
	}
}

static void cleanupRingmod_2i1o(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortRingmod_2i1o(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Ringmod_2i1o *plugin;

	plugin = (Ringmod_2i1o *)instance;
	switch (port) {
	case RINGMOD_2I1O_DEPTH:
		plugin->depth = data;
		break;
	case RINGMOD_2I1O_INPUT:
		plugin->input = data;
		break;
	case RINGMOD_2I1O_MODULATOR:
		plugin->modulator = data;
		break;
	case RINGMOD_2I1O_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateRingmod_2i1o(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Ringmod_2i1o *plugin_data = (Ringmod_2i1o *)malloc(sizeof(Ringmod_2i1o));
	plugin_data->run_adding_gain = 1.0f;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runRingmod_2i1o(LADSPA_Handle instance, unsigned long sample_count) {
	Ringmod_2i1o *plugin_data = (Ringmod_2i1o *)instance;

	/* Modulation depth (0=none, 1=AM, 2=RM) (float value) */
	const LADSPA_Data depth = *(plugin_data->depth);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Modulator (array of floats of length sample_count) */
	const LADSPA_Data * const modulator = plugin_data->modulator;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

#line 24 "ringmod_1188.xml"
	unsigned long pos;
	float tmpa = depth * 0.5f;
	float tmpb = 2.0f - depth;

	for (pos = 0; pos < sample_count; pos++) {
	        buffer_write(output[pos], input[pos] * (tmpa * modulator[pos] + tmpb));
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainRingmod_2i1o(LADSPA_Handle instance, LADSPA_Data gain) {
	((Ringmod_2i1o *)instance)->run_adding_gain = gain;
}

static void runAddingRingmod_2i1o(LADSPA_Handle instance, unsigned long sample_count) {
	Ringmod_2i1o *plugin_data = (Ringmod_2i1o *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Modulation depth (0=none, 1=AM, 2=RM) (float value) */
	const LADSPA_Data depth = *(plugin_data->depth);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Modulator (array of floats of length sample_count) */
	const LADSPA_Data * const modulator = plugin_data->modulator;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

#line 24 "ringmod_1188.xml"
	unsigned long pos;
	float tmpa = depth * 0.5f;
	float tmpb = 2.0f - depth;

	for (pos = 0; pos < sample_count; pos++) {
	        buffer_write(output[pos], input[pos] * (tmpa * modulator[pos] + tmpb));
	}
}

static void activateRingmod_1i1o1l(LADSPA_Handle instance) {
	Ringmod_1i1o1l *plugin_data = (Ringmod_1i1o1l *)instance;
	LADSPA_Data offset = plugin_data->offset;
#line 89 "ringmod_1188.xml"
	offset = 0;
	plugin_data->offset = offset;

}

static void cleanupRingmod_1i1o1l(LADSPA_Handle instance) {
#line 93 "ringmod_1188.xml"
	Ringmod_1i1o1l *plugin_data = (Ringmod_1i1o1l *)instance;
	plugin_data = plugin_data;
	if (--refcount == 0) {
	        free(sin_tbl);
	        free(tri_tbl);
	        free(squ_tbl);
	        free(saw_tbl);
	}
	free(instance);
}

static void connectPortRingmod_1i1o1l(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Ringmod_1i1o1l *plugin;

	plugin = (Ringmod_1i1o1l *)instance;
	switch (port) {
	case RINGMOD_1I1O1L_DEPTHP:
		plugin->depthp = data;
		break;
	case RINGMOD_1I1O1L_FREQ:
		plugin->freq = data;
		break;
	case RINGMOD_1I1O1L_SIN:
		plugin->sin = data;
		break;
	case RINGMOD_1I1O1L_TRI:
		plugin->tri = data;
		break;
	case RINGMOD_1I1O1L_SAW:
		plugin->saw = data;
		break;
	case RINGMOD_1I1O1L_SQU:
		plugin->squ = data;
		break;
	case RINGMOD_1I1O1L_INPUT:
		plugin->input = data;
		break;
	case RINGMOD_1I1O1L_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateRingmod_1i1o1l(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Ringmod_1i1o1l *plugin_data = (Ringmod_1i1o1l *)malloc(sizeof(Ringmod_1i1o1l));
	LADSPA_Data offset;

#line 59 "ringmod_1188.xml"
	long i;

	sample_rate = s_rate;

	if (refcount++ == 0) {
	        sin_tbl = malloc(sizeof(LADSPA_Data) * sample_rate);
	        for (i = 0; i < sample_rate; i++) {
	                sin_tbl[i] = sin(i * 2 * M_PI / sample_rate);
	        }
	        
	        tri_tbl = malloc(sizeof(LADSPA_Data) * sample_rate);
	        for (i = 0; i < sample_rate; i++) {
	                tri_tbl[i] = acos(cos(i * 2 * M_PI / sample_rate)) / M_PI * 2 - 1;
	        }

	        squ_tbl = malloc(sizeof(LADSPA_Data) * sample_rate);
	        for (i = 0; i < sample_rate; i++) {
	                squ_tbl[i] = (i < sample_rate/2) ? 1 : -1;
	        }

	        saw_tbl = malloc(sizeof(LADSPA_Data) * sample_rate);
	        for (i = 0; i < sample_rate; i++) {
	                saw_tbl[i] = ((2.0 * i) - (float)sample_rate) / (float)sample_rate;
	        }
	}

	offset = 0;

	plugin_data->offset = offset;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runRingmod_1i1o1l(LADSPA_Handle instance, unsigned long sample_count) {
	Ringmod_1i1o1l *plugin_data = (Ringmod_1i1o1l *)instance;

	/* Modulation depth (0=none, 1=AM, 2=RM) (float value) */
	const LADSPA_Data depthp = *(plugin_data->depthp);

	/* Frequency (Hz) (float value) */
	const LADSPA_Data freq = *(plugin_data->freq);

	/* Sine level (float value) */
	const LADSPA_Data sin = *(plugin_data->sin);

	/* Triangle level (float value) */
	const LADSPA_Data tri = *(plugin_data->tri);

	/* Sawtooth level (float value) */
	const LADSPA_Data saw = *(plugin_data->saw);

	/* Square level (float value) */
	const LADSPA_Data squ = *(plugin_data->squ);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data offset = plugin_data->offset;

#line 24 "ringmod_1188.xml"
	LADSPA_Data scale = fabs(sin) + fabs(tri) +
	 fabs(saw) + fabs(squ);
	int o;
	unsigned long pos;

	// Rescale to more useful value
	const float depth = depthp * 0.5f;

	if (scale == 0.0) {
	        scale = 1.0;
	}

	for (pos = 0; pos < sample_count; pos++) {
	        o = f_round(offset);
	        buffer_write(output[pos], input[pos] *
	         (depth * (((sin / scale) * sin_tbl[o]) +
	           ((tri / scale) * tri_tbl[o]) +
	           ((saw / scale) * saw_tbl[o]) +
	           ((squ / scale) * squ_tbl[o])) +
	           (1.0f - depth)));
	        offset += freq;
	        if (offset > sample_rate) {
	                offset -= sample_rate;
	        }
	}

	plugin_data->offset = offset;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainRingmod_1i1o1l(LADSPA_Handle instance, LADSPA_Data gain) {
	((Ringmod_1i1o1l *)instance)->run_adding_gain = gain;
}

static void runAddingRingmod_1i1o1l(LADSPA_Handle instance, unsigned long sample_count) {
	Ringmod_1i1o1l *plugin_data = (Ringmod_1i1o1l *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Modulation depth (0=none, 1=AM, 2=RM) (float value) */
	const LADSPA_Data depthp = *(plugin_data->depthp);

	/* Frequency (Hz) (float value) */
	const LADSPA_Data freq = *(plugin_data->freq);

	/* Sine level (float value) */
	const LADSPA_Data sin = *(plugin_data->sin);

	/* Triangle level (float value) */
	const LADSPA_Data tri = *(plugin_data->tri);

	/* Sawtooth level (float value) */
	const LADSPA_Data saw = *(plugin_data->saw);

	/* Square level (float value) */
	const LADSPA_Data squ = *(plugin_data->squ);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data offset = plugin_data->offset;

#line 24 "ringmod_1188.xml"
	LADSPA_Data scale = fabs(sin) + fabs(tri) +
	 fabs(saw) + fabs(squ);
	int o;
	unsigned long pos;

	// Rescale to more useful value
	const float depth = depthp * 0.5f;

	if (scale == 0.0) {
	        scale = 1.0;
	}

	for (pos = 0; pos < sample_count; pos++) {
	        o = f_round(offset);
	        buffer_write(output[pos], input[pos] *
	         (depth * (((sin / scale) * sin_tbl[o]) +
	           ((tri / scale) * tri_tbl[o]) +
	           ((saw / scale) * saw_tbl[o]) +
	           ((squ / scale) * squ_tbl[o])) +
	           (1.0f - depth)));
	        offset += freq;
	        if (offset > sample_rate) {
	                offset -= sample_rate;
	        }
	}

	plugin_data->offset = offset;
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


	ringmod_2i1oDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (ringmod_2i1oDescriptor) {
		ringmod_2i1oDescriptor->UniqueID = 1188;
		ringmod_2i1oDescriptor->Label = "ringmod_2i1o";
		ringmod_2i1oDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		ringmod_2i1oDescriptor->Name =
		 D_("Ringmod with two inputs");
		ringmod_2i1oDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		ringmod_2i1oDescriptor->Copyright =
		 "GPL";
		ringmod_2i1oDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		ringmod_2i1oDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		ringmod_2i1oDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		ringmod_2i1oDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Modulation depth (0=none, 1=AM, 2=RM) */
		port_descriptors[RINGMOD_2I1O_DEPTH] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[RINGMOD_2I1O_DEPTH] =
		 D_("Modulation depth (0=none, 1=AM, 2=RM)");
		port_range_hints[RINGMOD_2I1O_DEPTH].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[RINGMOD_2I1O_DEPTH].LowerBound = 0;
		port_range_hints[RINGMOD_2I1O_DEPTH].UpperBound = 2;

		/* Parameters for Input */
		port_descriptors[RINGMOD_2I1O_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[RINGMOD_2I1O_INPUT] =
		 D_("Input");
		port_range_hints[RINGMOD_2I1O_INPUT].HintDescriptor = 0;

		/* Parameters for Modulator */
		port_descriptors[RINGMOD_2I1O_MODULATOR] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[RINGMOD_2I1O_MODULATOR] =
		 D_("Modulator");
		port_range_hints[RINGMOD_2I1O_MODULATOR].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[RINGMOD_2I1O_MODULATOR].LowerBound = -1;
		port_range_hints[RINGMOD_2I1O_MODULATOR].UpperBound = +1;

		/* Parameters for Output */
		port_descriptors[RINGMOD_2I1O_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[RINGMOD_2I1O_OUTPUT] =
		 D_("Output");
		port_range_hints[RINGMOD_2I1O_OUTPUT].HintDescriptor = 0;

		ringmod_2i1oDescriptor->activate = NULL;
		ringmod_2i1oDescriptor->cleanup = cleanupRingmod_2i1o;
		ringmod_2i1oDescriptor->connect_port = connectPortRingmod_2i1o;
		ringmod_2i1oDescriptor->deactivate = NULL;
		ringmod_2i1oDescriptor->instantiate = instantiateRingmod_2i1o;
		ringmod_2i1oDescriptor->run = runRingmod_2i1o;
		ringmod_2i1oDescriptor->run_adding = runAddingRingmod_2i1o;
		ringmod_2i1oDescriptor->set_run_adding_gain = setRunAddingGainRingmod_2i1o;
	}

	ringmod_1i1o1lDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (ringmod_1i1o1lDescriptor) {
		ringmod_1i1o1lDescriptor->UniqueID = 1189;
		ringmod_1i1o1lDescriptor->Label = "ringmod_1i1o1l";
		ringmod_1i1o1lDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		ringmod_1i1o1lDescriptor->Name =
		 D_("Ringmod with LFO");
		ringmod_1i1o1lDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		ringmod_1i1o1lDescriptor->Copyright =
		 "GPL";
		ringmod_1i1o1lDescriptor->PortCount = 8;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(8,
		 sizeof(LADSPA_PortDescriptor));
		ringmod_1i1o1lDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(8,
		 sizeof(LADSPA_PortRangeHint));
		ringmod_1i1o1lDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(8, sizeof(char*));
		ringmod_1i1o1lDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Modulation depth (0=none, 1=AM, 2=RM) */
		port_descriptors[RINGMOD_1I1O1L_DEPTHP] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[RINGMOD_1I1O1L_DEPTHP] =
		 D_("Modulation depth (0=none, 1=AM, 2=RM)");
		port_range_hints[RINGMOD_1I1O1L_DEPTHP].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[RINGMOD_1I1O1L_DEPTHP].LowerBound = 0;
		port_range_hints[RINGMOD_1I1O1L_DEPTHP].UpperBound = 2;

		/* Parameters for Frequency (Hz) */
		port_descriptors[RINGMOD_1I1O1L_FREQ] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[RINGMOD_1I1O1L_FREQ] =
		 D_("Frequency (Hz)");
		port_range_hints[RINGMOD_1I1O1L_FREQ].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_440;
		port_range_hints[RINGMOD_1I1O1L_FREQ].LowerBound = 1;
		port_range_hints[RINGMOD_1I1O1L_FREQ].UpperBound = 1000;

		/* Parameters for Sine level */
		port_descriptors[RINGMOD_1I1O1L_SIN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[RINGMOD_1I1O1L_SIN] =
		 D_("Sine level");
		port_range_hints[RINGMOD_1I1O1L_SIN].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[RINGMOD_1I1O1L_SIN].LowerBound = -1;
		port_range_hints[RINGMOD_1I1O1L_SIN].UpperBound = +1;

		/* Parameters for Triangle level */
		port_descriptors[RINGMOD_1I1O1L_TRI] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[RINGMOD_1I1O1L_TRI] =
		 D_("Triangle level");
		port_range_hints[RINGMOD_1I1O1L_TRI].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[RINGMOD_1I1O1L_TRI].LowerBound = -1;
		port_range_hints[RINGMOD_1I1O1L_TRI].UpperBound = +1;

		/* Parameters for Sawtooth level */
		port_descriptors[RINGMOD_1I1O1L_SAW] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[RINGMOD_1I1O1L_SAW] =
		 D_("Sawtooth level");
		port_range_hints[RINGMOD_1I1O1L_SAW].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[RINGMOD_1I1O1L_SAW].LowerBound = -1;
		port_range_hints[RINGMOD_1I1O1L_SAW].UpperBound = +1;

		/* Parameters for Square level */
		port_descriptors[RINGMOD_1I1O1L_SQU] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[RINGMOD_1I1O1L_SQU] =
		 D_("Square level");
		port_range_hints[RINGMOD_1I1O1L_SQU].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[RINGMOD_1I1O1L_SQU].LowerBound = -1;
		port_range_hints[RINGMOD_1I1O1L_SQU].UpperBound = +1;

		/* Parameters for Input */
		port_descriptors[RINGMOD_1I1O1L_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[RINGMOD_1I1O1L_INPUT] =
		 D_("Input");
		port_range_hints[RINGMOD_1I1O1L_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[RINGMOD_1I1O1L_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[RINGMOD_1I1O1L_OUTPUT] =
		 D_("Output");
		port_range_hints[RINGMOD_1I1O1L_OUTPUT].HintDescriptor = 0;

		ringmod_1i1o1lDescriptor->activate = activateRingmod_1i1o1l;
		ringmod_1i1o1lDescriptor->cleanup = cleanupRingmod_1i1o1l;
		ringmod_1i1o1lDescriptor->connect_port = connectPortRingmod_1i1o1l;
		ringmod_1i1o1lDescriptor->deactivate = NULL;
		ringmod_1i1o1lDescriptor->instantiate = instantiateRingmod_1i1o1l;
		ringmod_1i1o1lDescriptor->run = runRingmod_1i1o1l;
		ringmod_1i1o1lDescriptor->run_adding = runAddingRingmod_1i1o1l;
		ringmod_1i1o1lDescriptor->set_run_adding_gain = setRunAddingGainRingmod_1i1o1l;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (ringmod_2i1oDescriptor) {
		free((LADSPA_PortDescriptor *)ringmod_2i1oDescriptor->PortDescriptors);
		free((char **)ringmod_2i1oDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)ringmod_2i1oDescriptor->PortRangeHints);
		free(ringmod_2i1oDescriptor);
	}
	if (ringmod_1i1o1lDescriptor) {
		free((LADSPA_PortDescriptor *)ringmod_1i1o1lDescriptor->PortDescriptors);
		free((char **)ringmod_1i1o1lDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)ringmod_1i1o1lDescriptor->PortRangeHints);
		free(ringmod_1i1o1lDescriptor);
	}

}
