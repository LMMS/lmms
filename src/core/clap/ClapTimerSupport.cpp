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

#include <cassert>

#include "ClapInstance.h"

namespace lmms
{

auto ClapTimerSupport::init(const clap_plugin* plugin) -> bool
{
	assert(ClapInstance::isMainThread());
	assert(m_ext == nullptr && "Plugin extension already initialized");

	m_plugin = plugin;
	const auto ext = static_cast<const clap_plugin_timer_support*>(plugin->get_extension(plugin, CLAP_EXT_TIMER_SUPPORT));
	if (!ext || !ext->on_timer) { return false; }

	m_ext = ext;
	return true;
}

auto ClapTimerSupport::clapRegisterTimer(const clap_host* host, std::uint32_t periodMilliseconds, clap_id* timerId) -> bool
{
	assert(ClapInstance::isMainThread());

	if (!timerId) { return false; }
	*timerId = CLAP_INVALID_ID;

	auto& timerSupport = ClapInstance::fromHost(host)->timerSupport();
	if (!timerSupport.supported()) { return false; }

	// Period should be no lower than 20 ms (arbitrary)
	periodMilliseconds = std::max<std::uint32_t>(20, periodMilliseconds);

	const auto id = timerSupport.startTimer(periodMilliseconds);
	if (id <= 0) { return false; }

	*timerId = static_cast<clap_id>(id);
	return true;
}

auto ClapTimerSupport::clapUnregisterTimer(const clap_host* host, clap_id timerId) -> bool
{
	assert(ClapInstance::isMainThread());

	if (timerId == 0 || timerId == CLAP_INVALID_ID) { return false; }

	auto& timerSupport = ClapInstance::fromHost(host)->timerSupport();
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
