/*
 * Semaphore.h - Semaphore declaration
 *
 * Copyright (c) 2022-2022 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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

/*
 * This code has been copied and adapted from https://github.com/drobilla/jalv
 * File src/zix/sem.h
 */

#ifndef LMMS_SEMAPHORE_H
#define LMMS_SEMAPHORE_H

#include "lmmsconfig.h"

#ifdef LMMS_BUILD_APPLE
#    include <mach/mach.h>
#elif defined(LMMS_BUILD_WIN32)
#    include <windows.h>
#else
#    include <semaphore.h>
#endif


namespace lmms {

/**
   A counting semaphore.

   This is an integer that is always positive, and has two main operations:
   increment (post) and decrement (wait).  If a decrement can not be performed
   (i.e. the value is 0) the caller will be blocked until another thread posts
   and the operation can succeed.

   Semaphores can be created with any starting value, but typically this will
   be 0 so the semaphore can be used as a simple signal where each post
   corresponds to one wait.

   Semaphores are very efficient (much moreso than a mutex/cond pair).  In
   particular, at least on Linux, post is async-signal-safe, which means it
   does not block and will not be interrupted.  If you need to signal from
   a realtime thread, this is the most appropriate primitive to use.

   @note Likely outdated with C++20's std::counting_semaphore
		 (though we have to check that this will be RT conforming on all platforms)
*/
class Semaphore
{
public:
	Semaphore(unsigned initial);
	Semaphore(const Semaphore&) = delete;
	Semaphore& operator=(const Semaphore&) = delete;
	Semaphore(Semaphore&&) = delete;
	Semaphore& operator=(Semaphore&&) = delete;
	~Semaphore();

	void post();
	void wait();
	bool tryWait();

private:
#ifdef LMMS_BUILD_APPLE
	semaphore_t m_sem;
#elif defined(LMMS_BUILD_WIN32)
	HANDLE m_sem;
#else
	sem_t m_sem;
#endif
};

} // namespace lmms

#endif // LMMS_SEMAPHORE_H
