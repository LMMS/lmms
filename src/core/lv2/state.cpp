/*
  Copyright 2007-2016 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
extern "C" {
#include <sys/stat.h>
#include <sys/types.h>
}

#ifdef HAVE_LV2_STATE
#    include "lv2/lv2plug.in/ns/ext/state/state.h"
#endif
#include "lv2/lv2plug.in/ns/ext/presets/presets.h"

#include <lilv/lilv.h>

#include "Lv2Plugin.h"
#include "Lv2Manager.h"
#include "log.h"



static const void*
get_port_value(const char* port_symbol,
               void*       user_data,
               uint32_t*   size,
               uint32_t*   type)
{
	Lv2Plugin*        jalv = (Lv2Plugin*)user_data;
	Port* port = jalv->port_by_symbol(port_symbol);
	if (port && port->flow == FLOW_INPUT && port->type == TYPE_CONTROL) {
		*size = sizeof(float);
		*type = jalv->forge.Float;
		return &port->control;
	}
	*size = *type = 0;
	return NULL;
}

void
lv2_save(Lv2Plugin* jalv, const char* dir)
{
	jalv->save_dir = lv2_strjoin(dir, "/");

	LilvState* const state = lilv_state_new_from_instance(
		jalv->plugin, jalv->instance, &jalv->map,
		jalv->temp_dir, dir, dir, dir,
		get_port_value, jalv,
		LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE, NULL);

	lilv_state_save(Lv2Manager::getInstance().world,
                  &jalv->map, &jalv->unmap, state, NULL,
	                dir, "state.ttl");

	lilv_state_free(state);

	free(jalv->save_dir);
	jalv->save_dir = NULL;
}

int
lv2_load_presets(Lv2Plugin* jalv, PresetSink sink, void* data)
{
	LilvNodes* presets = lilv_plugin_get_related(jalv->plugin,
	                                             jalv->nodes.pset_Preset);
	LILV_FOREACH(nodes, i, presets) {
		const LilvNode* preset = lilv_nodes_get(presets, i);
		lilv_world_load_resource(Lv2Manager::getInstance().world,
                             preset);
		if (!sink) {
			continue;
		}

		LilvNodes* labels = lilv_world_find_nodes(
			Lv2Manager::getInstance().world, preset, jalv->nodes.rdfs_label, NULL);
		if (labels) {
			const LilvNode* label = lilv_nodes_get_first(labels);
			sink(jalv, preset, label, data);
			lilv_nodes_free(labels);
		} else {
			fprintf(stderr, "Preset <%s> has no rdfs:label\n",
			        lilv_node_as_string(lilv_nodes_get(presets, i)));
		}
	}
	lilv_nodes_free(presets);

	return 0;
}

int
lv2_unload_presets(Lv2Plugin* jalv)
{
	LilvNodes* presets = lilv_plugin_get_related(jalv->plugin,
	                                             jalv->nodes.pset_Preset);
	LILV_FOREACH(nodes, i, presets) {
		const LilvNode* preset = lilv_nodes_get(presets, i);
		lilv_world_unload_resource(Lv2Manager::getInstance().world,
                               preset);
	}
	lilv_nodes_free(presets);

	return 0;
}

static void
set_port_value(const char* port_symbol,
               void*       user_data,
               const void* value,
               uint32_t    size,
               uint32_t    type)
{
	Lv2Plugin*        jalv = (Lv2Plugin*)user_data;
	Port* port = jalv->port_by_symbol(port_symbol);
	if (!port) {
		fprintf(stderr, "error: Preset port `%s' is missing\n", port_symbol);
		return;
	}

	float fvalue;
	if (type == jalv->forge.Float) {
		fvalue = *(const float*)value;
	} else if (type == jalv->forge.Double) {
		fvalue = *(const double*)value;
	} else if (type == jalv->forge.Int) {
		fvalue = *(const int32_t*)value;
	} else if (type == jalv->forge.Long) {
		fvalue = *(const int64_t*)value;
	} else {
		fprintf(stderr, "error: Preset `%s' value has bad type <%s>\n",
		        port_symbol, jalv->unmap.unmap(jalv->unmap.handle, type));
		return;
	}

	//if (jalv->play_state != JALV_RUNNING) {
		// Set value on port struct directly
		//port->control = fvalue;
	//} else {
		// Send value to running plugin
		lv2_ui_write(jalv, port->index, sizeof(fvalue), 0, &fvalue);
	//}

	if (jalv->has_ui) {
		// Update UI
		char buf[sizeof(ControlChange) + sizeof(fvalue)];
		ControlChange* ev = (ControlChange*)buf;
		ev->index    = port->index;
		ev->protocol = 0;
		ev->size     = sizeof(fvalue);
		*(float*)ev->body = fvalue;
		zix_ring_write(jalv->plugin_events, buf, sizeof(buf));
	}
}

void
Lv2Plugin::apply_state(LilvState* state)
{
	//bool must_pause = !safe_restore && play_state == JALV_RUNNING;
  bool must_pause = 0;
	if (state) {
		if (must_pause) {
			zix_sem_wait(&paused);
		}

		lilv_state_restore(
			state, instance, set_port_value, this, 0, state_features);

		if (must_pause) {
			request_update = true;
			//play_state     = JALV_RUNNING;
		}
	}
}

int
Lv2Plugin::apply_preset(const LilvNode* _preset)
{
	lilv_state_free(this->preset);
	preset = lilv_state_new_from_world(
        Lv2Manager::getInstance().world, &map, _preset);
	apply_state(this->preset);
	return 0;
}

int
Lv2Plugin::save_preset(
                 const char* dir,
                 const char* uri,
                 const char* label,
                 const char* filename)
{
	LilvState* const state = lilv_state_new_from_instance(
		plugin, instance, &map,
		temp_dir, dir, dir, dir,
		get_port_value, this,
		LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE, NULL);

	if (label) {
		lilv_state_set_label(state, label);
	}

	int ret = lilv_state_save(
		Lv2Manager::getInstance().world,
    &map, &unmap, state, uri, dir, filename);

	lilv_state_free(preset);
	preset = state;

	return ret;
}

int
Lv2Plugin::delete_current_preset()
{
	if (!preset) {
		return 1;
	}

	lilv_world_unload_resource(Lv2Manager::getInstance().world,
                             lilv_state_get_uri(preset));
	lilv_state_delete(Lv2Manager::getInstance().world,
                    preset);
	lilv_state_free(preset);
	preset = NULL;
	return 0;
}
