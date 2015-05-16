/*
 * BufferManager.h - A buffer caching/memory management system
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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

#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include "export.h"
#include "lmms_basics.h"

class QAtomicInt;


namespace lmms
{


const int BM_INITIAL_BUFFERS = 512;
//const int BM_INCREMENT = 64;

class EXPORT BufferManager
{
public:
	static void init( fpp_t framesPerPeriod );
	static sampleFrame * acquire();
	static void release( sampleFrame * buf );
	static void refresh();
//	static void extend( int c );

private:
	static sampleFrame ** s_available;
	static QAtomicInt s_availableIndex;

	static sampleFrame ** s_released;
	static QAtomicInt s_releasedIndex;
//	static QReadWriteLock s_mutex;
	static int s_size;
};


}
#endif
