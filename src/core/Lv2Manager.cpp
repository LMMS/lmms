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

#include "Engine.h"
#include "ConfigManager.h"
#include "Song.h"

#define NS_RDF "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_XSD "http://www.w3.org/2001/XMLSchema#"


//const LilvPlugin* Lv2Manager::find_by_uri(const char* uri)
//{
	//const LilvNode* node = lilv_new_uri(
			//world, uri);
	//const LilvPlugins* plugins = lilv_world_get_all_plugins(world);
	//return lilv_plugins_get_by_uri(plugins, node);
//}

Lv2Manager::Lv2Manager()
{
	suil_init(0, NULL, SUIL_ARG_NONE);


	/* Find all installed plugins */
	world = lilv_world_new();
	lilv_world_load_all(world);
	plugins = lilv_world_get_all_plugins(world);

}

