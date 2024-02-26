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

#include <atomic>
#include <queue>
#include <tuple>
#include <type_traits>
#include <vector>

#ifdef __MINGW32__
#include <mingw.condition_variable.h>
#include <mingw.future.h>
#include <mingw.mutex.h>
#include <mingw.thread.h>
#else
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>
#endif

namespace lmms {
//! A thread pool that can be used for asynchronous processing.
class ThreadPool
{
public:
	//! Creates a `ThreadPool` containing `numWorkers` worker threads.
	//! Precondition: `numWorkers > 0`.
	ThreadPool(size_t numWorkers = std::thread::hardware_concurrency());

	//! Destroys the `ThreadPool` object.
	//! This blocks until all workers have finished executing.
	~ThreadPool();

	//! Enqueue function `fn` with arguments `args` to be ran asynchronously.
	template <typename Fn, typename... Args>
	auto enqueue(Fn&& fn, Args&&... args) -> std::future<std::invoke_result_t<Fn, Args...>>
	{
		using ReturnType = std::invoke_result_t<Fn, Args...>;
		using PackagedTaskType = std::packaged_task<ReturnType()>;

		auto packagedTask = std::make_shared<PackagedTaskType>(
			[fn = std::forward<Fn>(fn), args = std::make_tuple(std::forward<Args>(args)...)] {
				return std::apply(fn, args);
			});

		{
			const auto lock = std::unique_lock{m_runMutex};
			m_queue.push([packagedTask] { (*packagedTask)(); });
		}

		m_runCond.notify_one();
		return packagedTask->get_future();
	}

	//! Returns the number of worker threads used.
	auto numWorkers() const -> size_t;

private:
	void run();
	std::vector<std::thread> m_workers;
	std::queue<std::function<void()>> m_queue;
	std::atomic<bool> m_done = false;
	std::condition_variable m_runCond;
	std::mutex m_runMutex;
};
} // namespace lmms

#endif // LMMS_THREAD_POOL_H