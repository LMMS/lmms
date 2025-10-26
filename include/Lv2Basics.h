/*
 * Lv2Basics.h - basic Lv2 utils
 *
 * Copyright (c) 2018-2024 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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

#ifndef LMMS_LV2BASICS_H
#define LMMS_LV2BASICS_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_LV2

#include <lilv/lilv.h>
#include <memory>
#include <QString>
#include <string>

namespace lmms
{

template<class T>
struct LilvPtrDeleter
{
	void operator()(T* n) { lilv_free(static_cast<void*>(n)); }
};

struct LilvNodeDeleter
{
	void operator()(LilvNode* n) { lilv_node_free(n); }
};

struct LilvNodesDeleter
{
	void operator()(LilvNodes* n) { lilv_nodes_free(n); }
};

struct LilvScalePointsDeleter
{
	void operator()(LilvScalePoints* s) { lilv_scale_points_free(s); }
};

template<class T>
using AutoLilvPtr = std::unique_ptr<T, LilvPtrDeleter<T>>;
using AutoLilvNode = std::unique_ptr<LilvNode, LilvNodeDeleter>;
using AutoLilvNodes = std::unique_ptr<LilvNodes, LilvNodesDeleter>;
using AutoLilvScalePoints = std::unique_ptr<LilvScalePoints, LilvScalePointsDeleter>;

/**
	Return QString from a plugin's node, everything will be freed automatically
	@param plug The plugin where the node is
	@param getFunc The function to return the node from the plugin
*/
QString qStringFromPluginNode(const LilvPlugin* plug,
		LilvNode * (*getFunc)(const LilvPlugin*));

//! Return port name as QString, everything will be freed automatically
QString qStringFromPortName(const LilvPlugin* plug, const LilvPort* port);

//! Return port name as std::string, everything will be freed automatically
std::string stdStringFromPortName(const LilvPlugin* plug, const LilvPort* port);

} // namespace lmms

#endif // LMMS_HAVE_LV2

#endif // LMMS_LV2BASICS_H
