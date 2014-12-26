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

#line 10 "dj_eq_1901.xml"

#include "ladspa-util.h"
#include "util/biquad.h"

#define BANDS 3

#define PEAK_BW          0.3f /* Peak EQ bandwidth (octaves) */
#define SHELF_SLOPE 1.5f /* Shelf EQ slope (arb. units) */

#define DJ_EQ_MONO_LO                  0
#define DJ_EQ_MONO_MID                 1
#define DJ_EQ_MONO_HI                  2
#define DJ_EQ_MONO_INPUT               3
#define DJ_EQ_MONO_OUTPUT              4
#define DJ_EQ_MONO_LATENCY             5
#define DJ_EQ_LO                       0
#define DJ_EQ_MID                      1
#define DJ_EQ_HI                       2
#define DJ_EQ_LEFT_INPUT               3
#define DJ_EQ_RIGHT_INPUT              4
#define DJ_EQ_LEFT_OUTPUT              5
#define DJ_EQ_RIGHT_OUTPUT             6
#define DJ_EQ_LATENCY                  7

static LADSPA_Descriptor *dj_eq_monoDescriptor = NULL;

typedef struct {
	LADSPA_Data *lo;
	LADSPA_Data *mid;
	LADSPA_Data *hi;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data *latency;
	biquad *     filters;
	float        fs;
	LADSPA_Data run_adding_gain;
} Dj_eq_mono;

static LADSPA_Descriptor *dj_eqDescriptor = NULL;

typedef struct {
	LADSPA_Data *lo;
	LADSPA_Data *mid;
	LADSPA_Data *hi;
	LADSPA_Data *left_input;
	LADSPA_Data *right_input;
	LADSPA_Data *left_output;
	LADSPA_Data *right_output;
	LADSPA_Data *latency;
	biquad *     filters;
	float        fs;
	LADSPA_Data run_adding_gain;
} Dj_eq;

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
		return dj_eq_monoDescriptor;
	case 1:
		return dj_eqDescriptor;
	default:
		return NULL;
	}
}

static void activateDj_eq_mono(LADSPA_Handle instance) {
	Dj_eq_mono *plugin_data = (Dj_eq_mono *)instance;
	biquad *filters = plugin_data->filters;
	float fs = plugin_data->fs;
#line 33 "dj_eq_1901.xml"
	biquad_init(&filters[0]);
	eq_set_params(&filters[0], 100.0f, 0.0f, PEAK_BW, fs);
	biquad_init(&filters[1]);
	eq_set_params(&filters[1], 1000.0f, 0.0f, PEAK_BW, fs);
	biquad_init(&filters[2]);
	hs_set_params(&filters[2], 10000.0f, 0.0f, SHELF_SLOPE, fs);
	plugin_data->filters = filters;
	plugin_data->fs = fs;

}

static void cleanupDj_eq_mono(LADSPA_Handle instance) {
	Dj_eq_mono *plugin_data = (Dj_eq_mono *)instance;
	free(plugin_data->filters);
	free(instance);
}

static void connectPortDj_eq_mono(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Dj_eq_mono *plugin;

	plugin = (Dj_eq_mono *)instance;
	switch (port) {
	case DJ_EQ_MONO_LO:
		plugin->lo = data;
		break;
	case DJ_EQ_MONO_MID:
		plugin->mid = data;
		break;
	case DJ_EQ_MONO_HI:
		plugin->hi = data;
		break;
	case DJ_EQ_MONO_INPUT:
		plugin->input = data;
		break;
	case DJ_EQ_MONO_OUTPUT:
		plugin->output = data;
		break;
	case DJ_EQ_MONO_LATENCY:
		plugin->latency = data;
		break;
	}
}

