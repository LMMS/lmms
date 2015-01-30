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

#line 10 "freq_tracker_1418.xml"

#include "ladspa-util.h"

#define FREQTRACKER_SPEED              0
#define FREQTRACKER_INPUT              1
#define FREQTRACKER_FREQ               2

static LADSPA_Descriptor *freqTrackerDescriptor = NULL;

typedef struct {
	LADSPA_Data *speed;
	LADSPA_Data *input;
	LADSPA_Data *freq;
	int          cross_time;
	LADSPA_Data  f;
	LADSPA_Data  fo;
	float        fs;
	LADSPA_Data  last_amp;
	LADSPA_Data run_adding_gain;
} FreqTracker;

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
		return freqTrackerDescriptor;
	default:
		return NULL;
	}
}

static void activateFreqTracker(LADSPA_Handle instance) {
	FreqTracker *plugin_data = (FreqTracker *)instance;
	int cross_time = plugin_data->cross_time;
	LADSPA_Data f = plugin_data->f;
	LADSPA_Data fo = plugin_data->fo;
	float fs = plugin_data->fs;
	LADSPA_Data last_amp = plugin_data->last_amp;
#line 27 "freq_tracker_1418.xml"
	cross_time = 0;
	f = 0.0f;
	fo = 0.0f;
	last_amp = 0.0f;
	plugin_data->cross_time = cross_time;
	plugin_data->f = f;
	plugin_data->fo = fo;
	plugin_data->fs = fs;
	plugin_data->last_amp = last_amp;

}

