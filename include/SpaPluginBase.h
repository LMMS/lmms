/*
 * SpaPluginBase.h - implementation of SPA interface
 *
 * Copyright (c) 2018-2018 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#ifndef SPA_PLUGIN_BASE_H
#define SPA_PLUGIN_BASE_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_SPA

#include <cstdarg>

#include "Plugin.h"

namespace spa
{
class descriptor;
}

// interface that defines common functions, which are called by the LMMS core
class SpaPluginBase
{
public:
	virtual ~SpaPluginBase();

	virtual void writeOsc(
		const char *dest, const char *args, va_list va) = 0;
	virtual void writeOsc(const char *dest, const char *args, ...) = 0;

	virtual class AutomatableModel *modelAtPort(
		const class QString &dest) = 0;

	virtual unsigned netPort() const = 0;

	static Plugin::PluginTypes getPluginType(spa::descriptor *desc);
};

#endif // LMMS_HAVE_SPA

#endif // SPA_PLUGIN_BASE_H
