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

#line 10 "impulse_1885.xml"

#include "ladspa-util.h"

#define LOG001 -6.9077552789f

#define IMPULSE_FC_FREQUENCY           0
#define IMPULSE_FC_OUT                 1

static LADSPA_Descriptor *impulse_fcDescriptor = NULL;

typedef struct {
	LADSPA_Data *frequency;
	LADSPA_Data *out;
	float        phase;
	LADSPA_Data  sample_rate;
	LADSPA_Data run_adding_gain;
} Impulse_fc;

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
		return impulse_fcDescriptor;
	default:
		return NULL;
	}
}

static void activateImpulse_fc(LADSPA_Handle instance) {
	Impulse_fc *plugin_data = (Impulse_fc *)instance;
	float phase = plugin_data->phase;
	LADSPA_Data sample_rate = plugin_data->sample_rate;
#line 29 "impulse_1885.xml"
	phase = 0.f;
	plugin_data->phase = phase;
	plugin_data->sample_rate = sample_rate;

}

static void cleanupImpulse_fc(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortImpulse_fc(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Impulse_fc *plugin;

	plugin = (Impulse_fc *)instance;
	switch (port) {
	case IMPULSE_FC_FREQUENCY:
		plugin->frequency = data;
		break;
	case IMPULSE_FC_OUT:
		plugin->out = data;
		break;
	}
}

static LADSPA_Handle instantiateImpulse_fc(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Impulse_fc *plugin_data = (Impulse_fc *)malloc(sizeof(Impulse_fc));
	float phase;
	LADSPA_Data sample_rate;

#line 24 "impulse_1885.xml"
	sample_rate = s_rate;
	phase = 0.f;

	plugin_data->phase = phase;
	plugin_data->sample_rate = sample_rate;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runImpulse_fc(LADSPA_Handle instance, unsigned long sample_count) {
	Impulse_fc *plugin_data = (Impulse_fc *)instance;

	/* Frequency (Hz) (float value) */
	const LADSPA_Data frequency = *(plugin_data->frequency);

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const out = plugin_data->out;
	float phase = plugin_data->phase;
	LADSPA_Data sample_rate = plugin_data->sample_rate;

#line 33 "impulse_1885.xml"
	int i;
	float phase_step = frequency / sample_rate;

	for (i=0; i<sample_count; i++) {
	  if (phase > 1.f) {
	    phase -= 1.f;
	    buffer_write(out[i], 1.f);
	  } else {
	    buffer_write(out[i], 0.f);
	  }
	  phase += phase_step;
	}

	plugin_data->phase = phase;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainImpulse_fc(LADSPA_Handle instance, LADSPA_Data gain) {
	((Impulse_fc *)instance)->run_adding_gain = gain;
}

static void runAddingImpulse_fc(LADSPA_Handle instance, unsigned long sample_count) {
	Impulse_fc *plugin_data = (Impulse_fc *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Frequency (Hz) (float value) */
	const LADSPA_Data frequency = *(plugin_data->frequency);

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const out = plugin_data->out;
	float phase = plugin_data->phase;
	LADSPA_Data sample_rate = plugin_data->sample_rate;

#line 33 "impulse_1885.xml"
	int i;
	float phase_step = frequency / sample_rate;

	for (i=0; i<sample_count; i++) {
	  if (phase > 1.f) {
	    phase -= 1.f;
	    buffer_write(out[i], 1.f);
	  } else {
	    buffer_write(out[i], 0.f);
	  }
	  phase += phase_step;
	}

	plugin_data->phase = phase;
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


	impulse_fcDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (impulse_fcDescriptor) {
		impulse_fcDescriptor->UniqueID = 1885;
		impulse_fcDescriptor->Label = "impulse_fc";
		impulse_fcDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		impulse_fcDescriptor->Name =
		 D_("Nonbandlimited single-sample impulses (Frequency: Control)");
		impulse_fcDescriptor->Maker =
		 "Andy Wingo <wingo at pobox dot com>";
		impulse_fcDescriptor->Copyright =
		 "GPL";
		impulse_fcDescriptor->PortCount = 2;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(2,
		 sizeof(LADSPA_PortDescriptor));
		impulse_fcDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(2,
		 sizeof(LADSPA_PortRangeHint));
		impulse_fcDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(2, sizeof(char*));
		impulse_fcDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Frequency (Hz) */
		port_descriptors[IMPULSE_FC_FREQUENCY] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[IMPULSE_FC_FREQUENCY] =
		 D_("Frequency (Hz)");
		port_range_hints[IMPULSE_FC_FREQUENCY].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW;
		port_range_hints[IMPULSE_FC_FREQUENCY].LowerBound = 0;

		/* Parameters for Output */
		port_descriptors[IMPULSE_FC_OUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[IMPULSE_FC_OUT] =
		 D_("Output");
		port_range_hints[IMPULSE_FC_OUT].HintDescriptor = 0;

		impulse_fcDescriptor->activate = activateImpulse_fc;
		impulse_fcDescriptor->cleanup = cleanupImpulse_fc;
		impulse_fcDescriptor->connect_port = connectPortImpulse_fc;
		impulse_fcDescriptor->deactivate = NULL;
		impulse_fcDescriptor->instantiate = instantiateImpulse_fc;
		impulse_fcDescriptor->run = runImpulse_fc;
		impulse_fcDescriptor->run_adding = runAddingImpulse_fc;
		impulse_fcDescriptor->set_run_adding_gain = setRunAddingGainImpulse_fc;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (impulse_fcDescriptor) {
		free((LADSPA_PortDescriptor *)impulse_fcDescriptor->PortDescriptors);
		free((char **)impulse_fcDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)impulse_fcDescriptor->PortRangeHints);
		free(impulse_fcDescriptor);
	}

}
