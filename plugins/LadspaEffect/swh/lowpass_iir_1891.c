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
#include "ladspa-util.h"

#define LOWPASS_IIR_CUTOFF             0
#define LOWPASS_IIR_STAGES             1
#define LOWPASS_IIR_INPUT              2
#define LOWPASS_IIR_OUTPUT             3

static LADSPA_Descriptor *lowpass_iirDescriptor = NULL;

typedef struct {
	LADSPA_Data *cutoff;
	LADSPA_Data *stages;
	LADSPA_Data *input;
	LADSPA_Data *output;
	iir_stage_t* gt;
	iirf_t*      iirf;
	long         sample_rate;
	LADSPA_Data run_adding_gain;
} Lowpass_iir;

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
		return lowpass_iirDescriptor;
	default:
		return NULL;
	}
}

static void activateLowpass_iir(LADSPA_Handle instance) {
	Lowpass_iir *plugin_data = (Lowpass_iir *)instance;
	iir_stage_t*gt = plugin_data->gt;
	iirf_t*iirf = plugin_data->iirf;
	long sample_rate = plugin_data->sample_rate;
	
	gt = init_iir_stage(IIR_STAGE_LOWPASS,10,3,2);
	iirf = init_iirf_t(gt);
	chebyshev(iirf, gt, 2*CLAMP(f_round(*(plugin_data->stages)),1,10), IIR_STAGE_LOWPASS, 
	          *(plugin_data->cutoff)/(float)sample_rate, 0.5f);
	plugin_data->gt = gt;
	plugin_data->iirf = iirf;
	plugin_data->sample_rate = sample_rate;

}

static void cleanupLowpass_iir(LADSPA_Handle instance) {
	Lowpass_iir *plugin_data = (Lowpass_iir *)instance;
	free_iirf_t(plugin_data->iirf, plugin_data->gt);
	free_iir_stage(plugin_data->gt);
	free(instance);
}

static void connectPortLowpass_iir(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Lowpass_iir *plugin;

	plugin = (Lowpass_iir *)instance;
	switch (port) {
	case LOWPASS_IIR_CUTOFF:
		plugin->cutoff = data;
		break;
	case LOWPASS_IIR_STAGES:
		plugin->stages = data;
		break;
	case LOWPASS_IIR_INPUT:
		plugin->input = data;
		break;
	case LOWPASS_IIR_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateLowpass_iir(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Lowpass_iir *plugin_data = (Lowpass_iir *)malloc(sizeof(Lowpass_iir));
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

static void runLowpass_iir(LADSPA_Handle instance, unsigned long sample_count) {
	Lowpass_iir *plugin_data = (Lowpass_iir *)instance;

	/* Cutoff Frequency (float value) */
	const LADSPA_Data cutoff = *(plugin_data->cutoff);

	/* Stages(2 poles per stage) (float value) */
	const LADSPA_Data stages = *(plugin_data->stages);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	iir_stage_t* gt = plugin_data->gt;
	iirf_t* iirf = plugin_data->iirf;
	long sample_rate = plugin_data->sample_rate;

	chebyshev(iirf, gt, 2*CLAMP((int)stages,1,10), IIR_STAGE_LOWPASS, cutoff/(float)sample_rate, 0.5f);
	iir_process_buffer_ns_5(iirf, gt, input, output, sample_count,RUN_ADDING);
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainLowpass_iir(LADSPA_Handle instance, LADSPA_Data gain) {
	((Lowpass_iir *)instance)->run_adding_gain = gain;
}

static void runAddingLowpass_iir(LADSPA_Handle instance, unsigned long sample_count) {
	Lowpass_iir *plugin_data = (Lowpass_iir *)instance;

	/* Cutoff Frequency (float value) */
	const LADSPA_Data cutoff = *(plugin_data->cutoff);

	/* Stages(2 poles per stage) (float value) */
	const LADSPA_Data stages = *(plugin_data->stages);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	iir_stage_t* gt = plugin_data->gt;
	iirf_t* iirf = plugin_data->iirf;
	long sample_rate = plugin_data->sample_rate;

	chebyshev(iirf, gt, 2*CLAMP((int)stages,1,10), IIR_STAGE_LOWPASS, cutoff/(float)sample_rate, 0.5f);
	iir_process_buffer_ns_5(iirf, gt, input, output, sample_count,RUN_ADDING);
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


	lowpass_iirDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (lowpass_iirDescriptor) {
		lowpass_iirDescriptor->UniqueID = 1891;
		lowpass_iirDescriptor->Label = "lowpass_iir";
		lowpass_iirDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		lowpass_iirDescriptor->Name =
		 D_("Glame Lowpass Filter");
		lowpass_iirDescriptor->Maker =
		 "Alexander Ehlert <mag@glame.de>";
		lowpass_iirDescriptor->Copyright =
		 "GPL";
		lowpass_iirDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		lowpass_iirDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		lowpass_iirDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		lowpass_iirDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Cutoff Frequency */
		port_descriptors[LOWPASS_IIR_CUTOFF] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[LOWPASS_IIR_CUTOFF] =
		 D_("Cutoff Frequency");
		port_range_hints[LOWPASS_IIR_CUTOFF].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_HIGH | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_LOGARITHMIC;
		port_range_hints[LOWPASS_IIR_CUTOFF].LowerBound = 0.0001;
		port_range_hints[LOWPASS_IIR_CUTOFF].UpperBound = 0.45;

		/* Parameters for Stages(2 poles per stage) */
		port_descriptors[LOWPASS_IIR_STAGES] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[LOWPASS_IIR_STAGES] =
		 D_("Stages(2 poles per stage)");
		port_range_hints[LOWPASS_IIR_STAGES].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1 | LADSPA_HINT_INTEGER;
		port_range_hints[LOWPASS_IIR_STAGES].LowerBound = 1.0;
		port_range_hints[LOWPASS_IIR_STAGES].UpperBound = 10.0;

		/* Parameters for Input */
		port_descriptors[LOWPASS_IIR_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[LOWPASS_IIR_INPUT] =
		 D_("Input");
		port_range_hints[LOWPASS_IIR_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[LOWPASS_IIR_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[LOWPASS_IIR_OUTPUT] =
		 D_("Output");
		port_range_hints[LOWPASS_IIR_OUTPUT].HintDescriptor = 0;

		lowpass_iirDescriptor->activate = activateLowpass_iir;
		lowpass_iirDescriptor->cleanup = cleanupLowpass_iir;
		lowpass_iirDescriptor->connect_port = connectPortLowpass_iir;
		lowpass_iirDescriptor->deactivate = NULL;
		lowpass_iirDescriptor->instantiate = instantiateLowpass_iir;
		lowpass_iirDescriptor->run = runLowpass_iir;
		lowpass_iirDescriptor->run_adding = runAddingLowpass_iir;
		lowpass_iirDescriptor->set_run_adding_gain = setRunAddingGainLowpass_iir;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (lowpass_iirDescriptor) {
		free((LADSPA_PortDescriptor *)lowpass_iirDescriptor->PortDescriptors);
		free((char **)lowpass_iirDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)lowpass_iirDescriptor->PortRangeHints);
		free(lowpass_iirDescriptor);
	}

}
