/*
 * Lv2Manager.cpp - a class to manage loading and instantiation
 *						of LV2 plugins
 *
 * Copyright (c) Alexandros Theodotou @faiyadesu
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include <QtCore>

#include "Lv2Manager.h"

#define _POSIX_C_SOURCE 200809L /* for mkdtemp */
#define _DARWIN_C_SOURCE        /* for mkdtemp on OSX */

extern "C" {
#include <cassert>
#include <malloc.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef __cplusplus
#    include <stdbool.h>
#endif

#ifdef _WIN32
#    include <io.h>  /* for _mktemp */
#    define snprintf _snprintf
#else
#    include <unistd.h>
#endif



#include "lilv/lilv.h"

#include "suil/suil.h"

#include "lv2_evbuf.h"
#include "worker.h"
}

#include "Engine.h"
#include "ConfigManager.h"
#include "Song.h"

#define NS_RDF "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_XSD "http://www.w3.org/2001/XMLSchema#"




LV2_Extension_Data_Feature Lv2Manager::ext_data = { NULL };


/** These features have no data */
static LV2_Feature buf_size_features[3] = {
	{ LV2_BUF_SIZE__powerOf2BlockLength, NULL },
	{ LV2_BUF_SIZE__fixedBlockLength, NULL },
	{ LV2_BUF_SIZE__boundedBlockLength, NULL } };


extern LV2_Feature uri_map_feature;
extern LV2_Feature map_feature;
extern LV2_Feature unmap_feature;
extern LV2_Feature make_path_feature;
extern LV2_Feature sched_feature;
extern LV2_Feature state_sched_feature;
extern LV2_Feature safe_restore_feature;
extern LV2_Feature log_feature;
extern LV2_Feature options_feature;
extern LV2_Feature def_state_feature;

const LV2_Feature* Lv2Manager::features[12] = {
	&uri_map_feature,
	&map_feature,
	&unmap_feature,
	&sched_feature,
	&log_feature,
	&options_feature,
	&def_state_feature,
	&safe_restore_feature,
	&buf_size_features[0],
	&buf_size_features[1],
	&buf_size_features[2],
	NULL
};


/** Return true iff Jalv supports the given feature. */
bool
Lv2Manager::feature_is_supported(const char* uri)
{
	if (!strcmp(uri, "http://lv2plug.in/ns/lv2core#isLive")) {
		return true;
	}
	for (const LV2_Feature*const* f = features; *f; ++f) {
		if (!strcmp(uri, (*f)->URI)) {
			return true;
		}
	}
	return false;
}
//static bool
//jalv_apply_control_arg(Jalv* jalv, const char* s)
//{
	//char  sym[256];
	//float val = 0.0f;
	//if (sscanf(s, "%[^=]=%f", sym, &val) != 2) {
		//fprintf(stderr, "warning: Ignoring invalid value `%s'\n", s);
		//return false;
	//}

	//ControlID* control = jalv_control_by_symbol(jalv, sym);
	//if (!control) {
		//fprintf(stderr, "warning: Ignoring value for unknown control `%s'\n", sym);
		//return false;
	//}

	//jalv_set_control(control, sizeof(float), jalv->urids.atom_Float, &val);
	//printf("%-*s = %f\n", jalv->longest_sym, sym, val);

	//return true;
//}


// TODO do this when plugin is deleted
//static void
//signal_handler(int ignored)
//{
	//zix_sem_post(&exit_sem);
//}

const LilvPlugin* Lv2Manager::find_by_uri(const char* uri)
{
	const LilvNode* node = lilv_new_uri(
			world, uri);
	const LilvPlugins* plugins = lilv_world_get_all_plugins(world);
	return lilv_plugins_get_by_uri(plugins, node);
}

Lv2Manager::Lv2Manager()
{
	suil_init(0, NULL, SUIL_ARG_NONE);


	/* Find all installed plugins */
	world = lilv_world_new();
	lilv_world_load_all(world);
	const LilvPlugins* plugins = lilv_world_get_all_plugins(world);

}

