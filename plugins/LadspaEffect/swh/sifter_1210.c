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

#line 9 "sifter_1210.xml"

#include "ladspa-util.h"

#define MAX_BSIZE 1000

inline int partition(LADSPA_Data array[], int left, int right);

#ifdef __clang__
void q_sort(LADSPA_Data array[], int left, int right) {
#else
inline void q_sort(LADSPA_Data array[], int left, int right) {
#endif
        float pivot = partition(array, left, right);

        if (left < pivot) {
                q_sort(array, left, pivot-1);
        }
        if (right > pivot) {
                q_sort(array, pivot+1, right);
        }
}

inline int partition(LADSPA_Data array[], int left, int right) {
        float pivot = array[left];

        while (left < right) {
                while (array[right] >= pivot && left < right) {
                        right--;
                }
                if (left != right) {
                        array[left] = array[right];
                        left++;
                }
                while (array[left] <= pivot && left < right) {
                        left++;
                }
                if (left != right) {
                        array[right] = array[left];
                        right--;
                }
        }
        array[left] = pivot;

        return left;
}

#define SIFTER_SIZE                    0
#define SIFTER_INPUT                   1
#define SIFTER_OUTPUT                  2

static LADSPA_Descriptor *sifterDescriptor = NULL;

typedef struct {
	LADSPA_Data *size;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data *b1;
	long         b1ptr;
	LADSPA_Data *b2;
	long         b2ptr;
	LADSPA_Data *ob;
	LADSPA_Data *rc;
	LADSPA_Data run_adding_gain;
} Sifter;

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
		return sifterDescriptor;
	default:
		return NULL;
	}
}

static void activateSifter(LADSPA_Handle instance) {
	Sifter *plugin_data = (Sifter *)instance;
	LADSPA_Data *b1 = plugin_data->b1;
	long b1ptr = plugin_data->b1ptr;
	LADSPA_Data *b2 = plugin_data->b2;
	long b2ptr = plugin_data->b2ptr;
	LADSPA_Data *ob = plugin_data->ob;
	LADSPA_Data *rc = plugin_data->rc;
#line 84 "sifter_1210.xml"
	b1ptr = 0;
	b2ptr = 0;
	memset(b1, 0, MAX_BSIZE * sizeof(LADSPA_Data));
	memset(b2, 0, MAX_BSIZE * sizeof(LADSPA_Data));
	memset(ob, 0, MAX_BSIZE * sizeof(LADSPA_Data));
	plugin_data->b1 = b1;
	plugin_data->b1ptr = b1ptr;
	plugin_data->b2 = b2;
	plugin_data->b2ptr = b2ptr;
	plugin_data->ob = ob;
	plugin_data->rc = rc;

}

static void cleanupSifter(LADSPA_Handle instance) {
#line 92 "sifter_1210.xml"
	Sifter *plugin_data = (Sifter *)instance;
	free(plugin_data->b1);
	free(plugin_data->b2);
	free(plugin_data->ob);
	free(plugin_data->rc);
	free(instance);
}

