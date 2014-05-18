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

#line 10 "gverb_1216.xml"

/*

GVerb algorithm designed and implemented by Juhana Sadeharju.
LADSPA implementation and GVerb speeds ups by Steve Harris.

Comments and suggestions should be mailed to Juhana Sadeharju
(kouhia at nic funet fi).

*/

#include "ladspa-util.h"
#include "gverb/gverbdsp.h"
#include "gverb/gverb.h"

#define GVERB_ROOMSIZE                 0
#define GVERB_REVTIME                  1
#define GVERB_DAMPING                  2
#define GVERB_INPUTBANDWIDTH           3
#define GVERB_DRYLEVEL                 4
#define GVERB_EARLYLEVEL               5
#define GVERB_TAILLEVEL                6
#define GVERB_INPUT                    7
#define GVERB_OUTL                     8
#define GVERB_OUTR                     9

static LADSPA_Descriptor *gverbDescriptor = NULL;

typedef struct {
	LADSPA_Data *roomsize;
	LADSPA_Data *revtime;
	LADSPA_Data *damping;
	LADSPA_Data *inputbandwidth;
	LADSPA_Data *drylevel;
	LADSPA_Data *earlylevel;
	LADSPA_Data *taillevel;
	LADSPA_Data *input;
	LADSPA_Data *outl;
	LADSPA_Data *outr;
	ty_gverb *   verb;
	LADSPA_Data run_adding_gain;
} Gverb;

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
		return gverbDescriptor;
	default:
		return NULL;
	}
}

static void activateGverb(LADSPA_Handle instance) {
	Gverb *plugin_data = (Gverb *)instance;
	ty_gverb *verb = plugin_data->verb;
#line 54 "gverb_1216.xml"
	gverb_flush(plugin_data->verb);
	plugin_data->verb = verb;

}

static void cleanupGverb(LADSPA_Handle instance) {
#line 58 "gverb_1216.xml"
	Gverb *plugin_data = (Gverb *)instance;
	gverb_free(plugin_data->verb);
	free(instance);
}

static void connectPortGverb(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Gverb *plugin;

	plugin = (Gverb *)instance;
	switch (port) {
	case GVERB_ROOMSIZE:
		plugin->roomsize = data;
		break;
	case GVERB_REVTIME:
		plugin->revtime = data;
		break;
	case GVERB_DAMPING:
		plugin->damping = data;
		break;
	case GVERB_INPUTBANDWIDTH:
		plugin->inputbandwidth = data;
		break;
	case GVERB_DRYLEVEL:
		plugin->drylevel = data;
		break;
	case GVERB_EARLYLEVEL:
		plugin->earlylevel = data;
		break;
	case GVERB_TAILLEVEL:
		plugin->taillevel = data;
		break;
	case GVERB_INPUT:
		plugin->input = data;
		break;
	case GVERB_OUTL:
		plugin->outl = data;
		break;
	case GVERB_OUTR:
		plugin->outr = data;
		break;
	}
}

