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

#define HIGHPASS_IIR_CUTOFF            0
#define HIGHPASS_IIR_STAGES            1
#define HIGHPASS_IIR_INPUT             2
#define HIGHPASS_IIR_OUTPUT            3

static LADSPA_Descriptor *highpass_iirDescriptor = NULL;

typedef struct {
	LADSPA_Data *cutoff;
	LADSPA_Data *stages;
	LADSPA_Data *input;
	LADSPA_Data *output;
	iir_stage_t* gt;
	iirf_t*      iirf;
	long         sample_rate;
	LADSPA_Data run_adding_gain;
} Highpass_iir;

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
		return highpass_iirDescriptor;
	default:
		return NULL;
	}
}

static void activateHighpass_iir(LADSPA_Handle instance) {
	Highpass_iir *plugin_data = (Highpass_iir *)instance;
	iir_stage_t*gt = plugin_data->gt;
	iirf_t*iirf = plugin_data->iirf;
	long sample_rate = plugin_data->sample_rate;
	
	gt = init_iir_stage(IIR_STAGE_HIGHPASS,10,3,2);
	iirf = init_iirf_t(gt);
	chebyshev(iirf, gt, 2*CLAMP((int)(*(plugin_data->stages)),1,10), IIR_STAGE_HIGHPASS, *(plugin_data->cutoff)/(float)sample_rate, 0.5f);
	plugin_data->gt = gt;
	plugin_data->iirf = iirf;
	plugin_data->sample_rate = sample_rate;

}

static void cleanupHighpass_iir(LADSPA_Handle instance) {
	Highpass_iir *plugin_data = (Highpass_iir *)instance;
	free_iirf_t(plugin_data->iirf, plugin_data->gt);
	free_iir_stage(plugin_data->gt);
	free(instance);
}

static void connectPortHighpass_iir(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Highpass_iir *plugin;

	plugin = (Highpass_iir *)instance;
	switch (port) {
	case HIGHPASS_IIR_CUTOFF:
		plugin->cutoff = data;
		break;
	case HIGHPASS_IIR_STAGES:
		plugin->stages = data;
		break;
	case HIGHPASS_IIR_INPUT:
		plugin->input = data;
		break;
	case HIGHPASS_IIR_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateHighpass_iir(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Highpass_iir *plugin_data = (Highpass_iir *)malloc(sizeof(Highpass_iir));
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

static void runHighpass_iir(LADSPA_Handle instance, unsigned long sample_count) {
	Highpass_iir *plugin_data = (Highpass_iir *)instance;

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

	chebyshev(iirf, gt, 2*CLAMP((int)stages,1,10), IIR_STAGE_HIGHPASS, cutoff/(float)sample_rate, 0.5f);
	iir_process_buffer_ns_5(iirf, gt, input, output, sample_count,RUN_ADDING);
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainHighpass_iir(LADSPA_Handle instance, LADSPA_Data gain) {
	((Highpass_iir *)instance)->run_adding_gain = gain;
}

static void runAddingHighpass_iir(LADSPA_Handle instance, unsigned long sample_count) {
	Highpass_iir *plugin_data = (Highpass_iir *)instance;

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

	chebyshev(iirf, gt, 2*CLAMP((int)stages,1,10), IIR_STAGE_HIGHPASS, cutoff/(float)sample_rate, 0.5f);
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


	highpass_iirDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (highpass_iirDescriptor) {
		highpass_iirDescriptor->UniqueID = 1890;
		highpass_iirDescriptor->Label = "highpass_iir";
		highpass_iirDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		highpass_iirDescriptor->Name =
		 D_("Glame Highpass Filter");
		highpass_iirDescriptor->Maker =
		 "Alexander Ehlert <mag@glame.de>";
		highpass_iirDescriptor->Copyright =
		 "GPL";
		highpass_iirDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		highpass_iirDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		highpass_iirDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		highpass_iirDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Cutoff Frequency */
		port_descriptors[HIGHPASS_IIR_CUTOFF] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HIGHPASS_IIR_CUTOFF] =
		 D_("Cutoff Frequency");
		port_range_hints[HIGHPASS_IIR_CUTOFF].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_LOGARITHMIC;
		port_range_hints[HIGHPASS_IIR_CUTOFF].LowerBound = 0.0001;
		port_range_hints[HIGHPASS_IIR_CUTOFF].UpperBound = 0.45;

		/* Parameters for Stages(2 poles per stage) */
		port_descriptors[HIGHPASS_IIR_STAGES] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HIGHPASS_IIR_STAGES] =
		 D_("Stages(2 poles per stage)");
		port_range_hints[HIGHPASS_IIR_STAGES].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1 | LADSPA_HINT_INTEGER;
		port_range_hints[HIGHPASS_IIR_STAGES].LowerBound = 1.0;
		port_range_hints[HIGHPASS_IIR_STAGES].UpperBound = 10.0;

		/* Parameters for Input */
		port_descriptors[HIGHPASS_IIR_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[HIGHPASS_IIR_INPUT] =
		 D_("Input");
		port_range_hints[HIGHPASS_IIR_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[HIGHPASS_IIR_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[HIGHPASS_IIR_OUTPUT] =
		 D_("Output");
		port_range_hints[HIGHPASS_IIR_OUTPUT].HintDescriptor = 0;

		highpass_iirDescriptor->activate = activateHighpass_iir;
		highpass_iirDescriptor->cleanup = cleanupHighpass_iir;
		highpass_iirDescriptor->connect_port = connectPortHighpass_iir;
		highpass_iirDescriptor->deactivate = NULL;
		highpass_iirDescriptor->instantiate = instantiateHighpass_iir;
		highpass_iirDescriptor->run = runHighpass_iir;
		highpass_iirDescriptor->run_adding = runAddingHighpass_iir;
		highpass_iirDescriptor->set_run_adding_gain = setRunAddingGainHighpass_iir;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (highpass_iirDescriptor) {
		free((LADSPA_PortDescriptor *)highpass_iirDescriptor->PortDescriptors);
		free((char **)highpass_iirDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)highpass_iirDescriptor->PortRangeHints);
		free(highpass_iirDescriptor);
	}

}
