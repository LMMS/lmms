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

#line 10 "single_para_1203.xml"

#include "util/biquad.h"

#define SINGLEPARA_GAIN                0
#define SINGLEPARA_FC                  1
#define SINGLEPARA_BW                  2
#define SINGLEPARA_INPUT               3
#define SINGLEPARA_OUTPUT              4

static LADSPA_Descriptor *singleParaDescriptor = NULL;

typedef struct {
	LADSPA_Data *gain;
	LADSPA_Data *fc;
	LADSPA_Data *bw;
	LADSPA_Data *input;
	LADSPA_Data *output;
	biquad *     filter;
	float        fs;
	LADSPA_Data run_adding_gain;
} SinglePara;

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
		return singleParaDescriptor;
	default:
		return NULL;
	}
}

static void activateSinglePara(LADSPA_Handle instance) {
	SinglePara *plugin_data = (SinglePara *)instance;
	biquad *filter = plugin_data->filter;
	float fs = plugin_data->fs;
#line 26 "single_para_1203.xml"
	biquad_init(filter);
	plugin_data->filter = filter;
	plugin_data->fs = fs;

}

static void cleanupSinglePara(LADSPA_Handle instance) {
#line 30 "single_para_1203.xml"
	SinglePara *plugin_data = (SinglePara *)instance;
	free(plugin_data->filter);
	free(instance);
}

static void connectPortSinglePara(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	SinglePara *plugin;

	plugin = (SinglePara *)instance;
	switch (port) {
	case SINGLEPARA_GAIN:
		plugin->gain = data;
		break;
	case SINGLEPARA_FC:
		plugin->fc = data;
		break;
	case SINGLEPARA_BW:
		plugin->bw = data;
		break;
	case SINGLEPARA_INPUT:
		plugin->input = data;
		break;
	case SINGLEPARA_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateSinglePara(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	SinglePara *plugin_data = (SinglePara *)malloc(sizeof(SinglePara));
	biquad *filter = NULL;
	float fs;

#line 20 "single_para_1203.xml"
	fs = (float)s_rate;
	filter = malloc(sizeof(biquad));
	biquad_init(filter);

	plugin_data->filter = filter;
	plugin_data->fs = fs;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runSinglePara(LADSPA_Handle instance, unsigned long sample_count) {
	SinglePara *plugin_data = (SinglePara *)instance;

	/* Gain (dB) (float value) */
	const LADSPA_Data gain = *(plugin_data->gain);

	/* Frequency (Hz) (float value) */
	const LADSPA_Data fc = *(plugin_data->fc);

	/* Bandwidth (octaves) (float value) */
	const LADSPA_Data bw = *(plugin_data->bw);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	biquad * filter = plugin_data->filter;
	float fs = plugin_data->fs;

#line 34 "single_para_1203.xml"
	unsigned long pos;

	eq_set_params(filter, fc, gain, bw, fs);

	for (pos = 0; pos < sample_count; pos++) {
	  buffer_write(output[pos], biquad_run(filter, input[pos]));
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainSinglePara(LADSPA_Handle instance, LADSPA_Data gain) {
	((SinglePara *)instance)->run_adding_gain = gain;
}

static void runAddingSinglePara(LADSPA_Handle instance, unsigned long sample_count) {
	SinglePara *plugin_data = (SinglePara *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Gain (dB) (float value) */
	const LADSPA_Data gain = *(plugin_data->gain);

	/* Frequency (Hz) (float value) */
	const LADSPA_Data fc = *(plugin_data->fc);

	/* Bandwidth (octaves) (float value) */
	const LADSPA_Data bw = *(plugin_data->bw);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	biquad * filter = plugin_data->filter;
	float fs = plugin_data->fs;

#line 34 "single_para_1203.xml"
	unsigned long pos;

	eq_set_params(filter, fc, gain, bw, fs);

	for (pos = 0; pos < sample_count; pos++) {
	  buffer_write(output[pos], biquad_run(filter, input[pos]));
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


	singleParaDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (singleParaDescriptor) {
		singleParaDescriptor->UniqueID = 1203;
		singleParaDescriptor->Label = "singlePara";
		singleParaDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		singleParaDescriptor->Name =
		 D_("Single band parametric");
		singleParaDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		singleParaDescriptor->Copyright =
		 "GPL";
		singleParaDescriptor->PortCount = 5;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(5,
		 sizeof(LADSPA_PortDescriptor));
		singleParaDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(5,
		 sizeof(LADSPA_PortRangeHint));
		singleParaDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(5, sizeof(char*));
		singleParaDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Gain (dB) */
		port_descriptors[SINGLEPARA_GAIN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SINGLEPARA_GAIN] =
		 D_("Gain (dB)");
		port_range_hints[SINGLEPARA_GAIN].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[SINGLEPARA_GAIN].LowerBound = -70;
		port_range_hints[SINGLEPARA_GAIN].UpperBound = +30;

		/* Parameters for Frequency (Hz) */
		port_descriptors[SINGLEPARA_FC] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SINGLEPARA_FC] =
		 D_("Frequency (Hz)");
		port_range_hints[SINGLEPARA_FC].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_DEFAULT_440;
		port_range_hints[SINGLEPARA_FC].LowerBound = 0;
		port_range_hints[SINGLEPARA_FC].UpperBound = 0.4;

		/* Parameters for Bandwidth (octaves) */
		port_descriptors[SINGLEPARA_BW] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SINGLEPARA_BW] =
		 D_("Bandwidth (octaves)");
		port_range_hints[SINGLEPARA_BW].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[SINGLEPARA_BW].LowerBound = 0;
		port_range_hints[SINGLEPARA_BW].UpperBound = 4;

		/* Parameters for Input */
		port_descriptors[SINGLEPARA_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[SINGLEPARA_INPUT] =
		 D_("Input");
		port_range_hints[SINGLEPARA_INPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[SINGLEPARA_INPUT].LowerBound = -1.0;
		port_range_hints[SINGLEPARA_INPUT].UpperBound = +1.0;

		/* Parameters for Output */
		port_descriptors[SINGLEPARA_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[SINGLEPARA_OUTPUT] =
		 D_("Output");
		port_range_hints[SINGLEPARA_OUTPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[SINGLEPARA_OUTPUT].LowerBound = -1.0;
		port_range_hints[SINGLEPARA_OUTPUT].UpperBound = +1.0;

		singleParaDescriptor->activate = activateSinglePara;
		singleParaDescriptor->cleanup = cleanupSinglePara;
		singleParaDescriptor->connect_port = connectPortSinglePara;
		singleParaDescriptor->deactivate = NULL;
		singleParaDescriptor->instantiate = instantiateSinglePara;
		singleParaDescriptor->run = runSinglePara;
		singleParaDescriptor->run_adding = runAddingSinglePara;
		singleParaDescriptor->set_run_adding_gain = setRunAddingGainSinglePara;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (singleParaDescriptor) {
		free((LADSPA_PortDescriptor *)singleParaDescriptor->PortDescriptors);
		free((char **)singleParaDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)singleParaDescriptor->PortRangeHints);
		free(singleParaDescriptor);
	}

}
