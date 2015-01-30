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

#line 10 "triple_para_1204.xml"

#include "util/biquad.h"

#define TRIPLEPARA_GAIN_L              0
#define TRIPLEPARA_FC_L                1
#define TRIPLEPARA_BW_L                2
#define TRIPLEPARA_GAIN_1              3
#define TRIPLEPARA_FC_1                4
#define TRIPLEPARA_BW_1                5
#define TRIPLEPARA_GAIN_2              6
#define TRIPLEPARA_FC_2                7
#define TRIPLEPARA_BW_2                8
#define TRIPLEPARA_GAIN_3              9
#define TRIPLEPARA_FC_3                10
#define TRIPLEPARA_BW_3                11
#define TRIPLEPARA_GAIN_H              12
#define TRIPLEPARA_FC_H                13
#define TRIPLEPARA_BW_H                14
#define TRIPLEPARA_INPUT               15
#define TRIPLEPARA_OUTPUT              16

static LADSPA_Descriptor *tripleParaDescriptor = NULL;

typedef struct {
	LADSPA_Data *gain_L;
	LADSPA_Data *fc_L;
	LADSPA_Data *bw_L;
	LADSPA_Data *gain_1;
	LADSPA_Data *fc_1;
	LADSPA_Data *bw_1;
	LADSPA_Data *gain_2;
	LADSPA_Data *fc_2;
	LADSPA_Data *bw_2;
	LADSPA_Data *gain_3;
	LADSPA_Data *fc_3;
	LADSPA_Data *bw_3;
	LADSPA_Data *gain_H;
	LADSPA_Data *fc_H;
	LADSPA_Data *bw_H;
	LADSPA_Data *input;
	LADSPA_Data *output;
	biquad *     filters;
	float        fs;
	LADSPA_Data run_adding_gain;
} TriplePara;

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
		return tripleParaDescriptor;
	default:
		return NULL;
	}
}

static void activateTriplePara(LADSPA_Handle instance) {
	TriplePara *plugin_data = (TriplePara *)instance;
	biquad *filters = plugin_data->filters;
	float fs = plugin_data->fs;
#line 32 "triple_para_1204.xml"
	biquad_init(&filters[0]);
	biquad_init(&filters[1]);
	biquad_init(&filters[2]);
	biquad_init(&filters[3]);
	biquad_init(&filters[4]);
	plugin_data->filters = filters;
	plugin_data->fs = fs;

}

static void cleanupTriplePara(LADSPA_Handle instance) {
#line 68 "triple_para_1204.xml"
	TriplePara *plugin_data = (TriplePara *)instance;
	free(plugin_data->filters);
	free(instance);
}

