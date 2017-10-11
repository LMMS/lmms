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
static void __attribute__((constructor)) swh_init(); // forward declaration
#else
#define _WINDOWS_DLL_EXPORT_ 
#endif

#line 10 "vocoder_1337.xml"

#include "util/iir.h"

#define MAX_BANDS                16
#define AMPLIFIER                16.0

// 0 = formant, 1 = carrier, 2 = output, 3 = output2, 4 = bandcount, 5 = pan
#define START_BANDS        6                                /* start of bands level */
#define PORT_COUNT        START_BANDS + MAX_BANDS                /* bands level */

struct bandpasses
{
  LADSPA_Data c[MAX_BANDS], f[MAX_BANDS], att[MAX_BANDS];

  LADSPA_Data freq[MAX_BANDS];
  LADSPA_Data low1[MAX_BANDS], low2[MAX_BANDS];
  LADSPA_Data mid1[MAX_BANDS], mid2[MAX_BANDS];
  LADSPA_Data high1[MAX_BANDS], high2[MAX_BANDS];
  LADSPA_Data y[MAX_BANDS];
};

void doBandpasses(struct bandpasses *bands, LADSPA_Data sample, int num_bands);

struct bands_out{
  LADSPA_Data decay[MAX_BANDS];
  LADSPA_Data oldval[MAX_BANDS];
  LADSPA_Data level[MAX_BANDS];     /* 0.0 - 1.0 level of this output band */
};

const LADSPA_Data decay_table[] =
{
  1/100.0,
  1/100.0, 1/100.0, 1/100.0,
  1/125.0, 1/125.0, 1/125.0,
  1/166.0, 1/166.0, 1/166.0,
  1/200.0, 1/200.0, 1/200.0,
  1/250.0, 1/250.0, 1/250.0
};

void doBandpasses(struct bandpasses *bands, LADSPA_Data sample, int num_bands)
{
  int i;
  for (i=0; i < num_bands; i++)
  {
    bands->high1[i] = sample - bands->f[i] * bands->mid1[i] - bands->low1[i];
    bands->mid1[i] += bands->high1[i] * bands->c[i];
    bands->low1[i] += bands->mid1[i];

    bands->high2[i] = bands->low1[i] - bands->f[i] * bands->mid2[i] - bands->low2[i];
    bands->mid2[i] += bands->high2[i] * bands->c[i];
    bands->low2[i] += bands->mid2[i];
    bands->y[i]     = bands->high2[i] * bands->att[i];
  }
}

#define VOCODER_PORT_FORMANT           0
#define VOCODER_PORT_CARRIER           1
#define VOCODER_PORT_OUTPUT            2
#define VOCODER_PORT_OUTPUT2           3
#define VOCODER_CTRL_BAND_COUNT        4
#define VOCODER_CTRL_PAN               5
#define VOCODER_BAND1                  6
#define VOCODER_BAND2                  7
#define VOCODER_BAND3                  8
#define VOCODER_BAND4                  9
#define VOCODER_BAND5                  10
#define VOCODER_BAND6                  11
#define VOCODER_BAND7                  12
#define VOCODER_BAND8                  13
#define VOCODER_BAND9                  14
#define VOCODER_BAND10                 15
#define VOCODER_BAND11                 16
#define VOCODER_BAND12                 17
#define VOCODER_BAND13                 18
#define VOCODER_BAND14                 19
#define VOCODER_BAND15                 20
#define VOCODER_BAND16                 21

static LADSPA_Descriptor *vocoderDescriptor = NULL;

typedef struct {
	LADSPA_Data *port_formant;
	LADSPA_Data *port_carrier;
	LADSPA_Data *port_output;
	LADSPA_Data *port_output2;
	LADSPA_Data *ctrl_band_count;
	LADSPA_Data *ctrl_pan;
	LADSPA_Data *band1;
	LADSPA_Data *band2;
	LADSPA_Data *band3;
	LADSPA_Data *band4;
	LADSPA_Data *band5;
	LADSPA_Data *band6;
	LADSPA_Data *band7;
	LADSPA_Data *band8;
	LADSPA_Data *band9;
	LADSPA_Data *band10;
	LADSPA_Data *band11;
	LADSPA_Data *band12;
	LADSPA_Data *band13;
	LADSPA_Data *band14;
	LADSPA_Data *band15;
	LADSPA_Data *band16;
	struct bandpasses bands_carrier;
	struct bandpasses bands_formant;
	struct bands_out bands_out;
	LADSPA_Data *ctrl_band_levels;
	float        main_vol;
	int          num_bands;
	LADSPA_Data  sample_rate;
	LADSPA_Data run_adding_gain;
} Vocoder;

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
		return vocoderDescriptor;
	default:
		return NULL;
	}
}

