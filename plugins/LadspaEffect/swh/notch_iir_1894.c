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

#define NOTCH_IIR_CENTER               0
#define NOTCH_IIR_WIDTH                1
#define NOTCH_IIR_STAGES               2
#define NOTCH_IIR_INPUT                3
#define NOTCH_IIR_OUTPUT               4

static LADSPA_Descriptor *notch_iirDescriptor = NULL;

typedef struct {
	LADSPA_Data *center;
	LADSPA_Data *width;
	LADSPA_Data *stages;
	LADSPA_Data *input;
	LADSPA_Data *output;
	iir_stage_t* first;
	iirf_t*      iirf1;
	iirf_t*      iirf2;
	float        lfc;
	long         sample_rate;
	iir_stage_t* second;
	float        ufc;
	LADSPA_Data run_adding_gain;
} Notch_iir;

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
		return notch_iirDescriptor;
	default:
		return NULL;
	}
}

static void activateNotch_iir(LADSPA_Handle instance) {
	Notch_iir *plugin_data = (Notch_iir *)instance;
	iir_stage_t*first = plugin_data->first;
	iirf_t*iirf1 = plugin_data->iirf1;
	iirf_t*iirf2 = plugin_data->iirf2;
	float lfc = plugin_data->lfc;
	long sample_rate = plugin_data->sample_rate;
	iir_stage_t*second = plugin_data->second;
	float ufc = plugin_data->ufc;
	
	ufc = (*(plugin_data->center) - *(plugin_data->width)*0.5f)/(float)sample_rate;
	lfc = (*(plugin_data->center) + *(plugin_data->width)*0.5f)/(float)sample_rate;
	first = init_iir_stage(IIR_STAGE_LOWPASS,10,3,2);
	second = init_iir_stage(IIR_STAGE_HIGHPASS,10,3,2);                  
	iirf1 = init_iirf_t(first);
	iirf2 = init_iirf_t(second);
	chebyshev(iirf1, first, 2*CLAMP((int)(*(plugin_data->stages)),1,10), IIR_STAGE_LOWPASS, ufc, 0.5f);
	chebyshev(iirf2, second, 2*CLAMP((int)(*(plugin_data->stages)),1,10), IIR_STAGE_HIGHPASS, lfc, 0.5f);
	plugin_data->first = first;
	plugin_data->iirf1 = iirf1;
	plugin_data->iirf2 = iirf2;
	plugin_data->lfc = lfc;
	plugin_data->sample_rate = sample_rate;
	plugin_data->second = second;
	plugin_data->ufc = ufc;

}

static void cleanupNotch_iir(LADSPA_Handle instance) {
	Notch_iir *plugin_data = (Notch_iir *)instance;
	free_iirf_t(plugin_data->iirf1, plugin_data->first);
	free_iirf_t(plugin_data->iirf2, plugin_data->second);
	free_iir_stage(plugin_data->first);
	free_iir_stage(plugin_data->second);
	free(instance);
}