static void connectPortTriplePara(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	TriplePara *plugin;

	plugin = (TriplePara *)instance;
	switch (port) {
	case TRIPLEPARA_GAIN_L:
		plugin->gain_L = data;
		break;
	case TRIPLEPARA_FC_L:
		plugin->fc_L = data;
		break;
	case TRIPLEPARA_BW_L:
		plugin->bw_L = data;
		break;
	case TRIPLEPARA_GAIN_1:
		plugin->gain_1 = data;
		break;
	case TRIPLEPARA_FC_1:
		plugin->fc_1 = data;
		break;
	case TRIPLEPARA_BW_1:
		plugin->bw_1 = data;
		break;
	case TRIPLEPARA_GAIN_2:
		plugin->gain_2 = data;
		break;
	case TRIPLEPARA_FC_2:
		plugin->fc_2 = data;
		break;
	case TRIPLEPARA_BW_2:
		plugin->bw_2 = data;
		break;
	case TRIPLEPARA_GAIN_3:
		plugin->gain_3 = data;
		break;
	case TRIPLEPARA_FC_3:
		plugin->fc_3 = data;
		break;
	case TRIPLEPARA_BW_3:
		plugin->bw_3 = data;
		break;
	case TRIPLEPARA_GAIN_H:
		plugin->gain_H = data;
		break;
	case TRIPLEPARA_FC_H:
		plugin->fc_H = data;
		break;
	case TRIPLEPARA_BW_H:
		plugin->bw_H = data;
		break;
	case TRIPLEPARA_INPUT:
		plugin->input = data;
		break;
	case TRIPLEPARA_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateTriplePara(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	TriplePara *plugin_data = (TriplePara *)malloc(sizeof(TriplePara));
	biquad *filters = NULL;
	float fs;

#line 21 "triple_para_1204.xml"
	fs = s_rate;
	
	filters = calloc(5, sizeof(biquad));
	biquad_init(&filters[0]);
	biquad_init(&filters[1]);
	biquad_init(&filters[2]);
	biquad_init(&filters[3]);
	biquad_init(&filters[4]);

	plugin_data->filters = filters;
	plugin_data->fs = fs;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runTriplePara(LADSPA_Handle instance, unsigned long sample_count) {
	TriplePara *plugin_data = (TriplePara *)instance;

	/* Low-shelving gain (dB) (float value) */
	const LADSPA_Data gain_L = *(plugin_data->gain_L);

	/* Low-shelving frequency (Hz) (float value) */
	const LADSPA_Data fc_L = *(plugin_data->fc_L);

	/* Low-shelving slope (float value) */
	const LADSPA_Data bw_L = *(plugin_data->bw_L);

	/* Band 1 gain (dB) (float value) */
	const LADSPA_Data gain_1 = *(plugin_data->gain_1);

	/* Band 1 frequency (Hz) (float value) */
	const LADSPA_Data fc_1 = *(plugin_data->fc_1);

	/* Band 1 bandwidth (octaves) (float value) */
	const LADSPA_Data bw_1 = *(plugin_data->bw_1);

	/* Band 2 gain (dB) (float value) */
	const LADSPA_Data gain_2 = *(plugin_data->gain_2);

	/* Band 2 frequency (Hz) (float value) */
	const LADSPA_Data fc_2 = *(plugin_data->fc_2);

	/* Band 2 bandwidth (octaves) (float value) */
	const LADSPA_Data bw_2 = *(plugin_data->bw_2);

	/* Band 3 gain (dB) (float value) */
	const LADSPA_Data gain_3 = *(plugin_data->gain_3);

	/* Band 3 frequency (Hz) (float value) */
	const LADSPA_Data fc_3 = *(plugin_data->fc_3);

	/* Band 3 bandwidth (octaves) (float value) */
	const LADSPA_Data bw_3 = *(plugin_data->bw_3);

	/* High-shelving gain (dB) (float value) */
	const LADSPA_Data gain_H = *(plugin_data->gain_H);

	/* High-shelving frequency (Hz) (float value) */
	const LADSPA_Data fc_H = *(plugin_data->fc_H);

	/* High-shelving slope (float value) */
	const LADSPA_Data bw_H = *(plugin_data->bw_H);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	biquad * filters = plugin_data->filters;
	float fs = plugin_data->fs;

#line 40 "triple_para_1204.xml"
	unsigned long pos;
	float in;
	
	ls_set_params(&filters[0], fc_L, gain_L, bw_L, fs);
	eq_set_params(&filters[1], fc_1, gain_1, bw_1, fs);
	eq_set_params(&filters[2], fc_2, gain_2, bw_2, fs);
	eq_set_params(&filters[3], fc_3, gain_3, bw_3, fs);
	hs_set_params(&filters[4], fc_H, gain_H, bw_H, fs);
	
	for (pos = 0; pos < sample_count; pos++) {
	        in = biquad_run(&filters[0], input[pos]);
	        in = biquad_run(&filters[1], in);
	        in = biquad_run(&filters[2], in);
	        in = biquad_run(&filters[3], in);
	        in = biquad_run(&filters[4], in);
	        buffer_write(output[pos], in);
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainTriplePara(LADSPA_Handle instance, LADSPA_Data gain) {
	((TriplePara *)instance)->run_adding_gain = gain;
}

static void runAddingTriplePara(LADSPA_Handle instance, unsigned long sample_count) {
	TriplePara *plugin_data = (TriplePara *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Low-shelving gain (dB) (float value) */
	const LADSPA_Data gain_L = *(plugin_data->gain_L);

	/* Low-shelving frequency (Hz) (float value) */
	const LADSPA_Data fc_L = *(plugin_data->fc_L);

	/* Low-shelving slope (float value) */
	const LADSPA_Data bw_L = *(plugin_data->bw_L);

	/* Band 1 gain (dB) (float value) */
	const LADSPA_Data gain_1 = *(plugin_data->gain_1);

	/* Band 1 frequency (Hz) (float value) */
	const LADSPA_Data fc_1 = *(plugin_data->fc_1);

	/* Band 1 bandwidth (octaves) (float value) */
	const LADSPA_Data bw_1 = *(plugin_data->bw_1);

	/* Band 2 gain (dB) (float value) */
	const LADSPA_Data gain_2 = *(plugin_data->gain_2);

	/* Band 2 frequency (Hz) (float value) */
	const LADSPA_Data fc_2 = *(plugin_data->fc_2);

	/* Band 2 bandwidth (octaves) (float value) */
	const LADSPA_Data bw_2 = *(plugin_data->bw_2);

	/* Band 3 gain (dB) (float value) */
	const LADSPA_Data gain_3 = *(plugin_data->gain_3);

	/* Band 3 frequency (Hz) (float value) */
	const LADSPA_Data fc_3 = *(plugin_data->fc_3);

	/* Band 3 bandwidth (octaves) (float value) */
	const LADSPA_Data bw_3 = *(plugin_data->bw_3);

	/* High-shelving gain (dB) (float value) */
	const LADSPA_Data gain_H = *(plugin_data->gain_H);

	/* High-shelving frequency (Hz) (float value) */
	const LADSPA_Data fc_H = *(plugin_data->fc_H);

	/* High-shelving slope (float value) */
	const LADSPA_Data bw_H = *(plugin_data->bw_H);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	biquad * filters = plugin_data->filters;
	float fs = plugin_data->fs;

#line 40 "triple_para_1204.xml"
	unsigned long pos;
	float in;
	
	ls_set_params(&filters[0], fc_L, gain_L, bw_L, fs);
	eq_set_params(&filters[1], fc_1, gain_1, bw_1, fs);
	eq_set_params(&filters[2], fc_2, gain_2, bw_2, fs);
	eq_set_params(&filters[3], fc_3, gain_3, bw_3, fs);
	hs_set_params(&filters[4], fc_H, gain_H, bw_H, fs);
	
	for (pos = 0; pos < sample_count; pos++) {
	        in = biquad_run(&filters[0], input[pos]);
	        in = biquad_run(&filters[1], in);
	        in = biquad_run(&filters[2], in);
	        in = biquad_run(&filters[3], in);
	        in = biquad_run(&filters[4], in);
	        buffer_write(output[pos], in);
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


	tripleParaDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (tripleParaDescriptor) {
		tripleParaDescriptor->UniqueID = 1204;
		tripleParaDescriptor->Label = "triplePara";
		tripleParaDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		tripleParaDescriptor->Name =
		 D_("Triple band parametric with shelves");
		tripleParaDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		tripleParaDescriptor->Copyright =
		 "GPL";
		tripleParaDescriptor->PortCount = 17;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(17,
		 sizeof(LADSPA_PortDescriptor));
		tripleParaDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(17,
		 sizeof(LADSPA_PortRangeHint));
		tripleParaDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(17, sizeof(char*));
		tripleParaDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Low-shelving gain (dB) */
		port_descriptors[TRIPLEPARA_GAIN_L] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TRIPLEPARA_GAIN_L] =
		 D_("Low-shelving gain (dB)");
		port_range_hints[TRIPLEPARA_GAIN_L].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[TRIPLEPARA_GAIN_L].LowerBound = -70;
		port_range_hints[TRIPLEPARA_GAIN_L].UpperBound = +30;

		/* Parameters for Low-shelving frequency (Hz) */
		port_descriptors[TRIPLEPARA_FC_L] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TRIPLEPARA_FC_L] =
		 D_("Low-shelving frequency (Hz)");
		port_range_hints[TRIPLEPARA_FC_L].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_LOGARITHMIC | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_DEFAULT_MINIMUM;
		port_range_hints[TRIPLEPARA_FC_L].LowerBound = 0.0001;
		port_range_hints[TRIPLEPARA_FC_L].UpperBound = 0.49;

		/* Parameters for Low-shelving slope */
		port_descriptors[TRIPLEPARA_BW_L] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TRIPLEPARA_BW_L] =
		 D_("Low-shelving slope");
		port_range_hints[TRIPLEPARA_BW_L].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[TRIPLEPARA_BW_L].LowerBound = 0;
		port_range_hints[TRIPLEPARA_BW_L].UpperBound = 1;

		/* Parameters for Band 1 gain (dB) */
		port_descriptors[TRIPLEPARA_GAIN_1] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TRIPLEPARA_GAIN_1] =
		 D_("Band 1 gain (dB)");
		port_range_hints[TRIPLEPARA_GAIN_1].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[TRIPLEPARA_GAIN_1].LowerBound = -70;
		port_range_hints[TRIPLEPARA_GAIN_1].UpperBound = +30;

		/* Parameters for Band 1 frequency (Hz) */
		port_descriptors[TRIPLEPARA_FC_1] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TRIPLEPARA_FC_1] =
		 D_("Band 1 frequency (Hz)");
		port_range_hints[TRIPLEPARA_FC_1].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_LOGARITHMIC | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[TRIPLEPARA_FC_1].LowerBound = 0.0001;
		port_range_hints[TRIPLEPARA_FC_1].UpperBound = 0.49;

		/* Parameters for Band 1 bandwidth (octaves) */
		port_descriptors[TRIPLEPARA_BW_1] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TRIPLEPARA_BW_1] =
		 D_("Band 1 bandwidth (octaves)");
		port_range_hints[TRIPLEPARA_BW_1].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[TRIPLEPARA_BW_1].LowerBound = 0;
		port_range_hints[TRIPLEPARA_BW_1].UpperBound = 4;

		/* Parameters for Band 2 gain (dB) */
		port_descriptors[TRIPLEPARA_GAIN_2] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TRIPLEPARA_GAIN_2] =
		 D_("Band 2 gain (dB)");
		port_range_hints[TRIPLEPARA_GAIN_2].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[TRIPLEPARA_GAIN_2].LowerBound = -70;
		port_range_hints[TRIPLEPARA_GAIN_2].UpperBound = +30;

		/* Parameters for Band 2 frequency (Hz) */
		port_descriptors[TRIPLEPARA_FC_2] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TRIPLEPARA_FC_2] =
		 D_("Band 2 frequency (Hz)");
		port_range_hints[TRIPLEPARA_FC_2].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_LOGARITHMIC | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[TRIPLEPARA_FC_2].LowerBound = 0.0001;
		port_range_hints[TRIPLEPARA_FC_2].UpperBound = 0.49;

		/* Parameters for Band 2 bandwidth (octaves) */
		port_descriptors[TRIPLEPARA_BW_2] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TRIPLEPARA_BW_2] =
		 D_("Band 2 bandwidth (octaves)");
		port_range_hints[TRIPLEPARA_BW_2].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[TRIPLEPARA_BW_2].LowerBound = 0;
		port_range_hints[TRIPLEPARA_BW_2].UpperBound = 4;

		/* Parameters for Band 3 gain (dB) */
		port_descriptors[TRIPLEPARA_GAIN_3] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TRIPLEPARA_GAIN_3] =
		 D_("Band 3 gain (dB)");
		port_range_hints[TRIPLEPARA_GAIN_3].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[TRIPLEPARA_GAIN_3].LowerBound = -70;
		port_range_hints[TRIPLEPARA_GAIN_3].UpperBound = +30;

		/* Parameters for Band 3 frequency (Hz) */
		port_descriptors[TRIPLEPARA_FC_3] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TRIPLEPARA_FC_3] =
		 D_("Band 3 frequency (Hz)");
		port_range_hints[TRIPLEPARA_FC_3].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_LOGARITHMIC | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_DEFAULT_HIGH;
		port_range_hints[TRIPLEPARA_FC_3].LowerBound = 0.0001;
		port_range_hints[TRIPLEPARA_FC_3].UpperBound = 0.49;

		/* Parameters for Band 3 bandwidth (octaves) */
		port_descriptors[TRIPLEPARA_BW_3] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TRIPLEPARA_BW_3] =
		 D_("Band 3 bandwidth (octaves)");
		port_range_hints[TRIPLEPARA_BW_3].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[TRIPLEPARA_BW_3].LowerBound = 0;
		port_range_hints[TRIPLEPARA_BW_3].UpperBound = 4;

		/* Parameters for High-shelving gain (dB) */
		port_descriptors[TRIPLEPARA_GAIN_H] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TRIPLEPARA_GAIN_H] =
		 D_("High-shelving gain (dB)");
		port_range_hints[TRIPLEPARA_GAIN_H].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[TRIPLEPARA_GAIN_H].LowerBound = -70;
		port_range_hints[TRIPLEPARA_GAIN_H].UpperBound = +30;

		/* Parameters for High-shelving frequency (Hz) */
		port_descriptors[TRIPLEPARA_FC_H] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TRIPLEPARA_FC_H] =
		 D_("High-shelving frequency (Hz)");
		port_range_hints[TRIPLEPARA_FC_H].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_LOGARITHMIC | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_DEFAULT_MAXIMUM;
		port_range_hints[TRIPLEPARA_FC_H].LowerBound = 0.0001;
		port_range_hints[TRIPLEPARA_FC_H].UpperBound = 0.49;

		/* Parameters for High-shelving slope */
		port_descriptors[TRIPLEPARA_BW_H] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[TRIPLEPARA_BW_H] =
		 D_("High-shelving slope");
		port_range_hints[TRIPLEPARA_BW_H].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[TRIPLEPARA_BW_H].LowerBound = 0;
		port_range_hints[TRIPLEPARA_BW_H].UpperBound = 1;

		/* Parameters for Input */
		port_descriptors[TRIPLEPARA_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[TRIPLEPARA_INPUT] =
		 D_("Input");
		port_range_hints[TRIPLEPARA_INPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[TRIPLEPARA_INPUT].LowerBound = -1.0;
		port_range_hints[TRIPLEPARA_INPUT].UpperBound = +1.0;

		/* Parameters for Output */
		port_descriptors[TRIPLEPARA_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[TRIPLEPARA_OUTPUT] =
		 D_("Output");
		port_range_hints[TRIPLEPARA_OUTPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[TRIPLEPARA_OUTPUT].LowerBound = -1.0;
		port_range_hints[TRIPLEPARA_OUTPUT].UpperBound = +1.0;

		tripleParaDescriptor->activate = activateTriplePara;
		tripleParaDescriptor->cleanup = cleanupTriplePara;
		tripleParaDescriptor->connect_port = connectPortTriplePara;
		tripleParaDescriptor->deactivate = NULL;
		tripleParaDescriptor->instantiate = instantiateTriplePara;
		tripleParaDescriptor->run = runTriplePara;
		tripleParaDescriptor->run_adding = runAddingTriplePara;
		tripleParaDescriptor->set_run_adding_gain = setRunAddingGainTriplePara;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (tripleParaDescriptor) {
		free((LADSPA_PortDescriptor *)tripleParaDescriptor->PortDescriptors);
		free((char **)tripleParaDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)tripleParaDescriptor->PortRangeHints);
		free(tripleParaDescriptor);
	}

}
