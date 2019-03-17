/*
 * Lv2Basics.h - basic Lv2 utils
 *
 * Copyright (c) 2018-2019 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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


#ifndef LV2BASICS_H
#define LV2BASICS_H


#include "lmmsconfig.h"

#ifdef LMMS_HAVE_LV2

#include <lilv/lilv.h>


//! a simple RAII class for lilv nodes that shall be freed
struct AutoLilvNode
{
	LilvNode* n;
	AutoLilvNode(LilvNode* n) : n(n) {}
	AutoLilvNode(const AutoLilvNode& other) = delete;
	AutoLilvNode(AutoLilvNode&& other) {
		n = other.n;
		other.n = nullptr;
	}
	~AutoLilvNode() { if(n) lilv_node_free(n); }
	const LilvNode* get() const { return n; }
};


#endif // LMMS_HAVE_LV2
#endif // LV2BASICS_H
