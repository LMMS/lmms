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

#line 9 "pitch_scale_1194.xml"

#include "util/pitchscale.h"

#define FRAME_LENGTH 4096
#define OVER_SAMP    16

#define PITCHSCALEHQ_MULT              0
#define PITCHSCALEHQ_INPUT             1
#define PITCHSCALEHQ_OUTPUT            2
#define PITCHSCALEHQ_LATENCY           3

static LADSPA_Descriptor *pitchScaleHQDescriptor = NULL;

typedef struct {
	LADSPA_Data *mult;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data *latency;
	sbuffers *   buffers;
	long         sample_rate;
	LADSPA_Data run_adding_gain;
} PitchScaleHQ;

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
		return pitchScaleHQDescriptor;
	default:
		return NULL;
	}
}

static void activatePitchScaleHQ(LADSPA_Handle instance) {
	PitchScaleHQ *plugin_data = (PitchScaleHQ *)instance;
	sbuffers *buffers = plugin_data->buffers;
	long sample_rate = plugin_data->sample_rate;
#line 57 "pitch_scale_1194.xml"
	memset(buffers->gInFIFO, 0, FRAME_LENGTH*sizeof(float));
	memset(buffers->gOutFIFO, 0, FRAME_LENGTH*sizeof(float));
	memset(buffers->gLastPhase, 0, FRAME_LENGTH*sizeof(float)/2);
	memset(buffers->gSumPhase, 0, FRAME_LENGTH*sizeof(float)/2);
	memset(buffers->gOutputAccum, 0, 2*FRAME_LENGTH*sizeof(float));
	memset(buffers->gAnaFreq, 0, FRAME_LENGTH*sizeof(float));
	memset(buffers->gAnaMagn, 0, FRAME_LENGTH*sizeof(float));
	buffers->gRover = 0;
	pitch_scale(buffers, 1.0, FRAME_LENGTH, 16, FRAME_LENGTH, sample_rate, buffers->gInFIFO, buffers->gOutFIFO, 0, 0.0f);
	plugin_data->buffers = buffers;
	plugin_data->sample_rate = sample_rate;

}

static void cleanupPitchScaleHQ(LADSPA_Handle instance) {
#line 69 "pitch_scale_1194.xml"
	PitchScaleHQ *plugin_data = (PitchScaleHQ *)instance;
	free (plugin_data->buffers->gInFIFO);
	free (plugin_data->buffers->gOutFIFO);
	free (plugin_data->buffers->gLastPhase);
	free (plugin_data->buffers->gSumPhase);
	free (plugin_data->buffers->gOutputAccum);
	free (plugin_data->buffers->gAnaFreq);
	free (plugin_data->buffers->gAnaMagn);
	free (plugin_data->buffers->gSynFreq);
	free (plugin_data->buffers->gSynMagn);
	free (plugin_data->buffers->gWindow);
	free (plugin_data->buffers);
	free(instance);
}

static void connectPortPitchScaleHQ(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	PitchScaleHQ *plugin;

	plugin = (PitchScaleHQ *)instance;
	switch (port) {
	case PITCHSCALEHQ_MULT:
		plugin->mult = data;
		break;
	case PITCHSCALEHQ_INPUT:
		plugin->input = data;
		break;
	case PITCHSCALEHQ_OUTPUT:
		plugin->output = data;
		break;
	case PITCHSCALEHQ_LATENCY:
		plugin->latency = data;
		break;
	}
}