static LADSPA_Handle instantiateDj_eq_mono(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Dj_eq_mono *plugin_data = (Dj_eq_mono *)malloc(sizeof(Dj_eq_mono));
	biquad *filters = NULL;
	float fs;

#line 27 "dj_eq_1901.xml"
	fs = s_rate;
	
	filters = calloc(BANDS, sizeof(biquad));

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

static void runDj_eq_mono(LADSPA_Handle instance, unsigned long sample_count) {
	Dj_eq_mono *plugin_data = (Dj_eq_mono *)instance;

	/* Lo gain (dB) (float value) */
	const LADSPA_Data lo = *(plugin_data->lo);

	/* Mid gain (dB) (float value) */
	const LADSPA_Data mid = *(plugin_data->mid);

	/* Hi gain (dB) (float value) */
	const LADSPA_Data hi = *(plugin_data->hi);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	biquad * filters = plugin_data->filters;
	float fs = plugin_data->fs;

#line 42 "dj_eq_1901.xml"
	unsigned long pos;
	float samp;

	eq_set_params(&filters[0], 100.0f, lo, PEAK_BW, fs);
	eq_set_params(&filters[1], 1000.0f, mid, PEAK_BW, fs);
	hs_set_params(&filters[2], 10000.0f, hi, SHELF_SLOPE, fs);

	for (pos = 0; pos < sample_count; pos++) {
	  samp = biquad_run(&filters[0], input[pos]);
	  samp = biquad_run(&filters[1], samp);
	  samp = biquad_run(&filters[2], samp);
	  buffer_write(output[pos], samp);
	}

	*(plugin_data->latency) = 3; //XXX is this right?
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainDj_eq_mono(LADSPA_Handle instance, LADSPA_Data gain) {
	((Dj_eq_mono *)instance)->run_adding_gain = gain;
}

static void runAddingDj_eq_mono(LADSPA_Handle instance, unsigned long sample_count) {
	Dj_eq_mono *plugin_data = (Dj_eq_mono *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Lo gain (dB) (float value) */
	const LADSPA_Data lo = *(plugin_data->lo);

	/* Mid gain (dB) (float value) */
	const LADSPA_Data mid = *(plugin_data->mid);

	/* Hi gain (dB) (float value) */
	const LADSPA_Data hi = *(plugin_data->hi);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	biquad * filters = plugin_data->filters;
	float fs = plugin_data->fs;

#line 42 "dj_eq_1901.xml"
	unsigned long pos;
	float samp;

	eq_set_params(&filters[0], 100.0f, lo, PEAK_BW, fs);
	eq_set_params(&filters[1], 1000.0f, mid, PEAK_BW, fs);
	hs_set_params(&filters[2], 10000.0f, hi, SHELF_SLOPE, fs);

	for (pos = 0; pos < sample_count; pos++) {
	  samp = biquad_run(&filters[0], input[pos]);
	  samp = biquad_run(&filters[1], samp);
	  samp = biquad_run(&filters[2], samp);
	  buffer_write(output[pos], samp);
	}

	*(plugin_data->latency) = 3; //XXX is this right?
}

static void activateDj_eq(LADSPA_Handle instance) {
	Dj_eq *plugin_data = (Dj_eq *)instance;
	biquad *filters = plugin_data->filters;
	float fs = plugin_data->fs;
#line 33 "dj_eq_1901.xml"
	int i;

	for (i=0; i<2; i++) {
	  biquad_init(&filters[i*BANDS + 0]);
	  eq_set_params(&filters[i*BANDS + 0], 100.0f, 0.0f, PEAK_BW, fs);
	  biquad_init(&filters[i*BANDS + 1]);
	  eq_set_params(&filters[i*BANDS + 1], 1000.0f, 0.0f, PEAK_BW, fs);
	  biquad_init(&filters[i*BANDS + 2]);
	  hs_set_params(&filters[i*BANDS + 2], 10000.0f, 0.0f, SHELF_SLOPE, fs);
	}
	plugin_data->filters = filters;
	plugin_data->fs = fs;

}

static void cleanupDj_eq(LADSPA_Handle instance) {
	Dj_eq *plugin_data = (Dj_eq *)instance;
	free(plugin_data->filters);
	free(instance);
}

static void connectPortDj_eq(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Dj_eq *plugin;

	plugin = (Dj_eq *)instance;
	switch (port) {
	case DJ_EQ_LO:
		plugin->lo = data;
		break;
	case DJ_EQ_MID:
		plugin->mid = data;
		break;
	case DJ_EQ_HI:
		plugin->hi = data;
		break;
	case DJ_EQ_LEFT_INPUT:
		plugin->left_input = data;
		break;
	case DJ_EQ_RIGHT_INPUT:
		plugin->right_input = data;
		break;
	case DJ_EQ_LEFT_OUTPUT:
		plugin->left_output = data;
		break;
	case DJ_EQ_RIGHT_OUTPUT:
		plugin->right_output = data;
		break;
	case DJ_EQ_LATENCY:
		plugin->latency = data;
		break;
	}
}

static LADSPA_Handle instantiateDj_eq(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Dj_eq *plugin_data = (Dj_eq *)malloc(sizeof(Dj_eq));
	biquad *filters = NULL;
	float fs;

#line 27 "dj_eq_1901.xml"
	fs = s_rate;
	
	filters = calloc(BANDS * 2, sizeof(biquad));

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

static void runDj_eq(LADSPA_Handle instance, unsigned long sample_count) {
	Dj_eq *plugin_data = (Dj_eq *)instance;

	/* Lo gain (dB) (float value) */
	const LADSPA_Data lo = *(plugin_data->lo);

	/* Mid gain (dB) (float value) */
	const LADSPA_Data mid = *(plugin_data->mid);

	/* Hi gain (dB) (float value) */
	const LADSPA_Data hi = *(plugin_data->hi);

	/* Input L (array of floats of length sample_count) */
	const LADSPA_Data * const left_input = plugin_data->left_input;

	/* Input R (array of floats of length sample_count) */
	const LADSPA_Data * const right_input = plugin_data->right_input;

	/* Output L (array of floats of length sample_count) */
	LADSPA_Data * const left_output = plugin_data->left_output;

	/* Output R (array of floats of length sample_count) */
	LADSPA_Data * const right_output = plugin_data->right_output;
	biquad * filters = plugin_data->filters;
	float fs = plugin_data->fs;

#line 42 "dj_eq_1901.xml"
	unsigned long pos;
	unsigned int i;
	float samp;

	for (i=0; i<2; i++) {
	  eq_set_params(&filters[i*BANDS + 0], 100.0f, lo, PEAK_BW, fs);
	  eq_set_params(&filters[i*BANDS + 1], 1000.0f, mid, PEAK_BW, fs);
	  hs_set_params(&filters[i*BANDS + 2], 10000.0f, hi, SHELF_SLOPE, fs);
	}

	for (pos = 0; pos < sample_count; pos++) {
	  samp = biquad_run(&filters[0], left_input[pos]);
	  samp = biquad_run(&filters[1], samp);
	  samp = biquad_run(&filters[2], samp);
	  buffer_write(left_output[pos], samp);

	  samp = biquad_run(&filters[3], right_input[pos]);
	  samp = biquad_run(&filters[4], samp);
	  samp = biquad_run(&filters[5], samp);
	  buffer_write(right_output[pos], samp);
	}

	*(plugin_data->latency) = 3; //XXX is this right?
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainDj_eq(LADSPA_Handle instance, LADSPA_Data gain) {
	((Dj_eq *)instance)->run_adding_gain = gain;
}

static void runAddingDj_eq(LADSPA_Handle instance, unsigned long sample_count) {
	Dj_eq *plugin_data = (Dj_eq *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Lo gain (dB) (float value) */
	const LADSPA_Data lo = *(plugin_data->lo);

	/* Mid gain (dB) (float value) */
	const LADSPA_Data mid = *(plugin_data->mid);

	/* Hi gain (dB) (float value) */
	const LADSPA_Data hi = *(plugin_data->hi);

	/* Input L (array of floats of length sample_count) */
	const LADSPA_Data * const left_input = plugin_data->left_input;

	/* Input R (array of floats of length sample_count) */
	const LADSPA_Data * const right_input = plugin_data->right_input;

	/* Output L (array of floats of length sample_count) */
	LADSPA_Data * const left_output = plugin_data->left_output;

	/* Output R (array of floats of length sample_count) */
	LADSPA_Data * const right_output = plugin_data->right_output;
	biquad * filters = plugin_data->filters;
	float fs = plugin_data->fs;

#line 42 "dj_eq_1901.xml"
	unsigned long pos;
	unsigned int i;
	float samp;

	for (i=0; i<2; i++) {
	  eq_set_params(&filters[i*BANDS + 0], 100.0f, lo, PEAK_BW, fs);
	  eq_set_params(&filters[i*BANDS + 1], 1000.0f, mid, PEAK_BW, fs);
	  hs_set_params(&filters[i*BANDS + 2], 10000.0f, hi, SHELF_SLOPE, fs);
	}

	for (pos = 0; pos < sample_count; pos++) {
	  samp = biquad_run(&filters[0], left_input[pos]);
	  samp = biquad_run(&filters[1], samp);
	  samp = biquad_run(&filters[2], samp);
	  buffer_write(left_output[pos], samp);

	  samp = biquad_run(&filters[3], right_input[pos]);
	  samp = biquad_run(&filters[4], samp);
	  samp = biquad_run(&filters[5], samp);
	  buffer_write(right_output[pos], samp);
	}

	*(plugin_data->latency) = 3; //XXX is this right?
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


	dj_eq_monoDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (dj_eq_monoDescriptor) {
		dj_eq_monoDescriptor->UniqueID = 1907;
		dj_eq_monoDescriptor->Label = "dj_eq_mono";
		dj_eq_monoDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		dj_eq_monoDescriptor->Name =
		 D_("DJ EQ (mono)");
		dj_eq_monoDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		dj_eq_monoDescriptor->Copyright =
		 "GPL";
		dj_eq_monoDescriptor->PortCount = 6;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(6,
		 sizeof(LADSPA_PortDescriptor));
		dj_eq_monoDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(6,
		 sizeof(LADSPA_PortRangeHint));
		dj_eq_monoDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(6, sizeof(char*));
		dj_eq_monoDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Lo gain (dB) */
		port_descriptors[DJ_EQ_MONO_LO] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DJ_EQ_MONO_LO] =
		 D_("Lo gain (dB)");
		port_range_hints[DJ_EQ_MONO_LO].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[DJ_EQ_MONO_LO].LowerBound = -70;
		port_range_hints[DJ_EQ_MONO_LO].UpperBound = +6;

		/* Parameters for Mid gain (dB) */
		port_descriptors[DJ_EQ_MONO_MID] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DJ_EQ_MONO_MID] =
		 D_("Mid gain (dB)");
		port_range_hints[DJ_EQ_MONO_MID].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[DJ_EQ_MONO_MID].LowerBound = -70;
		port_range_hints[DJ_EQ_MONO_MID].UpperBound = +6;

		/* Parameters for Hi gain (dB) */
		port_descriptors[DJ_EQ_MONO_HI] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DJ_EQ_MONO_HI] =
		 D_("Hi gain (dB)");
		port_range_hints[DJ_EQ_MONO_HI].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[DJ_EQ_MONO_HI].LowerBound = -70;
		port_range_hints[DJ_EQ_MONO_HI].UpperBound = +6;

		/* Parameters for Input */
		port_descriptors[DJ_EQ_MONO_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[DJ_EQ_MONO_INPUT] =
		 D_("Input");
		port_range_hints[DJ_EQ_MONO_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[DJ_EQ_MONO_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[DJ_EQ_MONO_OUTPUT] =
		 D_("Output");
		port_range_hints[DJ_EQ_MONO_OUTPUT].HintDescriptor = 0;

		/* Parameters for latency */
		port_descriptors[DJ_EQ_MONO_LATENCY] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
		port_names[DJ_EQ_MONO_LATENCY] =
		 D_("latency");
		port_range_hints[DJ_EQ_MONO_LATENCY].HintDescriptor = 0;

		dj_eq_monoDescriptor->activate = activateDj_eq_mono;
		dj_eq_monoDescriptor->cleanup = cleanupDj_eq_mono;
		dj_eq_monoDescriptor->connect_port = connectPortDj_eq_mono;
		dj_eq_monoDescriptor->deactivate = NULL;
		dj_eq_monoDescriptor->instantiate = instantiateDj_eq_mono;
		dj_eq_monoDescriptor->run = runDj_eq_mono;
		dj_eq_monoDescriptor->run_adding = runAddingDj_eq_mono;
		dj_eq_monoDescriptor->set_run_adding_gain = setRunAddingGainDj_eq_mono;
	}

	dj_eqDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (dj_eqDescriptor) {
		dj_eqDescriptor->UniqueID = 1901;
		dj_eqDescriptor->Label = "dj_eq";
		dj_eqDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		dj_eqDescriptor->Name =
		 D_("DJ EQ");
		dj_eqDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		dj_eqDescriptor->Copyright =
		 "GPL";
		dj_eqDescriptor->PortCount = 8;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(8,
		 sizeof(LADSPA_PortDescriptor));
		dj_eqDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(8,
		 sizeof(LADSPA_PortRangeHint));
		dj_eqDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(8, sizeof(char*));
		dj_eqDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Lo gain (dB) */
		port_descriptors[DJ_EQ_LO] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DJ_EQ_LO] =
		 D_("Lo gain (dB)");
		port_range_hints[DJ_EQ_LO].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[DJ_EQ_LO].LowerBound = -70;
		port_range_hints[DJ_EQ_LO].UpperBound = +6;

		/* Parameters for Mid gain (dB) */
		port_descriptors[DJ_EQ_MID] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DJ_EQ_MID] =
		 D_("Mid gain (dB)");
		port_range_hints[DJ_EQ_MID].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[DJ_EQ_MID].LowerBound = -70;
		port_range_hints[DJ_EQ_MID].UpperBound = +6;

		/* Parameters for Hi gain (dB) */
		port_descriptors[DJ_EQ_HI] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DJ_EQ_HI] =
		 D_("Hi gain (dB)");
		port_range_hints[DJ_EQ_HI].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[DJ_EQ_HI].LowerBound = -70;
		port_range_hints[DJ_EQ_HI].UpperBound = +6;

		/* Parameters for Input L */
		port_descriptors[DJ_EQ_LEFT_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[DJ_EQ_LEFT_INPUT] =
		 D_("Input L");
		port_range_hints[DJ_EQ_LEFT_INPUT].HintDescriptor = 0;

		/* Parameters for Input R */
		port_descriptors[DJ_EQ_RIGHT_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[DJ_EQ_RIGHT_INPUT] =
		 D_("Input R");
		port_range_hints[DJ_EQ_RIGHT_INPUT].HintDescriptor = 0;

		/* Parameters for Output L */
		port_descriptors[DJ_EQ_LEFT_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[DJ_EQ_LEFT_OUTPUT] =
		 D_("Output L");
		port_range_hints[DJ_EQ_LEFT_OUTPUT].HintDescriptor = 0;

		/* Parameters for Output R */
		port_descriptors[DJ_EQ_RIGHT_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[DJ_EQ_RIGHT_OUTPUT] =
		 D_("Output R");
		port_range_hints[DJ_EQ_RIGHT_OUTPUT].HintDescriptor = 0;

		/* Parameters for latency */
		port_descriptors[DJ_EQ_LATENCY] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
		port_names[DJ_EQ_LATENCY] =
		 D_("latency");
		port_range_hints[DJ_EQ_LATENCY].HintDescriptor = 0;

		dj_eqDescriptor->activate = activateDj_eq;
		dj_eqDescriptor->cleanup = cleanupDj_eq;
		dj_eqDescriptor->connect_port = connectPortDj_eq;
		dj_eqDescriptor->deactivate = NULL;
		dj_eqDescriptor->instantiate = instantiateDj_eq;
		dj_eqDescriptor->run = runDj_eq;
		dj_eqDescriptor->run_adding = runAddingDj_eq;
		dj_eqDescriptor->set_run_adding_gain = setRunAddingGainDj_eq;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (dj_eq_monoDescriptor) {
		free((LADSPA_PortDescriptor *)dj_eq_monoDescriptor->PortDescriptors);
		free((char **)dj_eq_monoDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)dj_eq_monoDescriptor->PortRangeHints);
		free(dj_eq_monoDescriptor);
	}
	if (dj_eqDescriptor) {
		free((LADSPA_PortDescriptor *)dj_eqDescriptor->PortDescriptors);
		free((char **)dj_eqDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)dj_eqDescriptor->PortRangeHints);
		free(dj_eqDescriptor);
	}

}
