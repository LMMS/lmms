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

#line 10 "ls_filter_1908.xml"

#include "ladspa-util.h"
#include "util/ls_filter.h"

#define LSFILTER_TYPE                  0
#define LSFILTER_CUTOFF                1
#define LSFILTER_RESONANCE             2
#define LSFILTER_INPUT                 3
#define LSFILTER_OUTPUT                4

static LADSPA_Descriptor *lsFilterDescriptor = NULL;

typedef struct {
	LADSPA_Data *type;
	LADSPA_Data *cutoff;
	LADSPA_Data *resonance;
	LADSPA_Data *input;
	LADSPA_Data *output;
	ls_filt *    filt;
	float        fs;
	LADSPA_Data run_adding_gain;
} LsFilter;

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
		return lsFilterDescriptor;
	default:
		return NULL;
	}
}

static void activateLsFilter(LADSPA_Handle instance) {
	LsFilter *plugin_data = (LsFilter *)instance;
	ls_filt *filt = plugin_data->filt;
	float fs = plugin_data->fs;
#line 26 "ls_filter_1908.xml"
	ls_filt_init(filt);
	plugin_data->filt = filt;
	plugin_data->fs = fs;

}

static void cleanupLsFilter(LADSPA_Handle instance) {
#line 42 "ls_filter_1908.xml"
	LsFilter *plugin_data = (LsFilter *)instance;
	free(plugin_data->filt);
	free(instance);
}

static void connectPortLsFilter(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	LsFilter *plugin;

	plugin = (LsFilter *)instance;
	switch (port) {
	case LSFILTER_TYPE:
		plugin->type = data;
		break;
	case LSFILTER_CUTOFF:
		plugin->cutoff = data;
		break;
	case LSFILTER_RESONANCE:
		plugin->resonance = data;
		break;
	case LSFILTER_INPUT:
		plugin->input = data;
		break;
	case LSFILTER_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateLsFilter(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	LsFilter *plugin_data = (LsFilter *)malloc(sizeof(LsFilter));
	ls_filt *filt = NULL;
	float fs;

#line 21 "ls_filter_1908.xml"
	filt = malloc(sizeof(ls_filt));
	fs = s_rate;

	plugin_data->filt = filt;
	plugin_data->fs = fs;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runLsFilter(LADSPA_Handle instance, unsigned long sample_count) {
	LsFilter *plugin_data = (LsFilter *)instance;

	/* Filter type (0=LP, 1=BP, 2=HP) (float value) */
	const LADSPA_Data type = *(plugin_data->type);

	/* Cutoff frequency (Hz) (float value) */
	const LADSPA_Data cutoff = *(plugin_data->cutoff);

	/* Resonance (float value) */
	const LADSPA_Data resonance = *(plugin_data->resonance);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	ls_filt * filt = plugin_data->filt;
	float fs = plugin_data->fs;

#line 30 "ls_filter_1908.xml"
	unsigned long pos;
	const ls_filt_type t = (ls_filt_type)f_round(type);

	ls_filt_setup(filt, t, cutoff, resonance, fs);

	for (pos = 0; pos < sample_count; pos++) {
	  buffer_write(output[pos], ls_filt_run(filt, input[pos]));
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainLsFilter(LADSPA_Handle instance, LADSPA_Data gain) {
	((LsFilter *)instance)->run_adding_gain = gain;
}

static void runAddingLsFilter(LADSPA_Handle instance, unsigned long sample_count) {
	LsFilter *plugin_data = (LsFilter *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Filter type (0=LP, 1=BP, 2=HP) (float value) */
	const LADSPA_Data type = *(plugin_data->type);

	/* Cutoff frequency (Hz) (float value) */
	const LADSPA_Data cutoff = *(plugin_data->cutoff);

	/* Resonance (float value) */
	const LADSPA_Data resonance = *(plugin_data->resonance);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	ls_filt * filt = plugin_data->filt;
	float fs = plugin_data->fs;

#line 30 "ls_filter_1908.xml"
	unsigned long pos;
	const ls_filt_type t = (ls_filt_type)f_round(type);

	ls_filt_setup(filt, t, cutoff, resonance, fs);

	for (pos = 0; pos < sample_count; pos++) {
	  buffer_write(output[pos], ls_filt_run(filt, input[pos]));
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


	lsFilterDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (lsFilterDescriptor) {
		lsFilterDescriptor->UniqueID = 1908;
		lsFilterDescriptor->Label = "lsFilter";
		lsFilterDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		lsFilterDescriptor->Name =
		 D_("LS Filter");
		lsFilterDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		lsFilterDescriptor->Copyright =
		 "GPL";
		lsFilterDescriptor->PortCount = 5;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(5,
		 sizeof(LADSPA_PortDescriptor));
		lsFilterDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(5,
		 sizeof(LADSPA_PortRangeHint));
		lsFilterDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(5, sizeof(char*));
		lsFilterDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Filter type (0=LP, 1=BP, 2=HP) */
		port_descriptors[LSFILTER_TYPE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[LSFILTER_TYPE] =
		 D_("Filter type (0=LP, 1=BP, 2=HP)");
		port_range_hints[LSFILTER_TYPE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0 | LADSPA_HINT_INTEGER;
		port_range_hints[LSFILTER_TYPE].LowerBound = 0;
		port_range_hints[LSFILTER_TYPE].UpperBound = 2;

		/* Parameters for Cutoff frequency (Hz) */
		port_descriptors[LSFILTER_CUTOFF] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[LSFILTER_CUTOFF] =
		 D_("Cutoff frequency (Hz)");
		port_range_hints[LSFILTER_CUTOFF].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE | LADSPA_HINT_LOGARITHMIC | LADSPA_HINT_SAMPLE_RATE;
		port_range_hints[LSFILTER_CUTOFF].LowerBound = 0.002;
		port_range_hints[LSFILTER_CUTOFF].UpperBound = 0.5;

		/* Parameters for Resonance */
		port_descriptors[LSFILTER_RESONANCE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[LSFILTER_RESONANCE] =
		 D_("Resonance");
		port_range_hints[LSFILTER_RESONANCE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[LSFILTER_RESONANCE].LowerBound = 0.0;
		port_range_hints[LSFILTER_RESONANCE].UpperBound = 1.0;

		/* Parameters for Input */
		port_descriptors[LSFILTER_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[LSFILTER_INPUT] =
		 D_("Input");
		port_range_hints[LSFILTER_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[LSFILTER_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[LSFILTER_OUTPUT] =
		 D_("Output");
		port_range_hints[LSFILTER_OUTPUT].HintDescriptor = 0;

		lsFilterDescriptor->activate = activateLsFilter;
		lsFilterDescriptor->cleanup = cleanupLsFilter;
		lsFilterDescriptor->connect_port = connectPortLsFilter;
		lsFilterDescriptor->deactivate = NULL;
		lsFilterDescriptor->instantiate = instantiateLsFilter;
		lsFilterDescriptor->run = runLsFilter;
		lsFilterDescriptor->run_adding = runAddingLsFilter;
		lsFilterDescriptor->set_run_adding_gain = setRunAddingGainLsFilter;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (lsFilterDescriptor) {
		free((LADSPA_PortDescriptor *)lsFilterDescriptor->PortDescriptors);
		free((char **)lsFilterDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)lsFilterDescriptor->PortRangeHints);
		free(lsFilterDescriptor);
	}

}