static LADSPA_Handle instantiateGverb(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Gverb *plugin_data = (Gverb *)malloc(sizeof(Gverb));
	ty_gverb *verb = NULL;

#line 50 "gverb_1216.xml"
	verb = gverb_new(s_rate, 300.0f, 50.0f, 7.0f, 0.5f, 15.0f, 0.5f, 0.5f, 0.5f);

	plugin_data->verb = verb;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runGverb(LADSPA_Handle instance, unsigned long sample_count) {
	Gverb *plugin_data = (Gverb *)instance;

	/* Roomsize (m) (float value) */
	const LADSPA_Data roomsize = *(plugin_data->roomsize);

	/* Reverb time (s) (float value) */
	const LADSPA_Data revtime = *(plugin_data->revtime);

	/* Damping (float value) */
	const LADSPA_Data damping = *(plugin_data->damping);

	/* Input bandwidth (float value) */
	const LADSPA_Data inputbandwidth = *(plugin_data->inputbandwidth);

	/* Dry signal level (dB) (float value) */
	const LADSPA_Data drylevel = *(plugin_data->drylevel);

	/* Early reflection level (dB) (float value) */
	const LADSPA_Data earlylevel = *(plugin_data->earlylevel);

	/* Tail level (dB) (float value) */
	const LADSPA_Data taillevel = *(plugin_data->taillevel);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Left output (array of floats of length sample_count) */
	LADSPA_Data * const outl = plugin_data->outl;

	/* Right output (array of floats of length sample_count) */
	LADSPA_Data * const outr = plugin_data->outr;
	ty_gverb * verb = plugin_data->verb;

#line 62 "gverb_1216.xml"
	unsigned long pos;
	float l, r;
	float dryc = DB_CO(drylevel);

	gverb_set_roomsize(verb, roomsize);
	gverb_set_revtime(verb, revtime);
	gverb_set_damping(verb, damping);
	gverb_set_inputbandwidth(verb, inputbandwidth);
	gverb_set_earlylevel(verb, DB_CO(earlylevel));
	gverb_set_taillevel(verb, DB_CO(taillevel));

	for (pos = 0; pos < sample_count; pos++) {
	  gverb_do(verb, input[pos], &l, &r);
	  buffer_write(outl[pos], l + input[pos] * dryc);
	  buffer_write(outr[pos], r + input[pos] * dryc);
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainGverb(LADSPA_Handle instance, LADSPA_Data gain) {
	((Gverb *)instance)->run_adding_gain = gain;
}

static void runAddingGverb(LADSPA_Handle instance, unsigned long sample_count) {
	Gverb *plugin_data = (Gverb *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Roomsize (m) (float value) */
	const LADSPA_Data roomsize = *(plugin_data->roomsize);

	/* Reverb time (s) (float value) */
	const LADSPA_Data revtime = *(plugin_data->revtime);

	/* Damping (float value) */
	const LADSPA_Data damping = *(plugin_data->damping);

	/* Input bandwidth (float value) */
	const LADSPA_Data inputbandwidth = *(plugin_data->inputbandwidth);

	/* Dry signal level (dB) (float value) */
	const LADSPA_Data drylevel = *(plugin_data->drylevel);

	/* Early reflection level (dB) (float value) */
	const LADSPA_Data earlylevel = *(plugin_data->earlylevel);

	/* Tail level (dB) (float value) */
	const LADSPA_Data taillevel = *(plugin_data->taillevel);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Left output (array of floats of length sample_count) */
	LADSPA_Data * const outl = plugin_data->outl;

	/* Right output (array of floats of length sample_count) */
	LADSPA_Data * const outr = plugin_data->outr;
	ty_gverb * verb = plugin_data->verb;

#line 62 "gverb_1216.xml"
	unsigned long pos;
	float l, r;
	float dryc = DB_CO(drylevel);

	gverb_set_roomsize(verb, roomsize);
	gverb_set_revtime(verb, revtime);
	gverb_set_damping(verb, damping);
	gverb_set_inputbandwidth(verb, inputbandwidth);
	gverb_set_earlylevel(verb, DB_CO(earlylevel));
	gverb_set_taillevel(verb, DB_CO(taillevel));

	for (pos = 0; pos < sample_count; pos++) {
	  gverb_do(verb, input[pos], &l, &r);
	  buffer_write(outl[pos], l + input[pos] * dryc);
	  buffer_write(outr[pos], r + input[pos] * dryc);
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


	gverbDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (gverbDescriptor) {
		gverbDescriptor->UniqueID = 1216;
		gverbDescriptor->Label = "gverb";
		gverbDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		gverbDescriptor->Name =
		 D_("GVerb");
		gverbDescriptor->Maker =
		 "Juhana Sadeharju <kouhia at nic.funet.fi>, LADSPAification by Steve Harris <steve@plugin.org.uk>";
		gverbDescriptor->Copyright =
		 "GPL";
		gverbDescriptor->PortCount = 10;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(10,
		 sizeof(LADSPA_PortDescriptor));
		gverbDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(10,
		 sizeof(LADSPA_PortRangeHint));
		gverbDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(10, sizeof(char*));
		gverbDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Roomsize (m) */
		port_descriptors[GVERB_ROOMSIZE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GVERB_ROOMSIZE] =
		 D_("Roomsize (m)");
		port_range_hints[GVERB_ROOMSIZE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[GVERB_ROOMSIZE].LowerBound = 1;
		port_range_hints[GVERB_ROOMSIZE].UpperBound = 300;

		/* Parameters for Reverb time (s) */
		port_descriptors[GVERB_REVTIME] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GVERB_REVTIME] =
		 D_("Reverb time (s)");
		port_range_hints[GVERB_REVTIME].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[GVERB_REVTIME].LowerBound = 0.1;
		port_range_hints[GVERB_REVTIME].UpperBound = 30;

		/* Parameters for Damping */
		port_descriptors[GVERB_DAMPING] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GVERB_DAMPING] =
		 D_("Damping");
		port_range_hints[GVERB_DAMPING].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GVERB_DAMPING].LowerBound = 0;
		port_range_hints[GVERB_DAMPING].UpperBound = 1;

		/* Parameters for Input bandwidth */
		port_descriptors[GVERB_INPUTBANDWIDTH] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GVERB_INPUTBANDWIDTH] =
		 D_("Input bandwidth");
		port_range_hints[GVERB_INPUTBANDWIDTH].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_HIGH;
		port_range_hints[GVERB_INPUTBANDWIDTH].LowerBound = 0;
		port_range_hints[GVERB_INPUTBANDWIDTH].UpperBound = 1;

		/* Parameters for Dry signal level (dB) */
		port_descriptors[GVERB_DRYLEVEL] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GVERB_DRYLEVEL] =
		 D_("Dry signal level (dB)");
		port_range_hints[GVERB_DRYLEVEL].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MINIMUM;
		port_range_hints[GVERB_DRYLEVEL].LowerBound = -70;
		port_range_hints[GVERB_DRYLEVEL].UpperBound = 0;

		/* Parameters for Early reflection level (dB) */
		port_descriptors[GVERB_EARLYLEVEL] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GVERB_EARLYLEVEL] =
		 D_("Early reflection level (dB)");
		port_range_hints[GVERB_EARLYLEVEL].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[GVERB_EARLYLEVEL].LowerBound = -70;
		port_range_hints[GVERB_EARLYLEVEL].UpperBound = 0;

		/* Parameters for Tail level (dB) */
		port_descriptors[GVERB_TAILLEVEL] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GVERB_TAILLEVEL] =
		 D_("Tail level (dB)");
		port_range_hints[GVERB_TAILLEVEL].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_HIGH;
		port_range_hints[GVERB_TAILLEVEL].LowerBound = -70;
		port_range_hints[GVERB_TAILLEVEL].UpperBound = 0;

		/* Parameters for Input */
		port_descriptors[GVERB_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[GVERB_INPUT] =
		 D_("Input");
		port_range_hints[GVERB_INPUT].HintDescriptor = 0;

		/* Parameters for Left output */
		port_descriptors[GVERB_OUTL] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[GVERB_OUTL] =
		 D_("Left output");
		port_range_hints[GVERB_OUTL].HintDescriptor = 0;

		/* Parameters for Right output */
		port_descriptors[GVERB_OUTR] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[GVERB_OUTR] =
		 D_("Right output");
		port_range_hints[GVERB_OUTR].HintDescriptor = 0;

		gverbDescriptor->activate = activateGverb;
		gverbDescriptor->cleanup = cleanupGverb;
		gverbDescriptor->connect_port = connectPortGverb;
		gverbDescriptor->deactivate = NULL;
		gverbDescriptor->instantiate = instantiateGverb;
		gverbDescriptor->run = runGverb;
		gverbDescriptor->run_adding = runAddingGverb;
		gverbDescriptor->set_run_adding_gain = setRunAddingGainGverb;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (gverbDescriptor) {
		free((LADSPA_PortDescriptor *)gverbDescriptor->PortDescriptors);
		free((char **)gverbDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)gverbDescriptor->PortRangeHints);
		free(gverbDescriptor);
	}

}
