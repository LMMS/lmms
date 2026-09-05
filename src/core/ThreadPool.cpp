/*
 * ThreadPool.cpp
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
#include "ThreadPool.h"

#include <cassert>

namespace lmms {
ThreadPool::ThreadPool(size_t numWorkers)
{
	assert(numWorkers > 0);

	m_workers.reserve(numWorkers);
	for (size_t i = 0; i < numWorkers; ++i)
	{
		m_workers.emplace_back([this] { run(); });
	}
}

ThreadPool::~ThreadPool()
{
	{
		const auto lock = std::unique_lock{m_runMutex};
		m_done = true;
	}

	m_runCond.notify_all();

	for (auto& worker : m_workers)
	{
		if (worker.joinable()) { worker.join(); }
	}
}

auto ThreadPool::numWorkers() const -> size_t
{
	return m_workers.size();
}

void ThreadPool::run()
{
	while (!m_done)
	{
		std::function<void()> task;
		{
			auto lock = std::unique_lock{m_runMutex};
			m_runCond.wait(lock, [this] { return !m_queue.empty() || m_done; });

			if (m_done) { break; }
			task = m_queue.front();
			m_queue.pop();
		}
		task();
	}
}

auto ThreadPool::instance() -> ThreadPool&
{
	static auto s_pool = ThreadPool{s_numWorkers};
	return s_pool;
}

} // namespace lmms