static void activateVocoder(LADSPA_Handle instance) {
	Vocoder *plugin_data = (Vocoder *)instance;
	struct bandpasses bands_carrier = plugin_data->bands_carrier;
	struct bandpasses bands_formant = plugin_data->bands_formant;
	struct bands_out bands_out = plugin_data->bands_out;
	LADSPA_Data *ctrl_band_levels = plugin_data->ctrl_band_levels;
	float main_vol = plugin_data->main_vol;
	int num_bands = plugin_data->num_bands;
	LADSPA_Data sample_rate = plugin_data->sample_rate;
#line 83 "vocoder_1337.xml"
	int i;
	for (i = 0; i < MAX_BANDS; i++)
	{
	  bands_out.oldval[i] = 0.0f;
	}
	plugin_data->bands_carrier = bands_carrier;
	plugin_data->bands_formant = bands_formant;
	plugin_data->bands_out = bands_out;
	plugin_data->ctrl_band_levels = ctrl_band_levels;
	plugin_data->main_vol = main_vol;
	plugin_data->num_bands = num_bands;
	plugin_data->sample_rate = sample_rate;

}

static void cleanupVocoder(LADSPA_Handle instance) {
#line 92 "vocoder_1337.xml"
	Vocoder *plugin_data = (Vocoder *)instance;
	free(plugin_data->ctrl_band_levels);
	free(instance);
}

static void connectPortVocoder(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Vocoder *plugin;

	plugin = (Vocoder *)instance;
	switch (port) {
	case VOCODER_PORT_FORMANT:
		plugin->port_formant = data;
		break;
	case VOCODER_PORT_CARRIER:
		plugin->port_carrier = data;
		break;
	case VOCODER_PORT_OUTPUT:
		plugin->port_output = data;
		break;
	case VOCODER_PORT_OUTPUT2:
		plugin->port_output2 = data;
		break;
	case VOCODER_CTRL_BAND_COUNT:
		plugin->ctrl_band_count = data;
		break;
	case VOCODER_CTRL_PAN:
		plugin->ctrl_pan = data;
		break;
	case VOCODER_BAND1:
		plugin->band1 = data;
		break;
	case VOCODER_BAND2:
		plugin->band2 = data;
		break;
	case VOCODER_BAND3:
		plugin->band3 = data;
		break;
	case VOCODER_BAND4:
		plugin->band4 = data;
		break;
	case VOCODER_BAND5:
		plugin->band5 = data;
		break;
	case VOCODER_BAND6:
		plugin->band6 = data;
		break;
	case VOCODER_BAND7:
		plugin->band7 = data;
		break;
	case VOCODER_BAND8:
		plugin->band8 = data;
		break;
	case VOCODER_BAND9:
		plugin->band9 = data;
		break;
	case VOCODER_BAND10:
		plugin->band10 = data;
		break;
	case VOCODER_BAND11:
		plugin->band11 = data;
		break;
	case VOCODER_BAND12:
		plugin->band12 = data;
		break;
	case VOCODER_BAND13:
		plugin->band13 = data;
		break;
	case VOCODER_BAND14:
		plugin->band14 = data;
		break;
	case VOCODER_BAND15:
		plugin->band15 = data;
		break;
	case VOCODER_BAND16:
		plugin->band16 = data;
		break;
	}
}

