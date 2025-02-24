/*
 * ThreadableJob.h - declaration of class ThreadableJob
 *
 * Copyright (c) 2009-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_THREADABLE_JOB_H
#define LMMS_THREADABLE_JOB_H

#include <atomic>
#include <memory_resource>

namespace lmms
{

class ThreadableJob
{
public:

	enum class ProcessingState : int
	{
		Unstarted,
		Queued,
		InProgress,
		Done
	};

	ThreadableJob() :
		m_state(ProcessingState::Unstarted)
	{
	}

	inline ProcessingState state() const
	{
		return m_state.load();
	}

	inline void reset()
	{
		m_state = ProcessingState::Unstarted;
	}

	inline void queue()
	{
		m_state = ProcessingState::Queued;
	}
	
	inline void done()
	{
		m_state = ProcessingState::Done;
	}

	void process()
	{
		auto expected = ProcessingState::Queued;
		if (m_state.compare_exchange_strong(expected, ProcessingState::InProgress))
		{
			doProcessing();
			m_state = ProcessingState::Done;
		}
	}

	virtual bool requiresProcessing() const = 0;

	static void* operator new(std::size_t bytes) { return s_pool.allocate(bytes); }
	static void operator delete(void* ptr, std::size_t bytes) { s_pool.deallocate(ptr, bytes); }

protected:
	virtual void doProcessing() = 0;

	std::atomic<ProcessingState> m_state;
	inline static thread_local std::pmr::unsynchronized_pool_resource s_pool;
} ;

} // namespace lmms

#endif // LMMS_THREADABLE_JOB_H
