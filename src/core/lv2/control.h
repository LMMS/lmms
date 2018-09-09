/*
 * control.h - LV2 control
 *
 * Copyright 2018 Alexandros Theodotou
 * Copyright 2007-2016 David Robillard <http://drobilla.net>
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

#ifndef LV2_CONTROL_H
#define LV2_CONTROL_H

extern "C" {
#include <lilv/lilv.h>

#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
}

class Lv2Plugin;

/* Controls */

/** Type of plugin control. */
typedef enum {
	PORT,     ///< Control port
	PROPERTY  ///< Property (set via atom message)
} ControlType;

typedef struct {
	float value;
	char* label;
} ScalePoint;

/** Order scale points by value. */
int scale_point_cmp(const ScalePoint* a, const ScalePoint* b);

/** Plugin control. */
typedef struct {
	Lv2Plugin*       jalv;
	ControlType type;
	LilvNode*   node;
	LilvNode*   symbol;          ///< Symbol
	LilvNode*   label;           ///< Human readable label
	LV2_URID    property;        ///< Iff type == PROPERTY
	uint32_t    index;           ///< Iff type == PORT
	LilvNode*   group;           ///< Port/control group, or NULL
	void*       widget;          ///< Control Widget
	size_t      n_points;        ///< Number of scale points
	ScalePoint* points;          ///< Scale points
	LV2_URID    value_type;      ///< Type of control value
	LV2_Atom    value;           ///< Current value
	LilvNode*   min;             ///< Minimum value
	LilvNode*   max;             ///< Maximum value
	LilvNode*   def;             ///< Default value
	bool        is_toggle;       ///< Boolean (0 and 1 only)
	bool        is_integer;      ///< Integer values only
	bool        is_enumeration;  ///< Point values only
	bool        is_logarithmic;  ///< Logarithmic scale
	bool        is_writable;     ///< Writable (input)
	bool        is_readable;     ///< Readable (output)
} ControlID;

ControlID*
new_port_control(Lv2Plugin* jalv, uint32_t index);

ControlID*
new_property_control(Lv2Plugin* jalv, const LilvNode* property);

typedef struct {
	size_t      n_controls;
	ControlID** controls;
} Controls;

void
add_control(Controls* controls, ControlID* control);

ControlID*
get_property_control(const Controls* controls, LV2_URID property);

/**
   Control change event, sent through ring buffers for UI updates.
*/
typedef struct {
	uint32_t index;
	uint32_t protocol;
	uint32_t size;
	uint8_t  body[];
} ControlChange;

#endif