static LADSPA_Handle instantiateVocoder(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Vocoder *plugin_data = (Vocoder *)calloc(1, sizeof(Vocoder));
	struct bandpasses bands_carrier;
	struct bandpasses bands_formant;
	struct bands_out bands_out;
	LADSPA_Data *ctrl_band_levels = NULL;
	float main_vol;
	int num_bands;
	LADSPA_Data sample_rate;

#line 75 "vocoder_1337.xml"
	sample_rate = s_rate;
	main_vol = 1.0 * AMPLIFIER;

	ctrl_band_levels = malloc(MAX_BANDS * sizeof(LADSPA_Data));
	num_bands = -1;

	plugin_data->bands_carrier = bands_carrier;
	plugin_data->bands_formant = bands_formant;
	plugin_data->bands_out = bands_out;
	plugin_data->ctrl_band_levels = ctrl_band_levels;
	plugin_data->main_vol = main_vol;
	plugin_data->num_bands = num_bands;
	plugin_data->sample_rate = sample_rate;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runVocoder(LADSPA_Handle instance, unsigned long sample_count) {
	Vocoder *plugin_data = (Vocoder *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Formant-in (array of floats of length sample_count) */
	const LADSPA_Data * const port_formant = plugin_data->port_formant;

	/* Carrier-in (array of floats of length sample_count) */
	const LADSPA_Data * const port_carrier = plugin_data->port_carrier;

	/* Output-out (array of floats of length sample_count) */
	LADSPA_Data * const port_output = plugin_data->port_output;

	/* Output2-out (array of floats of length sample_count) */
	LADSPA_Data * const port_output2 = plugin_data->port_output2;

	/* Number of bands (float value) */
	const LADSPA_Data ctrl_band_count = *(plugin_data->ctrl_band_count);

	/* Left/Right (float value) */
	const LADSPA_Data ctrl_pan = *(plugin_data->ctrl_pan);

	/* Band 1 Level (float value) */
	const LADSPA_Data band1 = *(plugin_data->band1);

	/* Band 2 Level (float value) */
	const LADSPA_Data band2 = *(plugin_data->band2);

	/* Band 3 Level (float value) */
	const LADSPA_Data band3 = *(plugin_data->band3);

	/* Band 4 Level (float value) */
	const LADSPA_Data band4 = *(plugin_data->band4);

	/* Band 5 Level (float value) */
	const LADSPA_Data band5 = *(plugin_data->band5);

	/* Band 6 Level (float value) */
	const LADSPA_Data band6 = *(plugin_data->band6);

	/* Band 7 Level (float value) */
	const LADSPA_Data band7 = *(plugin_data->band7);

	/* Band 8 Level (float value) */
	const LADSPA_Data band8 = *(plugin_data->band8);

	/* Band 9 Level (float value) */
	const LADSPA_Data band9 = *(plugin_data->band9);

	/* Band 10 Level (float value) */
	const LADSPA_Data band10 = *(plugin_data->band10);

	/* Band 11 Level (float value) */
	const LADSPA_Data band11 = *(plugin_data->band11);

	/* Band 12 Level (float value) */
	const LADSPA_Data band12 = *(plugin_data->band12);

	/* Band 13 Level (float value) */
	const LADSPA_Data band13 = *(plugin_data->band13);

	/* Band 14 Level (float value) */
	const LADSPA_Data band14 = *(plugin_data->band14);

	/* Band 15 Level (float value) */
	const LADSPA_Data band15 = *(plugin_data->band15);

	/* Band 16 Level (float value) */
	const LADSPA_Data band16 = *(plugin_data->band16);
	struct bandpasses bands_carrier = plugin_data->bands_carrier;
	struct bandpasses bands_formant = plugin_data->bands_formant;
	struct bands_out bands_out = plugin_data->bands_out;
	LADSPA_Data * ctrl_band_levels = plugin_data->ctrl_band_levels;
	float main_vol = plugin_data->main_vol;
	int num_bands = plugin_data->num_bands;
	LADSPA_Data sample_rate = plugin_data->sample_rate;

#line 96 "vocoder_1337.xml"
	int i, j, numbands, pan;
	float a;
	LADSPA_Data x, c;
	float fl, fr;

	// Bind band level controls
	plugin_data->ctrl_band_levels[0] = band1;
	plugin_data->ctrl_band_levels[1] = band2;
	plugin_data->ctrl_band_levels[2] = band3;
	plugin_data->ctrl_band_levels[3] = band4;
	plugin_data->ctrl_band_levels[4] = band5;
	plugin_data->ctrl_band_levels[5] = band6;
	plugin_data->ctrl_band_levels[6] = band7;
	plugin_data->ctrl_band_levels[7] = band8;
	plugin_data->ctrl_band_levels[8] = band9;
	plugin_data->ctrl_band_levels[9] = band10;
	plugin_data->ctrl_band_levels[10] = band11;
	plugin_data->ctrl_band_levels[11] = band12;
	plugin_data->ctrl_band_levels[12] = band13;
	plugin_data->ctrl_band_levels[13] = band14;
	plugin_data->ctrl_band_levels[14] = band15;
	plugin_data->ctrl_band_levels[15] = band16;

	numbands = (int)(*plugin_data->ctrl_band_count);
	if (numbands < 1 || numbands > MAX_BANDS) numbands = MAX_BANDS;

	/* initialize bandpass information if num_bands control has changed,
	   or on first run */
	if (plugin_data->num_bands != numbands)
	{
	  plugin_data->num_bands = numbands;

	  memset(&plugin_data->bands_formant, 0, sizeof(struct bandpasses));
	  for(i=0; i < numbands; i++)
	  {
	    a = 16.0 * i/(double)numbands;  // stretch existing bands

	    if (a < 4.0)
	      plugin_data->bands_formant.freq[i] = 150 + 420 * a / 4.0;
	    else
	      plugin_data->bands_formant.freq[i] = 600 * pow (1.23, a - 4.0);

	      c = plugin_data->bands_formant.freq[i] * 2 * M_PI / plugin_data->sample_rate;
	      plugin_data->bands_formant.c[i] = c * c;

	      plugin_data->bands_formant.f[i] = 0.4/c;
	      plugin_data->bands_formant.att[i] =
	        1/(6.0 + ((exp (plugin_data->bands_formant.freq[i]
	                      / plugin_data->sample_rate) - 1) * 10));

	      plugin_data->bands_out.decay[i] = decay_table[(int)a];
	      plugin_data->bands_out.level[i] =
	        CLAMP (plugin_data->ctrl_band_levels[i], 0.0, 1.0);
	  }
	  memcpy(&plugin_data->bands_carrier,
	    &plugin_data->bands_formant, sizeof(struct bandpasses));

	}
	else                       /* get current values of band level controls */
	{
	  for (i = 0; i < numbands; i++)
	    plugin_data->bands_out.level[i] = CLAMP (plugin_data->ctrl_band_levels[i],
	      0.0, 1.0);
	}

	for (i=0; i < sample_count; i++)
	{
	  doBandpasses (&(plugin_data->bands_carrier),
	    plugin_data->port_carrier[i],
	    plugin_data->num_bands);
	  doBandpasses (&(plugin_data->bands_formant),
	    plugin_data->port_formant[i],
	    plugin_data->num_bands);

	  LADSPA_Data sample = 0.0;
	  for (j=0; j < numbands; j++)
	  {
	    plugin_data->bands_out.oldval[j] = plugin_data->bands_out.oldval[j]
	      + (fabs (plugin_data->bands_formant.y[j])
	         - plugin_data->bands_out.oldval[j])
	      * plugin_data->bands_out.decay[j];
	    x = plugin_data->bands_carrier.y[j] * plugin_data->bands_out.oldval[j];

	    sample += x * plugin_data->bands_out.level[j];
	  }
	  /* treat paning + main volume */
	  pan = (int)(*plugin_data->ctrl_pan);
	  fl = fr = 1.0f;
	  if (pan != 0) { /* no paning, don't compute useless values */
	    if (pan > 0) { /* reduce left */
	      fl = (100.-pan)/100.;
	    } else {
	      fr = (100.+pan)/100.;
	    }
	  }
	  /* apply volume and paning */
	  plugin_data->port_output[i] = sample * plugin_data->main_vol * fl;
	  plugin_data->port_output2[i] = sample * plugin_data->main_vol * fr;
	}

	// Suppress unused warnings
	(void)(sample_rate);
	(void)(num_bands);
	(void)(main_vol);
	(void)(bands_formant);
	(void)(bands_carrier);
	(void)(bands_out);
	(void)(ctrl_band_levels);
	(void)(port_formant);
	(void)(port_carrier);
	(void)(port_output);
	(void)(port_output2);
	(void)(ctrl_band_count);
	(void)(ctrl_pan);
	(void)(run_adding_gain);
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainVocoder(LADSPA_Handle instance, LADSPA_Data gain) {
	((Vocoder *)instance)->run_adding_gain = gain;
}

static void runAddingVocoder(LADSPA_Handle instance, unsigned long sample_count) {
	Vocoder *plugin_data = (Vocoder *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Formant-in (array of floats of length sample_count) */
	const LADSPA_Data * const port_formant = plugin_data->port_formant;

	/* Carrier-in (array of floats of length sample_count) */
	const LADSPA_Data * const port_carrier = plugin_data->port_carrier;

	/* Output-out (array of floats of length sample_count) */
	LADSPA_Data * const port_output = plugin_data->port_output;

	/* Output2-out (array of floats of length sample_count) */
	LADSPA_Data * const port_output2 = plugin_data->port_output2;

	/* Number of bands (float value) */
	const LADSPA_Data ctrl_band_count = *(plugin_data->ctrl_band_count);

	/* Left/Right (float value) */
	const LADSPA_Data ctrl_pan = *(plugin_data->ctrl_pan);

	/* Band 1 Level (float value) */
	const LADSPA_Data band1 = *(plugin_data->band1);

	/* Band 2 Level (float value) */
	const LADSPA_Data band2 = *(plugin_data->band2);

	/* Band 3 Level (float value) */
	const LADSPA_Data band3 = *(plugin_data->band3);

	/* Band 4 Level (float value) */
	const LADSPA_Data band4 = *(plugin_data->band4);

	/* Band 5 Level (float value) */
	const LADSPA_Data band5 = *(plugin_data->band5);

	/* Band 6 Level (float value) */
	const LADSPA_Data band6 = *(plugin_data->band6);

	/* Band 7 Level (float value) */
	const LADSPA_Data band7 = *(plugin_data->band7);

	/* Band 8 Level (float value) */
	const LADSPA_Data band8 = *(plugin_data->band8);

	/* Band 9 Level (float value) */
	const LADSPA_Data band9 = *(plugin_data->band9);

	/* Band 10 Level (float value) */
	const LADSPA_Data band10 = *(plugin_data->band10);

	/* Band 11 Level (float value) */
	const LADSPA_Data band11 = *(plugin_data->band11);

	/* Band 12 Level (float value) */
	const LADSPA_Data band12 = *(plugin_data->band12);

	/* Band 13 Level (float value) */
	const LADSPA_Data band13 = *(plugin_data->band13);

	/* Band 14 Level (float value) */
	const LADSPA_Data band14 = *(plugin_data->band14);

	/* Band 15 Level (float value) */
	const LADSPA_Data band15 = *(plugin_data->band15);

	/* Band 16 Level (float value) */
	const LADSPA_Data band16 = *(plugin_data->band16);
	struct bandpasses bands_carrier = plugin_data->bands_carrier;
	struct bandpasses bands_formant = plugin_data->bands_formant;
	struct bands_out bands_out = plugin_data->bands_out;
	LADSPA_Data * ctrl_band_levels = plugin_data->ctrl_band_levels;
	float main_vol = plugin_data->main_vol;
	int num_bands = plugin_data->num_bands;
	LADSPA_Data sample_rate = plugin_data->sample_rate;

#line 96 "vocoder_1337.xml"
	int i, j, numbands, pan;
	float a;
	LADSPA_Data x, c;
	float fl, fr;

	// Bind band level controls
	plugin_data->ctrl_band_levels[0] = band1;
	plugin_data->ctrl_band_levels[1] = band2;
	plugin_data->ctrl_band_levels[2] = band3;
	plugin_data->ctrl_band_levels[3] = band4;
	plugin_data->ctrl_band_levels[4] = band5;
	plugin_data->ctrl_band_levels[5] = band6;
	plugin_data->ctrl_band_levels[6] = band7;
	plugin_data->ctrl_band_levels[7] = band8;
	plugin_data->ctrl_band_levels[8] = band9;
	plugin_data->ctrl_band_levels[9] = band10;
	plugin_data->ctrl_band_levels[10] = band11;
	plugin_data->ctrl_band_levels[11] = band12;
	plugin_data->ctrl_band_levels[12] = band13;
	plugin_data->ctrl_band_levels[13] = band14;
	plugin_data->ctrl_band_levels[14] = band15;
	plugin_data->ctrl_band_levels[15] = band16;

	numbands = (int)(*plugin_data->ctrl_band_count);
	if (numbands < 1 || numbands > MAX_BANDS) numbands = MAX_BANDS;

	/* initialize bandpass information if num_bands control has changed,
	   or on first run */
	if (plugin_data->num_bands != numbands)
	{
	  plugin_data->num_bands = numbands;

	  memset(&plugin_data->bands_formant, 0, sizeof(struct bandpasses));
	  for(i=0; i < numbands; i++)
	  {
	    a = 16.0 * i/(double)numbands;  // stretch existing bands

	    if (a < 4.0)
	      plugin_data->bands_formant.freq[i] = 150 + 420 * a / 4.0;
	    else
	      plugin_data->bands_formant.freq[i] = 600 * pow (1.23, a - 4.0);

	      c = plugin_data->bands_formant.freq[i] * 2 * M_PI / plugin_data->sample_rate;
	      plugin_data->bands_formant.c[i] = c * c;

	      plugin_data->bands_formant.f[i] = 0.4/c;
	      plugin_data->bands_formant.att[i] =
	        1/(6.0 + ((exp (plugin_data->bands_formant.freq[i]
	                      / plugin_data->sample_rate) - 1) * 10));

	      plugin_data->bands_out.decay[i] = decay_table[(int)a];
	      plugin_data->bands_out.level[i] =
	        CLAMP (plugin_data->ctrl_band_levels[i], 0.0, 1.0);
	  }
	  memcpy(&plugin_data->bands_carrier,
	    &plugin_data->bands_formant, sizeof(struct bandpasses));

	}
	else                       /* get current values of band level controls */
	{
	  for (i = 0; i < numbands; i++)
	    plugin_data->bands_out.level[i] = CLAMP (plugin_data->ctrl_band_levels[i],
	      0.0, 1.0);
	}

	for (i=0; i < sample_count; i++)
	{
	  doBandpasses (&(plugin_data->bands_carrier),
	    plugin_data->port_carrier[i],
	    plugin_data->num_bands);
	  doBandpasses (&(plugin_data->bands_formant),
	    plugin_data->port_formant[i],
	    plugin_data->num_bands);

	  LADSPA_Data sample = 0.0;
	  for (j=0; j < numbands; j++)
	  {
	    plugin_data->bands_out.oldval[j] = plugin_data->bands_out.oldval[j]
	      + (fabs (plugin_data->bands_formant.y[j])
	         - plugin_data->bands_out.oldval[j])
	      * plugin_data->bands_out.decay[j];
	    x = plugin_data->bands_carrier.y[j] * plugin_data->bands_out.oldval[j];

	    sample += x * plugin_data->bands_out.level[j];
	  }
	  /* treat paning + main volume */
	  pan = (int)(*plugin_data->ctrl_pan);
	  fl = fr = 1.0f;
	  if (pan != 0) { /* no paning, don't compute useless values */
	    if (pan > 0) { /* reduce left */
	      fl = (100.-pan)/100.;
	    } else {
	      fr = (100.+pan)/100.;
	    }
	  }
	  /* apply volume and paning */
	  plugin_data->port_output[i] = sample * plugin_data->main_vol * fl;
	  plugin_data->port_output2[i] = sample * plugin_data->main_vol * fr;
	}

	// Suppress unused warnings
	(void)(sample_rate);
	(void)(num_bands);
	(void)(main_vol);
	(void)(bands_formant);
	(void)(bands_carrier);
	(void)(bands_out);
	(void)(ctrl_band_levels);
	(void)(port_formant);
	(void)(port_carrier);
	(void)(port_output);
	(void)(port_output2);
	(void)(ctrl_band_count);
	(void)(ctrl_pan);
	(void)(run_adding_gain);
}

static void __attribute__((constructor)) swh_init() {
	char **port_names;
	LADSPA_PortDescriptor *port_descriptors;
	LADSPA_PortRangeHint *port_range_hints;

#ifdef ENABLE_NLS
#define D_(s) dgettext(PACKAGE, s)
	bindtextdomain(PACKAGE, PACKAGE_LOCALE_DIR);
#else
#define D_(s) (s)
#endif


	vocoderDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (vocoderDescriptor) {
		vocoderDescriptor->UniqueID = 1337;
		vocoderDescriptor->Label = "vocoder";
		vocoderDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		vocoderDescriptor->Name =
		 D_("Vocoder");
		vocoderDescriptor->Maker =
		 "Achim Settelmeier <settel-linux@sirlab.de> (adapted by Josh Green and Hexasoft)";
		vocoderDescriptor->Copyright =
		 "GPL";
		vocoderDescriptor->PortCount = 22;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(22,
		 sizeof(LADSPA_PortDescriptor));
		vocoderDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(22,
		 sizeof(LADSPA_PortRangeHint));
		vocoderDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(22, sizeof(char*));
		vocoderDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Formant-in */
		port_descriptors[VOCODER_PORT_FORMANT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[VOCODER_PORT_FORMANT] =
		 D_("Formant-in");
		port_range_hints[VOCODER_PORT_FORMANT].HintDescriptor = 0;

		/* Parameters for Carrier-in */
		port_descriptors[VOCODER_PORT_CARRIER] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[VOCODER_PORT_CARRIER] =
		 D_("Carrier-in");
		port_range_hints[VOCODER_PORT_CARRIER].HintDescriptor = 0;

		/* Parameters for Output-out */
		port_descriptors[VOCODER_PORT_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[VOCODER_PORT_OUTPUT] =
		 D_("Output-out");
		port_range_hints[VOCODER_PORT_OUTPUT].HintDescriptor = 0;

		/* Parameters for Output2-out */
		port_descriptors[VOCODER_PORT_OUTPUT2] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[VOCODER_PORT_OUTPUT2] =
		 D_("Output2-out");
		port_range_hints[VOCODER_PORT_OUTPUT2].HintDescriptor = 0;

		/* Parameters for Number of bands */
		port_descriptors[VOCODER_CTRL_BAND_COUNT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VOCODER_CTRL_BAND_COUNT] =
		 D_("Number of bands");
		port_range_hints[VOCODER_CTRL_BAND_COUNT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER;
		port_range_hints[VOCODER_CTRL_BAND_COUNT].LowerBound = 1;
		port_range_hints[VOCODER_CTRL_BAND_COUNT].UpperBound = MAX_BANDS;

		/* Parameters for Left/Right */
		port_descriptors[VOCODER_CTRL_PAN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VOCODER_CTRL_PAN] =
		 D_("Left/Right");
		port_range_hints[VOCODER_CTRL_PAN].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER;
		port_range_hints[VOCODER_CTRL_PAN].LowerBound = -100;
		port_range_hints[VOCODER_CTRL_PAN].UpperBound = +100;

		/* Parameters for Band 1 Level */
		port_descriptors[VOCODER_BAND1] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VOCODER_BAND1] =
		 D_("Band 1 Level");
		port_range_hints[VOCODER_BAND1].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[VOCODER_BAND1].LowerBound = 0;
		port_range_hints[VOCODER_BAND1].UpperBound = 1;

		/* Parameters for Band 2 Level */
		port_descriptors[VOCODER_BAND2] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VOCODER_BAND2] =
		 D_("Band 2 Level");
		port_range_hints[VOCODER_BAND2].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[VOCODER_BAND2].LowerBound = 0;
		port_range_hints[VOCODER_BAND2].UpperBound = 1;

		/* Parameters for Band 3 Level */
		port_descriptors[VOCODER_BAND3] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VOCODER_BAND3] =
		 D_("Band 3 Level");
		port_range_hints[VOCODER_BAND3].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[VOCODER_BAND3].LowerBound = 0;
		port_range_hints[VOCODER_BAND3].UpperBound = 1;

		/* Parameters for Band 4 Level */
		port_descriptors[VOCODER_BAND4] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VOCODER_BAND4] =
		 D_("Band 4 Level");
		port_range_hints[VOCODER_BAND4].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[VOCODER_BAND4].LowerBound = 0;
		port_range_hints[VOCODER_BAND4].UpperBound = 1;

		/* Parameters for Band 5 Level */
		port_descriptors[VOCODER_BAND5] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VOCODER_BAND5] =
		 D_("Band 5 Level");
		port_range_hints[VOCODER_BAND5].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[VOCODER_BAND5].LowerBound = 0;
		port_range_hints[VOCODER_BAND5].UpperBound = 1;

		/* Parameters for Band 6 Level */
		port_descriptors[VOCODER_BAND6] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VOCODER_BAND6] =
		 D_("Band 6 Level");
		port_range_hints[VOCODER_BAND6].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[VOCODER_BAND6].LowerBound = 0;
		port_range_hints[VOCODER_BAND6].UpperBound = 1;

		/* Parameters for Band 7 Level */
		port_descriptors[VOCODER_BAND7] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VOCODER_BAND7] =
		 D_("Band 7 Level");
		port_range_hints[VOCODER_BAND7].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[VOCODER_BAND7].LowerBound = 0;
		port_range_hints[VOCODER_BAND7].UpperBound = 1;

		/* Parameters for Band 8 Level */
		port_descriptors[VOCODER_BAND8] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VOCODER_BAND8] =
		 D_("Band 8 Level");
		port_range_hints[VOCODER_BAND8].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[VOCODER_BAND8].LowerBound = 0;
		port_range_hints[VOCODER_BAND8].UpperBound = 1;

		/* Parameters for Band 9 Level */
		port_descriptors[VOCODER_BAND9] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VOCODER_BAND9] =
		 D_("Band 9 Level");
		port_range_hints[VOCODER_BAND9].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[VOCODER_BAND9].LowerBound = 0;
		port_range_hints[VOCODER_BAND9].UpperBound = 1;

		/* Parameters for Band 10 Level */
		port_descriptors[VOCODER_BAND10] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VOCODER_BAND10] =
		 D_("Band 10 Level");
		port_range_hints[VOCODER_BAND10].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[VOCODER_BAND10].LowerBound = 0;
		port_range_hints[VOCODER_BAND10].UpperBound = 1;

		/* Parameters for Band 11 Level */
		port_descriptors[VOCODER_BAND11] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VOCODER_BAND11] =
		 D_("Band 11 Level");
		port_range_hints[VOCODER_BAND11].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[VOCODER_BAND11].LowerBound = 0;
		port_range_hints[VOCODER_BAND11].UpperBound = 1;

		/* Parameters for Band 12 Level */
		port_descriptors[VOCODER_BAND12] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VOCODER_BAND12] =
		 D_("Band 12 Level");
		port_range_hints[VOCODER_BAND12].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[VOCODER_BAND12].LowerBound = 0;
		port_range_hints[VOCODER_BAND12].UpperBound = 1;

		/* Parameters for Band 13 Level */
		port_descriptors[VOCODER_BAND13] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VOCODER_BAND13] =
		 D_("Band 13 Level");
		port_range_hints[VOCODER_BAND13].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[VOCODER_BAND13].LowerBound = 0;
		port_range_hints[VOCODER_BAND13].UpperBound = 1;

		/* Parameters for Band 14 Level */
		port_descriptors[VOCODER_BAND14] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VOCODER_BAND14] =
		 D_("Band 14 Level");
		port_range_hints[VOCODER_BAND14].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[VOCODER_BAND14].LowerBound = 0;
		port_range_hints[VOCODER_BAND14].UpperBound = 1;

		/* Parameters for Band 15 Level */
		port_descriptors[VOCODER_BAND15] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VOCODER_BAND15] =
		 D_("Band 15 Level");
		port_range_hints[VOCODER_BAND15].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[VOCODER_BAND15].LowerBound = 0;
		port_range_hints[VOCODER_BAND15].UpperBound = 1;

		/* Parameters for Band 16 Level */
		port_descriptors[VOCODER_BAND16] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VOCODER_BAND16] =
		 D_("Band 16 Level");
		port_range_hints[VOCODER_BAND16].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[VOCODER_BAND16].LowerBound = 0;
		port_range_hints[VOCODER_BAND16].UpperBound = 1;

		vocoderDescriptor->activate = activateVocoder;
		vocoderDescriptor->cleanup = cleanupVocoder;
		vocoderDescriptor->connect_port = connectPortVocoder;
		vocoderDescriptor->deactivate = NULL;
		vocoderDescriptor->instantiate = instantiateVocoder;
		vocoderDescriptor->run = runVocoder;
		vocoderDescriptor->run_adding = runAddingVocoder;
		vocoderDescriptor->set_run_adding_gain = setRunAddingGainVocoder;
	}
}

static void __attribute__((destructor)) swh_fini() {
	if (vocoderDescriptor) {
		free((LADSPA_PortDescriptor *)vocoderDescriptor->PortDescriptors);
		free((char **)vocoderDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)vocoderDescriptor->PortRangeHints);
		free(vocoderDescriptor);
	}
	vocoderDescriptor = NULL;

}
