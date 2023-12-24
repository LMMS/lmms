/*
 * ClapTimerSupport.cpp - Implements CLAP timer support extension
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

#include "ClapTimerSupport.h"

#ifdef LMMS_HAVE_CLAP

#include <QTimerEvent>

#include <string>
#include <cassert>

#include "ClapInstance.h"

namespace lmms
{

void ClapTimerSupport::deinit()
{
	killTimers();
	m_ext = nullptr;
	m_plugin = nullptr;
}

auto ClapTimerSupport::hostExt() const -> const clap_host_timer_support*
{
	static clap_host_timer_support ext {
		&clapRegisterTimer,
		&clapUnregisterTimer
	};
	return &ext;
}

auto ClapTimerSupport::checkSupported(const clap_plugin_timer_support* ext) -> bool
{
	return ext->on_timer;
}

void ClapTimerSupport::killTimers()
{
	for (int timerId : m_timerIds)
	{
		killTimer(timerId);
	}
	m_timerIds.clear();
}

auto ClapTimerSupport::clapRegisterTimer(const clap_host* host, std::uint32_t periodMilliseconds, clap_id* timerId) -> bool
{
	assert(ClapInstance::isMainThread());
	const auto h = ClapInstance::fromHost(host);

	if (!timerId)
	{
		h->log(CLAP_LOG_PLUGIN_MISBEHAVING, "register_timer()'s `timerId` cannot be null");
		return false;
	}
	*timerId = CLAP_INVALID_ID;

	auto& timerSupport = h->timerSupport();
	if (!timerSupport.supported())
	{
		h->log(CLAP_LOG_PLUGIN_MISBEHAVING, "Plugin cannot register a timer when it does not implement the timer support extension");
		return false;
	}

	// Period should be no lower than 15 ms (arbitrary)
	periodMilliseconds = std::max<std::uint32_t>(15, periodMilliseconds);

	const auto id = timerSupport.startTimer(periodMilliseconds);
	if (id <= 0)
	{
		h->log(CLAP_LOG_WARNING, "Failed to start timer");
		return false;
	}

	*timerId = static_cast<clap_id>(id);
	timerSupport.m_timerIds.insert(*timerId);

	return true;
}

auto ClapTimerSupport::clapUnregisterTimer(const clap_host* host, clap_id timerId) -> bool
{
	assert(ClapInstance::isMainThread());
	const auto h = ClapInstance::fromHost(host);

	if (timerId == 0 || timerId == CLAP_INVALID_ID)
	{
		h->log(CLAP_LOG_PLUGIN_MISBEHAVING, "Invalid timer id");
		return false;
	}

	auto& timerSupport = h->timerSupport();
	if (timerSupport.m_timerIds.find(timerId) == timerSupport.m_timerIds.end())
	{
		const auto msg = "Unrecognized timer id: " + std::to_string(timerId);
		h->log(CLAP_LOG_PLUGIN_MISBEHAVING, msg.c_str());
		return false;
	}

	timerSupport.killTimer(static_cast<int>(timerId));

	return true; // assume successful
}

void ClapTimerSupport::timerEvent(QTimerEvent* event)
{
	assert(ClapInstance::isMainThread());
	const auto timerId = static_cast<clap_id>(event->timerId());
	assert(timerId > 0 && "Timer must be active");
	if (m_ext && m_plugin)
	{
		m_ext->on_timer(m_plugin, timerId);
	}
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
