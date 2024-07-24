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

#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>

namespace lmms {
//! A thread pool that can be used for asynchronous processing.
class ThreadPool
{
public:
	//! Destroys the `ThreadPool` object.
	//! This blocks until all workers have finished executing.
	~ThreadPool();

	//! Enqueue function `fn` with arguments `args` to be ran asynchronously.
	template <typename Fn, typename... Args>
	auto enqueue(Fn&& fn, Args&&... args) -> std::future<std::invoke_result_t<Fn, Args...>>
	{
		using ReturnType = std::invoke_result_t<Fn, Args...>;

		auto promise = std::make_shared<std::promise<ReturnType>>();
		auto task = [promise, fn = std::forward<Fn>(fn), args = std::make_tuple(std::forward<Args>(args)...)] 
		{
			if constexpr (!std::is_same_v<ReturnType, void>)
			{
				promise->set_value(std::apply(fn, args));
				return;
			}
			std::apply(fn, args);
			promise->set_value();
		};

		{
			const auto lock = std::unique_lock{m_runMutex};
			m_queue.push(std::move(task));
		}

		m_runCond.notify_one();
		return promise->get_future();
	}

	//! Return the number of worker threads used.
	auto numWorkers() const -> size_t;

	//! Return the global `ThreadPool` instance.
	static auto instance() -> ThreadPool&;

private:
	ThreadPool(size_t numWorkers);
	void run();
	std::vector<std::thread> m_workers;
	std::queue<std::function<void()>> m_queue;
	std::atomic<bool> m_done = false;
	std::condition_variable m_runCond;
	std::mutex m_runMutex;
	inline static size_t s_numWorkers = std::thread::hardware_concurrency();
};
} // namespace lmms

#endif // LMMS_THREAD_POOL_H
