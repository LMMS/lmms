/*
 * vst_base.cpp - VST-base-code to be used by any LMMS plugins dealing with VST-
 *                plugins
 *
 * Copyright (c) 2006-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "LmmsCommonMacros.h"
#include "Plugin.h"
#include "vstbase_export.h"

namespace lmms
{


extern "C"
{

Plugin::Descriptor VSTBASE_EXPORT vstbase_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"VST Base",
	"library for all LMMS plugins dealing with VST-plugins",
	"Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>",
	0x0100,
	Plugin::Type::Library,
	nullptr,
	nullptr,
} ;

}


} // namespace lmms
