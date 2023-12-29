/*
 * ClapThreadPool.cpp - Implements CLAP thread pool extension
 *
 * Copyright (c) 2023 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include "ClapThreadPool.h"

#ifdef LMMS_HAVE_CLAP

#include <cassert>

#include "ClapInstance.h"

namespace lmms
{

auto ClapThreadPool::initImpl(const clap_host* host, const clap_plugin* plugin) noexcept -> bool
{
	m_stop = false;
	m_taskIndex = 0;
	auto numThreads = QThread::idealThreadCount();
	m_threads.resize(numThreads);
	for (int i = 0; i < numThreads; ++i)
	{
		m_threads[i].reset(QThread::create(&ClapThreadPool::entry, this));
		m_threads[i]->start(QThread::HighestPriority);
	}

	return true;
}

void ClapThreadPool::deinitImpl() noexcept
{
	terminate();
}

auto ClapThreadPool::hostExt() const -> const clap_host_thread_pool*
{
	static clap_host_thread_pool ext {
		&clapRequestExec
	};
	return &ext;
}

auto ClapThreadPool::checkSupported(const clap_plugin_thread_pool& ext) -> bool
{
	return ext.exec;
}

void ClapThreadPool::entry()
{
	while (true)
	{
		m_semaphoreProd.acquire();
		if (m_stop) { return; }

		const auto taskIndex = m_taskIndex++;
		pluginExt()->exec(plugin(), taskIndex);
		m_semaphoreDone.release();
	}
}

void ClapThreadPool::terminate()
{
	assert(ClapThreadCheck::isMainThread());

	m_stop = true;
	m_semaphoreProd.release(m_threads.size());
	for (auto& thread : m_threads)
	{
		if (thread) { thread->wait(); }
	}
	m_threads.clear();
}

auto ClapThreadPool::clapRequestExec(const clap_host* host, std::uint32_t numTasks) -> bool
{
	assert(ClapThreadCheck::isAudioThread());
	// TODO: Check that this is called from within the process method

	const auto h = fromHost(host);
	if (!h) { return false; }
	auto& threadPool = h->threadPool();

	if (!threadPool.supported())
	{
		h->log(CLAP_LOG_PLUGIN_MISBEHAVING, "Plugin cannot use the thread pool without implementing the thread pool extension");
		return false;
	}

	assert(!threadPool.m_stop);
	assert(!threadPool.m_threads.empty());

	if (numTasks == 0) { return true; }

	if (numTasks == 1)
	{
		threadPool.pluginExt()->exec(threadPool.plugin(), 0);
		return true;
	}

	threadPool.m_taskIndex = 0;
	threadPool.m_semaphoreProd.release(numTasks);
	threadPool.m_semaphoreDone.acquire(numTasks);
	return true;
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
