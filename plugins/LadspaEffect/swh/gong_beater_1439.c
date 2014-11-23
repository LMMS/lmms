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

#line 10 "gong_beater_1439.xml"

#include "ladspa-util.h"

#define GONGBEATER_IMP_GAIN            0
#define GONGBEATER_STRIKE_GAIN         1
#define GONGBEATER_STRIKE_DURATION     2
#define GONGBEATER_INPUT               3
#define GONGBEATER_OUTPUT              4

static LADSPA_Descriptor *gongBeaterDescriptor = NULL;

typedef struct {
	LADSPA_Data *imp_gain;
	LADSPA_Data *strike_gain;
	LADSPA_Data *strike_duration;
	LADSPA_Data *input;
	LADSPA_Data *output;
	float        fs;
	float        imp_level;
	unsigned int running;
	float        x;
	float        xm;
	float        y;
	float        ym;
	LADSPA_Data run_adding_gain;
} GongBeater;

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
		return gongBeaterDescriptor;
	default:
		return NULL;
	}
}

static void activateGongBeater(LADSPA_Handle instance) {
	GongBeater *plugin_data = (GongBeater *)instance;
	float fs = plugin_data->fs;
	float imp_level = plugin_data->imp_level;
	unsigned int running = plugin_data->running;
	float x = plugin_data->x;
	float xm = plugin_data->xm;
	float y = plugin_data->y;
	float ym = plugin_data->ym;
#line 31 "gong_beater_1439.xml"
	running = 0;
	x = 0.5f;
	y = 0.0f;
	xm = 0.5f;
	ym = 0.0f;
	plugin_data->fs = fs;
	plugin_data->imp_level = imp_level;
	plugin_data->running = running;
	plugin_data->x = x;
	plugin_data->xm = xm;
	plugin_data->y = y;
	plugin_data->ym = ym;

}

