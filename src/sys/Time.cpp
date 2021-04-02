/*
 * TimePos.cpp - Windows compatible implementation of sys/time.cpp.
 * Copied from https://www.codefull.net/2015/12/systime-h-replacement-for-windows/
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net
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

#ifndef TIMES_H
#define TIMES_H

#include "sys/Times.h"

#endif

#ifndef TIME
#define TIME

//int gettimeofday(struct timeval* t, void* timezone)
//{
//	struct _timeb timebuffer;
//	_ftime(&timebuffer);
//	t->tv_sec = timebuffer.time;
//	t->tv_usec = 1000 * timebuffer.millitm;
//	return 0;
//}
//
//clock_t times(struct tms* __buffer) {
//
//	__buffer->tms_utime = clock();
//	__buffer->tms_stime = 0;
//	__buffer->tms_cstime = 0;
//	__buffer->tms_cutime = 0;
//	return __buffer->tms_utime;
//}

#endif