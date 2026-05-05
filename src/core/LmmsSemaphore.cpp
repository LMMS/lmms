/*
 * Semaphore.cpp - Semaphore implementation
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

#include "LmmsSemaphore.h"

#if defined(LMMS_BUILD_WIN32)
#    include <limits.h>
#else
#    include <errno.h>
#endif

#include <system_error>

namespace lmms {

#ifdef LMMS_BUILD_APPLE
Semaphore::Semaphore(unsigned val)
{
	kern_return_t rval = semaphore_create(mach_task_self(), &m_sem, SYNC_POLICY_FIFO, val);
	if(rval != 0) {
		throw std::system_error(rval, std::system_category(), "Could not create semaphore");
	}
}

Semaphore::~Semaphore()
{
	semaphore_destroy(mach_task_self(), m_sem);
}

void Semaphore::post()
{
	semaphore_signal(m_sem);
}

void Semaphore::wait()
{
	kern_return_t rval = semaphore_wait(m_sem);
	if (rval != KERN_SUCCESS) {
		throw std::system_error(rval, std::system_category(), "Waiting for semaphore failed");
	}
}

bool Semaphore::tryWait()
{
	const mach_timespec_t zero = { 0, 0 };
	return semaphore_timedwait(m_sem, zero) == KERN_SUCCESS;
}

#elif defined(LMMS_BUILD_WIN32)

Semaphore::Semaphore(unsigned initial)
{
	if(CreateSemaphore(nullptr, initial, LONG_MAX, nullptr) == nullptr) {
		throw std::system_error(GetLastError(), std::system_category(), "Could not create semaphore");
	}
}

Semaphore::~Semaphore()
{
	CloseHandle(m_sem);
}

void Semaphore::post()
{
	ReleaseSemaphore(m_sem, 1, nullptr);
}

void Semaphore::wait()
{
	if (WaitForSingleObject(m_sem, INFINITE) != WAIT_OBJECT_0) {
		throw std::system_error(GetLastError(), std::system_category(), "Waiting for semaphore failed");
	}
}

bool Semaphore::tryWait()
{
	return WaitForSingleObject(m_sem, 0) == WAIT_OBJECT_0;
}

#else  /* !defined(LMMS_BUILD_APPLE) && !defined(LMMS_BUILD_WIN32) */

Semaphore::Semaphore(unsigned initial)
{
	if(sem_init(&m_sem, 0, initial) != 0) {
		throw std::system_error(errno, std::generic_category(), "Could not create semaphore");
	}
}

Semaphore::~Semaphore()
{
	sem_destroy(&m_sem);
}

void Semaphore::post()
{
	sem_post(&m_sem);
}

void Semaphore::wait()
{
	while (sem_wait(&m_sem) != 0) {
		if (errno != EINTR) {
			throw std::system_error(errno, std::generic_category(), "Waiting for semaphore failed");
		}
		/* Otherwise, interrupted, so try again. */
	}
}

bool Semaphore::tryWait()
{
	return (sem_trywait(&m_sem) == 0);
}

#endif

} // namespace lmms