static void connectPortSifter(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Sifter *plugin;

	plugin = (Sifter *)instance;
	switch (port) {
	case SIFTER_SIZE:
		plugin->size = data;
		break;
	case SIFTER_INPUT:
		plugin->input = data;
		break;
	case SIFTER_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateSifter(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Sifter *plugin_data = (Sifter *)malloc(sizeof(Sifter));
	LADSPA_Data *b1 = NULL;
	long b1ptr;
	LADSPA_Data *b2 = NULL;
	long b2ptr;
	LADSPA_Data *ob = NULL;
	LADSPA_Data *rc = NULL;

#line 60 "sifter_1210.xml"
	long i;
	float scla = (float)MAX_BSIZE * 0.5f;
	float sclb = (float)MAX_BSIZE;

	b1 = (LADSPA_Data *)calloc(MAX_BSIZE, sizeof(LADSPA_Data));
	b2 = (LADSPA_Data *)calloc(MAX_BSIZE, sizeof(LADSPA_Data));
	ob = (LADSPA_Data *)calloc(MAX_BSIZE, sizeof(LADSPA_Data));
	rc = (LADSPA_Data *)calloc(MAX_BSIZE, sizeof(LADSPA_Data));

	// Calculate raised cosine table, to build windowing function from
	rc[0] = cos(((0.0f - scla) / sclb) * M_PI);
	rc[0] *= rc[0];
	for (i=1; i<MAX_BSIZE / 2; i++) {
	  rc[i] = cos((((float)i - scla) / sclb) * M_PI);
	  rc[i] *= rc[i];
	  rc[MAX_BSIZE - i] = rc[i];
	}
	rc[MAX_BSIZE / 2] = 1.0f;

	b1ptr = 0;
	b2ptr = 0;

	plugin_data->b1 = b1;
	plugin_data->b1ptr = b1ptr;
	plugin_data->b2 = b2;
	plugin_data->b2ptr = b2ptr;
	plugin_data->ob = ob;
	plugin_data->rc = rc;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runSifter(LADSPA_Handle instance, unsigned long sample_count) {
	Sifter *plugin_data = (Sifter *)instance;

	/* Sift size (float value) */
	const LADSPA_Data size = *(plugin_data->size);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data * b1 = plugin_data->b1;
	long b1ptr = plugin_data->b1ptr;
	LADSPA_Data * b2 = plugin_data->b2;
	long b2ptr = plugin_data->b2ptr;
	LADSPA_Data * ob = plugin_data->ob;
	LADSPA_Data * rc = plugin_data->rc;

#line 99 "sifter_1210.xml"
	unsigned long pos, i;
	long bsize = f_round(LIMIT(size, 1, MAX_BSIZE));
	
	for (pos = 0; pos < sample_count; pos++) {
	        if (b1ptr >= bsize) {
	                float wstep = (float)MAX_BSIZE / (float)b1ptr, wpos = 0.0f;
	
	                q_sort(b1, 0, b1ptr);
	                for (i=0; i<b1ptr; i++) {
	                        ob[i] += b1[i] * rc[f_round(wpos)];
	                        wpos += wstep;
	                }
	                b1ptr = 0;
	                b2ptr = (bsize+1) / 2;
	        }
	
	        if (b2ptr >= bsize) {
	                float wstep = (float)MAX_BSIZE / (float)b2ptr, wpos = 0.0f;
	                int offset = (b2ptr+1)/2;
	
	                q_sort(b2, 0, b2ptr);
	                for (i=0; i<offset; i++) {
	                        ob[i + offset] += b2[i] * rc[f_round(wpos)];
	                        wpos += wstep;
	                }
	                for (; i<b2ptr; i++) {
	                        ob[i - offset] += b2[i] * rc[f_round(wpos)];
	                        wpos += wstep;
	                }
	                b2ptr = 0;
	        }
	
	        if (bsize < 2) {
	                ob[b1ptr] = input[pos];
	        }
	
	        b1[b1ptr] = input[pos];
	        b2[b2ptr] = input[pos];
	        buffer_write(output[pos], ob[b1ptr]);
	        ob[b1ptr] = 0.0f;
	        b1ptr++;
	        b2ptr++;
	}
	
	plugin_data->b1ptr = b1ptr;
	plugin_data->b2ptr = b2ptr;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainSifter(LADSPA_Handle instance, LADSPA_Data gain) {
	((Sifter *)instance)->run_adding_gain = gain;
}

static void runAddingSifter(LADSPA_Handle instance, unsigned long sample_count) {
	Sifter *plugin_data = (Sifter *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Sift size (float value) */
	const LADSPA_Data size = *(plugin_data->size);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data * b1 = plugin_data->b1;
	long b1ptr = plugin_data->b1ptr;
	LADSPA_Data * b2 = plugin_data->b2;
	long b2ptr = plugin_data->b2ptr;
	LADSPA_Data * ob = plugin_data->ob;
	LADSPA_Data * rc = plugin_data->rc;

#line 99 "sifter_1210.xml"
	unsigned long pos, i;
	long bsize = f_round(LIMIT(size, 1, MAX_BSIZE));
	
	for (pos = 0; pos < sample_count; pos++) {
	        if (b1ptr >= bsize) {
	                float wstep = (float)MAX_BSIZE / (float)b1ptr, wpos = 0.0f;
	
	                q_sort(b1, 0, b1ptr);
	                for (i=0; i<b1ptr; i++) {
	                        ob[i] += b1[i] * rc[f_round(wpos)];
	                        wpos += wstep;
	                }
	                b1ptr = 0;
	                b2ptr = (bsize+1) / 2;
	        }
	
	        if (b2ptr >= bsize) {
	                float wstep = (float)MAX_BSIZE / (float)b2ptr, wpos = 0.0f;
	                int offset = (b2ptr+1)/2;
	
	                q_sort(b2, 0, b2ptr);
	                for (i=0; i<offset; i++) {
	                        ob[i + offset] += b2[i] * rc[f_round(wpos)];
	                        wpos += wstep;
	                }
	                for (; i<b2ptr; i++) {
	                        ob[i - offset] += b2[i] * rc[f_round(wpos)];
	                        wpos += wstep;
	                }
	                b2ptr = 0;
	        }
	
	        if (bsize < 2) {
	                ob[b1ptr] = input[pos];
	        }
	
	        b1[b1ptr] = input[pos];
	        b2[b2ptr] = input[pos];
	        buffer_write(output[pos], ob[b1ptr]);
	        ob[b1ptr] = 0.0f;
	        b1ptr++;
	        b2ptr++;
	}
	
	plugin_data->b1ptr = b1ptr;
	plugin_data->b2ptr = b2ptr;
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


	sifterDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (sifterDescriptor) {
		sifterDescriptor->UniqueID = 1210;
		sifterDescriptor->Label = "sifter";
		sifterDescriptor->Properties =
		 0;
		sifterDescriptor->Name =
		 D_("Signal sifter");
		sifterDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		sifterDescriptor->Copyright =
		 "GPL";
		sifterDescriptor->PortCount = 3;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(3,
		 sizeof(LADSPA_PortDescriptor));
		sifterDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(3,
		 sizeof(LADSPA_PortRangeHint));
		sifterDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(3, sizeof(char*));
		sifterDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Sift size */
		port_descriptors[SIFTER_SIZE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SIFTER_SIZE] =
		 D_("Sift size");
		port_range_hints[SIFTER_SIZE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[SIFTER_SIZE].LowerBound = 1;
		port_range_hints[SIFTER_SIZE].UpperBound = MAX_BSIZE;

		/* Parameters for Input */
		port_descriptors[SIFTER_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[SIFTER_INPUT] =
		 D_("Input");
		port_range_hints[SIFTER_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[SIFTER_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[SIFTER_OUTPUT] =
		 D_("Output");
		port_range_hints[SIFTER_OUTPUT].HintDescriptor = 0;

		sifterDescriptor->activate = activateSifter;
		sifterDescriptor->cleanup = cleanupSifter;
		sifterDescriptor->connect_port = connectPortSifter;
		sifterDescriptor->deactivate = NULL;
		sifterDescriptor->instantiate = instantiateSifter;
		sifterDescriptor->run = runSifter;
		sifterDescriptor->run_adding = runAddingSifter;
		sifterDescriptor->set_run_adding_gain = setRunAddingGainSifter;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (sifterDescriptor) {
		free((LADSPA_PortDescriptor *)sifterDescriptor->PortDescriptors);
		free((char **)sifterDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)sifterDescriptor->PortRangeHints);
		free(sifterDescriptor);
	}

}
