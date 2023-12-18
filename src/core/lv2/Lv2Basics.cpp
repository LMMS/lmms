/*
 * Lv2Basics.cpp - basic Lv2 functions
 *
 * Copyright (c) 2019-2020 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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

#include "Lv2Basics.h"

#ifdef LMMS_HAVE_LV2

namespace lmms
{

QString qStringFromPluginNode(const LilvPlugin* plug,
	LilvNode* (*getFunc)(const LilvPlugin*))
{
	return QString::fromUtf8(
		lilv_node_as_string(AutoLilvNode((*getFunc)(plug)).get()));
}

QString qStringFromPortName(const LilvPlugin* plug, const LilvPort* port)
{
	return QString::fromUtf8(
		lilv_node_as_string(AutoLilvNode(lilv_port_get_name(plug, port)).get()));
}

std::string stdStringFromPortName(const LilvPlugin* plug, const LilvPort* port)
{
	return std::string(
			lilv_node_as_string(AutoLilvNode(lilv_port_get_name(plug, port)).get()));
}

} // namespace lmms

#endif // LMMS_HAVE_LV2

