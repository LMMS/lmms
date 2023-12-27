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

#include "ClapExtension.h"

namespace lmms
{

class ClapThreadPool final : public ClapExtension<clap_host_thread_pool, clap_plugin_thread_pool>
{
public:
	using ClapExtension::ClapExtension;
	~ClapThreadPool() override { deinit(); }

	auto init(const clap_host* host, const clap_plugin* plugin) -> bool override;
	void deinit() override;

	auto extensionId() const -> std::string_view override { return CLAP_EXT_THREAD_POOL; }
	auto hostExt() const -> const clap_host_thread_pool* override;

private:
	auto checkSupported(const clap_plugin_thread_pool* ext) -> bool override;

	void entry();
	void terminate();

	/**
	 * clap_host_thread_pool implementation
	 */
	static auto clapRequestExec(const clap_host* host, std::uint32_t numTasks) -> bool;

	std::vector<std::unique_ptr<QThread>> m_threads;
	std::atomic<bool> m_stop{ false };
	std::atomic<std::uint32_t> m_taskIndex{ 0 };
	QSemaphore m_semaphoreProd;
	QSemaphore m_semaphoreDone;
};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_THREAD_POOL_H
