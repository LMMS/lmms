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

#line 9 "hermes_filter_1200.xml"

#include "ladspa-util.h"
#include "util/blo.h"

// Return the value of the LDO's for given coeffs
#define LFO(a,b) (a*lfo1 + b*lfo2)

// Ampmod / ringmod two signals together with given depth
#define RINGMOD(c,m,d) (c * ((d * 0.5f) * m + (2.0f - d)))

// Stuff needed for the soft clipping code
#define MAX_AMP 1.0f
#define CLIP 0.8f
#define CLIP_A ((MAX_AMP - CLIP) * (MAX_AMP - CLIP))
#define CLIP_B (MAX_AMP - 2.0f * CLIP)

// Constants to match filter types
#define F_LP 1
#define F_HP 2
#define F_BP 3
#define F_BR 4
#define F_AP 5

// Number of filter oversamples
#define F_R 3

// Magic number
#define NOISE 23

LADSPA_Data *sin_tbl, *tri_tbl, *saw_tbl, *squ_tbl;
int tbl_ref_count = 0;
long sample_rate;

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

inline float soft_clip(float sc_in) {
        if ((sc_in < CLIP) && (sc_in > -CLIP)) {
                return sc_in;
        } else if (sc_in > 0.0f) {
                return MAX_AMP - (CLIP_A / (CLIP_B + sc_in));
        } else {
                return -(MAX_AMP - (CLIP_A / (CLIP_B - sc_in)));
        }
}

/* Store data in SVF struct, takes the sampling frequency, cutoff frequency
   and Q, and fills in the structure passed */

inline void setup_svf(sv_filter *sv, float fs, float fc, float q, int t) {
        sv->f = 2.0f * sinf(M_PI * fc / (float)(fs * F_R));
        sv->q = 2.0f * cosf(powf(q, 0.1f) * M_PI * 0.5f);
        sv->qnrm = sqrtf(sv->q*0.5f + 0.01f);

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

/* Change the frequency of a running SVF */

inline void setup_f_svf(sv_filter *sv, const float fs, const float fc) {
        sv->f = 2.0f * sin(M_PI * fc / ((float)(fs * F_R)));
}

/* Run one sample through the SV filter. Filter is by andy@vellocet */

inline float run_svf(sv_filter *sv, float in) {
        float out;
        int i;

        in = sv->qnrm * in ;
        for (i=0; i < F_R; i++) {
                // only needed for pentium chips
                in  = flush_to_zero(in);
                sv->l = flush_to_zero(sv->l);
                // very slight waveshape for extra stability
                sv->b = sv->b - sv->b * sv->b * sv->b * 0.001f;

                // regular state variable code here
                // the notch and peaking outputs are optional
                sv->h = in - sv->l - sv->q * sv->b;
                sv->b = sv->b + sv->f * sv->h;
                sv->l = sv->l + sv->f * sv->b;
                sv->n = sv->l + sv->h;
                sv->p = sv->l - sv->h;

                out = *(sv->op);
                in = out;
        }

        return out;
}

inline int wave_tbl(const float wave) {
        switch (f_round(wave)) {
                case 0:
                return BLO_SINE;
                break;

                case 1:
                return BLO_TRI;
                break;

                case 2:
                return BLO_SAW;
                break;

                case 3:
                return BLO_SQUARE;
                break;
        }
        return NOISE;
}

#define HERMESFILTER_LFO1_FREQ         0
#define HERMESFILTER_LFO1_WAVE         1
#define HERMESFILTER_LFO2_FREQ         2
#define HERMESFILTER_LFO2_WAVE         3
#define HERMESFILTER_OSC1_FREQ         4
#define HERMESFILTER_OSC1_WAVE         5
#define HERMESFILTER_OSC2_FREQ         6
#define HERMESFILTER_OSC2_WAVE         7
#define HERMESFILTER_RM1_DEPTH         8
#define HERMESFILTER_RM2_DEPTH         9
#define HERMESFILTER_RM3_DEPTH         10
#define HERMESFILTER_OSC1_GAIN_DB      11
#define HERMESFILTER_RM1_GAIN_DB       12
#define HERMESFILTER_OSC2_GAIN_DB      13
#define HERMESFILTER_RM2_GAIN_DB       14
#define HERMESFILTER_IN_GAIN_DB        15
#define HERMESFILTER_RM3_GAIN_DB       16
#define HERMESFILTER_XOVER_LFREQP      17
#define HERMESFILTER_XOVER_UFREQP      18
#define HERMESFILTER_DRIVE1            19
#define HERMESFILTER_DRIVE2            20
#define HERMESFILTER_DRIVE3            21
#define HERMESFILTER_FILT1_TYPE        22
#define HERMESFILTER_FILT1_FREQ        23
#define HERMESFILTER_FILT1_Q           24
#define HERMESFILTER_FILT1_RES         25
#define HERMESFILTER_FILT1_LFO1        26
#define HERMESFILTER_FILT1_LFO2        27
#define HERMESFILTER_FILT2_TYPE        28
#define HERMESFILTER_FILT2_FREQ        29
#define HERMESFILTER_FILT2_Q           30
#define HERMESFILTER_FILT2_RES         31
#define HERMESFILTER_FILT2_LFO1        32
#define HERMESFILTER_FILT2_LFO2        33
#define HERMESFILTER_FILT3_TYPE        34
#define HERMESFILTER_FILT3_FREQ        35
#define HERMESFILTER_FILT3_Q           36
#define HERMESFILTER_FILT3_RES         37
#define HERMESFILTER_FILT3_LFO1        38
#define HERMESFILTER_FILT3_LFO2        39
#define HERMESFILTER_DELA1_LENGTH      40
#define HERMESFILTER_DELA1_FB          41
#define HERMESFILTER_DELA1_WET         42
#define HERMESFILTER_DELA2_LENGTH      43
#define HERMESFILTER_DELA2_FB          44
#define HERMESFILTER_DELA2_WET         45
#define HERMESFILTER_DELA3_LENGTH      46
#define HERMESFILTER_DELA3_FB          47
#define HERMESFILTER_DELA3_WET         48
#define HERMESFILTER_BAND1_GAIN_DB     49
#define HERMESFILTER_BAND2_GAIN_DB     50
#define HERMESFILTER_BAND3_GAIN_DB     51
#define HERMESFILTER_INPUT             52
#define HERMESFILTER_OUTPUT            53

static LADSPA_Descriptor *hermesFilterDescriptor = NULL;

typedef struct {
	LADSPA_Data *lfo1_freq;
	LADSPA_Data *lfo1_wave;
	LADSPA_Data *lfo2_freq;
	LADSPA_Data *lfo2_wave;
	LADSPA_Data *osc1_freq;
	LADSPA_Data *osc1_wave;
	LADSPA_Data *osc2_freq;
	LADSPA_Data *osc2_wave;
	LADSPA_Data *rm1_depth;
	LADSPA_Data *rm2_depth;
	LADSPA_Data *rm3_depth;
	LADSPA_Data *osc1_gain_db;
	LADSPA_Data *rm1_gain_db;
	LADSPA_Data *osc2_gain_db;
	LADSPA_Data *rm2_gain_db;
	LADSPA_Data *in_gain_db;
	LADSPA_Data *rm3_gain_db;
	LADSPA_Data *xover_lfreqp;
	LADSPA_Data *xover_ufreqp;
	LADSPA_Data *drive1;
	LADSPA_Data *drive2;
	LADSPA_Data *drive3;
	LADSPA_Data *filt1_type;
	LADSPA_Data *filt1_freq;
	LADSPA_Data *filt1_q;
	LADSPA_Data *filt1_res;
	LADSPA_Data *filt1_lfo1;
	LADSPA_Data *filt1_lfo2;
	LADSPA_Data *filt2_type;
	LADSPA_Data *filt2_freq;
	LADSPA_Data *filt2_q;
	LADSPA_Data *filt2_res;
	LADSPA_Data *filt2_lfo1;
	LADSPA_Data *filt2_lfo2;
	LADSPA_Data *filt3_type;
	LADSPA_Data *filt3_freq;
	LADSPA_Data *filt3_q;
	LADSPA_Data *filt3_res;
	LADSPA_Data *filt3_lfo1;
	LADSPA_Data *filt3_lfo2;
	LADSPA_Data *dela1_length;
	LADSPA_Data *dela1_fb;
	LADSPA_Data *dela1_wet;
	LADSPA_Data *dela2_length;
	LADSPA_Data *dela2_fb;
	LADSPA_Data *dela2_wet;
	LADSPA_Data *dela3_length;
	LADSPA_Data *dela3_fb;
	LADSPA_Data *dela3_wet;
	LADSPA_Data *band1_gain_db;
	LADSPA_Data *band2_gain_db;
	LADSPA_Data *band3_gain_db;
	LADSPA_Data *input;
	LADSPA_Data *output;
	long         count;
	float **     dela_data;
	int *        dela_pos;
	sv_filter ** filt_data;
	float        lfo1;
	blo_h_osc *  lfo1_d;
	float        lfo1_phase;
	float        lfo2;
	blo_h_osc *  lfo2_d;
	float        lfo2_phase;
	blo_h_osc *  osc1_d;
	blo_h_osc *  osc2_d;
	blo_h_tables *tables;
	sv_filter *  xover_b1_data;
	sv_filter *  xover_b2_data;
	LADSPA_Data run_adding_gain;
} HermesFilter;

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
		return hermesFilterDescriptor;
	default:
		return NULL;
	}
}

