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

#line 9 "svf_1214.xml"

#include "ladspa-util.h"

// Constants to match filter types
#define F_LP 1
#define F_HP 2
#define F_BP 3
#define F_BR 4
#define F_AP 5

// Number of filter oversamples
#define F_R 3

/* Structure to hold parameters for SV filter */

typedef struct {
        float f;     // 2.0*sin(PI*fs/(fc*r));
        float q;     // 2.0*cos(pow(q, 0.1)*PI*0.5);
        float qnrm;  // sqrt(m/2.0f+0.01f);
        float h;     // high pass output
        float b;     // band pass output
        float l;     // low pass output
        float p;     // peaking output (allpass with resonance)
        float n;     // notch output
        float *op;   // pointer to output value
} sv_filter;

/* Store data in SVF struct, takes the sampling frequency, cutoff frequency
   and Q, and fills in the structure passed */

static inline void setup_svf(sv_filter *sv, float fs, float fc, float q, int t) {
        sv->f = 2.0f * sin(M_PI * fc / (float)(fs * F_R));
        sv->q = 2.0f * cos(pow(q, 0.1f) * M_PI * 0.5f);
        sv->qnrm = sqrt(sv->q/2.0+0.01);
        switch(t) {
        case F_LP:
                sv->op = &(sv->l);
                break;
        case F_HP:
                sv->op = &(sv->h);
                break;
        case F_BP:
                sv->op = &(sv->b);
                break;
        case F_BR:
                sv->op = &(sv->n);
                break;
        default:
                sv->op = &(sv->p);
        }
}

/* Run one sample through the SV filter. Filter is by andy@vellocet */

static inline float run_svf(sv_filter *sv, float in) {
        float out;
        int i;

        in = sv->qnrm * in ;
        for (i=0; i < F_R; i++) {
                // very slight waveshape for extra stability
                sv->b = flush_to_zero(sv->b - sv->b * sv->b * sv->b * 0.001f);

                // regular state variable code here
                // the notch and peaking outputs are optional
                sv->h = flush_to_zero(in - sv->l - sv->q * sv->b);
                sv->b = sv->b + sv->f * sv->h;
                sv->l = flush_to_zero(sv->l + sv->f * sv->b);
                sv->n = sv->l + sv->h;
                sv->p = sv->l - sv->h;

                out = *(sv->op);
                in = out;
        }

        return out;
}

#define SVF_INPUT                      0
#define SVF_OUTPUT                     1
#define SVF_FILT_TYPE                  2
#define SVF_FILT_FREQ                  3
#define SVF_FILT_Q                     4
#define SVF_FILT_RES                   5

static LADSPA_Descriptor *svfDescriptor = NULL;

typedef struct {
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data *filt_type;
	LADSPA_Data *filt_freq;
	LADSPA_Data *filt_q;
	LADSPA_Data *filt_res;
	int          sample_rate;
	sv_filter *  svf;
	LADSPA_Data run_adding_gain;
} Svf;

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
		return svfDescriptor;
	default:
		return NULL;
	}
}

static void activateSvf(LADSPA_Handle instance) {
	Svf *plugin_data = (Svf *)instance;
	int sample_rate = plugin_data->sample_rate;
	sv_filter *svf = plugin_data->svf;
#line 100 "svf_1214.xml"
	setup_svf(svf, 0, 0, 0, 0);
	plugin_data->sample_rate = sample_rate;
	plugin_data->svf = svf;

}

static void cleanupSvf(LADSPA_Handle instance) {
#line 104 "svf_1214.xml"
	Svf *plugin_data = (Svf *)instance;
	free(plugin_data->svf);
	free(instance);
}

static void connectPortSvf(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Svf *plugin;

	plugin = (Svf *)instance;
	switch (port) {
	case SVF_INPUT:
		plugin->input = data;
		break;
	case SVF_OUTPUT:
		plugin->output = data;
		break;
	case SVF_FILT_TYPE:
		plugin->filt_type = data;
		break;
	case SVF_FILT_FREQ:
		plugin->filt_freq = data;
		break;
	case SVF_FILT_Q:
		plugin->filt_q = data;
		break;
	case SVF_FILT_RES:
		plugin->filt_res = data;
		break;
	}
}

