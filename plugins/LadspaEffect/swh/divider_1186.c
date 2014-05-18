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


#define DIVIDER_DENOMINATOR            0
#define DIVIDER_INPUT                  1
#define DIVIDER_OUTPUT                 2

static LADSPA_Descriptor *dividerDescriptor = NULL;

typedef struct {
	LADSPA_Data *denominator;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data  amp;
	float        count;
	LADSPA_Data  lamp;
	LADSPA_Data  last;
	LADSPA_Data  out;
	int          zeroxs;
	LADSPA_Data run_adding_gain;
} Divider;

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
		return dividerDescriptor;
	default:
		return NULL;
	}
}

static void cleanupDivider(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortDivider(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Divider *plugin;

	plugin = (Divider *)instance;
	switch (port) {
	case DIVIDER_DENOMINATOR:
		plugin->denominator = data;
		break;
	case DIVIDER_INPUT:
		plugin->input = data;
		break;
	case DIVIDER_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateDivider(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Divider *plugin_data = (Divider *)malloc(sizeof(Divider));
	LADSPA_Data amp;
	float count;
	LADSPA_Data lamp;
	LADSPA_Data last;
	LADSPA_Data out;
	int zeroxs;

#line 16 "divider_1186.xml"
	out = 1.0f;
	amp = 0.0f;
	count = 0.0f;
	lamp = 0.0f;
	last = 0.0f;
	zeroxs = 0;

	plugin_data->amp = amp;
	plugin_data->count = count;
	plugin_data->lamp = lamp;
	plugin_data->last = last;
	plugin_data->out = out;
	plugin_data->zeroxs = zeroxs;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runDivider(LADSPA_Handle instance, unsigned long sample_count) {
	Divider *plugin_data = (Divider *)instance;

	/* Denominator (float value) */
	const LADSPA_Data denominator = *(plugin_data->denominator);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data amp = plugin_data->amp;
	float count = plugin_data->count;
	LADSPA_Data lamp = plugin_data->lamp;
	LADSPA_Data last = plugin_data->last;
	LADSPA_Data out = plugin_data->out;
	int zeroxs = plugin_data->zeroxs;

#line 25 "divider_1186.xml"
	/* Integer version of denominator */
	int den = (int)denominator;
	
	unsigned long pos;
	
	for (pos = 0; pos < sample_count; pos++) {
	        count += 1.0f;
	        if ((input[pos] > 0.0f && last <= 0.0f) ||
	         (input[pos] < 0.0f && last >= 0.0)) {
	                zeroxs++;
	                if (den == 1) {
	                        out = out > 0.0f ? -1.0f : 1.0f;
	                        lamp = amp / count;
	                        zeroxs = 0;
	                        count = 0;
	                        amp = 0;
	                }
	        }
	        amp += fabs(input[pos]);
	        if (den > 1 && (zeroxs % den) == den-1) {
	                out = out > 0.0f ? -1.0f : 1.0f;
	                lamp = amp / count;
	                zeroxs = 0;
	                count = 0;
	                amp = 0;
	        }
	        last = input[pos];
	        buffer_write(output[pos], out * lamp);
	}
	
	plugin_data->last = last;
	plugin_data->amp = amp;
	plugin_data->lamp = lamp;
	plugin_data->zeroxs = zeroxs;
	plugin_data->count = count;
	plugin_data->out = out;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainDivider(LADSPA_Handle instance, LADSPA_Data gain) {
	((Divider *)instance)->run_adding_gain = gain;
}

static void runAddingDivider(LADSPA_Handle instance, unsigned long sample_count) {
	Divider *plugin_data = (Divider *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Denominator (float value) */
	const LADSPA_Data denominator = *(plugin_data->denominator);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data amp = plugin_data->amp;
	float count = plugin_data->count;
	LADSPA_Data lamp = plugin_data->lamp;
	LADSPA_Data last = plugin_data->last;
	LADSPA_Data out = plugin_data->out;
	int zeroxs = plugin_data->zeroxs;

#line 25 "divider_1186.xml"
	/* Integer version of denominator */
	int den = (int)denominator;
	
	unsigned long pos;
	
	for (pos = 0; pos < sample_count; pos++) {
	        count += 1.0f;
	        if ((input[pos] > 0.0f && last <= 0.0f) ||
	         (input[pos] < 0.0f && last >= 0.0)) {
	                zeroxs++;
	                if (den == 1) {
	                        out = out > 0.0f ? -1.0f : 1.0f;
	                        lamp = amp / count;
	                        zeroxs = 0;
	                        count = 0;
	                        amp = 0;
	                }
	        }
	        amp += fabs(input[pos]);
	        if (den > 1 && (zeroxs % den) == den-1) {
	                out = out > 0.0f ? -1.0f : 1.0f;
	                lamp = amp / count;
	                zeroxs = 0;
	                count = 0;
	                amp = 0;
	        }
	        last = input[pos];
	        buffer_write(output[pos], out * lamp);
	}
	
	plugin_data->last = last;
	plugin_data->amp = amp;
	plugin_data->lamp = lamp;
	plugin_data->zeroxs = zeroxs;
	plugin_data->count = count;
	plugin_data->out = out;
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


	dividerDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (dividerDescriptor) {
		dividerDescriptor->UniqueID = 1186;
		dividerDescriptor->Label = "divider";
		dividerDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		dividerDescriptor->Name =
		 D_("Audio Divider (Suboctave Generator)");
		dividerDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		dividerDescriptor->Copyright =
		 "GPL";
		dividerDescriptor->PortCount = 3;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(3,
		 sizeof(LADSPA_PortDescriptor));
		dividerDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(3,
		 sizeof(LADSPA_PortRangeHint));
		dividerDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(3, sizeof(char*));
		dividerDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Denominator */
		port_descriptors[DIVIDER_DENOMINATOR] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DIVIDER_DENOMINATOR] =
		 D_("Denominator");
		port_range_hints[DIVIDER_DENOMINATOR].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_1;
		port_range_hints[DIVIDER_DENOMINATOR].LowerBound = 1;
		port_range_hints[DIVIDER_DENOMINATOR].UpperBound = 8;

		/* Parameters for Input */
		port_descriptors[DIVIDER_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[DIVIDER_INPUT] =
		 D_("Input");
		port_range_hints[DIVIDER_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[DIVIDER_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[DIVIDER_OUTPUT] =
		 D_("Output");
		port_range_hints[DIVIDER_OUTPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[DIVIDER_OUTPUT].LowerBound = -1;
		port_range_hints[DIVIDER_OUTPUT].UpperBound = +1;

		dividerDescriptor->activate = NULL;
		dividerDescriptor->cleanup = cleanupDivider;
		dividerDescriptor->connect_port = connectPortDivider;
		dividerDescriptor->deactivate = NULL;
		dividerDescriptor->instantiate = instantiateDivider;
		dividerDescriptor->run = runDivider;
		dividerDescriptor->run_adding = runAddingDivider;
		dividerDescriptor->set_run_adding_gain = setRunAddingGainDivider;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (dividerDescriptor) {
		free((LADSPA_PortDescriptor *)dividerDescriptor->PortDescriptors);
		free((char **)dividerDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)dividerDescriptor->PortRangeHints);
		free(dividerDescriptor);
	}

}
