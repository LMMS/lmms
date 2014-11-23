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
#include "util/buffer.h"

#define BWXOVER_IIR_CUTOFF             0
#define BWXOVER_IIR_RESONANCE          1
#define BWXOVER_IIR_INPUT              2
#define BWXOVER_IIR_LPOUTPUT           3
#define BWXOVER_IIR_HPOUTPUT           4
#define BUTTLOW_IIR_CUTOFF             0
#define BUTTLOW_IIR_RESONANCE          1
#define BUTTLOW_IIR_INPUT              2
#define BUTTLOW_IIR_OUTPUT             3
#define BUTTHIGH_IIR_CUTOFF            0
#define BUTTHIGH_IIR_RESONANCE         1
#define BUTTHIGH_IIR_INPUT             2
#define BUTTHIGH_IIR_OUTPUT            3

static LADSPA_Descriptor *bwxover_iirDescriptor = NULL;

typedef struct {
	LADSPA_Data *cutoff;
	LADSPA_Data *resonance;
	LADSPA_Data *input;
	LADSPA_Data *lpoutput;
	LADSPA_Data *hpoutput;
	iir_stage_t* gt;
	iirf_t*      iirf;
	long         sample_rate;
	LADSPA_Data run_adding_gain;
} Bwxover_iir;

static LADSPA_Descriptor *buttlow_iirDescriptor = NULL;

typedef struct {
	LADSPA_Data *cutoff;
	LADSPA_Data *resonance;
	LADSPA_Data *input;
	LADSPA_Data *output;
	iir_stage_t* gt;
	iirf_t*      iirf;
	long         sample_rate;
	LADSPA_Data run_adding_gain;
} Buttlow_iir;

static LADSPA_Descriptor *butthigh_iirDescriptor = NULL;

typedef struct {
	LADSPA_Data *cutoff;
	LADSPA_Data *resonance;
	LADSPA_Data *input;
	LADSPA_Data *output;
	iir_stage_t* gt;
	iirf_t*      iirf;
	long         sample_rate;
	LADSPA_Data run_adding_gain;
} Butthigh_iir;

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
		return bwxover_iirDescriptor;
	case 1:
		return buttlow_iirDescriptor;
	case 2:
		return butthigh_iirDescriptor;
	default:
		return NULL;
	}
}

static void activateBwxover_iir(LADSPA_Handle instance) {
	Bwxover_iir *plugin_data = (Bwxover_iir *)instance;
	iir_stage_t*gt = plugin_data->gt;
	iirf_t*iirf = plugin_data->iirf;
	long sample_rate = plugin_data->sample_rate;
	
	gt = init_iir_stage(IIR_STAGE_LOWPASS,1,3,2);
	iirf = init_iirf_t(gt);
	butterworth_stage(gt, 0, *(plugin_data->cutoff), 
	                           *(plugin_data->resonance), 
	                         sample_rate);
	plugin_data->gt = gt;
	plugin_data->iirf = iirf;
	plugin_data->sample_rate = sample_rate;

}

static void cleanupBwxover_iir(LADSPA_Handle instance) {
	Bwxover_iir *plugin_data = (Bwxover_iir *)instance;
	free_iirf_t(plugin_data->iirf, plugin_data->gt);
	free_iir_stage(plugin_data->gt);
	free(instance);
}

static void connectPortBwxover_iir(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Bwxover_iir *plugin;

	plugin = (Bwxover_iir *)instance;
	switch (port) {
	case BWXOVER_IIR_CUTOFF:
		plugin->cutoff = data;
		break;
	case BWXOVER_IIR_RESONANCE:
		plugin->resonance = data;
		break;
	case BWXOVER_IIR_INPUT:
		plugin->input = data;
		break;
	case BWXOVER_IIR_LPOUTPUT:
		plugin->lpoutput = data;
		break;
	case BWXOVER_IIR_HPOUTPUT:
		plugin->hpoutput = data;
		break;
	}
}

