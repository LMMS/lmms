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

#include "config.h"
#include "util/iir.h"

#define BANDPASS_A_IIR_CENTER          0
#define BANDPASS_A_IIR_WIDTH           1
#define BANDPASS_A_IIR_INPUT           2
#define BANDPASS_A_IIR_OUTPUT          3

static LADSPA_Descriptor *bandpass_a_iirDescriptor = NULL;

typedef struct {
	LADSPA_Data *center;
	LADSPA_Data *width;
	LADSPA_Data *input;
	LADSPA_Data *output;
	iir_stage_t* gt;
	iirf_t*      iirf;
	long         sample_rate;
	LADSPA_Data run_adding_gain;
} Bandpass_a_iir;

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
		return bandpass_a_iirDescriptor;
	default:
		return NULL;
	}
}

static void activateBandpass_a_iir(LADSPA_Handle instance) {
	Bandpass_a_iir *plugin_data = (Bandpass_a_iir *)instance;
	iir_stage_t*gt = plugin_data->gt;
	iirf_t*iirf = plugin_data->iirf;
	long sample_rate = plugin_data->sample_rate;
	
	gt = init_iir_stage(IIR_STAGE_LOWPASS,1,3,2);
	iirf = init_iirf_t(gt);
	calc_2polebandpass(iirf, gt, *(plugin_data->center), *(plugin_data->width), sample_rate);
	plugin_data->gt = gt;
	plugin_data->iirf = iirf;
	plugin_data->sample_rate = sample_rate;

}

static void cleanupBandpass_a_iir(LADSPA_Handle instance) {
	Bandpass_a_iir *plugin_data = (Bandpass_a_iir *)instance;
	free_iirf_t(plugin_data->iirf, plugin_data->gt);
	free_iir_stage(plugin_data->gt);
	free(instance);
}

static void connectPortBandpass_a_iir(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Bandpass_a_iir *plugin;

	plugin = (Bandpass_a_iir *)instance;
	switch (port) {
	case BANDPASS_A_IIR_CENTER:
		plugin->center = data;
		break;
	case BANDPASS_A_IIR_WIDTH:
		plugin->width = data;
		break;
	case BANDPASS_A_IIR_INPUT:
		plugin->input = data;
		break;
	case BANDPASS_A_IIR_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateBandpass_a_iir(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Bandpass_a_iir *plugin_data = (Bandpass_a_iir *)malloc(sizeof(Bandpass_a_iir));
	iir_stage_t*gt = NULL;
	iirf_t*iirf = NULL;
	long sample_rate;

	sample_rate = s_rate;

	plugin_data->gt = gt;
	plugin_data->iirf = iirf;
	plugin_data->sample_rate = sample_rate;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runBandpass_a_iir(LADSPA_Handle instance, unsigned long sample_count) {
	Bandpass_a_iir *plugin_data = (Bandpass_a_iir *)instance;

	/* Center Frequency (Hz) (float value) */
	const LADSPA_Data center = *(plugin_data->center);

	/* Bandwidth (Hz) (float value) */
	const LADSPA_Data width = *(plugin_data->width);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	iir_stage_t* gt = plugin_data->gt;
	iirf_t* iirf = plugin_data->iirf;
	long sample_rate = plugin_data->sample_rate;

	calc_2polebandpass(iirf, gt, center, width, sample_rate);
	iir_process_buffer_1s_5(iirf, gt, input, output, sample_count,0);
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainBandpass_a_iir(LADSPA_Handle instance, LADSPA_Data gain) {
	((Bandpass_a_iir *)instance)->run_adding_gain = gain;
}

static void runAddingBandpass_a_iir(LADSPA_Handle instance, unsigned long sample_count) {
	Bandpass_a_iir *plugin_data = (Bandpass_a_iir *)instance;

	/* Center Frequency (Hz) (float value) */
	const LADSPA_Data center = *(plugin_data->center);

	/* Bandwidth (Hz) (float value) */
	const LADSPA_Data width = *(plugin_data->width);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	iir_stage_t* gt = plugin_data->gt;
	iirf_t* iirf = plugin_data->iirf;
	long sample_rate = plugin_data->sample_rate;

	calc_2polebandpass(iirf, gt, center, width, sample_rate);
	iir_process_buffer_1s_5(iirf, gt, input, output, sample_count,0);
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


	bandpass_a_iirDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (bandpass_a_iirDescriptor) {
		bandpass_a_iirDescriptor->UniqueID = 1893;
		bandpass_a_iirDescriptor->Label = "bandpass_a_iir";
		bandpass_a_iirDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		bandpass_a_iirDescriptor->Name =
		 D_("Glame Bandpass Analog Filter");
		bandpass_a_iirDescriptor->Maker =
		 "Alexander Ehlert <mag@glame.de>";
		bandpass_a_iirDescriptor->Copyright =
		 "GPL";
		bandpass_a_iirDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		bandpass_a_iirDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		bandpass_a_iirDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		bandpass_a_iirDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Center Frequency (Hz) */
		port_descriptors[BANDPASS_A_IIR_CENTER] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[BANDPASS_A_IIR_CENTER] =
		 D_("Center Frequency (Hz)");
		port_range_hints[BANDPASS_A_IIR_CENTER].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_LOGARITHMIC;
		port_range_hints[BANDPASS_A_IIR_CENTER].LowerBound = 0.0001;
		port_range_hints[BANDPASS_A_IIR_CENTER].UpperBound = 0.45;

		/* Parameters for Bandwidth (Hz) */
		port_descriptors[BANDPASS_A_IIR_WIDTH] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[BANDPASS_A_IIR_WIDTH] =
		 D_("Bandwidth (Hz)");
		port_range_hints[BANDPASS_A_IIR_WIDTH].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_LOGARITHMIC;
		port_range_hints[BANDPASS_A_IIR_WIDTH].LowerBound = 0.0001;
		port_range_hints[BANDPASS_A_IIR_WIDTH].UpperBound = 0.45;

		/* Parameters for Input */
		port_descriptors[BANDPASS_A_IIR_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[BANDPASS_A_IIR_INPUT] =
		 D_("Input");
		port_range_hints[BANDPASS_A_IIR_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[BANDPASS_A_IIR_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[BANDPASS_A_IIR_OUTPUT] =
		 D_("Output");
		port_range_hints[BANDPASS_A_IIR_OUTPUT].HintDescriptor = 0;

		bandpass_a_iirDescriptor->activate = activateBandpass_a_iir;
		bandpass_a_iirDescriptor->cleanup = cleanupBandpass_a_iir;
		bandpass_a_iirDescriptor->connect_port = connectPortBandpass_a_iir;
		bandpass_a_iirDescriptor->deactivate = NULL;
		bandpass_a_iirDescriptor->instantiate = instantiateBandpass_a_iir;
		bandpass_a_iirDescriptor->run = runBandpass_a_iir;
		bandpass_a_iirDescriptor->run_adding = runAddingBandpass_a_iir;
		bandpass_a_iirDescriptor->set_run_adding_gain = setRunAddingGainBandpass_a_iir;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (bandpass_a_iirDescriptor) {
		free((LADSPA_PortDescriptor *)bandpass_a_iirDescriptor->PortDescriptors);
		free((char **)bandpass_a_iirDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)bandpass_a_iirDescriptor->PortRangeHints);
		free(bandpass_a_iirDescriptor);
	}

}