static void activateHermesFilter(LADSPA_Handle instance) {
	HermesFilter *plugin_data = (HermesFilter *)instance;
	long count = plugin_data->count;
	float **dela_data = plugin_data->dela_data;
	int *dela_pos = plugin_data->dela_pos;
	sv_filter **filt_data = plugin_data->filt_data;
	float lfo1 = plugin_data->lfo1;
	blo_h_osc *lfo1_d = plugin_data->lfo1_d;
	float lfo1_phase = plugin_data->lfo1_phase;
	float lfo2 = plugin_data->lfo2;
	blo_h_osc *lfo2_d = plugin_data->lfo2_d;
	float lfo2_phase = plugin_data->lfo2_phase;
	blo_h_osc *osc1_d = plugin_data->osc1_d;
	blo_h_osc *osc2_d = plugin_data->osc2_d;
	blo_h_tables *tables = plugin_data->tables;
	sv_filter *xover_b1_data = plugin_data->xover_b1_data;
	sv_filter *xover_b2_data = plugin_data->xover_b2_data;
#line 186 "hermes_filter_1200.xml"
	setup_svf(filt_data[0], 0, 0, 0, 0);
	setup_svf(filt_data[1], 0, 0, 0, 0);
	setup_svf(filt_data[2], 0, 0, 0, 0);
	setup_svf(xover_b1_data, sample_rate, 1000.0, 0.0, F_HP);
	setup_svf(xover_b2_data, sample_rate, 100.0, 0.0, F_LP);
	memset(dela_data[0], 0, sample_rate * 2 * sizeof(float));
	memset(dela_data[1], 0, sample_rate * 2 * sizeof(float));
	memset(dela_data[2], 0, sample_rate * 2 * sizeof(float));
	dela_pos[0] = 0;
	dela_pos[1] = 0;
	dela_pos[2] = 0;
	/*
	osc1_d->ph.all = 0;
	osc2_d->ph.all = 0;
	lfo1_d->ph.all = 0;
	lfo2_d->ph.all = 0;
	*/
	count = 0;
	lfo1 = 0.0f;
	lfo2 = 0.0f;
	lfo1_phase = 0.0f;
	lfo2_phase = 0.0f;
	plugin_data->count = count;
	plugin_data->dela_data = dela_data;
	plugin_data->dela_pos = dela_pos;
	plugin_data->filt_data = filt_data;
	plugin_data->lfo1 = lfo1;
	plugin_data->lfo1_d = lfo1_d;
	plugin_data->lfo1_phase = lfo1_phase;
	plugin_data->lfo2 = lfo2;
	plugin_data->lfo2_d = lfo2_d;
	plugin_data->lfo2_phase = lfo2_phase;
	plugin_data->osc1_d = osc1_d;
	plugin_data->osc2_d = osc2_d;
	plugin_data->tables = tables;
	plugin_data->xover_b1_data = xover_b1_data;
	plugin_data->xover_b2_data = xover_b2_data;

}

static void cleanupHermesFilter(LADSPA_Handle instance) {
#line 211 "hermes_filter_1200.xml"
	HermesFilter *plugin_data = (HermesFilter *)instance;
	free(plugin_data->filt_data[0]);
	free(plugin_data->filt_data[1]);
	free(plugin_data->filt_data[2]);
	free(plugin_data->dela_data[0]);
	free(plugin_data->dela_data[1]);
	free(plugin_data->dela_data[2]);
	free(plugin_data->filt_data);
	free(plugin_data->dela_data);
	free(plugin_data->dela_pos);
	free(plugin_data->xover_b1_data);
	free(plugin_data->xover_b2_data);
	
	blo_h_free(plugin_data->osc1_d);
	blo_h_free(plugin_data->osc2_d);
	blo_h_free(plugin_data->lfo1_d);
	blo_h_free(plugin_data->lfo2_d);
	blo_h_tables_free(plugin_data->tables);
	free(instance);
}

