/* A minimal LV2 Shim, adopted from Version 2, Revison 1
 * Copyright (C) 2000-2008 Richard W.E. Furse, Paul Barton-Davis, Stefan Westerfeld, Steve Harris, Dave Robillard
 * License: LGPL 2.1+ (GPL2+ compatible)
 */

#ifndef LV2_H_INCLUDED
#define LV2_H_INCLUDED
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef void * LV2_Handle;
typedef struct _LV2_Feature { const char * URI;	void * data; } LV2_Feature;
typedef struct _LV2_Descriptor { 
	const char * URI;
	LV2_Handle (*instantiate)(const struct _LV2_Descriptor * descriptor, double sample_rate, const char * bundle_path, const LV2_Feature *const * features);
	void (*connect_port)(LV2_Handle instance, uint32_t port, void * data_location);
	void (*activate)(LV2_Handle instance);
	void (*run)(LV2_Handle instance, uint32_t   sample_count);
	void (*deactivate)(LV2_Handle instance);
	void (*cleanup)(LV2_Handle instance);
	const void* (*extension_data)(const char * uri); 

} LV2_Descriptor;
const LV2_Descriptor * lv2_descriptor(uint32_t index);
typedef const LV2_Descriptor * (*LV2_Descriptor_Function)(uint32_t index);
#ifdef WIN32
#define LV2_SYMBOL_EXPORT __declspec(dllexport)
#else
#define LV2_SYMBOL_EXPORT
#endif
#ifdef __cplusplus
}
#endif
#endif
