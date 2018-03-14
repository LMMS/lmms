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

#include "jalv_internal.h"

int
scale_point_cmp(const ScalePoint* a, const ScalePoint* b)
{
	if (a->value < b->value) {
		return -1;
	} else if (a->value == b->value) {
		return 0;
	}
	return 1;
}

ControlID*
new_port_control(Jalv* jalv, uint32_t index)
{
	struct Port*      port  = &jalv->ports[index];
	const LilvPort*   lport = port->lilv_port;
	const LilvPlugin* plug  = jalv->plugin;
	const JalvNodes*  nodes = &jalv->nodes;

	ControlID* id = (ControlID*)calloc(1, sizeof(ControlID));
	id->jalv           = jalv;
	id->type           = PORT;
	id->node           = lilv_node_duplicate(lilv_port_get_node(plug, lport));
	id->symbol         = lilv_node_duplicate(lilv_port_get_symbol(plug, lport));
	id->label          = lilv_port_get_name(plug, lport);
	id->index          = index;
	id->group          = lilv_port_get(plug, lport, jalv->nodes.pg_group);
	id->min            = lilv_port_get(plug, lport, nodes->lv2_minimum);
	id->max            = lilv_port_get(plug, lport, nodes->lv2_maximum);
	id->def            = lilv_port_get(plug, lport, nodes->lv2_default);
	id->value_type     = jalv->forge.Float;
	id->is_writable    = lilv_port_is_a(plug, lport, nodes->lv2_InputPort);
	id->is_readable    = lilv_port_is_a(plug, lport, nodes->lv2_OutputPort);
	id->is_toggle      = lilv_port_has_property(plug, lport, nodes->lv2_toggled);
	id->is_integer     = lilv_port_has_property(plug, lport, nodes->lv2_integer);
	id->is_enumeration = lilv_port_has_property(plug, lport, nodes->lv2_enumeration);
	id->is_logarithmic = lilv_port_has_property(plug, lport, nodes->pprops_logarithmic);

	lilv_port_get_range(plug, lport, &id->def, &id->min, &id->max);
	if (lilv_port_has_property(plug, lport, jalv->nodes.lv2_sampleRate)) {
		/* Adjust range for lv2:sampleRate controls */
		if (lilv_node_is_float(id->min) || lilv_node_is_int(id->min)) {
			const float min = lilv_node_as_float(id->min) * jalv->sample_rate;
			lilv_node_free(id->min);
			id->min = lilv_new_float(jalv->world, min);
		}
		if (lilv_node_is_float(id->max) || lilv_node_is_int(id->max)) {
			const float max = lilv_node_as_float(id->max) * jalv->sample_rate;
			lilv_node_free(id->max);
			id->max = lilv_new_float(jalv->world, max);
		}
	}

	/* Get scale points */
	LilvScalePoints* sp = lilv_port_get_scale_points(plug, lport);
	if (sp) {
		id->points = (ScalePoint*)malloc(
			lilv_scale_points_size(sp) * sizeof(ScalePoint));
		size_t np = 0;
		LILV_FOREACH(scale_points, s, sp) {
			const LilvScalePoint* p = lilv_scale_points_get(sp, s);
			if (lilv_node_is_float(lilv_scale_point_get_value(p)) ||
			    lilv_node_is_int(lilv_scale_point_get_value(p))) {
				id->points[np].value = lilv_node_as_float(
					lilv_scale_point_get_value(p));
				id->points[np].label = strdup(
					lilv_node_as_string(lilv_scale_point_get_label(p)));
				++np;
			}
			/* TODO: Non-float scale points? */
		}

		qsort(id->points, np, sizeof(ScalePoint),
		      (int (*)(const void*, const void*))scale_point_cmp);
		id->n_points = np;

		lilv_scale_points_free(sp);
	}

	return id;
}

static bool
has_range(Jalv* jalv, const LilvNode* subject, const char* range_uri)
{
	LilvNode*  range  = lilv_new_uri(jalv->world, range_uri);
	const bool result = lilv_world_ask(
		jalv->world, subject, jalv->nodes.rdfs_range, range);
	lilv_node_free(range);
	return result;
}

ControlID*
new_property_control(Jalv* jalv, const LilvNode* property)
{
	ControlID* id = (ControlID*)calloc(1, sizeof(ControlID));
	id->jalv     = jalv;
	id->type     = PROPERTY;
	id->node     = lilv_node_duplicate(property);
	id->symbol   = lilv_world_get_symbol(jalv->world, property);
	id->label    = lilv_world_get(jalv->world, property, jalv->nodes.rdfs_label, NULL);
	id->property = jalv->map.map(jalv, lilv_node_as_uri(property));

	id->min = lilv_world_get(jalv->world, property, jalv->nodes.lv2_minimum, NULL);
	id->max = lilv_world_get(jalv->world, property, jalv->nodes.lv2_maximum, NULL);
	id->def = lilv_world_get(jalv->world, property, jalv->nodes.lv2_default, NULL);

	const char* const types[] = {
		LV2_ATOM__Int, LV2_ATOM__Long, LV2_ATOM__Float, LV2_ATOM__Double,
		LV2_ATOM__Bool, LV2_ATOM__String, LV2_ATOM__Path, NULL
	};

	for (const char*const* t = types; *t; ++t) {
		if (has_range(jalv, property, *t)) {
			id->value_type = jalv->map.map(jalv, *t);
			break;
		}
	}

	id->is_toggle  = (id->value_type == jalv->forge.Bool);
	id->is_integer = (id->value_type == jalv->forge.Int ||
	                  id->value_type == jalv->forge.Long);

	const size_t sym_len = strlen(lilv_node_as_string(id->symbol));
	if (sym_len > jalv->longest_sym) {
		jalv->longest_sym = sym_len;
	}

	if (!id->value_type) {
		fprintf(stderr, "Unknown value type for property <%s>\n",
		        lilv_node_as_string(property));
	}

	return id;
}

void
add_control(Controls* controls, ControlID* control)
{
	controls->controls = (ControlID**)realloc(
		controls->controls, (controls->n_controls + 1) * sizeof(ControlID*));
	controls->controls[controls->n_controls++] = control;
}

ControlID*
get_property_control(const Controls* controls, LV2_URID property)
{
	for (size_t i = 0; i < controls->n_controls; ++i) {
		if (controls->controls[i]->property == property) {
			return controls->controls[i];
		}
	}

	return NULL;
}
