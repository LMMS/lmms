/*
 * ClapThreadPool.h - Implements CLAP thread pool extension
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

#ifndef LMMS_CLAP_THREAD_POOL_H
#define LMMS_CLAP_THREAD_POOL_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include <QThread>
#include <QSemaphore>

#include <memory>
#include <vector>
#include <atomic>

#include <clap/ext/thread-pool.h>

namespace lmms
{

class ClapThreadPool
{
public:
	friend class ClapInstance;

	~ClapThreadPool() { deinit(); }

	auto supported() const -> bool { return m_ext != nullptr; }

private:

	auto init(const clap_plugin* plugin) -> bool;
	void deinit();

	void entry();
	void terminate();

	/**
	 * clap_host_thread_pool implementation
	 */
	static auto clapRequestExec(const clap_host* host, std::uint32_t numTasks) -> bool;

	const clap_plugin* m_plugin = nullptr;
	const clap_plugin_thread_pool* m_ext = nullptr;

	std::vector<std::unique_ptr<QThread>> m_threads;
	std::atomic<bool> m_stop{ false };
	std::atomic<int> m_taskIndex{ 0 };
	QSemaphore m_semaphoreProd;
	QSemaphore m_semaphoreDone;
};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_THREAD_POOL_H
