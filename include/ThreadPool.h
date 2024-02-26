/*
 * ThreadPool.h
 *
 * Copyright (c) 2024 saker
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

#ifndef LMMS_THREAD_POOL_H
#define LMMS_THREAD_POOL_H

#include <thread>
#include <vector>

namespace lmms {
//! A thread pool that can be used for asynchronous processing.
class ThreadPool
{
public:
	//! Creates a `ThreadPool` containing `numWorkers` worker threads.
	//! Precondition: `numWorkers > 0`.
	ThreadPool(size_t numWorkers = std::thread::hardware_concurrency());

	//! Destroys `ThreadPool`, aborting all running tasks and joining the worker threads.
	~ThreadPool();

	//! Returns the number of worker threads used.
	auto numWorkers() const -> size_t;

private:
	void run();
	std::vector<std::thread> m_workers;
};
} // namespace lmms

#endif // LMMS_THREAD_POOL_H