static void cleanupFreqTracker(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortFreqTracker(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	FreqTracker *plugin;

	plugin = (FreqTracker *)instance;
	switch (port) {
	case FREQTRACKER_SPEED:
		plugin->speed = data;
		break;
	case FREQTRACKER_INPUT:
		plugin->input = data;
		break;
	case FREQTRACKER_FREQ:
		plugin->freq = data;
		break;
	}
}

static LADSPA_Handle instantiateFreqTracker(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	FreqTracker *plugin_data = (FreqTracker *)malloc(sizeof(FreqTracker));
	int cross_time;
	LADSPA_Data f;
	LADSPA_Data fo;
	float fs;
	LADSPA_Data last_amp;

#line 19 "freq_tracker_1418.xml"
	fs = s_rate;
	f = 0.0f;
	fo = 0.0f;
	cross_time = 0;
	last_amp = 0.0f;

	plugin_data->cross_time = cross_time;
	plugin_data->f = f;
	plugin_data->fo = fo;
	plugin_data->fs = fs;
	plugin_data->last_amp = last_amp;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runFreqTracker(LADSPA_Handle instance, unsigned long sample_count) {
	FreqTracker *plugin_data = (FreqTracker *)instance;

	/* Tracking speed (float value) */
	const LADSPA_Data speed = *(plugin_data->speed);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Frequency (Hz) (array of floats of length sample_count) */
	LADSPA_Data * const freq = plugin_data->freq;
	int cross_time = plugin_data->cross_time;
	LADSPA_Data f = plugin_data->f;
	LADSPA_Data fo = plugin_data->fo;
	float fs = plugin_data->fs;
	LADSPA_Data last_amp = plugin_data->last_amp;

#line 34 "freq_tracker_1418.xml"
	unsigned long pos;
	float xm1 = last_amp;
	const float damp_lp = (1.0f - speed) * 0.9f;
	const float damp_lpi = 1.0f - damp_lp;

	for (pos = 0; pos < sample_count; pos++) {
	  if (input[pos] < 0.0f && xm1 > 0.0f) {
	    if (cross_time > 3.0f) {
	      f = fs / ((float)cross_time * 2.0f);
	    }
	    cross_time = 0;
	  }
	  xm1 = input[pos];
	  cross_time++;
	  fo = fo * damp_lp + f * damp_lpi;
	  fo = flush_to_zero(fo);
	  buffer_write(freq[pos], fo);
	}

	plugin_data->last_amp = xm1;
	plugin_data->fo = fo;
	plugin_data->f = f;
	plugin_data->cross_time = cross_time;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainFreqTracker(LADSPA_Handle instance, LADSPA_Data gain) {
	((FreqTracker *)instance)->run_adding_gain = gain;
}

static void runAddingFreqTracker(LADSPA_Handle instance, unsigned long sample_count) {
	FreqTracker *plugin_data = (FreqTracker *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Tracking speed (float value) */
	const LADSPA_Data speed = *(plugin_data->speed);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Frequency (Hz) (array of floats of length sample_count) */
	LADSPA_Data * const freq = plugin_data->freq;
	int cross_time = plugin_data->cross_time;
	LADSPA_Data f = plugin_data->f;
	LADSPA_Data fo = plugin_data->fo;
	float fs = plugin_data->fs;
	LADSPA_Data last_amp = plugin_data->last_amp;

#line 34 "freq_tracker_1418.xml"
	unsigned long pos;
	float xm1 = last_amp;
	const float damp_lp = (1.0f - speed) * 0.9f;
	const float damp_lpi = 1.0f - damp_lp;

	for (pos = 0; pos < sample_count; pos++) {
	  if (input[pos] < 0.0f && xm1 > 0.0f) {
	    if (cross_time > 3.0f) {
	      f = fs / ((float)cross_time * 2.0f);
	    }
	    cross_time = 0;
	  }
	  xm1 = input[pos];
	  cross_time++;
	  fo = fo * damp_lp + f * damp_lpi;
	  fo = flush_to_zero(fo);
	  buffer_write(freq[pos], fo);
	}

	plugin_data->last_amp = xm1;
	plugin_data->fo = fo;
	plugin_data->f = f;
	plugin_data->cross_time = cross_time;
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


	freqTrackerDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (freqTrackerDescriptor) {
		freqTrackerDescriptor->UniqueID = 1418;
		freqTrackerDescriptor->Label = "freqTracker";
		freqTrackerDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		freqTrackerDescriptor->Name =
		 D_("Frequency tracker");
		freqTrackerDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		freqTrackerDescriptor->Copyright =
		 "GPL";
		freqTrackerDescriptor->PortCount = 3;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(3,
		 sizeof(LADSPA_PortDescriptor));
		freqTrackerDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(3,
		 sizeof(LADSPA_PortRangeHint));
		freqTrackerDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(3, sizeof(char*));
		freqTrackerDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Tracking speed */
		port_descriptors[FREQTRACKER_SPEED] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[FREQTRACKER_SPEED] =
		 D_("Tracking speed");
		port_range_hints[FREQTRACKER_SPEED].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[FREQTRACKER_SPEED].LowerBound = 0;
		port_range_hints[FREQTRACKER_SPEED].UpperBound = 1;

		/* Parameters for Input */
		port_descriptors[FREQTRACKER_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[FREQTRACKER_INPUT] =
		 D_("Input");
		port_range_hints[FREQTRACKER_INPUT].HintDescriptor = 0;

		/* Parameters for Frequency (Hz) */
		port_descriptors[FREQTRACKER_FREQ] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[FREQTRACKER_FREQ] =
		 D_("Frequency (Hz)");
		port_range_hints[FREQTRACKER_FREQ].HintDescriptor = 0;

		freqTrackerDescriptor->activate = activateFreqTracker;
		freqTrackerDescriptor->cleanup = cleanupFreqTracker;
		freqTrackerDescriptor->connect_port = connectPortFreqTracker;
		freqTrackerDescriptor->deactivate = NULL;
		freqTrackerDescriptor->instantiate = instantiateFreqTracker;
		freqTrackerDescriptor->run = runFreqTracker;
		freqTrackerDescriptor->run_adding = runAddingFreqTracker;
		freqTrackerDescriptor->set_run_adding_gain = setRunAddingGainFreqTracker;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (freqTrackerDescriptor) {
		free((LADSPA_PortDescriptor *)freqTrackerDescriptor->PortDescriptors);
		free((char **)freqTrackerDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)freqTrackerDescriptor->PortRangeHints);
		free(freqTrackerDescriptor);
	}

}