static void cleanupGongBeater(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortGongBeater(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	GongBeater *plugin;

	plugin = (GongBeater *)instance;
	switch (port) {
	case GONGBEATER_IMP_GAIN:
		plugin->imp_gain = data;
		break;
	case GONGBEATER_STRIKE_GAIN:
		plugin->strike_gain = data;
		break;
	case GONGBEATER_STRIKE_DURATION:
		plugin->strike_duration = data;
		break;
	case GONGBEATER_INPUT:
		plugin->input = data;
		break;
	case GONGBEATER_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateGongBeater(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	GongBeater *plugin_data = (GongBeater *)malloc(sizeof(GongBeater));
	float fs;
	float imp_level;
	unsigned int running;
	float x;
	float xm;
	float y;
	float ym;

#line 21 "gong_beater_1439.xml"
	running = 0;
	x = 0.5f;
	y = 0.0f;
	xm = 0.5f;
	ym = 0.0f;
	fs = (float)s_rate;
	imp_level = 0.0f;

	plugin_data->fs = fs;
	plugin_data->imp_level = imp_level;
	plugin_data->running = running;
	plugin_data->x = x;
	plugin_data->xm = xm;
	plugin_data->y = y;
	plugin_data->ym = ym;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runGongBeater(LADSPA_Handle instance, unsigned long sample_count) {
	GongBeater *plugin_data = (GongBeater *)instance;

	/* Impulse gain (dB) (float value) */
	const LADSPA_Data imp_gain = *(plugin_data->imp_gain);

	/* Strike gain (dB) (float value) */
	const LADSPA_Data strike_gain = *(plugin_data->strike_gain);

	/* Strike duration (s) (float value) */
	const LADSPA_Data strike_duration = *(plugin_data->strike_duration);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	float fs = plugin_data->fs;
	float imp_level = plugin_data->imp_level;
	unsigned int running = plugin_data->running;
	float x = plugin_data->x;
	float xm = plugin_data->xm;
	float y = plugin_data->y;
	float ym = plugin_data->ym;

#line 39 "gong_beater_1439.xml"
	unsigned long pos;
	const float imp_amp = DB_CO(imp_gain);
	const float strike_amp = DB_CO(strike_gain);
	const float omega = 6.2831852f / (strike_duration * fs);

	pos = 0;
	while (pos < sample_count) {
	  for (; !running && pos < sample_count; pos++) {
	    if (fabs(input[pos]) > 0.05f) {
	      running = strike_duration * fs;
	      imp_level = fabs(input[pos]);
	    }
	    buffer_write(output[pos], input[pos] * imp_amp);
	  }
	  for (; running && pos < sample_count; pos++, running--) {
	    if (fabs(input[pos]) > imp_level) {
	      imp_level = fabs(input[pos]);
	    }
	    x -= omega * y;
	    y += omega * x;
	    xm -= omega * 0.5f * ym;
	    ym += omega * 0.5f * xm;

	    buffer_write(output[pos], input[pos] * imp_amp + y * strike_amp *
	                      imp_level * 4.0f * ym);
	  }
	}

	plugin_data->x = x;
	plugin_data->y = y;
	plugin_data->xm = xm;
	plugin_data->ym = ym;
	plugin_data->running = running;
	plugin_data->imp_level = imp_level;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainGongBeater(LADSPA_Handle instance, LADSPA_Data gain) {
	((GongBeater *)instance)->run_adding_gain = gain;
}

static void runAddingGongBeater(LADSPA_Handle instance, unsigned long sample_count) {
	GongBeater *plugin_data = (GongBeater *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Impulse gain (dB) (float value) */
	const LADSPA_Data imp_gain = *(plugin_data->imp_gain);

	/* Strike gain (dB) (float value) */
	const LADSPA_Data strike_gain = *(plugin_data->strike_gain);

	/* Strike duration (s) (float value) */
	const LADSPA_Data strike_duration = *(plugin_data->strike_duration);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	float fs = plugin_data->fs;
	float imp_level = plugin_data->imp_level;
	unsigned int running = plugin_data->running;
	float x = plugin_data->x;
	float xm = plugin_data->xm;
	float y = plugin_data->y;
	float ym = plugin_data->ym;

#line 39 "gong_beater_1439.xml"
	unsigned long pos;
	const float imp_amp = DB_CO(imp_gain);
	const float strike_amp = DB_CO(strike_gain);
	const float omega = 6.2831852f / (strike_duration * fs);

	pos = 0;
	while (pos < sample_count) {
	  for (; !running && pos < sample_count; pos++) {
	    if (fabs(input[pos]) > 0.05f) {
	      running = strike_duration * fs;
	      imp_level = fabs(input[pos]);
	    }
	    buffer_write(output[pos], input[pos] * imp_amp);
	  }
	  for (; running && pos < sample_count; pos++, running--) {
	    if (fabs(input[pos]) > imp_level) {
	      imp_level = fabs(input[pos]);
	    }
	    x -= omega * y;
	    y += omega * x;
	    xm -= omega * 0.5f * ym;
	    ym += omega * 0.5f * xm;

	    buffer_write(output[pos], input[pos] * imp_amp + y * strike_amp *
	                      imp_level * 4.0f * ym);
	  }
	}

	plugin_data->x = x;
	plugin_data->y = y;
	plugin_data->xm = xm;
	plugin_data->ym = ym;
	plugin_data->running = running;
	plugin_data->imp_level = imp_level;
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


	gongBeaterDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (gongBeaterDescriptor) {
		gongBeaterDescriptor->UniqueID = 1439;
		gongBeaterDescriptor->Label = "gongBeater";
		gongBeaterDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		gongBeaterDescriptor->Name =
		 D_("Gong beater");
		gongBeaterDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		gongBeaterDescriptor->Copyright =
		 "GPL";
		gongBeaterDescriptor->PortCount = 5;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(5,
		 sizeof(LADSPA_PortDescriptor));
		gongBeaterDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(5,
		 sizeof(LADSPA_PortRangeHint));
		gongBeaterDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(5, sizeof(char*));
		gongBeaterDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Impulse gain (dB) */
		port_descriptors[GONGBEATER_IMP_GAIN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONGBEATER_IMP_GAIN] =
		 D_("Impulse gain (dB)");
		port_range_hints[GONGBEATER_IMP_GAIN].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MINIMUM;
		port_range_hints[GONGBEATER_IMP_GAIN].LowerBound = -70;
		port_range_hints[GONGBEATER_IMP_GAIN].UpperBound = 0;

		/* Parameters for Strike gain (dB) */
		port_descriptors[GONGBEATER_STRIKE_GAIN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONGBEATER_STRIKE_GAIN] =
		 D_("Strike gain (dB)");
		port_range_hints[GONGBEATER_STRIKE_GAIN].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MAXIMUM;
		port_range_hints[GONGBEATER_STRIKE_GAIN].LowerBound = -70;
		port_range_hints[GONGBEATER_STRIKE_GAIN].UpperBound = 0;

		/* Parameters for Strike duration (s) */
		port_descriptors[GONGBEATER_STRIKE_DURATION] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONGBEATER_STRIKE_DURATION] =
		 D_("Strike duration (s)");
		port_range_hints[GONGBEATER_STRIKE_DURATION].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONGBEATER_STRIKE_DURATION].LowerBound = 0.001;
		port_range_hints[GONGBEATER_STRIKE_DURATION].UpperBound = 0.2;

		/* Parameters for Input */
		port_descriptors[GONGBEATER_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[GONGBEATER_INPUT] =
		 D_("Input");
		port_range_hints[GONGBEATER_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[GONGBEATER_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[GONGBEATER_OUTPUT] =
		 D_("Output");
		port_range_hints[GONGBEATER_OUTPUT].HintDescriptor = 0;

		gongBeaterDescriptor->activate = activateGongBeater;
		gongBeaterDescriptor->cleanup = cleanupGongBeater;
		gongBeaterDescriptor->connect_port = connectPortGongBeater;
		gongBeaterDescriptor->deactivate = NULL;
		gongBeaterDescriptor->instantiate = instantiateGongBeater;
		gongBeaterDescriptor->run = runGongBeater;
		gongBeaterDescriptor->run_adding = runAddingGongBeater;
		gongBeaterDescriptor->set_run_adding_gain = setRunAddingGainGongBeater;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (gongBeaterDescriptor) {
		free((LADSPA_PortDescriptor *)gongBeaterDescriptor->PortDescriptors);
		free((char **)gongBeaterDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)gongBeaterDescriptor->PortRangeHints);
		free(gongBeaterDescriptor);
	}

}