static void connectPortHermesFilter(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	HermesFilter *plugin;

	plugin = (HermesFilter *)instance;
	switch (port) {
	case HERMESFILTER_LFO1_FREQ:
		plugin->lfo1_freq = data;
		break;
	case HERMESFILTER_LFO1_WAVE:
		plugin->lfo1_wave = data;
		break;
	case HERMESFILTER_LFO2_FREQ:
		plugin->lfo2_freq = data;
		break;
	case HERMESFILTER_LFO2_WAVE:
		plugin->lfo2_wave = data;
		break;
	case HERMESFILTER_OSC1_FREQ:
		plugin->osc1_freq = data;
		break;
	case HERMESFILTER_OSC1_WAVE:
		plugin->osc1_wave = data;
		break;
	case HERMESFILTER_OSC2_FREQ:
		plugin->osc2_freq = data;
		break;
	case HERMESFILTER_OSC2_WAVE:
		plugin->osc2_wave = data;
		break;
	case HERMESFILTER_RM1_DEPTH:
		plugin->rm1_depth = data;
		break;
	case HERMESFILTER_RM2_DEPTH:
		plugin->rm2_depth = data;
		break;
	case HERMESFILTER_RM3_DEPTH:
		plugin->rm3_depth = data;
		break;
	case HERMESFILTER_OSC1_GAIN_DB:
		plugin->osc1_gain_db = data;
		break;
	case HERMESFILTER_RM1_GAIN_DB:
		plugin->rm1_gain_db = data;
		break;
	case HERMESFILTER_OSC2_GAIN_DB:
		plugin->osc2_gain_db = data;
		break;
	case HERMESFILTER_RM2_GAIN_DB:
		plugin->rm2_gain_db = data;
		break;
	case HERMESFILTER_IN_GAIN_DB:
		plugin->in_gain_db = data;
		break;
	case HERMESFILTER_RM3_GAIN_DB:
		plugin->rm3_gain_db = data;
		break;
	case HERMESFILTER_XOVER_LFREQP:
		plugin->xover_lfreqp = data;
		break;
	case HERMESFILTER_XOVER_UFREQP:
		plugin->xover_ufreqp = data;
		break;
	case HERMESFILTER_DRIVE1:
		plugin->drive1 = data;
		break;
	case HERMESFILTER_DRIVE2:
		plugin->drive2 = data;
		break;
	case HERMESFILTER_DRIVE3:
		plugin->drive3 = data;
		break;
	case HERMESFILTER_FILT1_TYPE:
		plugin->filt1_type = data;
		break;
	case HERMESFILTER_FILT1_FREQ:
		plugin->filt1_freq = data;
		break;
	case HERMESFILTER_FILT1_Q:
		plugin->filt1_q = data;
		break;
	case HERMESFILTER_FILT1_RES:
		plugin->filt1_res = data;
		break;
	case HERMESFILTER_FILT1_LFO1:
		plugin->filt1_lfo1 = data;
		break;
	case HERMESFILTER_FILT1_LFO2:
		plugin->filt1_lfo2 = data;
		break;
	case HERMESFILTER_FILT2_TYPE:
		plugin->filt2_type = data;
		break;
	case HERMESFILTER_FILT2_FREQ:
		plugin->filt2_freq = data;
		break;
	case HERMESFILTER_FILT2_Q:
		plugin->filt2_q = data;
		break;
	case HERMESFILTER_FILT2_RES:
		plugin->filt2_res = data;
		break;
	case HERMESFILTER_FILT2_LFO1:
		plugin->filt2_lfo1 = data;
		break;
	case HERMESFILTER_FILT2_LFO2:
		plugin->filt2_lfo2 = data;
		break;
	case HERMESFILTER_FILT3_TYPE:
		plugin->filt3_type = data;
		break;
	case HERMESFILTER_FILT3_FREQ:
		plugin->filt3_freq = data;
		break;
	case HERMESFILTER_FILT3_Q:
		plugin->filt3_q = data;
		break;
	case HERMESFILTER_FILT3_RES:
		plugin->filt3_res = data;
		break;
	case HERMESFILTER_FILT3_LFO1:
		plugin->filt3_lfo1 = data;
		break;
	case HERMESFILTER_FILT3_LFO2:
		plugin->filt3_lfo2 = data;
		break;
	case HERMESFILTER_DELA1_LENGTH:
		plugin->dela1_length = data;
		break;
	case HERMESFILTER_DELA1_FB:
		plugin->dela1_fb = data;
		break;
	case HERMESFILTER_DELA1_WET:
		plugin->dela1_wet = data;
		break;
	case HERMESFILTER_DELA2_LENGTH:
		plugin->dela2_length = data;
		break;
	case HERMESFILTER_DELA2_FB:
		plugin->dela2_fb = data;
		break;
	case HERMESFILTER_DELA2_WET:
		plugin->dela2_wet = data;
		break;
	case HERMESFILTER_DELA3_LENGTH:
		plugin->dela3_length = data;
		break;
	case HERMESFILTER_DELA3_FB:
		plugin->dela3_fb = data;
		break;
	case HERMESFILTER_DELA3_WET:
		plugin->dela3_wet = data;
		break;
	case HERMESFILTER_BAND1_GAIN_DB:
		plugin->band1_gain_db = data;
		break;
	case HERMESFILTER_BAND2_GAIN_DB:
		plugin->band2_gain_db = data;
		break;
	case HERMESFILTER_BAND3_GAIN_DB:
		plugin->band3_gain_db = data;
		break;
	case HERMESFILTER_INPUT:
		plugin->input = data;
		break;
	case HERMESFILTER_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateHermesFilter(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	HermesFilter *plugin_data = (HermesFilter *)malloc(sizeof(HermesFilter));
	long count;
	float **dela_data = NULL;
	int *dela_pos = NULL;
	sv_filter **filt_data = NULL;
	float lfo1;
	blo_h_osc *lfo1_d = NULL;
	float lfo1_phase;
	float lfo2;
	blo_h_osc *lfo2_d = NULL;
	float lfo2_phase;
	blo_h_osc *osc1_d = NULL;
	blo_h_osc *osc2_d = NULL;
	blo_h_tables *tables = NULL;
	sv_filter *xover_b1_data = NULL;
	sv_filter *xover_b2_data = NULL;

#line 157 "hermes_filter_1200.xml"
	long i;
	
	sample_rate = s_rate;
	count = 0;
	
	tables = blo_h_tables_new(1024);
	osc1_d = blo_h_new(tables, BLO_SINE, (float)s_rate);
	osc2_d = blo_h_new(tables, BLO_SINE, (float)s_rate);
	lfo1_d = blo_h_new(tables, BLO_SINE, (float)s_rate);
	lfo2_d = blo_h_new(tables, BLO_SINE, (float)s_rate);
	
	xover_b1_data = calloc(1, sizeof(sv_filter));
	xover_b2_data = calloc(1, sizeof(sv_filter));
	
	dela_data = malloc(3 * sizeof(float));
	dela_pos = malloc(3 * sizeof(int));
	filt_data = malloc(3 * sizeof(sv_filter *));
	for (i = 0; i < 3; i++) {
	        dela_data[i] = malloc(sample_rate * 2 * sizeof(float));
	        dela_pos[i] = 0;
	        filt_data[i] = calloc(1, sizeof(sv_filter));
	}
	lfo1 = 0.0f;
	lfo2 = 0.0f;
	lfo1_phase = 0.0f;
	lfo2_phase = 0.0f;

	plugin_data->count = count;
	plugin_data->dela_data = dela_data;
	plugin_data->dela_pos = dela_pos;
	plugin_data->filt_data = filt_data;
	plugin_data->lfo1 = lfo1;
	plugin_data->lfo1_d = lfo1_d;
	plugin_data->lfo1_phase = lfo1_phase;
	plugin_data->lfo2 = lfo2;
	plugin_data->lfo2_d = lfo2_d;
	plugin_data->lfo2_phase = lfo2_phase;
	plugin_data->osc1_d = osc1_d;
	plugin_data->osc2_d = osc2_d;
	plugin_data->tables = tables;
	plugin_data->xover_b1_data = xover_b1_data;
	plugin_data->xover_b2_data = xover_b2_data;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runHermesFilter(LADSPA_Handle instance, unsigned long sample_count) {
	HermesFilter *plugin_data = (HermesFilter *)instance;

	/* LFO1 freq (Hz) (float value) */
	const LADSPA_Data lfo1_freq = *(plugin_data->lfo1_freq);

	/* LFO1 wave (0 = sin, 1 = tri, 2 = saw, 3 = squ, 4 = s&h) (float value) */
	const LADSPA_Data lfo1_wave = *(plugin_data->lfo1_wave);

	/* LFO2 freq (Hz) (float value) */
	const LADSPA_Data lfo2_freq = *(plugin_data->lfo2_freq);

	/* LFO2 wave (0 = sin, 1 = tri, 2 = saw, 3 = squ, 4 = s&h) (float value) */
	const LADSPA_Data lfo2_wave = *(plugin_data->lfo2_wave);

	/* Osc1 freq (Hz) (float value) */
	const LADSPA_Data osc1_freq = *(plugin_data->osc1_freq);

	/* Osc1 wave (0 = sin, 1 = tri, 2 = saw, 3 = squ, 4 = noise) (float value) */
	const LADSPA_Data osc1_wave = *(plugin_data->osc1_wave);

	/* Osc2 freq (Hz) (float value) */
	const LADSPA_Data osc2_freq = *(plugin_data->osc2_freq);

	/* Osc2 wave (0 = sin, 1 = tri, 2 = saw, 3 = squ, 4 = noise) (float value) */
	const LADSPA_Data osc2_wave = *(plugin_data->osc2_wave);

	/* Ringmod 1 depth (0=none, 1=AM, 2=RM) (float value) */
	const LADSPA_Data rm1_depth = *(plugin_data->rm1_depth);

	/* Ringmod 2 depth (0=none, 1=AM, 2=RM) (float value) */
	const LADSPA_Data rm2_depth = *(plugin_data->rm2_depth);

	/* Ringmod 3 depth (0=none, 1=AM, 2=RM) (float value) */
	const LADSPA_Data rm3_depth = *(plugin_data->rm3_depth);

	/* Osc1 gain (dB) (float value) */
	const LADSPA_Data osc1_gain_db = *(plugin_data->osc1_gain_db);

	/* RM1 gain (dB) (float value) */
	const LADSPA_Data rm1_gain_db = *(plugin_data->rm1_gain_db);

	/* Osc2 gain (dB) (float value) */
	const LADSPA_Data osc2_gain_db = *(plugin_data->osc2_gain_db);

	/* RM2 gain (dB) (float value) */
	const LADSPA_Data rm2_gain_db = *(plugin_data->rm2_gain_db);

	/* Input gain (dB) (float value) */
	const LADSPA_Data in_gain_db = *(plugin_data->in_gain_db);

	/* RM3 gain (dB) (float value) */
	const LADSPA_Data rm3_gain_db = *(plugin_data->rm3_gain_db);

	/* Xover lower freq (float value) */
	const LADSPA_Data xover_lfreqp = *(plugin_data->xover_lfreqp);

	/* Xover upper freq (float value) */
	const LADSPA_Data xover_ufreqp = *(plugin_data->xover_ufreqp);

	/* Dist1 drive (float value) */
	const LADSPA_Data drive1 = *(plugin_data->drive1);

	/* Dist2 drive (float value) */
	const LADSPA_Data drive2 = *(plugin_data->drive2);

	/* Dist3 drive (float value) */
	const LADSPA_Data drive3 = *(plugin_data->drive3);

	/* Filt1 type (0=none, 1=LP, 2=HP, 3=BP, 4=BR, 5=AP) (float value) */
	const LADSPA_Data filt1_type = *(plugin_data->filt1_type);

	/* Filt1 freq (float value) */
	const LADSPA_Data filt1_freq = *(plugin_data->filt1_freq);

	/* Filt1 q (float value) */
	const LADSPA_Data filt1_q = *(plugin_data->filt1_q);

	/* Filt1 resonance (float value) */
	const LADSPA_Data filt1_res = *(plugin_data->filt1_res);

	/* Filt1 LFO1 level (float value) */
	const LADSPA_Data filt1_lfo1 = *(plugin_data->filt1_lfo1);

	/* Filt1 LFO2 level (float value) */
	const LADSPA_Data filt1_lfo2 = *(plugin_data->filt1_lfo2);

	/* Filt2 type (0=none, 1=LP, 2=HP, 3=BP, 4=BR, 5=AP) (float value) */
	const LADSPA_Data filt2_type = *(plugin_data->filt2_type);

	/* Filt2 freq (float value) */
	const LADSPA_Data filt2_freq = *(plugin_data->filt2_freq);

	/* Filt2 q (float value) */
	const LADSPA_Data filt2_q = *(plugin_data->filt2_q);

	/* Filt2 resonance (float value) */
	const LADSPA_Data filt2_res = *(plugin_data->filt2_res);

	/* Filt2 LFO1 level (float value) */
	const LADSPA_Data filt2_lfo1 = *(plugin_data->filt2_lfo1);

	/* Filt2 LFO2 level (float value) */
	const LADSPA_Data filt2_lfo2 = *(plugin_data->filt2_lfo2);

	/* Filt3 type (0=none, 1=LP, 2=HP, 3=BP, 4=BR, 5=AP) (float value) */
	const LADSPA_Data filt3_type = *(plugin_data->filt3_type);

	/* Filt3 freq (float value) */
	const LADSPA_Data filt3_freq = *(plugin_data->filt3_freq);

	/* Filt3 q (float value) */
	const LADSPA_Data filt3_q = *(plugin_data->filt3_q);

	/* Filt3 resonance (float value) */
	const LADSPA_Data filt3_res = *(plugin_data->filt3_res);

	/* Filt3 LFO1 level (float value) */
	const LADSPA_Data filt3_lfo1 = *(plugin_data->filt3_lfo1);

	/* Filt3 LFO2 level (float value) */
	const LADSPA_Data filt3_lfo2 = *(plugin_data->filt3_lfo2);

	/* Delay1 length (s) (float value) */
	const LADSPA_Data dela1_length = *(plugin_data->dela1_length);

	/* Delay1 feedback (float value) */
	const LADSPA_Data dela1_fb = *(plugin_data->dela1_fb);

	/* Delay1 wetness (float value) */
	const LADSPA_Data dela1_wet = *(plugin_data->dela1_wet);

	/* Delay2 length (s) (float value) */
	const LADSPA_Data dela2_length = *(plugin_data->dela2_length);

	/* Delay2 feedback (float value) */
	const LADSPA_Data dela2_fb = *(plugin_data->dela2_fb);

	/* Delay2 wetness (float value) */
	const LADSPA_Data dela2_wet = *(plugin_data->dela2_wet);

	/* Delay3 length (s) (float value) */
	const LADSPA_Data dela3_length = *(plugin_data->dela3_length);

	/* Delay3 feedback (float value) */
	const LADSPA_Data dela3_fb = *(plugin_data->dela3_fb);

	/* Delay3 wetness (float value) */
	const LADSPA_Data dela3_wet = *(plugin_data->dela3_wet);

	/* Band 1 gain (dB) (float value) */
	const LADSPA_Data band1_gain_db = *(plugin_data->band1_gain_db);

	/* Band 2 gain (dB) (float value) */
	const LADSPA_Data band2_gain_db = *(plugin_data->band2_gain_db);

	/* Band 3 gain (dB) (float value) */
	const LADSPA_Data band3_gain_db = *(plugin_data->band3_gain_db);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	long count = plugin_data->count;
	float ** dela_data = plugin_data->dela_data;
	int * dela_pos = plugin_data->dela_pos;
	sv_filter ** filt_data = plugin_data->filt_data;
	float lfo1 = plugin_data->lfo1;
	blo_h_osc * lfo1_d = plugin_data->lfo1_d;
	float lfo1_phase = plugin_data->lfo1_phase;
	float lfo2 = plugin_data->lfo2;
	blo_h_osc * lfo2_d = plugin_data->lfo2_d;
	float lfo2_phase = plugin_data->lfo2_phase;
	blo_h_osc * osc1_d = plugin_data->osc1_d;
	blo_h_osc * osc2_d = plugin_data->osc2_d;
	blo_h_tables * tables = plugin_data->tables;
	sv_filter * xover_b1_data = plugin_data->xover_b1_data;
	sv_filter * xover_b2_data = plugin_data->xover_b2_data;

#line 231 "hermes_filter_1200.xml"
	unsigned long pos;
	int i;
	
	// dB gains converted to coefficients
	float osc1_gain, rm1_gain, osc2_gain, rm2_gain, in_gain, rm3_gain;
	
	// Output values for the oscilators etc.
	float osc1, osc2, in, rm1, rm2, rm3, mixer1;
	
	// Outputs from xover
	float xover[3], band_gain[3];
	
	// Output values for disortions
	float dist[3];
	
	// Stuff for distortions
	float drive[3];
	
	// Stuff for filters
	float filt[3];
	float filt_freq[3];
	float filt_res[3];
	float filt_lfo1[3];
	float filt_lfo2[3];
	int filt_t[3];
	
	// Values for delays
	float dela[3], dela_wet[3], dela_fb[3];
	int dela_offset[3];
	
	// Output of mixer2
	float mixer2;
	
	// X overs
	const float xover_ufreq = f_clamp(xover_ufreqp, 200.0f, (float)(sample_rate / 6));
	const float xover_lfreq = f_clamp(xover_lfreqp, 0.0f, xover_ufreq);
	setup_f_svf(xover_b1_data, sample_rate, xover_ufreq);
	setup_f_svf(xover_b2_data, sample_rate, xover_lfreq);
	
	// Calculate delay offsets
	dela_offset[0] = dela1_length * sample_rate;
	dela_offset[1] = dela2_length * sample_rate;
	dela_offset[2] = dela3_length * sample_rate;
	for (i = 0; i < 3; i++) {
	        if (dela_offset[i] > sample_rate * 2 || dela_offset[i] < 0) {
	                dela_offset[i] = 0;
	        }
	        dela[i] = 0.0f;
	        filt_t[i] = 0;
	}
	
	// Convert dB gains to coefficients
	osc1_gain = DB_CO(osc1_gain_db);
	osc2_gain = DB_CO(osc2_gain_db);
	in_gain   = DB_CO(in_gain_db);
	rm1_gain  = DB_CO(rm1_gain_db);
	rm2_gain  = DB_CO(rm2_gain_db);
	rm3_gain  = DB_CO(rm3_gain_db);
	band_gain[0] = DB_CO(band1_gain_db);
	band_gain[1] = DB_CO(band2_gain_db);
	band_gain[2] = DB_CO(band3_gain_db);
	
	osc1_d->wave = wave_tbl(osc1_wave);
	osc2_d->wave = wave_tbl(osc2_wave);
	lfo1_d->wave = wave_tbl(lfo1_wave);
	lfo2_d->wave = wave_tbl(lfo2_wave);
	
	blo_hd_set_freq(osc1_d, osc1_freq);
	blo_hd_set_freq(osc2_d, osc2_freq);
	blo_hd_set_freq(lfo1_d, lfo1_freq * 16);
	blo_hd_set_freq(lfo2_d, lfo2_freq * 16);
	
	#define SETUP_F(n,f,q,t) setup_svf(filt_data[n], sample_rate, f, q, (int)t)
	
	// Set filter stuff
	SETUP_F(0, filt1_freq, filt1_q, filt1_type);
	SETUP_F(1, filt2_freq, filt2_q, filt2_type);
	SETUP_F(2, filt3_freq, filt3_q, filt3_type);
	
	filt_freq[0] = filt1_freq;
	filt_freq[1] = filt2_freq;
	filt_freq[2] = filt3_freq;
	filt_res[0] = filt1_res;
	filt_res[1] = filt2_res;
	filt_res[2] = filt3_res;
	filt_lfo1[0] = filt1_lfo1;
	filt_lfo1[1] = filt2_lfo1;
	filt_lfo1[2] = filt3_lfo1;
	filt_lfo2[0] = filt1_lfo2;
	filt_lfo2[1] = filt2_lfo2;
	filt_lfo2[2] = filt3_lfo2;
	
	// Setup distortions
	drive[0] = drive1;
	drive[1] = drive2;
	drive[2] = drive3;
	
	// Setup delays
	dela_wet[0] = dela1_wet;
	dela_wet[1] = dela2_wet;
	dela_wet[2] = dela3_wet;
	dela_fb[0] = dela1_fb;
	dela_fb[1] = dela2_fb;
	dela_fb[2] = dela3_fb;
	
	tables = tables; // To shut up gcc
	
	for (pos = 0; pos < sample_count; pos++) {
	        count++; // Count of number of samples processed
	
	        // Calculate oscilator values for this sample
	
	        if (osc1_d->wave == NOISE) {
	                osc1 = rand() * (0.5f/(float)RAND_MAX) - 1.0f;
	        } else {
	                osc1 = blo_hd_run_lin(osc1_d);
	        }
	        if (osc2_d->wave == NOISE) {
	                osc2 = rand() * (0.5f/(float)RAND_MAX) - 1.0f;
	        } else {
	                osc2 = blo_hd_run_lin(osc2_d);
	        }
	
	        // Calculate LFO values every 16 samples
	        if ((count & 15) == 1) {
	                // Calculate lfo values
	                if (lfo1_d->wave == NOISE) {
	                        lfo1_phase += lfo1_freq;
	                        if (lfo1_phase >= sample_rate) {
	                                lfo1_phase -= sample_rate;
	                                lfo1 = rand() * (0.5f/(float)RAND_MAX) - 1.0f;
	                        }
	                } else {
	                        lfo1 = blo_hd_run_lin(lfo1_d);
	                }
	                if (lfo2_d->wave == NOISE) {
	                        lfo2_phase += lfo1_freq;
	                        if (lfo2_phase >= sample_rate) {
	                                lfo2_phase -= sample_rate;
	                                lfo2 = rand() * (0.5f/(float)RAND_MAX) - 1.0f;
	                        }
	                } else {
	                        lfo2 = blo_hd_run_lin(lfo2_d);
	                }
	        }
	
	        in = input[pos];
	        rm1 = RINGMOD(osc2, osc1, rm1_depth);
	        rm2 = RINGMOD(in, osc2, rm2_depth);
	        rm3 = RINGMOD(osc1, in, rm3_depth);
	
	        mixer1 = (osc1 * osc1_gain) + (osc2 * osc2_gain) + (in * in_gain) +
	                 (rm1 * rm1_gain) + (rm2 * rm2_gain) + (rm3 * rm3_gain);
	
	        mixer1 = soft_clip(mixer1);
	
	        // Higpass off the top band
	        xover[0] = run_svf(xover_b1_data, mixer1);
	        // Lowpass off the bottom band
	        xover[2] = run_svf(xover_b2_data, mixer1);
	        // The middle band is whats left
	        xover[1] = mixer1 - xover[0] - xover[2];
	
	        mixer2 = 0.0f;
	        for (i = 0; i < 3; i++) {
	                dist[i] = xover[i]*(fabs(xover[i]) + drive1)/(xover[i]*xover[i] + (drive[i]-1)*fabs(xover[i]) + 1.0f);
	
	                if (filt_t[i] == 0) {
	                        filt[i] = dist[i];
	                } else {
	                        if (count % 16 == 1) {
	                                setup_f_svf(filt_data[i], sample_rate, filt_freq[i]+LFO(filt_lfo1[i], filt_lfo2[i]));
	                        }
	                        filt[i] = run_svf(filt_data[i], dist[i] + (filt_res[i] * (filt_data[i])->b));
	                }
	
	                dela[i] = (dela_data[i][dela_pos[i]] * dela_wet[i]) + filt[i];
	                dela_data[i][(dela_pos[i] + dela_offset[i]) %
	                 (2 * sample_rate)] = filt[i] + (dela[i] * dela_fb[i]);
	                dela_pos[i] = (dela_pos[i] + 1) % (2 * sample_rate);
	
	                mixer2 += band_gain[i] * dela[i];
	        }
	
	        buffer_write(output[pos], soft_clip(mixer2));
	}
	
	plugin_data->count = count;
	plugin_data->lfo1 = lfo1;
	plugin_data->lfo2 = lfo2;
	plugin_data->lfo1_phase = lfo1_phase;
	plugin_data->lfo2_phase = lfo2_phase;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainHermesFilter(LADSPA_Handle instance, LADSPA_Data gain) {
	((HermesFilter *)instance)->run_adding_gain = gain;
}

static void runAddingHermesFilter(LADSPA_Handle instance, unsigned long sample_count) {
	HermesFilter *plugin_data = (HermesFilter *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* LFO1 freq (Hz) (float value) */
	const LADSPA_Data lfo1_freq = *(plugin_data->lfo1_freq);

	/* LFO1 wave (0 = sin, 1 = tri, 2 = saw, 3 = squ, 4 = s&h) (float value) */
	const LADSPA_Data lfo1_wave = *(plugin_data->lfo1_wave);

	/* LFO2 freq (Hz) (float value) */
	const LADSPA_Data lfo2_freq = *(plugin_data->lfo2_freq);

	/* LFO2 wave (0 = sin, 1 = tri, 2 = saw, 3 = squ, 4 = s&h) (float value) */
	const LADSPA_Data lfo2_wave = *(plugin_data->lfo2_wave);

	/* Osc1 freq (Hz) (float value) */
	const LADSPA_Data osc1_freq = *(plugin_data->osc1_freq);

	/* Osc1 wave (0 = sin, 1 = tri, 2 = saw, 3 = squ, 4 = noise) (float value) */
	const LADSPA_Data osc1_wave = *(plugin_data->osc1_wave);

	/* Osc2 freq (Hz) (float value) */
	const LADSPA_Data osc2_freq = *(plugin_data->osc2_freq);

	/* Osc2 wave (0 = sin, 1 = tri, 2 = saw, 3 = squ, 4 = noise) (float value) */
	const LADSPA_Data osc2_wave = *(plugin_data->osc2_wave);

	/* Ringmod 1 depth (0=none, 1=AM, 2=RM) (float value) */
	const LADSPA_Data rm1_depth = *(plugin_data->rm1_depth);

	/* Ringmod 2 depth (0=none, 1=AM, 2=RM) (float value) */
	const LADSPA_Data rm2_depth = *(plugin_data->rm2_depth);

	/* Ringmod 3 depth (0=none, 1=AM, 2=RM) (float value) */
	const LADSPA_Data rm3_depth = *(plugin_data->rm3_depth);

	/* Osc1 gain (dB) (float value) */
	const LADSPA_Data osc1_gain_db = *(plugin_data->osc1_gain_db);

	/* RM1 gain (dB) (float value) */
	const LADSPA_Data rm1_gain_db = *(plugin_data->rm1_gain_db);

	/* Osc2 gain (dB) (float value) */
	const LADSPA_Data osc2_gain_db = *(plugin_data->osc2_gain_db);

	/* RM2 gain (dB) (float value) */
	const LADSPA_Data rm2_gain_db = *(plugin_data->rm2_gain_db);

	/* Input gain (dB) (float value) */
	const LADSPA_Data in_gain_db = *(plugin_data->in_gain_db);

	/* RM3 gain (dB) (float value) */
	const LADSPA_Data rm3_gain_db = *(plugin_data->rm3_gain_db);

	/* Xover lower freq (float value) */
	const LADSPA_Data xover_lfreqp = *(plugin_data->xover_lfreqp);

	/* Xover upper freq (float value) */
	const LADSPA_Data xover_ufreqp = *(plugin_data->xover_ufreqp);

	/* Dist1 drive (float value) */
	const LADSPA_Data drive1 = *(plugin_data->drive1);

	/* Dist2 drive (float value) */
	const LADSPA_Data drive2 = *(plugin_data->drive2);

	/* Dist3 drive (float value) */
	const LADSPA_Data drive3 = *(plugin_data->drive3);

	/* Filt1 type (0=none, 1=LP, 2=HP, 3=BP, 4=BR, 5=AP) (float value) */
	const LADSPA_Data filt1_type = *(plugin_data->filt1_type);

	/* Filt1 freq (float value) */
	const LADSPA_Data filt1_freq = *(plugin_data->filt1_freq);

	/* Filt1 q (float value) */
	const LADSPA_Data filt1_q = *(plugin_data->filt1_q);

	/* Filt1 resonance (float value) */
	const LADSPA_Data filt1_res = *(plugin_data->filt1_res);

	/* Filt1 LFO1 level (float value) */
	const LADSPA_Data filt1_lfo1 = *(plugin_data->filt1_lfo1);

	/* Filt1 LFO2 level (float value) */
	const LADSPA_Data filt1_lfo2 = *(plugin_data->filt1_lfo2);

	/* Filt2 type (0=none, 1=LP, 2=HP, 3=BP, 4=BR, 5=AP) (float value) */
	const LADSPA_Data filt2_type = *(plugin_data->filt2_type);

	/* Filt2 freq (float value) */
	const LADSPA_Data filt2_freq = *(plugin_data->filt2_freq);

	/* Filt2 q (float value) */
	const LADSPA_Data filt2_q = *(plugin_data->filt2_q);

	/* Filt2 resonance (float value) */
	const LADSPA_Data filt2_res = *(plugin_data->filt2_res);

	/* Filt2 LFO1 level (float value) */
	const LADSPA_Data filt2_lfo1 = *(plugin_data->filt2_lfo1);

	/* Filt2 LFO2 level (float value) */
	const LADSPA_Data filt2_lfo2 = *(plugin_data->filt2_lfo2);

	/* Filt3 type (0=none, 1=LP, 2=HP, 3=BP, 4=BR, 5=AP) (float value) */
	const LADSPA_Data filt3_type = *(plugin_data->filt3_type);

	/* Filt3 freq (float value) */
	const LADSPA_Data filt3_freq = *(plugin_data->filt3_freq);

	/* Filt3 q (float value) */
	const LADSPA_Data filt3_q = *(plugin_data->filt3_q);

	/* Filt3 resonance (float value) */
	const LADSPA_Data filt3_res = *(plugin_data->filt3_res);

	/* Filt3 LFO1 level (float value) */
	const LADSPA_Data filt3_lfo1 = *(plugin_data->filt3_lfo1);

	/* Filt3 LFO2 level (float value) */
	const LADSPA_Data filt3_lfo2 = *(plugin_data->filt3_lfo2);

	/* Delay1 length (s) (float value) */
	const LADSPA_Data dela1_length = *(plugin_data->dela1_length);

	/* Delay1 feedback (float value) */
	const LADSPA_Data dela1_fb = *(plugin_data->dela1_fb);

	/* Delay1 wetness (float value) */
	const LADSPA_Data dela1_wet = *(plugin_data->dela1_wet);

	/* Delay2 length (s) (float value) */
	const LADSPA_Data dela2_length = *(plugin_data->dela2_length);

	/* Delay2 feedback (float value) */
	const LADSPA_Data dela2_fb = *(plugin_data->dela2_fb);

	/* Delay2 wetness (float value) */
	const LADSPA_Data dela2_wet = *(plugin_data->dela2_wet);

	/* Delay3 length (s) (float value) */
	const LADSPA_Data dela3_length = *(plugin_data->dela3_length);

	/* Delay3 feedback (float value) */
	const LADSPA_Data dela3_fb = *(plugin_data->dela3_fb);

	/* Delay3 wetness (float value) */
	const LADSPA_Data dela3_wet = *(plugin_data->dela3_wet);

	/* Band 1 gain (dB) (float value) */
	const LADSPA_Data band1_gain_db = *(plugin_data->band1_gain_db);

	/* Band 2 gain (dB) (float value) */
	const LADSPA_Data band2_gain_db = *(plugin_data->band2_gain_db);

	/* Band 3 gain (dB) (float value) */
	const LADSPA_Data band3_gain_db = *(plugin_data->band3_gain_db);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	long count = plugin_data->count;
	float ** dela_data = plugin_data->dela_data;
	int * dela_pos = plugin_data->dela_pos;
	sv_filter ** filt_data = plugin_data->filt_data;
	float lfo1 = plugin_data->lfo1;
	blo_h_osc * lfo1_d = plugin_data->lfo1_d;
	float lfo1_phase = plugin_data->lfo1_phase;
	float lfo2 = plugin_data->lfo2;
	blo_h_osc * lfo2_d = plugin_data->lfo2_d;
	float lfo2_phase = plugin_data->lfo2_phase;
	blo_h_osc * osc1_d = plugin_data->osc1_d;
	blo_h_osc * osc2_d = plugin_data->osc2_d;
	blo_h_tables * tables = plugin_data->tables;
	sv_filter * xover_b1_data = plugin_data->xover_b1_data;
	sv_filter * xover_b2_data = plugin_data->xover_b2_data;

#line 231 "hermes_filter_1200.xml"
	unsigned long pos;
	int i;
	
	// dB gains converted to coefficients
	float osc1_gain, rm1_gain, osc2_gain, rm2_gain, in_gain, rm3_gain;
	
	// Output values for the oscilators etc.
	float osc1, osc2, in, rm1, rm2, rm3, mixer1;
	
	// Outputs from xover
	float xover[3], band_gain[3];
	
	// Output values for disortions
	float dist[3];
	
	// Stuff for distortions
	float drive[3];
	
	// Stuff for filters
	float filt[3];
	float filt_freq[3];
	float filt_res[3];
	float filt_lfo1[3];
	float filt_lfo2[3];
	int filt_t[3];
	
	// Values for delays
	float dela[3], dela_wet[3], dela_fb[3];
	int dela_offset[3];
	
	// Output of mixer2
	float mixer2;
	
	// X overs
	const float xover_ufreq = f_clamp(xover_ufreqp, 200.0f, (float)(sample_rate / 6));
	const float xover_lfreq = f_clamp(xover_lfreqp, 0.0f, xover_ufreq);
	setup_f_svf(xover_b1_data, sample_rate, xover_ufreq);
	setup_f_svf(xover_b2_data, sample_rate, xover_lfreq);
	
	// Calculate delay offsets
	dela_offset[0] = dela1_length * sample_rate;
	dela_offset[1] = dela2_length * sample_rate;
	dela_offset[2] = dela3_length * sample_rate;
	for (i = 0; i < 3; i++) {
	        if (dela_offset[i] > sample_rate * 2 || dela_offset[i] < 0) {
	                dela_offset[i] = 0;
	        }
	        dela[i] = 0.0f;
	        filt_t[i] = 0;
	}
	
	// Convert dB gains to coefficients
	osc1_gain = DB_CO(osc1_gain_db);
	osc2_gain = DB_CO(osc2_gain_db);
	in_gain   = DB_CO(in_gain_db);
	rm1_gain  = DB_CO(rm1_gain_db);
	rm2_gain  = DB_CO(rm2_gain_db);
	rm3_gain  = DB_CO(rm3_gain_db);
	band_gain[0] = DB_CO(band1_gain_db);
	band_gain[1] = DB_CO(band2_gain_db);
	band_gain[2] = DB_CO(band3_gain_db);
	
	osc1_d->wave = wave_tbl(osc1_wave);
	osc2_d->wave = wave_tbl(osc2_wave);
	lfo1_d->wave = wave_tbl(lfo1_wave);
	lfo2_d->wave = wave_tbl(lfo2_wave);
	
	blo_hd_set_freq(osc1_d, osc1_freq);
	blo_hd_set_freq(osc2_d, osc2_freq);
	blo_hd_set_freq(lfo1_d, lfo1_freq * 16);
	blo_hd_set_freq(lfo2_d, lfo2_freq * 16);
	
	#define SETUP_F(n,f,q,t) setup_svf(filt_data[n], sample_rate, f, q, (int)t)
	
	// Set filter stuff
	SETUP_F(0, filt1_freq, filt1_q, filt1_type);
	SETUP_F(1, filt2_freq, filt2_q, filt2_type);
	SETUP_F(2, filt3_freq, filt3_q, filt3_type);
	
	filt_freq[0] = filt1_freq;
	filt_freq[1] = filt2_freq;
	filt_freq[2] = filt3_freq;
	filt_res[0] = filt1_res;
	filt_res[1] = filt2_res;
	filt_res[2] = filt3_res;
	filt_lfo1[0] = filt1_lfo1;
	filt_lfo1[1] = filt2_lfo1;
	filt_lfo1[2] = filt3_lfo1;
	filt_lfo2[0] = filt1_lfo2;
	filt_lfo2[1] = filt2_lfo2;
	filt_lfo2[2] = filt3_lfo2;
	
	// Setup distortions
	drive[0] = drive1;
	drive[1] = drive2;
	drive[2] = drive3;
	
	// Setup delays
	dela_wet[0] = dela1_wet;
	dela_wet[1] = dela2_wet;
	dela_wet[2] = dela3_wet;
	dela_fb[0] = dela1_fb;
	dela_fb[1] = dela2_fb;
	dela_fb[2] = dela3_fb;
	
	tables = tables; // To shut up gcc
	
	for (pos = 0; pos < sample_count; pos++) {
	        count++; // Count of number of samples processed
	
	        // Calculate oscilator values for this sample
	
	        if (osc1_d->wave == NOISE) {
	                osc1 = rand() * (0.5f/(float)RAND_MAX) - 1.0f;
	        } else {
	                osc1 = blo_hd_run_lin(osc1_d);
	        }
	        if (osc2_d->wave == NOISE) {
	                osc2 = rand() * (0.5f/(float)RAND_MAX) - 1.0f;
	        } else {
	                osc2 = blo_hd_run_lin(osc2_d);
	        }
	
	        // Calculate LFO values every 16 samples
	        if ((count & 15) == 1) {
	                // Calculate lfo values
	                if (lfo1_d->wave == NOISE) {
	                        lfo1_phase += lfo1_freq;
	                        if (lfo1_phase >= sample_rate) {
	                                lfo1_phase -= sample_rate;
	                                lfo1 = rand() * (0.5f/(float)RAND_MAX) - 1.0f;
	                        }
	                } else {
	                        lfo1 = blo_hd_run_lin(lfo1_d);
	                }
	                if (lfo2_d->wave == NOISE) {
	                        lfo2_phase += lfo1_freq;
	                        if (lfo2_phase >= sample_rate) {
	                                lfo2_phase -= sample_rate;
	                                lfo2 = rand() * (0.5f/(float)RAND_MAX) - 1.0f;
	                        }
	                } else {
	                        lfo2 = blo_hd_run_lin(lfo2_d);
	                }
	        }
	
	        in = input[pos];
	        rm1 = RINGMOD(osc2, osc1, rm1_depth);
	        rm2 = RINGMOD(in, osc2, rm2_depth);
	        rm3 = RINGMOD(osc1, in, rm3_depth);
	
	        mixer1 = (osc1 * osc1_gain) + (osc2 * osc2_gain) + (in * in_gain) +
	                 (rm1 * rm1_gain) + (rm2 * rm2_gain) + (rm3 * rm3_gain);
	
	        mixer1 = soft_clip(mixer1);
	
	        // Higpass off the top band
	        xover[0] = run_svf(xover_b1_data, mixer1);
	        // Lowpass off the bottom band
	        xover[2] = run_svf(xover_b2_data, mixer1);
	        // The middle band is whats left
	        xover[1] = mixer1 - xover[0] - xover[2];
	
	        mixer2 = 0.0f;
	        for (i = 0; i < 3; i++) {
	                dist[i] = xover[i]*(fabs(xover[i]) + drive1)/(xover[i]*xover[i] + (drive[i]-1)*fabs(xover[i]) + 1.0f);
	
	                if (filt_t[i] == 0) {
	                        filt[i] = dist[i];
	                } else {
	                        if (count % 16 == 1) {
	                                setup_f_svf(filt_data[i], sample_rate, filt_freq[i]+LFO(filt_lfo1[i], filt_lfo2[i]));
	                        }
	                        filt[i] = run_svf(filt_data[i], dist[i] + (filt_res[i] * (filt_data[i])->b));
	                }
	
	                dela[i] = (dela_data[i][dela_pos[i]] * dela_wet[i]) + filt[i];
	                dela_data[i][(dela_pos[i] + dela_offset[i]) %
	                 (2 * sample_rate)] = filt[i] + (dela[i] * dela_fb[i]);
	                dela_pos[i] = (dela_pos[i] + 1) % (2 * sample_rate);
	
	                mixer2 += band_gain[i] * dela[i];
	        }
	
	        buffer_write(output[pos], soft_clip(mixer2));
	}
	
	plugin_data->count = count;
	plugin_data->lfo1 = lfo1;
	plugin_data->lfo2 = lfo2;
	plugin_data->lfo1_phase = lfo1_phase;
	plugin_data->lfo2_phase = lfo2_phase;
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


	hermesFilterDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (hermesFilterDescriptor) {
		hermesFilterDescriptor->UniqueID = 1200;
		hermesFilterDescriptor->Label = "hermesFilter";
		hermesFilterDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		hermesFilterDescriptor->Name =
		 D_("Hermes Filter");
		hermesFilterDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		hermesFilterDescriptor->Copyright =
		 "GPL";
		hermesFilterDescriptor->PortCount = 54;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(54,
		 sizeof(LADSPA_PortDescriptor));
		hermesFilterDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(54,
		 sizeof(LADSPA_PortRangeHint));
		hermesFilterDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(54, sizeof(char*));
		hermesFilterDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for LFO1 freq (Hz) */
		port_descriptors[HERMESFILTER_LFO1_FREQ] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_LFO1_FREQ] =
		 D_("LFO1 freq (Hz)");
		port_range_hints[HERMESFILTER_LFO1_FREQ].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[HERMESFILTER_LFO1_FREQ].LowerBound = 0;
		port_range_hints[HERMESFILTER_LFO1_FREQ].UpperBound = 1000;

		/* Parameters for LFO1 wave (0 = sin, 1 = tri, 2 = saw, 3 = squ, 4 = s&h) */
		port_descriptors[HERMESFILTER_LFO1_WAVE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_LFO1_WAVE] =
		 D_("LFO1 wave (0 = sin, 1 = tri, 2 = saw, 3 = squ, 4 = s&h)");
		port_range_hints[HERMESFILTER_LFO1_WAVE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_LFO1_WAVE].LowerBound = 0;
		port_range_hints[HERMESFILTER_LFO1_WAVE].UpperBound = 4;

		/* Parameters for LFO2 freq (Hz) */
		port_descriptors[HERMESFILTER_LFO2_FREQ] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_LFO2_FREQ] =
		 D_("LFO2 freq (Hz)");
		port_range_hints[HERMESFILTER_LFO2_FREQ].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[HERMESFILTER_LFO2_FREQ].LowerBound = 0;
		port_range_hints[HERMESFILTER_LFO2_FREQ].UpperBound = 1000;

		/* Parameters for LFO2 wave (0 = sin, 1 = tri, 2 = saw, 3 = squ, 4 = s&h) */
		port_descriptors[HERMESFILTER_LFO2_WAVE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_LFO2_WAVE] =
		 D_("LFO2 wave (0 = sin, 1 = tri, 2 = saw, 3 = squ, 4 = s&h)");
		port_range_hints[HERMESFILTER_LFO2_WAVE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_LFO2_WAVE].LowerBound = 0;
		port_range_hints[HERMESFILTER_LFO2_WAVE].UpperBound = 4;

		/* Parameters for Osc1 freq (Hz) */
		port_descriptors[HERMESFILTER_OSC1_FREQ] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_OSC1_FREQ] =
		 D_("Osc1 freq (Hz)");
		port_range_hints[HERMESFILTER_OSC1_FREQ].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_440;
		port_range_hints[HERMESFILTER_OSC1_FREQ].LowerBound = 0;
		port_range_hints[HERMESFILTER_OSC1_FREQ].UpperBound = 4000;

		/* Parameters for Osc1 wave (0 = sin, 1 = tri, 2 = saw, 3 = squ, 4 = noise) */
		port_descriptors[HERMESFILTER_OSC1_WAVE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_OSC1_WAVE] =
		 D_("Osc1 wave (0 = sin, 1 = tri, 2 = saw, 3 = squ, 4 = noise)");
		port_range_hints[HERMESFILTER_OSC1_WAVE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_OSC1_WAVE].LowerBound = 0;
		port_range_hints[HERMESFILTER_OSC1_WAVE].UpperBound = 4;

		/* Parameters for Osc2 freq (Hz) */
		port_descriptors[HERMESFILTER_OSC2_FREQ] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_OSC2_FREQ] =
		 D_("Osc2 freq (Hz)");
		port_range_hints[HERMESFILTER_OSC2_FREQ].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_440;
		port_range_hints[HERMESFILTER_OSC2_FREQ].LowerBound = 0;
		port_range_hints[HERMESFILTER_OSC2_FREQ].UpperBound = 4000;

		/* Parameters for Osc2 wave (0 = sin, 1 = tri, 2 = saw, 3 = squ, 4 = noise) */
		port_descriptors[HERMESFILTER_OSC2_WAVE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_OSC2_WAVE] =
		 D_("Osc2 wave (0 = sin, 1 = tri, 2 = saw, 3 = squ, 4 = noise)");
		port_range_hints[HERMESFILTER_OSC2_WAVE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_OSC2_WAVE].LowerBound = 0;
		port_range_hints[HERMESFILTER_OSC2_WAVE].UpperBound = 4;

		/* Parameters for Ringmod 1 depth (0=none, 1=AM, 2=RM) */
		port_descriptors[HERMESFILTER_RM1_DEPTH] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_RM1_DEPTH] =
		 D_("Ringmod 1 depth (0=none, 1=AM, 2=RM)");
		port_range_hints[HERMESFILTER_RM1_DEPTH].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_RM1_DEPTH].LowerBound = 0;
		port_range_hints[HERMESFILTER_RM1_DEPTH].UpperBound = 2;

		/* Parameters for Ringmod 2 depth (0=none, 1=AM, 2=RM) */
		port_descriptors[HERMESFILTER_RM2_DEPTH] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_RM2_DEPTH] =
		 D_("Ringmod 2 depth (0=none, 1=AM, 2=RM)");
		port_range_hints[HERMESFILTER_RM2_DEPTH].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_RM2_DEPTH].LowerBound = 0;
		port_range_hints[HERMESFILTER_RM2_DEPTH].UpperBound = 2;

		/* Parameters for Ringmod 3 depth (0=none, 1=AM, 2=RM) */
		port_descriptors[HERMESFILTER_RM3_DEPTH] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_RM3_DEPTH] =
		 D_("Ringmod 3 depth (0=none, 1=AM, 2=RM)");
		port_range_hints[HERMESFILTER_RM3_DEPTH].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_RM3_DEPTH].LowerBound = 0;
		port_range_hints[HERMESFILTER_RM3_DEPTH].UpperBound = 2;

		/* Parameters for Osc1 gain (dB) */
		port_descriptors[HERMESFILTER_OSC1_GAIN_DB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_OSC1_GAIN_DB] =
		 D_("Osc1 gain (dB)");
		port_range_hints[HERMESFILTER_OSC1_GAIN_DB].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MINIMUM;
		port_range_hints[HERMESFILTER_OSC1_GAIN_DB].LowerBound = -70;
		port_range_hints[HERMESFILTER_OSC1_GAIN_DB].UpperBound = +20;

		/* Parameters for RM1 gain (dB) */
		port_descriptors[HERMESFILTER_RM1_GAIN_DB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_RM1_GAIN_DB] =
		 D_("RM1 gain (dB)");
		port_range_hints[HERMESFILTER_RM1_GAIN_DB].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MINIMUM;
		port_range_hints[HERMESFILTER_RM1_GAIN_DB].LowerBound = -70;
		port_range_hints[HERMESFILTER_RM1_GAIN_DB].UpperBound = +20;

		/* Parameters for Osc2 gain (dB) */
		port_descriptors[HERMESFILTER_OSC2_GAIN_DB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_OSC2_GAIN_DB] =
		 D_("Osc2 gain (dB)");
		port_range_hints[HERMESFILTER_OSC2_GAIN_DB].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MINIMUM;
		port_range_hints[HERMESFILTER_OSC2_GAIN_DB].LowerBound = -70;
		port_range_hints[HERMESFILTER_OSC2_GAIN_DB].UpperBound = +20;

		/* Parameters for RM2 gain (dB) */
		port_descriptors[HERMESFILTER_RM2_GAIN_DB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_RM2_GAIN_DB] =
		 D_("RM2 gain (dB)");
		port_range_hints[HERMESFILTER_RM2_GAIN_DB].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MINIMUM;
		port_range_hints[HERMESFILTER_RM2_GAIN_DB].LowerBound = -70;
		port_range_hints[HERMESFILTER_RM2_GAIN_DB].UpperBound = +20;

		/* Parameters for Input gain (dB) */
		port_descriptors[HERMESFILTER_IN_GAIN_DB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_IN_GAIN_DB] =
		 D_("Input gain (dB)");
		port_range_hints[HERMESFILTER_IN_GAIN_DB].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_IN_GAIN_DB].LowerBound = -70;
		port_range_hints[HERMESFILTER_IN_GAIN_DB].UpperBound = +20;

		/* Parameters for RM3 gain (dB) */
		port_descriptors[HERMESFILTER_RM3_GAIN_DB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_RM3_GAIN_DB] =
		 D_("RM3 gain (dB)");
		port_range_hints[HERMESFILTER_RM3_GAIN_DB].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MINIMUM;
		port_range_hints[HERMESFILTER_RM3_GAIN_DB].LowerBound = -70;
		port_range_hints[HERMESFILTER_RM3_GAIN_DB].UpperBound = +20;

		/* Parameters for Xover lower freq */
		port_descriptors[HERMESFILTER_XOVER_LFREQP] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_XOVER_LFREQP] =
		 D_("Xover lower freq");
		port_range_hints[HERMESFILTER_XOVER_LFREQP].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[HERMESFILTER_XOVER_LFREQP].LowerBound = 50;
		port_range_hints[HERMESFILTER_XOVER_LFREQP].UpperBound = 6000;

		/* Parameters for Xover upper freq */
		port_descriptors[HERMESFILTER_XOVER_UFREQP] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_XOVER_UFREQP] =
		 D_("Xover upper freq");
		port_range_hints[HERMESFILTER_XOVER_UFREQP].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_HIGH;
		port_range_hints[HERMESFILTER_XOVER_UFREQP].LowerBound = 1000;
		port_range_hints[HERMESFILTER_XOVER_UFREQP].UpperBound = 10000;

		/* Parameters for Dist1 drive */
		port_descriptors[HERMESFILTER_DRIVE1] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_DRIVE1] =
		 D_("Dist1 drive");
		port_range_hints[HERMESFILTER_DRIVE1].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_DRIVE1].LowerBound = 0;
		port_range_hints[HERMESFILTER_DRIVE1].UpperBound = 3;

		/* Parameters for Dist2 drive */
		port_descriptors[HERMESFILTER_DRIVE2] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_DRIVE2] =
		 D_("Dist2 drive");
		port_range_hints[HERMESFILTER_DRIVE2].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_DRIVE2].LowerBound = 0;
		port_range_hints[HERMESFILTER_DRIVE2].UpperBound = 3;

		/* Parameters for Dist3 drive */
		port_descriptors[HERMESFILTER_DRIVE3] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_DRIVE3] =
		 D_("Dist3 drive");
		port_range_hints[HERMESFILTER_DRIVE3].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_DRIVE3].LowerBound = 0;
		port_range_hints[HERMESFILTER_DRIVE3].UpperBound = 3;

		/* Parameters for Filt1 type (0=none, 1=LP, 2=HP, 3=BP, 4=BR, 5=AP) */
		port_descriptors[HERMESFILTER_FILT1_TYPE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_FILT1_TYPE] =
		 D_("Filt1 type (0=none, 1=LP, 2=HP, 3=BP, 4=BR, 5=AP)");
		port_range_hints[HERMESFILTER_FILT1_TYPE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_FILT1_TYPE].LowerBound = 0;
		port_range_hints[HERMESFILTER_FILT1_TYPE].UpperBound = 5;

		/* Parameters for Filt1 freq */
		port_descriptors[HERMESFILTER_FILT1_FREQ] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_FILT1_FREQ] =
		 D_("Filt1 freq");
		port_range_hints[HERMESFILTER_FILT1_FREQ].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_440;
		port_range_hints[HERMESFILTER_FILT1_FREQ].LowerBound = 0;
		port_range_hints[HERMESFILTER_FILT1_FREQ].UpperBound = 8000;

		/* Parameters for Filt1 q */
		port_descriptors[HERMESFILTER_FILT1_Q] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_FILT1_Q] =
		 D_("Filt1 q");
		port_range_hints[HERMESFILTER_FILT1_Q].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_FILT1_Q].LowerBound = 0;
		port_range_hints[HERMESFILTER_FILT1_Q].UpperBound = 1;

		/* Parameters for Filt1 resonance */
		port_descriptors[HERMESFILTER_FILT1_RES] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_FILT1_RES] =
		 D_("Filt1 resonance");
		port_range_hints[HERMESFILTER_FILT1_RES].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_FILT1_RES].LowerBound = 0;
		port_range_hints[HERMESFILTER_FILT1_RES].UpperBound = 1;

		/* Parameters for Filt1 LFO1 level */
		port_descriptors[HERMESFILTER_FILT1_LFO1] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_FILT1_LFO1] =
		 D_("Filt1 LFO1 level");
		port_range_hints[HERMESFILTER_FILT1_LFO1].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_FILT1_LFO1].LowerBound = -500;
		port_range_hints[HERMESFILTER_FILT1_LFO1].UpperBound = 500;

		/* Parameters for Filt1 LFO2 level */
		port_descriptors[HERMESFILTER_FILT1_LFO2] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_FILT1_LFO2] =
		 D_("Filt1 LFO2 level");
		port_range_hints[HERMESFILTER_FILT1_LFO2].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_FILT1_LFO2].LowerBound = -500;
		port_range_hints[HERMESFILTER_FILT1_LFO2].UpperBound = 500;

		/* Parameters for Filt2 type (0=none, 1=LP, 2=HP, 3=BP, 4=BR, 5=AP) */
		port_descriptors[HERMESFILTER_FILT2_TYPE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_FILT2_TYPE] =
		 D_("Filt2 type (0=none, 1=LP, 2=HP, 3=BP, 4=BR, 5=AP)");
		port_range_hints[HERMESFILTER_FILT2_TYPE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_FILT2_TYPE].LowerBound = 0;
		port_range_hints[HERMESFILTER_FILT2_TYPE].UpperBound = 5;

		/* Parameters for Filt2 freq */
		port_descriptors[HERMESFILTER_FILT2_FREQ] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_FILT2_FREQ] =
		 D_("Filt2 freq");
		port_range_hints[HERMESFILTER_FILT2_FREQ].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_440;
		port_range_hints[HERMESFILTER_FILT2_FREQ].LowerBound = 0;
		port_range_hints[HERMESFILTER_FILT2_FREQ].UpperBound = 8000;

		/* Parameters for Filt2 q */
		port_descriptors[HERMESFILTER_FILT2_Q] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_FILT2_Q] =
		 D_("Filt2 q");
		port_range_hints[HERMESFILTER_FILT2_Q].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_FILT2_Q].LowerBound = 0;
		port_range_hints[HERMESFILTER_FILT2_Q].UpperBound = 1;

		/* Parameters for Filt2 resonance */
		port_descriptors[HERMESFILTER_FILT2_RES] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_FILT2_RES] =
		 D_("Filt2 resonance");
		port_range_hints[HERMESFILTER_FILT2_RES].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_FILT2_RES].LowerBound = 0;
		port_range_hints[HERMESFILTER_FILT2_RES].UpperBound = 1;

		/* Parameters for Filt2 LFO1 level */
		port_descriptors[HERMESFILTER_FILT2_LFO1] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_FILT2_LFO1] =
		 D_("Filt2 LFO1 level");
		port_range_hints[HERMESFILTER_FILT2_LFO1].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_FILT2_LFO1].LowerBound = -500;
		port_range_hints[HERMESFILTER_FILT2_LFO1].UpperBound = 500;

		/* Parameters for Filt2 LFO2 level */
		port_descriptors[HERMESFILTER_FILT2_LFO2] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_FILT2_LFO2] =
		 D_("Filt2 LFO2 level");
		port_range_hints[HERMESFILTER_FILT2_LFO2].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_FILT2_LFO2].LowerBound = -500;
		port_range_hints[HERMESFILTER_FILT2_LFO2].UpperBound = 500;

		/* Parameters for Filt3 type (0=none, 1=LP, 2=HP, 3=BP, 4=BR, 5=AP) */
		port_descriptors[HERMESFILTER_FILT3_TYPE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_FILT3_TYPE] =
		 D_("Filt3 type (0=none, 1=LP, 2=HP, 3=BP, 4=BR, 5=AP)");
		port_range_hints[HERMESFILTER_FILT3_TYPE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_FILT3_TYPE].LowerBound = 0;
		port_range_hints[HERMESFILTER_FILT3_TYPE].UpperBound = 5;

		/* Parameters for Filt3 freq */
		port_descriptors[HERMESFILTER_FILT3_FREQ] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_FILT3_FREQ] =
		 D_("Filt3 freq");
		port_range_hints[HERMESFILTER_FILT3_FREQ].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_440;
		port_range_hints[HERMESFILTER_FILT3_FREQ].LowerBound = 0;
		port_range_hints[HERMESFILTER_FILT3_FREQ].UpperBound = 8000;

		/* Parameters for Filt3 q */
		port_descriptors[HERMESFILTER_FILT3_Q] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_FILT3_Q] =
		 D_("Filt3 q");
		port_range_hints[HERMESFILTER_FILT3_Q].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_FILT3_Q].LowerBound = 0;
		port_range_hints[HERMESFILTER_FILT3_Q].UpperBound = 1;

		/* Parameters for Filt3 resonance */
		port_descriptors[HERMESFILTER_FILT3_RES] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_FILT3_RES] =
		 D_("Filt3 resonance");
		port_range_hints[HERMESFILTER_FILT3_RES].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_FILT3_RES].LowerBound = 0;
		port_range_hints[HERMESFILTER_FILT3_RES].UpperBound = 1;

		/* Parameters for Filt3 LFO1 level */
		port_descriptors[HERMESFILTER_FILT3_LFO1] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_FILT3_LFO1] =
		 D_("Filt3 LFO1 level");
		port_range_hints[HERMESFILTER_FILT3_LFO1].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_FILT3_LFO1].LowerBound = -500;
		port_range_hints[HERMESFILTER_FILT3_LFO1].UpperBound = 500;

		/* Parameters for Filt3 LFO2 level */
		port_descriptors[HERMESFILTER_FILT3_LFO2] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_FILT3_LFO2] =
		 D_("Filt3 LFO2 level");
		port_range_hints[HERMESFILTER_FILT3_LFO2].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_FILT3_LFO2].LowerBound = -500;
		port_range_hints[HERMESFILTER_FILT3_LFO2].UpperBound = 500;

		/* Parameters for Delay1 length (s) */
		port_descriptors[HERMESFILTER_DELA1_LENGTH] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_DELA1_LENGTH] =
		 D_("Delay1 length (s)");
		port_range_hints[HERMESFILTER_DELA1_LENGTH].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_DELA1_LENGTH].LowerBound = 0;
		port_range_hints[HERMESFILTER_DELA1_LENGTH].UpperBound = 2;

		/* Parameters for Delay1 feedback */
		port_descriptors[HERMESFILTER_DELA1_FB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_DELA1_FB] =
		 D_("Delay1 feedback");
		port_range_hints[HERMESFILTER_DELA1_FB].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_DELA1_FB].LowerBound = 0;
		port_range_hints[HERMESFILTER_DELA1_FB].UpperBound = 1;

		/* Parameters for Delay1 wetness */
		port_descriptors[HERMESFILTER_DELA1_WET] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_DELA1_WET] =
		 D_("Delay1 wetness");
		port_range_hints[HERMESFILTER_DELA1_WET].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_DELA1_WET].LowerBound = 0;
		port_range_hints[HERMESFILTER_DELA1_WET].UpperBound = 1;

		/* Parameters for Delay2 length (s) */
		port_descriptors[HERMESFILTER_DELA2_LENGTH] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_DELA2_LENGTH] =
		 D_("Delay2 length (s)");
		port_range_hints[HERMESFILTER_DELA2_LENGTH].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_DELA2_LENGTH].LowerBound = 0;
		port_range_hints[HERMESFILTER_DELA2_LENGTH].UpperBound = 2;

		/* Parameters for Delay2 feedback */
		port_descriptors[HERMESFILTER_DELA2_FB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_DELA2_FB] =
		 D_("Delay2 feedback");
		port_range_hints[HERMESFILTER_DELA2_FB].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_DELA2_FB].LowerBound = 0;
		port_range_hints[HERMESFILTER_DELA2_FB].UpperBound = 1;

		/* Parameters for Delay2 wetness */
		port_descriptors[HERMESFILTER_DELA2_WET] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_DELA2_WET] =
		 D_("Delay2 wetness");
		port_range_hints[HERMESFILTER_DELA2_WET].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_DELA2_WET].LowerBound = 0;
		port_range_hints[HERMESFILTER_DELA2_WET].UpperBound = 1;

		/* Parameters for Delay3 length (s) */
		port_descriptors[HERMESFILTER_DELA3_LENGTH] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_DELA3_LENGTH] =
		 D_("Delay3 length (s)");
		port_range_hints[HERMESFILTER_DELA3_LENGTH].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_DELA3_LENGTH].LowerBound = 0;
		port_range_hints[HERMESFILTER_DELA3_LENGTH].UpperBound = 2;

		/* Parameters for Delay3 feedback */
		port_descriptors[HERMESFILTER_DELA3_FB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_DELA3_FB] =
		 D_("Delay3 feedback");
		port_range_hints[HERMESFILTER_DELA3_FB].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_DELA3_FB].LowerBound = 0;
		port_range_hints[HERMESFILTER_DELA3_FB].UpperBound = 1;

		/* Parameters for Delay3 wetness */
		port_descriptors[HERMESFILTER_DELA3_WET] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_DELA3_WET] =
		 D_("Delay3 wetness");
		port_range_hints[HERMESFILTER_DELA3_WET].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_DELA3_WET].LowerBound = 0;
		port_range_hints[HERMESFILTER_DELA3_WET].UpperBound = 1;

		/* Parameters for Band 1 gain (dB) */
		port_descriptors[HERMESFILTER_BAND1_GAIN_DB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_BAND1_GAIN_DB] =
		 D_("Band 1 gain (dB)");
		port_range_hints[HERMESFILTER_BAND1_GAIN_DB].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_BAND1_GAIN_DB].LowerBound = -70;
		port_range_hints[HERMESFILTER_BAND1_GAIN_DB].UpperBound = +20;

		/* Parameters for Band 2 gain (dB) */
		port_descriptors[HERMESFILTER_BAND2_GAIN_DB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_BAND2_GAIN_DB] =
		 D_("Band 2 gain (dB)");
		port_range_hints[HERMESFILTER_BAND2_GAIN_DB].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_BAND2_GAIN_DB].LowerBound = -70;
		port_range_hints[HERMESFILTER_BAND2_GAIN_DB].UpperBound = +20;

		/* Parameters for Band 3 gain (dB) */
		port_descriptors[HERMESFILTER_BAND3_GAIN_DB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[HERMESFILTER_BAND3_GAIN_DB] =
		 D_("Band 3 gain (dB)");
		port_range_hints[HERMESFILTER_BAND3_GAIN_DB].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[HERMESFILTER_BAND3_GAIN_DB].LowerBound = -70;
		port_range_hints[HERMESFILTER_BAND3_GAIN_DB].UpperBound = +20;

		/* Parameters for Input */
		port_descriptors[HERMESFILTER_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[HERMESFILTER_INPUT] =
		 D_("Input");
		port_range_hints[HERMESFILTER_INPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[HERMESFILTER_INPUT].LowerBound = -1;
		port_range_hints[HERMESFILTER_INPUT].UpperBound = +1;

		/* Parameters for Output */
		port_descriptors[HERMESFILTER_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[HERMESFILTER_OUTPUT] =
		 D_("Output");
		port_range_hints[HERMESFILTER_OUTPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[HERMESFILTER_OUTPUT].LowerBound = -1;
		port_range_hints[HERMESFILTER_OUTPUT].UpperBound = +1;

		hermesFilterDescriptor->activate = activateHermesFilter;
		hermesFilterDescriptor->cleanup = cleanupHermesFilter;
		hermesFilterDescriptor->connect_port = connectPortHermesFilter;
		hermesFilterDescriptor->deactivate = NULL;
		hermesFilterDescriptor->instantiate = instantiateHermesFilter;
		hermesFilterDescriptor->run = runHermesFilter;
		hermesFilterDescriptor->run_adding = runAddingHermesFilter;
		hermesFilterDescriptor->set_run_adding_gain = setRunAddingGainHermesFilter;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (hermesFilterDescriptor) {
		free((LADSPA_PortDescriptor *)hermesFilterDescriptor->PortDescriptors);
		free((char **)hermesFilterDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)hermesFilterDescriptor->PortRangeHints);
		free(hermesFilterDescriptor);
	}

}