static LADSPA_Handle instantiateBwxover_iir(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Bwxover_iir *plugin_data = (Bwxover_iir *)malloc(sizeof(Bwxover_iir));
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

static void runBwxover_iir(LADSPA_Handle instance, unsigned long sample_count) {
	Bwxover_iir *plugin_data = (Bwxover_iir *)instance;

	/* Cutoff Frequency (Hz) (float value) */
	const LADSPA_Data cutoff = *(plugin_data->cutoff);

	/* Resonance (float value) */
	const LADSPA_Data resonance = *(plugin_data->resonance);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* LP-Output (array of floats of length sample_count) */
	LADSPA_Data * const lpoutput = plugin_data->lpoutput;

	/* HP-Output (array of floats of length sample_count) */
	LADSPA_Data * const hpoutput = plugin_data->hpoutput;
	iir_stage_t* gt = plugin_data->gt;
	iirf_t* iirf = plugin_data->iirf;
	long sample_rate = plugin_data->sample_rate;

	butterworth_stage(gt, 0, cutoff, resonance, sample_rate);
	iir_process_buffer_1s_5(iirf, gt, input, lpoutput, sample_count,0);
	buffer_sub(input, lpoutput, hpoutput, sample_count);
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainBwxover_iir(LADSPA_Handle instance, LADSPA_Data gain) {
	((Bwxover_iir *)instance)->run_adding_gain = gain;
}

static void runAddingBwxover_iir(LADSPA_Handle instance, unsigned long sample_count) {
	Bwxover_iir *plugin_data = (Bwxover_iir *)instance;

	/* Cutoff Frequency (Hz) (float value) */
	const LADSPA_Data cutoff = *(plugin_data->cutoff);

	/* Resonance (float value) */
	const LADSPA_Data resonance = *(plugin_data->resonance);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* LP-Output (array of floats of length sample_count) */
	LADSPA_Data * const lpoutput = plugin_data->lpoutput;

	/* HP-Output (array of floats of length sample_count) */
	LADSPA_Data * const hpoutput = plugin_data->hpoutput;
	iir_stage_t* gt = plugin_data->gt;
	iirf_t* iirf = plugin_data->iirf;
	long sample_rate = plugin_data->sample_rate;

	butterworth_stage(gt, 0, cutoff, resonance, sample_rate);
	iir_process_buffer_1s_5(iirf, gt, input, lpoutput, sample_count,0);
	buffer_sub(input, lpoutput, hpoutput, sample_count);
}

static void activateButtlow_iir(LADSPA_Handle instance) {
	Buttlow_iir *plugin_data = (Buttlow_iir *)instance;
	iir_stage_t*gt = plugin_data->gt;
	iirf_t*iirf = plugin_data->iirf;
	long sample_rate = plugin_data->sample_rate;
	
	gt = init_iir_stage(IIR_STAGE_LOWPASS,1,3,2);
	iirf = init_iirf_t(gt);
	butterworth_stage(gt, 0, *(plugin_data->cutoff), 
	                           *(plugin_data->resonance), 
	                         sample_rate);
	plugin_data->gt = gt;
	plugin_data->iirf = iirf;
	plugin_data->sample_rate = sample_rate;

}

static void cleanupButtlow_iir(LADSPA_Handle instance) {
	Buttlow_iir *plugin_data = (Buttlow_iir *)instance;
	free_iirf_t(plugin_data->iirf, plugin_data->gt);
	free_iir_stage(plugin_data->gt);
	free(instance);
}

static void connectPortButtlow_iir(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Buttlow_iir *plugin;

	plugin = (Buttlow_iir *)instance;
	switch (port) {
	case BUTTLOW_IIR_CUTOFF:
		plugin->cutoff = data;
		break;
	case BUTTLOW_IIR_RESONANCE:
		plugin->resonance = data;
		break;
	case BUTTLOW_IIR_INPUT:
		plugin->input = data;
		break;
	case BUTTLOW_IIR_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateButtlow_iir(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Buttlow_iir *plugin_data = (Buttlow_iir *)malloc(sizeof(Buttlow_iir));
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

static void runButtlow_iir(LADSPA_Handle instance, unsigned long sample_count) {
	Buttlow_iir *plugin_data = (Buttlow_iir *)instance;

	/* Cutoff Frequency (Hz) (float value) */
	const LADSPA_Data cutoff = *(plugin_data->cutoff);

	/* Resonance (float value) */
	const LADSPA_Data resonance = *(plugin_data->resonance);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	iir_stage_t* gt = plugin_data->gt;
	iirf_t* iirf = plugin_data->iirf;
	long sample_rate = plugin_data->sample_rate;

	butterworth_stage(gt, 0, cutoff, resonance, sample_rate);
	iir_process_buffer_1s_5(iirf, gt, input, output, sample_count,0);
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainButtlow_iir(LADSPA_Handle instance, LADSPA_Data gain) {
	((Buttlow_iir *)instance)->run_adding_gain = gain;
}

static void runAddingButtlow_iir(LADSPA_Handle instance, unsigned long sample_count) {
	Buttlow_iir *plugin_data = (Buttlow_iir *)instance;

	/* Cutoff Frequency (Hz) (float value) */
	const LADSPA_Data cutoff = *(plugin_data->cutoff);

	/* Resonance (float value) */
	const LADSPA_Data resonance = *(plugin_data->resonance);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	iir_stage_t* gt = plugin_data->gt;
	iirf_t* iirf = plugin_data->iirf;
	long sample_rate = plugin_data->sample_rate;

	butterworth_stage(gt, 0, cutoff, resonance, sample_rate);
	iir_process_buffer_1s_5(iirf, gt, input, output, sample_count,0);
}

static void activateButthigh_iir(LADSPA_Handle instance) {
	Butthigh_iir *plugin_data = (Butthigh_iir *)instance;
	iir_stage_t*gt = plugin_data->gt;
	iirf_t*iirf = plugin_data->iirf;
	long sample_rate = plugin_data->sample_rate;
	
	gt = init_iir_stage(IIR_STAGE_LOWPASS,1,3,2);
	iirf = init_iirf_t(gt);
	butterworth_stage(gt, 1, *(plugin_data->cutoff), 
	                           *(plugin_data->resonance), 
	                         sample_rate);
	plugin_data->gt = gt;
	plugin_data->iirf = iirf;
	plugin_data->sample_rate = sample_rate;

}

static void cleanupButthigh_iir(LADSPA_Handle instance) {
	Butthigh_iir *plugin_data = (Butthigh_iir *)instance;
	free_iirf_t(plugin_data->iirf, plugin_data->gt);
	free_iir_stage(plugin_data->gt);
	free(instance);
}

static void connectPortButthigh_iir(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Butthigh_iir *plugin;

	plugin = (Butthigh_iir *)instance;
	switch (port) {
	case BUTTHIGH_IIR_CUTOFF:
		plugin->cutoff = data;
		break;
	case BUTTHIGH_IIR_RESONANCE:
		plugin->resonance = data;
		break;
	case BUTTHIGH_IIR_INPUT:
		plugin->input = data;
		break;
	case BUTTHIGH_IIR_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateButthigh_iir(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Butthigh_iir *plugin_data = (Butthigh_iir *)malloc(sizeof(Butthigh_iir));
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

static void runButthigh_iir(LADSPA_Handle instance, unsigned long sample_count) {
	Butthigh_iir *plugin_data = (Butthigh_iir *)instance;

	/* Cutoff Frequency (Hz) (float value) */
	const LADSPA_Data cutoff = *(plugin_data->cutoff);

	/* Resonance (float value) */
	const LADSPA_Data resonance = *(plugin_data->resonance);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	iir_stage_t* gt = plugin_data->gt;
	iirf_t* iirf = plugin_data->iirf;
	long sample_rate = plugin_data->sample_rate;

	butterworth_stage(gt, 1, cutoff, resonance, sample_rate);
	iir_process_buffer_1s_5(iirf, gt, input, output, sample_count,0);
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainButthigh_iir(LADSPA_Handle instance, LADSPA_Data gain) {
	((Butthigh_iir *)instance)->run_adding_gain = gain;
}

static void runAddingButthigh_iir(LADSPA_Handle instance, unsigned long sample_count) {
	Butthigh_iir *plugin_data = (Butthigh_iir *)instance;

	/* Cutoff Frequency (Hz) (float value) */
	const LADSPA_Data cutoff = *(plugin_data->cutoff);

	/* Resonance (float value) */
	const LADSPA_Data resonance = *(plugin_data->resonance);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	iir_stage_t* gt = plugin_data->gt;
	iirf_t* iirf = plugin_data->iirf;
	long sample_rate = plugin_data->sample_rate;

	butterworth_stage(gt, 1, cutoff, resonance, sample_rate);
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


	bwxover_iirDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (bwxover_iirDescriptor) {
		bwxover_iirDescriptor->UniqueID = 1902;
		bwxover_iirDescriptor->Label = "bwxover_iir";
		bwxover_iirDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		bwxover_iirDescriptor->Name =
		 D_("Glame Butterworth X-over Filter");
		bwxover_iirDescriptor->Maker =
		 "Alexander Ehlert <mag@glame.de>";
		bwxover_iirDescriptor->Copyright =
		 "GPL";
		bwxover_iirDescriptor->PortCount = 5;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(5,
		 sizeof(LADSPA_PortDescriptor));
		bwxover_iirDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(5,
		 sizeof(LADSPA_PortRangeHint));
		bwxover_iirDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(5, sizeof(char*));
		bwxover_iirDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Cutoff Frequency (Hz) */
		port_descriptors[BWXOVER_IIR_CUTOFF] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[BWXOVER_IIR_CUTOFF] =
		 D_("Cutoff Frequency (Hz)");
		port_range_hints[BWXOVER_IIR_CUTOFF].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_LOGARITHMIC;
		port_range_hints[BWXOVER_IIR_CUTOFF].LowerBound = 0.0001;
		port_range_hints[BWXOVER_IIR_CUTOFF].UpperBound = 0.45;

		/* Parameters for Resonance */
		port_descriptors[BWXOVER_IIR_RESONANCE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[BWXOVER_IIR_RESONANCE] =
		 D_("Resonance");
		port_range_hints[BWXOVER_IIR_RESONANCE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[BWXOVER_IIR_RESONANCE].LowerBound = 0.1;
		port_range_hints[BWXOVER_IIR_RESONANCE].UpperBound = 1.41;

		/* Parameters for Input */
		port_descriptors[BWXOVER_IIR_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[BWXOVER_IIR_INPUT] =
		 D_("Input");
		port_range_hints[BWXOVER_IIR_INPUT].HintDescriptor = 0;

		/* Parameters for LP-Output */
		port_descriptors[BWXOVER_IIR_LPOUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[BWXOVER_IIR_LPOUTPUT] =
		 D_("LP-Output");
		port_range_hints[BWXOVER_IIR_LPOUTPUT].HintDescriptor = 0;

		/* Parameters for HP-Output */
		port_descriptors[BWXOVER_IIR_HPOUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[BWXOVER_IIR_HPOUTPUT] =
		 D_("HP-Output");
		port_range_hints[BWXOVER_IIR_HPOUTPUT].HintDescriptor = 0;

		bwxover_iirDescriptor->activate = activateBwxover_iir;
		bwxover_iirDescriptor->cleanup = cleanupBwxover_iir;
		bwxover_iirDescriptor->connect_port = connectPortBwxover_iir;
		bwxover_iirDescriptor->deactivate = NULL;
		bwxover_iirDescriptor->instantiate = instantiateBwxover_iir;
		bwxover_iirDescriptor->run = runBwxover_iir;
		bwxover_iirDescriptor->run_adding = runAddingBwxover_iir;
		bwxover_iirDescriptor->set_run_adding_gain = setRunAddingGainBwxover_iir;
	}

	buttlow_iirDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (buttlow_iirDescriptor) {
		buttlow_iirDescriptor->UniqueID = 1903;
		buttlow_iirDescriptor->Label = "buttlow_iir";
		buttlow_iirDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		buttlow_iirDescriptor->Name =
		 D_("GLAME Butterworth Lowpass");
		buttlow_iirDescriptor->Maker =
		 "Alexander Ehlert <mag@glame.de>";
		buttlow_iirDescriptor->Copyright =
		 "GPL";
		buttlow_iirDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		buttlow_iirDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		buttlow_iirDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		buttlow_iirDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Cutoff Frequency (Hz) */
		port_descriptors[BUTTLOW_IIR_CUTOFF] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[BUTTLOW_IIR_CUTOFF] =
		 D_("Cutoff Frequency (Hz)");
		port_range_hints[BUTTLOW_IIR_CUTOFF].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_LOGARITHMIC;
		port_range_hints[BUTTLOW_IIR_CUTOFF].LowerBound = 0.0001;
		port_range_hints[BUTTLOW_IIR_CUTOFF].UpperBound = 0.45;

		/* Parameters for Resonance */
		port_descriptors[BUTTLOW_IIR_RESONANCE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[BUTTLOW_IIR_RESONANCE] =
		 D_("Resonance");
		port_range_hints[BUTTLOW_IIR_RESONANCE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[BUTTLOW_IIR_RESONANCE].LowerBound = 0.1;
		port_range_hints[BUTTLOW_IIR_RESONANCE].UpperBound = 1.41;

		/* Parameters for Input */
		port_descriptors[BUTTLOW_IIR_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[BUTTLOW_IIR_INPUT] =
		 D_("Input");
		port_range_hints[BUTTLOW_IIR_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[BUTTLOW_IIR_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[BUTTLOW_IIR_OUTPUT] =
		 D_("Output");
		port_range_hints[BUTTLOW_IIR_OUTPUT].HintDescriptor = 0;

		buttlow_iirDescriptor->activate = activateButtlow_iir;
		buttlow_iirDescriptor->cleanup = cleanupButtlow_iir;
		buttlow_iirDescriptor->connect_port = connectPortButtlow_iir;
		buttlow_iirDescriptor->deactivate = NULL;
		buttlow_iirDescriptor->instantiate = instantiateButtlow_iir;
		buttlow_iirDescriptor->run = runButtlow_iir;
		buttlow_iirDescriptor->run_adding = runAddingButtlow_iir;
		buttlow_iirDescriptor->set_run_adding_gain = setRunAddingGainButtlow_iir;
	}

	butthigh_iirDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (butthigh_iirDescriptor) {
		butthigh_iirDescriptor->UniqueID = 1904;
		butthigh_iirDescriptor->Label = "butthigh_iir";
		butthigh_iirDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		butthigh_iirDescriptor->Name =
		 D_("GLAME Butterworth Highpass");
		butthigh_iirDescriptor->Maker =
		 "Alexander Ehlert <mag@glame.de>";
		butthigh_iirDescriptor->Copyright =
		 "GPL";
		butthigh_iirDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		butthigh_iirDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		butthigh_iirDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		butthigh_iirDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Cutoff Frequency (Hz) */
		port_descriptors[BUTTHIGH_IIR_CUTOFF] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[BUTTHIGH_IIR_CUTOFF] =
		 D_("Cutoff Frequency (Hz)");
		port_range_hints[BUTTHIGH_IIR_CUTOFF].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_LOGARITHMIC;
		port_range_hints[BUTTHIGH_IIR_CUTOFF].LowerBound = 0.0001;
		port_range_hints[BUTTHIGH_IIR_CUTOFF].UpperBound = 0.45;

		/* Parameters for Resonance */
		port_descriptors[BUTTHIGH_IIR_RESONANCE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[BUTTHIGH_IIR_RESONANCE] =
		 D_("Resonance");
		port_range_hints[BUTTHIGH_IIR_RESONANCE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[BUTTHIGH_IIR_RESONANCE].LowerBound = 0.1;
		port_range_hints[BUTTHIGH_IIR_RESONANCE].UpperBound = 1.41;

		/* Parameters for Input */
		port_descriptors[BUTTHIGH_IIR_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[BUTTHIGH_IIR_INPUT] =
		 D_("Input");
		port_range_hints[BUTTHIGH_IIR_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[BUTTHIGH_IIR_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[BUTTHIGH_IIR_OUTPUT] =
		 D_("Output");
		port_range_hints[BUTTHIGH_IIR_OUTPUT].HintDescriptor = 0;

		butthigh_iirDescriptor->activate = activateButthigh_iir;
		butthigh_iirDescriptor->cleanup = cleanupButthigh_iir;
		butthigh_iirDescriptor->connect_port = connectPortButthigh_iir;
		butthigh_iirDescriptor->deactivate = NULL;
		butthigh_iirDescriptor->instantiate = instantiateButthigh_iir;
		butthigh_iirDescriptor->run = runButthigh_iir;
		butthigh_iirDescriptor->run_adding = runAddingButthigh_iir;
		butthigh_iirDescriptor->set_run_adding_gain = setRunAddingGainButthigh_iir;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (bwxover_iirDescriptor) {
		free((LADSPA_PortDescriptor *)bwxover_iirDescriptor->PortDescriptors);
		free((char **)bwxover_iirDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)bwxover_iirDescriptor->PortRangeHints);
		free(bwxover_iirDescriptor);
	}
	if (buttlow_iirDescriptor) {
		free((LADSPA_PortDescriptor *)buttlow_iirDescriptor->PortDescriptors);
		free((char **)buttlow_iirDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)buttlow_iirDescriptor->PortRangeHints);
		free(buttlow_iirDescriptor);
	}
	if (butthigh_iirDescriptor) {
		free((LADSPA_PortDescriptor *)butthigh_iirDescriptor->PortDescriptors);
		free((char **)butthigh_iirDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)butthigh_iirDescriptor->PortRangeHints);
		free(butthigh_iirDescriptor);
	}

}
