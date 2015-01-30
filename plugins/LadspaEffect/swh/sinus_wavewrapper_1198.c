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


#define SINUSWAVEWRAPPER_WRAP          0
#define SINUSWAVEWRAPPER_INPUT         1
#define SINUSWAVEWRAPPER_OUTPUT        2

static LADSPA_Descriptor *sinusWavewrapperDescriptor = NULL;

typedef struct {
	LADSPA_Data *wrap;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data run_adding_gain;
} SinusWavewrapper;

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
		return sinusWavewrapperDescriptor;
	default:
		return NULL;
	}
}

static void cleanupSinusWavewrapper(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortSinusWavewrapper(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	SinusWavewrapper *plugin;

	plugin = (SinusWavewrapper *)instance;
	switch (port) {
	case SINUSWAVEWRAPPER_WRAP:
		plugin->wrap = data;
		break;
	case SINUSWAVEWRAPPER_INPUT:
		plugin->input = data;
		break;
	case SINUSWAVEWRAPPER_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateSinusWavewrapper(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	SinusWavewrapper *plugin_data = (SinusWavewrapper *)malloc(sizeof(SinusWavewrapper));
	plugin_data->run_adding_gain = 1.0f;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runSinusWavewrapper(LADSPA_Handle instance, unsigned long sample_count) {
	SinusWavewrapper *plugin_data = (SinusWavewrapper *)instance;

	/* Wrap degree (float value) */
	const LADSPA_Data wrap = *(plugin_data->wrap);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

#line 16 "sinus_wavewrapper_1198.xml"
	float coef = wrap * M_PI;
	unsigned long pos;

	if (coef < 0.05f) {
	        coef = 0.05f;
	}

	for (pos = 0; pos < sample_count; pos++) {
	        buffer_write(output[pos], sin(input[pos] * coef));
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainSinusWavewrapper(LADSPA_Handle instance, LADSPA_Data gain) {
	((SinusWavewrapper *)instance)->run_adding_gain = gain;
}

static void runAddingSinusWavewrapper(LADSPA_Handle instance, unsigned long sample_count) {
	SinusWavewrapper *plugin_data = (SinusWavewrapper *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Wrap degree (float value) */
	const LADSPA_Data wrap = *(plugin_data->wrap);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

#line 16 "sinus_wavewrapper_1198.xml"
	float coef = wrap * M_PI;
	unsigned long pos;

	if (coef < 0.05f) {
	        coef = 0.05f;
	}

	for (pos = 0; pos < sample_count; pos++) {
	        buffer_write(output[pos], sin(input[pos] * coef));
	}
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


	sinusWavewrapperDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (sinusWavewrapperDescriptor) {
		sinusWavewrapperDescriptor->UniqueID = 1198;
		sinusWavewrapperDescriptor->Label = "sinusWavewrapper";
		sinusWavewrapperDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		sinusWavewrapperDescriptor->Name =
		 D_("Sinus wavewrapper");
		sinusWavewrapperDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		sinusWavewrapperDescriptor->Copyright =
		 "GPL";
		sinusWavewrapperDescriptor->PortCount = 3;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(3,
		 sizeof(LADSPA_PortDescriptor));
		sinusWavewrapperDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(3,
		 sizeof(LADSPA_PortRangeHint));
		sinusWavewrapperDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(3, sizeof(char*));
		sinusWavewrapperDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Wrap degree */
		port_descriptors[SINUSWAVEWRAPPER_WRAP] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SINUSWAVEWRAPPER_WRAP] =
		 D_("Wrap degree");
		port_range_hints[SINUSWAVEWRAPPER_WRAP].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[SINUSWAVEWRAPPER_WRAP].LowerBound = 0;
		port_range_hints[SINUSWAVEWRAPPER_WRAP].UpperBound = 10;

		/* Parameters for Input */
		port_descriptors[SINUSWAVEWRAPPER_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[SINUSWAVEWRAPPER_INPUT] =
		 D_("Input");
		port_range_hints[SINUSWAVEWRAPPER_INPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[SINUSWAVEWRAPPER_INPUT].LowerBound = -1;
		port_range_hints[SINUSWAVEWRAPPER_INPUT].UpperBound = +1;

		/* Parameters for Output */
		port_descriptors[SINUSWAVEWRAPPER_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[SINUSWAVEWRAPPER_OUTPUT] =
		 D_("Output");
		port_range_hints[SINUSWAVEWRAPPER_OUTPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[SINUSWAVEWRAPPER_OUTPUT].LowerBound = -1;
		port_range_hints[SINUSWAVEWRAPPER_OUTPUT].UpperBound = +1;

		sinusWavewrapperDescriptor->activate = NULL;
		sinusWavewrapperDescriptor->cleanup = cleanupSinusWavewrapper;
		sinusWavewrapperDescriptor->connect_port = connectPortSinusWavewrapper;
		sinusWavewrapperDescriptor->deactivate = NULL;
		sinusWavewrapperDescriptor->instantiate = instantiateSinusWavewrapper;
		sinusWavewrapperDescriptor->run = runSinusWavewrapper;
		sinusWavewrapperDescriptor->run_adding = runAddingSinusWavewrapper;
		sinusWavewrapperDescriptor->set_run_adding_gain = setRunAddingGainSinusWavewrapper;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (sinusWavewrapperDescriptor) {
		free((LADSPA_PortDescriptor *)sinusWavewrapperDescriptor->PortDescriptors);
		free((char **)sinusWavewrapperDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)sinusWavewrapperDescriptor->PortRangeHints);
		free(sinusWavewrapperDescriptor);
	}

}
