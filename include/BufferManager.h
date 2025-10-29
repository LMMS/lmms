/*
 * BufferManager.h - A buffer caching/memory management system
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_BUFFER_MANAGER_H
#define LMMS_BUFFER_MANAGER_H

#include "lmms_export.h"
#include "LmmsTypes.h"

namespace lmms
{

class SampleFrame;

class LMMS_EXPORT BufferManager
{
public:
	static void init( fpp_t fpp );
	static SampleFrame* acquire();
	static void release( SampleFrame* buf );

private:
	static fpp_t s_framesPerPeriod;
};


} // namespace lmms

#endif // LMMS_BUFFER_MANAGER_H