static LADSPA_Handle instantiateSvf(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Svf *plugin_data = (Svf *)malloc(sizeof(Svf));
	int sample_rate;
	sv_filter *svf = NULL;

#line 94 "svf_1214.xml"
	sample_rate = s_rate;
	
	svf = calloc(1, sizeof(sv_filter));

	plugin_data->sample_rate = sample_rate;
	plugin_data->svf = svf;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runSvf(LADSPA_Handle instance, unsigned long sample_count) {
	Svf *plugin_data = (Svf *)instance;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

	/* Filter type (0=none, 1=LP, 2=HP, 3=BP, 4=BR, 5=AP) (float value) */
	const LADSPA_Data filt_type = *(plugin_data->filt_type);

	/* Filter freq (float value) */
	const LADSPA_Data filt_freq = *(plugin_data->filt_freq);

	/* Filter Q (float value) */
	const LADSPA_Data filt_q = *(plugin_data->filt_q);

	/* Filter resonance (float value) */
	const LADSPA_Data filt_res = *(plugin_data->filt_res);
	int sample_rate = plugin_data->sample_rate;
	sv_filter * svf = plugin_data->svf;

#line 108 "svf_1214.xml"
	long int pos;
	
	setup_svf(svf, sample_rate, filt_freq, filt_q, f_round(filt_type));
	
	for (pos = 0; pos < sample_count; pos++) {
	        buffer_write(output[pos], run_svf(svf, input[pos] + (svf->b * filt_res)));
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainSvf(LADSPA_Handle instance, LADSPA_Data gain) {
	((Svf *)instance)->run_adding_gain = gain;
}

static void runAddingSvf(LADSPA_Handle instance, unsigned long sample_count) {
	Svf *plugin_data = (Svf *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

	/* Filter type (0=none, 1=LP, 2=HP, 3=BP, 4=BR, 5=AP) (float value) */
	const LADSPA_Data filt_type = *(plugin_data->filt_type);

	/* Filter freq (float value) */
	const LADSPA_Data filt_freq = *(plugin_data->filt_freq);

	/* Filter Q (float value) */
	const LADSPA_Data filt_q = *(plugin_data->filt_q);

	/* Filter resonance (float value) */
	const LADSPA_Data filt_res = *(plugin_data->filt_res);
	int sample_rate = plugin_data->sample_rate;
	sv_filter * svf = plugin_data->svf;

#line 108 "svf_1214.xml"
	long int pos;
	
	setup_svf(svf, sample_rate, filt_freq, filt_q, f_round(filt_type));
	
	for (pos = 0; pos < sample_count; pos++) {
	        buffer_write(output[pos], run_svf(svf, input[pos] + (svf->b * filt_res)));
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


	svfDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (svfDescriptor) {
		svfDescriptor->UniqueID = 1214;
		svfDescriptor->Label = "svf";
		svfDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		svfDescriptor->Name =
		 D_("State Variable Filter");
		svfDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		svfDescriptor->Copyright =
		 "GPL";
		svfDescriptor->PortCount = 6;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(6,
		 sizeof(LADSPA_PortDescriptor));
		svfDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(6,
		 sizeof(LADSPA_PortRangeHint));
		svfDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(6, sizeof(char*));
		svfDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Input */
		port_descriptors[SVF_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[SVF_INPUT] =
		 D_("Input");
		port_range_hints[SVF_INPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[SVF_INPUT].LowerBound = -1;
		port_range_hints[SVF_INPUT].UpperBound = 1;

		/* Parameters for Output */
		port_descriptors[SVF_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[SVF_OUTPUT] =
		 D_("Output");
		port_range_hints[SVF_OUTPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[SVF_OUTPUT].LowerBound = -1;
		port_range_hints[SVF_OUTPUT].UpperBound = 1;

		/* Parameters for Filter type (0=none, 1=LP, 2=HP, 3=BP, 4=BR, 5=AP) */
		port_descriptors[SVF_FILT_TYPE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SVF_FILT_TYPE] =
		 D_("Filter type (0=none, 1=LP, 2=HP, 3=BP, 4=BR, 5=AP)");
		port_range_hints[SVF_FILT_TYPE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_0;
		port_range_hints[SVF_FILT_TYPE].LowerBound = 0;
		port_range_hints[SVF_FILT_TYPE].UpperBound = 5;

		/* Parameters for Filter freq */
		port_descriptors[SVF_FILT_FREQ] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SVF_FILT_FREQ] =
		 D_("Filter freq");
		port_range_hints[SVF_FILT_FREQ].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_440;
		port_range_hints[SVF_FILT_FREQ].LowerBound = 0;
		port_range_hints[SVF_FILT_FREQ].UpperBound = 6000;

		/* Parameters for Filter Q */
		port_descriptors[SVF_FILT_Q] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SVF_FILT_Q] =
		 D_("Filter Q");
		port_range_hints[SVF_FILT_Q].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[SVF_FILT_Q].LowerBound = 0;
		port_range_hints[SVF_FILT_Q].UpperBound = 1;

		/* Parameters for Filter resonance */
		port_descriptors[SVF_FILT_RES] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SVF_FILT_RES] =
		 D_("Filter resonance");
		port_range_hints[SVF_FILT_RES].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[SVF_FILT_RES].LowerBound = 0;
		port_range_hints[SVF_FILT_RES].UpperBound = 1;

		svfDescriptor->activate = activateSvf;
		svfDescriptor->cleanup = cleanupSvf;
		svfDescriptor->connect_port = connectPortSvf;
		svfDescriptor->deactivate = NULL;
		svfDescriptor->instantiate = instantiateSvf;
		svfDescriptor->run = runSvf;
		svfDescriptor->run_adding = runAddingSvf;
		svfDescriptor->set_run_adding_gain = setRunAddingGainSvf;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (svfDescriptor) {
		free((LADSPA_PortDescriptor *)svfDescriptor->PortDescriptors);
		free((char **)svfDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)svfDescriptor->PortRangeHints);
		free(svfDescriptor);
	}

}