static void connectPortNotch_iir(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Notch_iir *plugin;

	plugin = (Notch_iir *)instance;
	switch (port) {
	case NOTCH_IIR_CENTER:
		plugin->center = data;
		break;
	case NOTCH_IIR_WIDTH:
		plugin->width = data;
		break;
	case NOTCH_IIR_STAGES:
		plugin->stages = data;
		break;
	case NOTCH_IIR_INPUT:
		plugin->input = data;
		break;
	case NOTCH_IIR_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateNotch_iir(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Notch_iir *plugin_data = (Notch_iir *)malloc(sizeof(Notch_iir));
	iir_stage_t*first = NULL;
	iirf_t*iirf1 = NULL;
	iirf_t*iirf2 = NULL;
	float lfc;
	long sample_rate;
	iir_stage_t*second = NULL;
	float ufc;

	sample_rate = s_rate;
	ufc = lfc = 0.0f;

	plugin_data->first = first;
	plugin_data->iirf1 = iirf1;
	plugin_data->iirf2 = iirf2;
	plugin_data->lfc = lfc;
	plugin_data->sample_rate = sample_rate;
	plugin_data->second = second;
	plugin_data->ufc = ufc;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runNotch_iir(LADSPA_Handle instance, unsigned long sample_count) {
	Notch_iir *plugin_data = (Notch_iir *)instance;

	/* Center Frequency (Hz) (float value) */
	const LADSPA_Data center = *(plugin_data->center);

	/* Bandwidth (Hz) (float value) */
	const LADSPA_Data width = *(plugin_data->width);

	/* Stages(2 poles per stage) (float value) */
	const LADSPA_Data stages = *(plugin_data->stages);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	iir_stage_t* first = plugin_data->first;
	iirf_t* iirf1 = plugin_data->iirf1;
	iirf_t* iirf2 = plugin_data->iirf2;
	float lfc = plugin_data->lfc;
	long sample_rate = plugin_data->sample_rate;
	iir_stage_t* second = plugin_data->second;
	float ufc = plugin_data->ufc;

	ufc = (center - width*0.5f)/(float)sample_rate;
	lfc = (center + width*0.5f)/(float)sample_rate;
	chebyshev(iirf1, first,  2*CLAMP((int)stages,1,10), IIR_STAGE_LOWPASS,  ufc, 0.5f);
	chebyshev(iirf2, second, 2*CLAMP((int)stages,1,10), IIR_STAGE_HIGHPASS, lfc, 0.5f);
	iir_process_buffer_ns_5(iirf1, first, input, output, sample_count, RUN_ADDING);
	iir_process_buffer_ns_5(iirf2, second, input, output, sample_count, 1); /* add to first buffer */
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainNotch_iir(LADSPA_Handle instance, LADSPA_Data gain) {
	((Notch_iir *)instance)->run_adding_gain = gain;
}

static void runAddingNotch_iir(LADSPA_Handle instance, unsigned long sample_count) {
	Notch_iir *plugin_data = (Notch_iir *)instance;

	/* Center Frequency (Hz) (float value) */
	const LADSPA_Data center = *(plugin_data->center);

	/* Bandwidth (Hz) (float value) */
	const LADSPA_Data width = *(plugin_data->width);

	/* Stages(2 poles per stage) (float value) */
	const LADSPA_Data stages = *(plugin_data->stages);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	iir_stage_t* first = plugin_data->first;
	iirf_t* iirf1 = plugin_data->iirf1;
	iirf_t* iirf2 = plugin_data->iirf2;
	float lfc = plugin_data->lfc;
	long sample_rate = plugin_data->sample_rate;
	iir_stage_t* second = plugin_data->second;
	float ufc = plugin_data->ufc;

	ufc = (center - width*0.5f)/(float)sample_rate;
	lfc = (center + width*0.5f)/(float)sample_rate;
	chebyshev(iirf1, first,  2*CLAMP((int)stages,1,10), IIR_STAGE_LOWPASS,  ufc, 0.5f);
	chebyshev(iirf2, second, 2*CLAMP((int)stages,1,10), IIR_STAGE_HIGHPASS, lfc, 0.5f);
	iir_process_buffer_ns_5(iirf1, first, input, output, sample_count, RUN_ADDING);
	iir_process_buffer_ns_5(iirf2, second, input, output, sample_count, 1); /* add to first buffer */
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


	notch_iirDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (notch_iirDescriptor) {
		notch_iirDescriptor->UniqueID = 1894;
		notch_iirDescriptor->Label = "notch_iir";
		notch_iirDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		notch_iirDescriptor->Name =
		 D_("Mag's Notch Filter");
		notch_iirDescriptor->Maker =
		 "Alexander Ehlert <mag@glame.de>";
		notch_iirDescriptor->Copyright =
		 "GPL";
		notch_iirDescriptor->PortCount = 5;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(5,
		 sizeof(LADSPA_PortDescriptor));
		notch_iirDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(5,
		 sizeof(LADSPA_PortRangeHint));
		notch_iirDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(5, sizeof(char*));
		notch_iirDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Center Frequency (Hz) */
		port_descriptors[NOTCH_IIR_CENTER] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[NOTCH_IIR_CENTER] =
		 D_("Center Frequency (Hz)");
		port_range_hints[NOTCH_IIR_CENTER].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_LOGARITHMIC;
		port_range_hints[NOTCH_IIR_CENTER].LowerBound = 0.0001;
		port_range_hints[NOTCH_IIR_CENTER].UpperBound = 0.45;

		/* Parameters for Bandwidth (Hz) */
		port_descriptors[NOTCH_IIR_WIDTH] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[NOTCH_IIR_WIDTH] =
		 D_("Bandwidth (Hz)");
		port_range_hints[NOTCH_IIR_WIDTH].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_LOGARITHMIC;
		port_range_hints[NOTCH_IIR_WIDTH].LowerBound = 0.0001;
		port_range_hints[NOTCH_IIR_WIDTH].UpperBound = 0.45;

		/* Parameters for Stages(2 poles per stage) */
		port_descriptors[NOTCH_IIR_STAGES] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[NOTCH_IIR_STAGES] =
		 D_("Stages(2 poles per stage)");
		port_range_hints[NOTCH_IIR_STAGES].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1 | LADSPA_HINT_INTEGER;
		port_range_hints[NOTCH_IIR_STAGES].LowerBound = 1.0;
		port_range_hints[NOTCH_IIR_STAGES].UpperBound = 10.0;

		/* Parameters for Input */
		port_descriptors[NOTCH_IIR_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[NOTCH_IIR_INPUT] =
		 D_("Input");
		port_range_hints[NOTCH_IIR_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[NOTCH_IIR_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[NOTCH_IIR_OUTPUT] =
		 D_("Output");
		port_range_hints[NOTCH_IIR_OUTPUT].HintDescriptor = 0;

		notch_iirDescriptor->activate = activateNotch_iir;
		notch_iirDescriptor->cleanup = cleanupNotch_iir;
		notch_iirDescriptor->connect_port = connectPortNotch_iir;
		notch_iirDescriptor->deactivate = NULL;
		notch_iirDescriptor->instantiate = instantiateNotch_iir;
		notch_iirDescriptor->run = runNotch_iir;
		notch_iirDescriptor->run_adding = runAddingNotch_iir;
		notch_iirDescriptor->set_run_adding_gain = setRunAddingGainNotch_iir;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (notch_iirDescriptor) {
		free((LADSPA_PortDescriptor *)notch_iirDescriptor->PortDescriptors);
		free((char **)notch_iirDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)notch_iirDescriptor->PortRangeHints);
		free(notch_iirDescriptor);
	}

}