static LADSPA_Handle instantiatePitchScaleHQ(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	PitchScaleHQ *plugin_data = (PitchScaleHQ *)malloc(sizeof(PitchScaleHQ));
	sbuffers *buffers = NULL;
	long sample_rate;

#line 29 "pitch_scale_1194.xml"
	int i;
	float arg;

	buffers = malloc(sizeof(sbuffers));
	sample_rate = s_rate;
	buffers->gInFIFO = malloc(FRAME_LENGTH * sizeof(float));
	buffers->gOutFIFO = malloc(FRAME_LENGTH * sizeof(float));
	buffers->gLastPhase = malloc(FRAME_LENGTH * sizeof(float));
	buffers->gSumPhase = malloc(FRAME_LENGTH * sizeof(float));
	buffers->gOutputAccum = malloc(2*FRAME_LENGTH * sizeof(float));
	buffers->gAnaFreq = malloc(FRAME_LENGTH * sizeof(float));
	buffers->gAnaMagn = malloc(FRAME_LENGTH * sizeof(float));
	buffers->gSynFreq = malloc(FRAME_LENGTH * sizeof(float));
	buffers->gSynMagn = malloc(FRAME_LENGTH * sizeof(float));
	buffers->gWindow = malloc(FRAME_LENGTH * sizeof(float));

	arg = 2.0f * M_PI / (float)(FRAME_LENGTH-1);
	for (i=0; i < FRAME_LENGTH; i++) {
	        // Blackman-Harris
	        buffers->gWindow[i] =  0.35875f - 0.48829f * cos(arg * (float)i) + 0.14128f * cos(2.0f * arg * (float)i) - 0.01168f * cos(3.0f * arg * (float)i);
	        // Gain correction
	        buffers->gWindow[i] *= 0.761f;

	}

	plugin_data->buffers = buffers;
	plugin_data->sample_rate = sample_rate;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runPitchScaleHQ(LADSPA_Handle instance, unsigned long sample_count) {
	PitchScaleHQ *plugin_data = (PitchScaleHQ *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Pitch co-efficient (float value) */
	const LADSPA_Data mult = *(plugin_data->mult);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	sbuffers * buffers = plugin_data->buffers;
	long sample_rate = plugin_data->sample_rate;

#line 23 "pitch_scale_1194.xml"
	pitch_scale(buffers, mult, FRAME_LENGTH, OVER_SAMP, sample_count, sample_rate, input, output, RUN_ADDING, run_adding_gain);
	*(plugin_data->latency) = FRAME_LENGTH - (FRAME_LENGTH
	                                / OVER_SAMP);
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainPitchScaleHQ(LADSPA_Handle instance, LADSPA_Data gain) {
	((PitchScaleHQ *)instance)->run_adding_gain = gain;
}

static void runAddingPitchScaleHQ(LADSPA_Handle instance, unsigned long sample_count) {
	PitchScaleHQ *plugin_data = (PitchScaleHQ *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Pitch co-efficient (float value) */
	const LADSPA_Data mult = *(plugin_data->mult);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	sbuffers * buffers = plugin_data->buffers;
	long sample_rate = plugin_data->sample_rate;

#line 23 "pitch_scale_1194.xml"
	pitch_scale(buffers, mult, FRAME_LENGTH, OVER_SAMP, sample_count, sample_rate, input, output, RUN_ADDING, run_adding_gain);
	*(plugin_data->latency) = FRAME_LENGTH - (FRAME_LENGTH
	                                / OVER_SAMP);
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


	pitchScaleHQDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (pitchScaleHQDescriptor) {
		pitchScaleHQDescriptor->UniqueID = 1194;
		pitchScaleHQDescriptor->Label = "pitchScaleHQ";
		pitchScaleHQDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		pitchScaleHQDescriptor->Name =
		 D_("Higher Quality Pitch Scaler");
		pitchScaleHQDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		pitchScaleHQDescriptor->Copyright =
		 "GPL";
		pitchScaleHQDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		pitchScaleHQDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		pitchScaleHQDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		pitchScaleHQDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Pitch co-efficient */
		port_descriptors[PITCHSCALEHQ_MULT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[PITCHSCALEHQ_MULT] =
		 D_("Pitch co-efficient");
		port_range_hints[PITCHSCALEHQ_MULT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[PITCHSCALEHQ_MULT].LowerBound = 0.5;
		port_range_hints[PITCHSCALEHQ_MULT].UpperBound = 2;

		/* Parameters for Input */
		port_descriptors[PITCHSCALEHQ_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[PITCHSCALEHQ_INPUT] =
		 D_("Input");
		port_range_hints[PITCHSCALEHQ_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[PITCHSCALEHQ_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[PITCHSCALEHQ_OUTPUT] =
		 D_("Output");
		port_range_hints[PITCHSCALEHQ_OUTPUT].HintDescriptor = 0;

		/* Parameters for latency */
		port_descriptors[PITCHSCALEHQ_LATENCY] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
		port_names[PITCHSCALEHQ_LATENCY] =
		 D_("latency");
		port_range_hints[PITCHSCALEHQ_LATENCY].HintDescriptor = 0;

		pitchScaleHQDescriptor->activate = activatePitchScaleHQ;
		pitchScaleHQDescriptor->cleanup = cleanupPitchScaleHQ;
		pitchScaleHQDescriptor->connect_port = connectPortPitchScaleHQ;
		pitchScaleHQDescriptor->deactivate = NULL;
		pitchScaleHQDescriptor->instantiate = instantiatePitchScaleHQ;
		pitchScaleHQDescriptor->run = runPitchScaleHQ;
		pitchScaleHQDescriptor->run_adding = runAddingPitchScaleHQ;
		pitchScaleHQDescriptor->set_run_adding_gain = setRunAddingGainPitchScaleHQ;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (pitchScaleHQDescriptor) {
		free((LADSPA_PortDescriptor *)pitchScaleHQDescriptor->PortDescriptors);
		free((char **)pitchScaleHQDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)pitchScaleHQDescriptor->PortRangeHints);
		free(pitchScaleHQDescriptor);
	}

}
